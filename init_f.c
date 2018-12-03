#include <string.h>

#include "main.h"

int readSettings(int *sock_port, int *retry_count, const char *data_path) {
    TSVresult tsv = TSVRESULT_INITIALIZER;
    TSVresult* r = &tsv;
    if (!TSVinit(r, data_path)) {
        TSVclear(r);
        return 0;
    }
    int p = TSVgetis(r, 0, "port");
    int c = (unsigned int) TSVgetis(r, 0, "retry_count");
    if (TSVnullreturned(r)) {
        return 0;
    }
    TSVclear(r);
    *sock_port = p;
    *retry_count = c;
    return 1;
}

int initDevice(DeviceList *list, DItemList *dl, int retry_count, const char *data_path) {
    TSVresult tsv = TSVRESULT_INITIALIZER;
    TSVresult* r = &tsv;
    if (!TSVinit(r, data_path)) {
        TSVclear(r);
        return 0;
    }
    int n = TSVntuples(r);
    if (n <= 0) {
        TSVclear(r);
        return 1;
    }
    RESIZE_M_LIST(list, n);
    if (LML != n) {
        putsde( "failure while resizing list\n");
        TSVclear(r);
        return 0;
    }
    NULL_LIST(list);
    for (int i = 0; i < LML; i++) {
        LIi.pin = TSVgetis(r, i, "pin");
        LIi.temp_id = TSVgetis(r, i, "temp_id");
        LIi.hum_id = TSVgetis(r, i, "hum_id");
        LIi.retry_count = retry_count;
        LIi.tm.tv_sec = 0;
        LIi.tm.tv_nsec = 0;
        LIi.retry_count = retry_count;
        ton_ts_touch(&LIi.read_tmr);
        SET_READ_INTERVAL(LIi.read_interval);
        if (TSVnullreturned(r)) {
            break;
        }
        LL++;
    }
    TSVclear(r);
    if (LL != LML) {
        putsde("failure while reading rows\n");
        return 0;
    }
    size_t dll=list->length * 2;
    RESIZE_M_LIST(dl, dll);
    if (dl->max_length != dll) {
        putsde( "failure while resizing device item list\n");
        return 0;
    }
    NULL_LIST(dl);
    dl->length=dll;
#define INDT i*2
#define INDH i*2+1
    FORLi{
        dl->item[INDT].id = LIi.temp_id;
        dl->item[INDT].device = &LIi;
        dl->item[INDT].value_state = 0;
        LIi.t = &dl->item[INDT];

        dl->item[INDH].id = LIi.hum_id;
        dl->item[INDH].device = &LIi;
        dl->item[INDH].value_state = 0;
        LIi.h = &dl->item[INDH];
    }
#undef INDT
#undef INDH
    return 1;
}

int assignMod(DItemList *list, LCorrectionList *lcl, const char *data_path) {
    TSVresult tsv = TSVRESULT_INITIALIZER;
    TSVresult* r = &tsv;
    if (!TSVinit(r, data_path)) {
        TSVclear(r);
        return 0;
    }
    int n = TSVntuples(r);
    if (n <= 0) {
        TSVclear(r);
        return 1;
    }
    for (int i = 0; i < n; i++) {
        int net_id = TSVgetis(r, i, "net_id");
        int lcorrection_id = TSVgetis(r, i, "lcorrection_id");
        if (TSVnullreturned(r)) {
            printde("row %d: bad format\n", i);
            break;
        }
        DItem *ditem;
        LIST_GETBYID(ditem, list, net_id);
        if (ditem == NULL) {
            printde( "row %d: device item for net_id=%d not found\n", i, net_id);
            continue;
        }
        LCorrection * lc;
        LIST_GETBYID(lc, lcl, lcorrection_id);
        if (lc == NULL) {
            printde("row %d: lcorrection not found for net_id=%d and lcorrection_id=%d\n", i, net_id, lcorrection_id);
            continue;
        }
        ditem->lcorrection = lc;
    }
    TSVclear(r);
    return 1;
}
