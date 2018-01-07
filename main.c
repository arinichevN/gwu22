#include "main.h"

char pid_path[LINE_SIZE];
int app_state = APP_INIT;
int pid_file = -1;
int proc_id = -1;
int data_initialized = 0;
int sock_port = -1;
int sock_fd = -1; //socket file descriptor
Peer peer_client = {.fd = &sock_fd, .addr_size = sizeof peer_client.addr};

unsigned int retry_count = 0;

I1List i1l;
DeviceList device_list = {NULL, 0};
DItemList ditem_list = {NULL, 0};

#include "util.c"
#include "init_f.c"

int checkDevice(DeviceList *list, DItemList *ilist) {
    size_t i, j;
    //valid pin address
    FORL{
        if (!checkPin(LIi.pin)) {
            fprintf(stderr, "checkDevice: check device table: bad pin=%d where pin=%d\n", LIi.pin, LIi.pin);
            return 0;
        }
    }
    //unique pin
    FORL{
        for (j = i + 1; j < list->length; j++) {
            if (LIi.pin == list->item[j].pin) {
                fprintf(stderr, "checkDevice: check device table: pins should be unique, repetition found where pin=%d\n", LIi.pin);
                return 0;
            }
        }
    }
    //unique id
    for (i = 0; i < ilist->length; i++) {
        for (j = i + 1; j < ilist->length; j++) {
            if (ilist->item[i].id == ilist->item[j].id) {
                fprintf(stderr, "checkDevice: check device table: h_id and t_id should be unique, repetition found where pin=%d\n", ilist->item[i].device->pin);
                return 0;
            }
        }
    }
    return 1;
}

void lcorrect(DItem *item) {
    if (item->lcorrection.active) {
        item->value = item->value * item->lcorrection.factor + item->lcorrection.delta;
    }
}

void readDevice(Device *item) {
    if (!ton_ts(item->read_interval, &item->read_tmr)) {
        return;
    }
    item->t->value_state = 0;
    item->h->value_state = 0;
    for (int i = 0; i < item->retry_count; i++) {
#ifdef MODE_DEBUG
        printf("reading from pin: %d\n", item->pin);
#endif
#ifndef CPU_ANY
        if (dht22_read(item->pin, &item->t->value, &item->h->value)) {
            item->tm = getCurrentTime();
            item->t->value_state = 1;
            item->h->value_state = 1;
            lcorrect(item->t);
            lcorrect(item->h);
            return;
        }
        delayUsIdle(400000);
#endif
#ifdef CPU_ANY
        item->t->value = item->h->value = 0.0f;
        item->tm = getCurrentTime();
        item->t->value_state = 1;
        item->h->value_state = 1;
        lcorrect(item->t);
        lcorrect(item->h);
        return;
#endif

    }
}

void serverRun(int *state, int init_state) {
    SERVER_HEADER
    SERVER_APP_ACTIONS
    if (ACP_CMD_IS(ACP_CMD_GET_FTS)) {
        acp_requestDataToI1List(&request, &i1l);
        if (i1l.length <= 0) {
            return;
        }
        for (int i = 0; i < i1l.length; i++) {
            DItem *ditem = getDItemById(i1l.item[i], &ditem_list);
            if (ditem != NULL) {
                readDevice(ditem->device);
                if (!catFTS(ditem, &response)) {
                    return;
                }
            }
        }
    }
    acp_responseSend(&response, &peer_client);
}

void initApp() {
    if (!readSettings()) {
        exit_nicely_e("initApp: failed to read settings\n");
    }
    //  peer_client.sock_buf_size = ACP_BUFFER_MAX_SIZE;
    if (!initPid(&pid_file, &proc_id, pid_path)) {
        exit_nicely_e("initApp: failed to initialize pid\n");
    }
    if (!initServer(&sock_fd, sock_port)) {
        exit_nicely_e("initApp: failed to initialize server\n");
    }
    if (!gpioSetup()) {
        exit_nicely_e("initApp: failed to initialize GPIO\n");
    }
}

int initData() {
    if (!initDevice(&device_list, &ditem_list, retry_count)) {
        FREE_LIST(&ditem_list);
        FREE_LIST(&device_list);
        return 0;
    }
    if (!checkDevice(&device_list, &ditem_list)) {
        FREE_LIST(&ditem_list);
        FREE_LIST(&device_list);
        return 0;
    }
    if (!initDeviceLCorrection(&ditem_list)) {
        ;
    }
    if (!initI1List(&i1l, ditem_list.length)) {
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

