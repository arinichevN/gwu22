
#include "main.h"

FUN_LIST_GET_BY_ID(DItem)

int catFTS(DItem *item, ACPResponse *response) {
    return acp_responseFTSCat(item->id, item->value, item->device->tm, item->value_state, response);

}

void printData(ACPResponse *response) {
    DeviceList *dl = &device_list;
    DItemList *il = &ditem_list;
    int i = 0;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "CONFIG_FILE: %s\n", CONFIG_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "DEVICE_FILE: %s\n", DEVICE_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "LCORRECTION_FILE: %s\n", LCORRECTION_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    SEND_STR(q)
    snprintf(q, sizeof q, "pid_path: %s\n", pid_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "DEVICE_FILE: %s\n", DEVICE_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "app_state: %s\n", getAppState(app_state));
    SEND_STR(q)
    snprintf(q, sizeof q, "PID: %d\n", proc_id);
    SEND_STR(q)
    snprintf(q, sizeof q, "device_list.length: %d\n", dl->length);
    SEND_STR(q)
    snprintf(q, sizeof q, "ditem_list.length: %d\n", il->length);
    SEND_STR(q)

    SEND_STR("+-----------------------------------------------------------------------+\n")
    SEND_STR("|                              device                                   |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|  pointer  |    pin    |    t_id   |    h_id   |   t_pt    |  h_pt     |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    for (i = 0; i < dl->length; i++) {
        snprintf(q, sizeof q, "|%11p|%11d|%11d|%11d|%11p|%11p|\n",
                (void *) &dl->item[i],
                dl->item[i].pin,
                dl->item[i].t_id,
                dl->item[i].h_id,
                (void *) dl->item[i].t,
                (void *) dl->item[i].h
                );
        SEND_STR(q)
    }
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+\n")

    SEND_STR("+-----------------------------------------------------------------------------------+\n")
    SEND_STR("|                                  device item                                      |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|  pointer  |     id    |   value   |value_state| time_sec  | time_nsec | device_pt |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    for (i = 0; i < il->length; i++) {
        snprintf(q, sizeof q, "|%11p|%11d|%11f|%11d|%11ld|%11ld|%11p|\n",
                (void *) &il->item[i],
                il->item[i].id,
                il->item[i].value,
                il->item[i].value_state,
                il->item[i].device->tm.tv_sec,
                il->item[i].device->tm.tv_nsec,
                (void *) il->item[i].device
                );
        SEND_STR(q)
    }
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")

    SEND_STR("+-----------------------------------------------+\n")
    SEND_STR("|                   correction                  |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+\n")
    SEND_STR("| device_id |  factor   |   delta   |  active   |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+\n")
    for (i = 0; i < il->length; i++) {
        snprintf(q, sizeof q, "|%11d|%11f|%11f|%11d|\n",
                il->item[i].id,
                il->item[i].lcorrection.factor,
                il->item[i].lcorrection.delta,
                il->item[i].lcorrection.active
                );
        SEND_STR(q)
    }
    SEND_STR_L("+-----------+-----------+-----------+-----------+\n")
}

void printHelp(ACPResponse *response) {
    char q[LINE_SIZE];
    SEND_STR("COMMAND LIST\n")
    snprintf(q, sizeof q, "%s\tput process into active mode; process will read configuration\n", ACP_CMD_APP_START);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tput process into standby mode; all running programs will be stopped\n", ACP_CMD_APP_STOP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tfirst stop and then start process\n", ACP_CMD_APP_RESET);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tterminate process\n", ACP_CMD_APP_EXIT);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget state of process; response: B - process is in active mode, I - process is in standby mode\n", ACP_CMD_APP_PING);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget some variable's values; response will be packed into multiple packets\n", ACP_CMD_APP_PRINT);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget this help; response will be packed into multiple packets\n", ACP_CMD_APP_HELP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget temperature in format: sensorId\\ttemperature\\ttimeSec\\ttimeNsec\\tvalid; program id expected\n", ACP_CMD_GET_FTS);
    SEND_STR_L(q)
}