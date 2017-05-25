
#include "main.h"

FUN_LIST_GET_BY_ID(DItem)

int sendStrPack(char qnf, char *cmd) {
    extern Peer peer_client;
    return acp_sendStrPack(qnf, cmd,  &peer_client);
}

int sendBufPack(char *buf, char qnf, char *cmd_str) {
    extern Peer peer_client;
    return acp_sendBufPack(buf, qnf, cmd_str,  &peer_client);
}

void sendStr(const char *s, uint8_t *crc) {
    acp_sendStr(s, crc, &peer_client);
}

void sendFooter(int8_t crc) {
    acp_sendFooter(crc, &peer_client);
}

int catFTS(DItem *item, char *buf, size_t buf_size) {
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR, item->id, item->value, item->device->tm.tv_sec, item->device->tm.tv_nsec, item->value_state);
    if (bufCat(buf, q, buf_size) == NULL) {
        sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
        return 0;
    }
    return 1;
}

void printData(DeviceList *dl, DItemList *il) {
    int i = 0;
    char q[LINE_SIZE];
    uint8_t crc = 0;
    snprintf(q, sizeof q, "CONFIG_FILE: %s\n", CONFIG_FILE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "tsv device file: %s\n", DEVICE_FILE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "pid_path: %s\n", pid_path);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "sock_buf_size: %d\n", sock_buf_size);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "DEVICE_FILE: %s\n", DEVICE_FILE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "app_state: %s\n", getAppState(app_state));
    sendStr(q, &crc);
    snprintf(q, sizeof q, "PID: %d\n", proc_id);
    sendStr(q, &crc);

    sendStr("+-----------------------------------------------------------------------+\n", &crc);
    sendStr("|                              device                                   |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("|  pointer  |    pin    |    t_id   |    h_id   |   t_pt    |  h_pt     |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    for (i = 0; i < dl->length; i++) {
        snprintf(q, sizeof q, "|%11p|%11d|%11d|%11d|%11p|%11p|\n",
                &dl->item[i],
                dl->item[i].pin,
                dl->item[i].t_id,
                dl->item[i].h_id,
                dl->item[i].t,
                dl->item[i].h
                );
        sendStr(q, &crc);
    }
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);

    sendStr("+-----------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                  device item                                      |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("|  pointer  |     id    |   value   |value_state| time_sec  | time_nsec | device_pt |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    for (i = 0; i < il->length; i++) {
        snprintf(q, sizeof q, "|%11p|%11d|%11f|%11d|%11ld|%11ld|%11p|\n",
                &il->item[i],
                il->item[i].id,
                il->item[i].value,
                il->item[i].value_state,
                il->item[i].device->tm.tv_sec,
                il->item[i].device->tm.tv_nsec,
                il->item[i].device
                );
        sendStr(q, &crc);
    }
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendFooter(crc);
}

void printHelp() {
    char q[LINE_SIZE];
    uint8_t crc = 0;
    sendStr("COMMAND LIST\n", &crc);
    snprintf(q, sizeof q, "%c\tput process into active mode; process will read configuration\n", ACP_CMD_APP_START);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tput process into standby mode; all running programs will be stopped\n", ACP_CMD_APP_STOP);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tfirst stop and then start process\n", ACP_CMD_APP_RESET);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tterminate process\n", ACP_CMD_APP_EXIT);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget state of process; response: B - process is in active mode, I - process is in standby mode\n", ACP_CMD_APP_PING);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget some variable's values; response will be packed into multiple packets\n", ACP_CMD_APP_PRINT);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget this help; response will be packed into multiple packets\n", ACP_CMD_APP_HELP);
    sendStr(q, &crc);

    snprintf(q, sizeof q, "%c\tget temperature in format: sensorId\\ttemperature\\ttimeSec\\ttimeNsec\\tvalid; program id expected if '.' quantifier is used\n", ACP_CMD_GET_FTS);
    sendStr(q, &crc);
    sendFooter(crc);
}