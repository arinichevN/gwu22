
#include "main.h"

//FUN_LIST_GET_BY_ID(DItem)

int checkDevice(DeviceList *list, DItemList *ilist) {
    int valid = 1;

    FORLi {
        //valid pin address
        if (!checkPin(LIi.pin)) {
            fprintf(stderr, "%s(): check device table: bad pin=%d where pin=%d\n", F, LIi.pin, LIi.pin);
            valid = 0;
        }
        //unique pin
        for (size_t j = i + 1; j < list->length; j++) {
            if (LIi.pin == LIj.pin) {
                fprintf(stderr, "%s(): check device table: pins should be unique, repetition found where pin=%d\n", F, LIi.pin);
                valid = 0;
            }
        }
    }
    //unique id

    FORLISTP(ilist, i) {
        for (size_t j = i + 1; j < ilist->length; j++) {
            if (ilist->item[i].id == ilist->item[j].id) {
                fprintf(stderr, "%s(): check device table: h_id and t_id should be unique, repetition found where pin=%d\n", F, ilist->item[i].device->pin);
                valid = 0;
            }
        }
    }
    return valid;
}

void readDevice(Device *item) {
    if (!ton_ts(item->read_interval, &item->read_tmr)) {
        return;
    }
    item->t->value_state = 0;
    item->h->value_state = 0;
#ifdef CPU_ANY
    item->t->value = item->h->value = 0.0f;
    item->tm = getCurrentTime();
    item->t->value_state = 1;
    item->h->value_state = 1;
    lcorrect(&item->t->value, item->t->lcorrection);
    lcorrect(&item->h->value, item->h->lcorrection);
    return;
#endif
    for (int i = 0; i < item->retry_count; i++) {
        printdo("reading from pin: %d\n", item->pin);
        if (dht22_read(item->pin, &item->t->value, &item->h->value)) {
            item->tm = getCurrentTime();
            item->t->value_state = 1;
            item->h->value_state = 1;
            lcorrect(&item->t->value, item->t->lcorrection);
            lcorrect(&item->h->value, item->h->lcorrection);
            return;
        }
        delayUsIdle(READ_DELAY_US);
    }
}

int catFTS(DItem *item, ACPResponse *response) {
    return acp_responseFTSCat(item->id, item->value, item->device->tm, item->value_state, response);

}

void printData(ACPResponse *response) {
    DeviceList *dl = &device_list;
    DItemList *il = &ditem_list;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "CONFIG_FILE: %s\n", CONF_MAIN_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "DEVICE_FILE: %s\n", CONF_DEVICE_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "LCORRECTION_FILE: %s\n", CONF_LCORRECTION_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "CONF_MOD_MAPPING_FILE: %s\n", CONF_MOD_MAPPING_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    SEND_STR(q)
    snprintf(q, sizeof q, "app_state: %s\n", getAppState(app_state));
    SEND_STR(q)
    snprintf(q, sizeof q, "PID: %d\n", getpid());
    SEND_STR(q)

    SEND_STR("+-----------------------------------------------------------------------+\n")
    SEND_STR("|                              device                                   |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|  pointer  |    pin    |    t_id   |    h_id   |   t_pt    |  h_pt     |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    FORLISTP(dl, i) {
        snprintf(q, sizeof q, "|%11p|%11d|%11d|%11d|%11p|%11p|\n",
                (void *) &dl->item[i],
                dl->item[i].pin,
                dl->item[i].temp_id,
                dl->item[i].hum_id,
                (void *) dl->item[i].t,
                (void *) dl->item[i].h
                );
        SEND_STR(q)
    }
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+\n")

    SEND_STR("+-----------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                       device item                                             |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|  pointer  |     id    |   value   |value_state| time_sec  | time_nsec | device_pt | lcorr_ptr |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    FORLISTP(il, i) {
        snprintf(q, sizeof q, "|%11p|%11d|%11f|%11d|%11ld|%11ld|%11p|%11p|\n",
                (void *) &il->item[i],
                il->item[i].id,
                il->item[i].value,
                il->item[i].value_state,
                il->item[i].device->tm.tv_sec,
                il->item[i].device->tm.tv_nsec,
                (void *) il->item[i].device,
                (void *) il->item[i].lcorrection
                );
        SEND_STR(q)
    }
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    acp_sendLCorrectionListInfo(&lcorrection_list, response, &peer_client);
    SEND_STR_L("-\n")
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
