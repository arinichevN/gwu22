/*
 * UPD gateway for DHT22
 */

#include "main.h"

char app_class[NAME_SIZE];
char pid_path[LINE_SIZE];
int app_state = APP_INIT;
int pid_file = -1;
int proc_id = -1;
int data_initialized = 0;
int sock_port = -1;
int sock_fd = -1; //socket file descriptor
size_t sock_buf_size = 0;
Peer peer_client = {.fd = &sock_fd, .addr_size = sizeof peer_client.addr};

char db_conninfo_settings[LINE_SIZE];
char db_conninfo_data[LINE_SIZE];


I1List i1l = {NULL, 0};
DeviceList device_list = {NULL, 0};
DItemList ditem_list = {NULL, 0};

#include "util.c"
#include "init_f.c"



#ifdef PLATFORM_ANY
#pragma message("MESSAGE: building for all platforms")
#else
#pragma message("MESSAGE: building for certain platform")
#endif

int checkDevice(DeviceList *list, DItemList *ilist) {
    size_t i, j;
#ifndef PLATFORM_ANY
    //valid pin address
    FORL{
        if (!checkPin(LIi.pin)) {
            fprintf(stderr, "checkDevice: check device table: bad pin=%d where app_class='%s' and pin=%d\n", LIi.pin, app_class, LIi.pin);
            return 0;
        }
    }
#endif
    //unique pin
    FORL{
        for (j = i + 1; j < list->length; j++) {
            if (LIi.pin == list->item[j].pin) {
                fprintf(stderr, "checkDevice: check device table: pins should be unique, repetition found where pin=%d and app_class='%s'\n", LIi.pin, app_class);
                return 0;
            }
        }
    }
    //unique id
    for (i = 0; i < ilist->length; i++) {
        for (j = i + 1; j < ilist->length; j++) {
            if (ilist->item[i].id == ilist->item[j].id) {
                fprintf(stderr, "checkDevice: check device table: h_id and t_id should be unique, repetition found where pin=%d and app_class='%s'\n", ilist->item[i].device->pin, app_class);
                return 0;
            }
        }
    }
    return 1;
}

void readDevice(Device *item) {
    int i;
    item->t->value_state = 0;
    item->h->value_state = 0;
    for (i = 0; i < RETRY_NUM; i++) {
#ifdef MODE_DEBUG
        printf("reading from pin: %d\n", item->pin);
#endif
#ifndef PLATFORM_ANY
        if (dht22_read(item->pin, &item->t->value, &item->h->value)) {
            item->tm = getCurrentTime();
            item->t->value_state = 1;
            item->h->value_state = 1;
            return;
        }
        delayUsIdle(400000);
#endif
#ifdef PLATFORM_ANY
        item->t->value = item->h->value = 0.0f;
        item->tm = getCurrentTime();
        item->t->value_state = 1;
        item->h->value_state = 1;
        return;
#endif

    }
}

void serverRun(int *state, int init_state) {
    char buf_in[sock_buf_size];
    char buf_out[sock_buf_size];
    size_t i, j;
    memset(buf_in, 0, sizeof buf_in);
    acp_initBuf(buf_out, sizeof buf_out);

    if (!serverRead((void *) buf_in, sizeof buf_in, sock_fd, (struct sockaddr*) (&(peer_client.addr)), &(peer_client.addr_size))) {
        return;
    }

    /*
            if (recvfrom(sock_fd, (void *) buf_in, sizeof buf_in, 0, (struct sockaddr*) (&(peer_client.addr)), &(peer_client.addr_size)) < 0) {
    #ifdef MODE_DEBUG
            perror("serverRun: recvfrom() error");
    #endif
            return;
        }
     */
#ifdef MODE_DEBUG
    acp_dumpBuf(buf_in, sizeof buf_in);
#endif    
    if (!acp_crc_check(buf_in, sizeof buf_in)) {
#ifdef MODE_DEBUG
        fputs("serverRun: crc check failed\n", stderr);
#endif
        return;
    }
    switch (buf_in[1]) {
        case ACP_CMD_APP_START:
            if (!init_state) {
                *state = APP_INIT_DATA;
            }
            return;
        case ACP_CMD_APP_STOP:
            if (init_state) {
                *state = APP_STOP;
            }
            return;
        case ACP_CMD_APP_RESET:
            *state = APP_RESET;
            return;
        case ACP_CMD_APP_EXIT:
            *state = APP_EXIT;
            return;
        case ACP_CMD_APP_PING:
            if (init_state) {
                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_APP_BUSY);
            } else {
                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_APP_IDLE);
            }
            return;
        case ACP_CMD_APP_PRINT:
            printData(&device_list, &ditem_list);
            return;
        case ACP_CMD_APP_HELP:
            printHelp();
            return;
        default:
            if (!init_state) {
                return;
            }
            break;
    }
    switch (buf_in[0]) {
        case ACP_QUANTIFIER_BROADCAST:
        case ACP_QUANTIFIER_SPECIFIC:
            break;
        default:
            return;
    }
    switch (buf_in[1]) {
        case ACP_CMD_GET_FTS:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                    for (i = 0; i < device_list.length; i++) {
                        readDevice(&device_list.item[i]);
                    }
                    for (i = 0; i < ditem_list.length; i++) {
                        if (!catFTS(&ditem_list.item[i], buf_out, sock_buf_size)) {
                            return;
                        }
                    }
                    break;
                case ACP_QUANTIFIER_SPECIFIC:
                    acp_parsePackI1(buf_in, &i1l, device_list.length); //id
                    if (i1l.length <= 0) {
                        return;
                    }
                    for (i = 0; i < i1l.length; i++) {
                        DItem *ditem = getDItemById(i1l.item[i], &ditem_list);
                        if (ditem != NULL) {
                            readDevice(ditem->device);
                            if (!catFTS(ditem, buf_out, sock_buf_size)) {
                                return;
                            }
                        }
                    }
                    break;
            }

            if (!sendBufPack(buf_out, ACP_QUANTIFIER_SPECIFIC, ACP_RESP_REQUEST_SUCCEEDED)) {
                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
                return;
            }
            break;
        default:
            return;
    }
}

void initApp() {
    if (!readSettings()) {
        exit_nicely_e("initApp: failed to read settings\n");
    }
    peer_client.sock_buf_size = sock_buf_size;
    if (!initPid(&pid_file, &proc_id, pid_path)) {
        exit_nicely_e("initApp: failed to initialize pid\n");
    }
    if (!initServer(&sock_fd, sock_port)) {
        exit_nicely_e("initApp: failed to initialize server\n");
    }
#ifndef PLATFORM_ANY
    if (!gpioSetup()) {
        exit_nicely_e("initApp: failed to initialize GPIO\n");
    }
#endif
}

int initData() {
    if (!initDevice(&device_list, &ditem_list)) {
        FREE_LIST(&ditem_list);
        FREE_LIST(&device_list);
        return 0;
    }
    if (!checkDevice(&device_list, &ditem_list)) {
        FREE_LIST(&ditem_list);
        FREE_LIST(&device_list);
        return 0;
    }
    i1l.item = (int *) malloc(sock_buf_size * sizeof *(i1l.item));
    if (i1l.item == NULL) {
        FREE_LIST(&ditem_list);
        FREE_LIST(&device_list);
        return 0;
    }
    return 1;
}

void freeData() {
    FREE_LIST(&i1l);
    FREE_LIST(&ditem_list);
    FREE_LIST(&device_list);
}

void freeApp() {
    freeData();
#ifndef PLATFORM_ANY
    gpioFree();
#endif
    freeSocketFd(&sock_fd);
    freePid(&pid_file, &proc_id, pid_path);
}

void exit_nicely() {
    freeApp();
    puts("\nBye...");
    exit(EXIT_SUCCESS);
}

void exit_nicely_e(char *s) {
    fprintf(stderr, "%s", s);
    freeApp();
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    if (geteuid() != 0) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s: root user expected\n", APP_NAME_STR);
#endif
        return (EXIT_FAILURE);
    }
#ifndef MODE_DEBUG
    daemon(0, 0);
#endif
    conSig(&exit_nicely);
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("main: memory locking failed");
    }
#ifndef MODE_DEBUG
    setPriorityMax(SCHED_FIFO);
#endif
    int data_initialized = 0;
    while (1) {
        switch (app_state) {
            case APP_INIT:
#ifdef MODE_DEBUG
                puts("MAIN: init");
#endif
                initApp();
                app_state = APP_INIT_DATA;
                break;
            case APP_INIT_DATA:
#ifdef MODE_DEBUG
                puts("MAIN: init data");
#endif
                data_initialized = initData();
                app_state = APP_RUN;
                break;
            case APP_RUN:
#ifdef MODE_DEBUG
                puts("MAIN: run");
#endif
                serverRun(&app_state, data_initialized);
                break;
            case APP_STOP:
#ifdef MODE_DEBUG
                puts("MAIN: stop");
#endif
                freeData();
                data_initialized = 0;
                app_state = APP_RUN;
                break;
            case APP_RESET:
#ifdef MODE_DEBUG
                puts("MAIN: reset");
#endif
                freeApp();
                data_initialized = 0;
                app_state = APP_INIT;
                break;
            case APP_EXIT:
#ifdef MODE_DEBUG
                puts("MAIN: exit");
#endif
                exit_nicely();
                break;
            default:
                exit_nicely_e("main: unknown application state");
                break;
        }
    }
    freeApp();
    return (EXIT_SUCCESS);
}

