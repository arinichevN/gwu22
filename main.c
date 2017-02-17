/*
 * UPD gateway for DHT22
 */

#include "main.h"

char app_class[NAME_SIZE];
char pid_path[LINE_SIZE];
int app_state = APP_INIT;
int pid_file = -1;
int proc_id = -1;
struct timespec cycle_duration = {0, 0};
int data_initialized = 0;
int udp_port = -1;
int udp_fd = -1; //udp socket file descriptor
size_t udp_buf_size = 0;
struct timespec interval = REQUEST_INTERVAL;
Peer peer_client = {.fd = &udp_fd, .addr_size = sizeof peer_client.addr};

char db_conninfo_settings[LINE_SIZE];
char db_conninfo_data[LINE_SIZE];

#ifndef FILE_INI
PGconn *db_conn_settings = NULL;
PGconn *db_conn_data = NULL;
#endif

I1List i1l = {NULL, 0};
DeviceList device_list = {NULL, 0};
DItemList ditem_list = {NULL, 0};

#include "util.c"

#ifdef FILE_INI
#pragma message("MESSAGE: data from file")
#include "init_f.c"
#else
#pragma message("MESSAGE: data from DBMS")
#include "init_d.c"
#endif

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

void readDevice(Device *item, struct timespec interval) {
    struct timespec now = getCurrentTime();
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
    char buf_in[udp_buf_size];
    char buf_out[udp_buf_size];
    uint8_t crc;
    size_t i, j;
    char q[LINE_SIZE];
    crc = 0;
    memset(buf_in, 0, sizeof buf_in);
    acp_initBuf(buf_out, sizeof buf_out);
    if (recvfrom(udp_fd, buf_in, sizeof buf_in, 0, (struct sockaddr*) (&(peer_client.addr)), &(peer_client.addr_size)) < 0) {
#ifdef MODE_DEBUG
        perror("serverRun: recvfrom() error");
#endif
    }
#ifdef MODE_DEBUG
    dumpBuf(buf_in, sizeof buf_in);
#endif    
    if (!crc_check(buf_in, sizeof buf_in)) {
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
                        readDevice(&device_list.item[i], interval);
                    }
                    for (i = 0; i < ditem_list.length; i++) {
                        snprintf(q, sizeof q, "%d_%f_%ld_%ld_%d\n", ditem_list.item[i].id, ditem_list.item[i].value, ditem_list.item[i].device->tm.tv_sec, ditem_list.item[i].device->tm.tv_nsec, ditem_list.item[i].value_state);
                        if (bufCat(buf_out, q, udp_buf_size) == NULL) {
                            sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
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
                            readDevice(ditem->device, interval);
                            snprintf(q, sizeof q, "%d_%f_%ld_%ld_%d\n", ditem->id, ditem->value, ditem->device->tm.tv_sec, ditem->device->tm.tv_nsec, ditem->value_state);
                            if (bufCat(buf_out, q, udp_buf_size) == NULL) {
                                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
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
    if (!readConf(CONFIG_FILE_DB, db_conninfo_settings, app_class)) {
        exit_nicely_e("initApp: failed to read configuration file\n");
    }
    if (!readSettings()) {
        exit_nicely_e("initApp: failed to read settings\n");
    }
    if (!initPid(&pid_file, &proc_id, pid_path)) {
        exit_nicely_e("initApp: failed to initialize pid\n");
    }
    if (!initUDPServer(&udp_fd, udp_port)) {
        exit_nicely_e("initApp: failed to initialize udp server\n");
    }
#ifndef PLATFORM_ANY
    if (!gpioSetup()) {
        exit_nicely_e("initApp: failed to initialize GPIO\n");
    }
#endif
}

int initData() {
#ifndef FILE_INI
    if (!initDB(&db_conn_data, db_conninfo_data)) {
        return 0;
    }
#endif
    if (!initDevice(&device_list, &ditem_list)) {
        FREE_LIST(&ditem_list);
        FREE_LIST(&device_list);
#ifndef FILE_INI
        freeDB(db_conn_data);
#endif
        return 0;
    }
    if (!checkDevice(&device_list, &ditem_list)) {
        FREE_LIST(&ditem_list);
        FREE_LIST(&device_list);
#ifndef FILE_INI
        freeDB(db_conn_data);
#endif
        return 0;
    }
    i1l.item = (int *) malloc(udp_buf_size * sizeof *(i1l.item));
    if (i1l.item == NULL) {
        FREE_LIST(&ditem_list);
        FREE_LIST(&device_list);
#ifndef FILE_INI
        freeDB(db_conn_data);
#endif
        return 0;
    }
    return 1;
}

void freeData() {
    FREE_LIST(&i1l);
    FREE_LIST(&ditem_list);
    FREE_LIST(&device_list);
#ifndef FILE_INI
    freeDB(db_conn_data);
#endif
}

void freeApp() {
    freeData();
#ifndef PLATFORM_ANY
    gpioFree();
#endif
    freeSocketFd(&udp_fd);
#ifndef FILE_INI
    freeDB(db_conn_settings);
#endif
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

