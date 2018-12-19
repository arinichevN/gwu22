#include "main.h"

int app_state = APP_INIT;
int sock_port = -1;
int sock_fd = -1;

Peer peer_client = {.fd = &sock_fd, .addr_size = sizeof peer_client.addr};

int retry_count = 0;

LCorrectionList lcorrection_list = LIST_INITIALIZER;
DeviceList device_list = LIST_INITIALIZER;
DItemList ditem_list = LIST_INITIALIZER;

#include "util.c"
#include "init_f.c"

void serverRun(int *state, int init_state) {
    SERVER_HEADER
    SERVER_APP_ACTIONS
    DEF_SERVER_I1LIST
    if (ACP_CMD_IS(ACP_CMD_GET_FTS)) {
        SERVER_PARSE_I1LIST
        FORLISTN(i1l, i) {
            DItem *item;
            LIST_GETBYID(item, &ditem_list, i1l.item[i]);
            if (item == NULL) continue;
            readDevice(item->device);
            if (!catFTS(item, &response)) return;
        }
    }
    acp_responseSend(&response, &peer_client);
}

int initApp() {
    if (!readSettings(&sock_port, &retry_count, CONF_MAIN_FILE)) {
        putsde("failed to read settings\n");
        return 0;
    }
    if (!initServer(&sock_fd, sock_port)) {
        putsde("failed to initialize server\n");
        return 0;
    }
    if (!gpioSetup()) {
	    freeSocketFd(&sock_fd);
        putsde("failed to initialize GPIO\n");
        return 0;
    }
    return 1;
}

int initData() {
    initLCorrection(&lcorrection_list, CONF_LCORRECTION_FILE);
    if (!initDevice(&device_list, &ditem_list, retry_count, CONF_DEVICE_FILE)) {
        FREE_LIST(&ditem_list);
        FREE_LIST(&device_list);
        FREE_LIST(&lcorrection_list);
        return 0;
    }
    if (!checkDevice(&device_list, &ditem_list)) {
        FREE_LIST(&ditem_list);
        FREE_LIST(&device_list);
        FREE_LIST(&lcorrection_list);
        return 0;
    }
    assignMod(&ditem_list, &lcorrection_list, CONF_MOD_MAPPING_FILE) ;
    return 1;
}

void freeData() {
    FREE_LIST(&ditem_list);
    FREE_LIST(&device_list);
    FREE_LIST(&lcorrection_list);
}

void freeApp() {
    freeData();
    freeSocketFd(&sock_fd);
    gpioFree();
}

void exit_nicely ( ) {
    freeApp();
    putsdo ( "\nexiting now...\n" );
    exit ( EXIT_SUCCESS );
}

int main(int argc, char** argv) {
    if (geteuid() != 0) {
        putsde("root user expected\n"); 
        return (EXIT_FAILURE);
    }
#ifndef MODE_DEBUG
    daemon(0, 0);
#endif
    conSig(&exit_nicely);
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perrorl ( "mlockall()" );
    }
#ifndef MODE_DEBUG
    setPriorityMax(SCHED_FIFO);
#endif
    int data_initialized = 0;
    while (1) {
#ifdef MODE_DEBUG
        printf("%s(): %s %d\n", F, getAppState(app_state), data_initialized);
#endif
        switch (app_state) {
            case APP_INIT:
                if ( !initApp() ) {
                  return ( EXIT_FAILURE );
                }
                app_state = APP_INIT_DATA;
                break;
            case APP_INIT_DATA:
                data_initialized = initData();
                app_state = APP_RUN;
                break;
            case APP_RUN:
                serverRun(&app_state, data_initialized);
                break;
            case APP_STOP:
                freeData();
                data_initialized = 0;
                app_state = APP_RUN;
                break;
            case APP_RESET:
                freeApp();
                data_initialized = 0;
                app_state = APP_INIT;
                break;
            case APP_EXIT:
                exit_nicely();
                break;
            default:
                freeApp();
                putsde ( "unknown application state\n" );
                return  ( EXIT_FAILURE );
        }
    }
    freeApp();
    return (EXIT_SUCCESS);
}

