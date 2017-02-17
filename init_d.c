int readSettings() {
    PGresult *r;
    char q[LINE_SIZE];
    memset(pid_path, 0, sizeof pid_path);
    memset(db_conninfo_data, 0, sizeof db_conninfo_data);
    if (!initDB(&db_conn_settings, db_conninfo_settings)) {
        return 0;
    }
    snprintf(q, sizeof q, "select db_public from " APP_NAME_STR ".config where app_class='%s'", app_class);
    if ((r = dbGetDataT(db_conn_settings, q, "readSettings: select: ")) == NULL) {
        freeDB(db_conn_settings);
        return 0;
    }
    if (PQntuples(r) != 1) {
        PQclear(r);
        freeDB(db_conn_settings);
        fputs("readSettings: need only one tuple (1)\n", stderr);
        return 0;
    }
    char db_conninfo_public[LINE_SIZE];
    PGconn *db_conn_public = NULL;
    memcpy(db_conninfo_public, PQgetvalue(r, 0, 0), sizeof db_conninfo_public);
    PQclear(r);

    if (dbConninfoEq(db_conninfo_public, db_conninfo_settings)) {
        db_conn_public = db_conn_settings;
    } else {
        if (!initDB(&db_conn_public, db_conninfo_public)) {
            freeDB(db_conn_settings);
            return 0;
        }
    }

    snprintf(q, sizeof q, "select udp_port, udp_buf_size, pid_path, db_data from " APP_NAME_STR ".config where app_class='%s'", app_class);
    if ((r = dbGetDataT(db_conn_settings, q, "readSettings: select: ")) == NULL) {
        freeDB(db_conn_public);
        freeDB(db_conn_settings);
        return 0;
    }
    if (PQntuples(r) == 1) {
        int done = 1;
        done = done && config_getUDPPort(db_conn_public, PQgetvalue(r, 0, 0), &udp_port);
        done = done && config_getBufSize(db_conn_public, PQgetvalue(r, 0, 1), &udp_buf_size);
        done = done && config_getPidPath(db_conn_public, PQgetvalue(r, 0, 2), pid_path, LINE_SIZE);
        done = done && config_getDbConninfo(db_conn_public, PQgetvalue(r, 0, 3), db_conninfo_data, LINE_SIZE);
        if (!done) {
            PQclear(r);
            freeDB(db_conn_public);
            freeDB(db_conn_settings);
            fputs("readSettings: failed to read some fields\n", stderr);
            return 0;
        }
    } else {
        PQclear(r);
        freeDB(db_conn_public);
        freeDB(db_conn_settings);
        fputs("readSettings: need only one tuple (2)\n", stderr);
        return 0;
    }
    PQclear(r);
    freeDB(db_conn_public);
    freeDB(db_conn_settings);
    return 1;
}

#define INDT i*2
#define INDH i*2+1

int initDevice(DeviceList *list, DItemList *dl) {
    PGresult *r;
    char q[LINE_SIZE];
    size_t i;
    snprintf(q, sizeof q, "select pin, t_id, h_id from " APP_NAME_STR ".device where app_class='%s' order by pin", app_class);
    if ((r = dbGetDataT(db_conn_data, q, "initAppData: select pin: ")) == NULL) {
        return 0;
    }
    list->length = PQntuples(r);
    if (list->length > 0) {
        list->item = (Device *) malloc(list->length * sizeof *(list->item));
        if (list->item == NULL) {
            list->length = 0;
            fputs("initDevice: failed to allocate memory for Device\n", stderr);
            PQclear(r);
            return 0;
        }
        FORL{
            LIi.pin = atoi(PQgetvalue(r, i, 0));
            LIi.t_id = atoi(PQgetvalue(r, i, 1));
            LIi.h_id = atoi(PQgetvalue(r, i, 2));
            LIi.tm.tv_sec = 0;
            LIi.tm.tv_nsec = 0;
        }
    }
    PQclear(r);

    dl->length = list->length * 2;
    if (dl->length > 0) {
        dl->item = (DItem *) malloc(dl->length * sizeof *(dl->item));
        if (dl->item == NULL) {
            dl->length = 0;
            fputs("initDevice: failed to allocate memory for DItem\n", stderr);
            return 0;
        }
        FORL{
            dl->item[INDT].id = LIi.t_id;
            dl->item[INDT].device = &LIi;
            dl->item[INDT].value_state = 0;
            LIi.t = &dl->item[INDT];

            dl->item[INDH].id = LIi.h_id;
            dl->item[INDH].device = &LIi;
            dl->item[INDH].value_state = 0;
            LIi.h = &dl->item[INDH];
        }
    }
    return 1;
}

