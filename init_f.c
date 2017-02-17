#include <string.h>

int readSettings() {
    FILE* stream = fopen(CONFIG_FILE, "r");
    if (stream == NULL) {
        fputs("ERROR: readSettings: fopen", stderr);
        return 0;
    }

    int n;
    n = fscanf(stream, "%d\t%255s\t%d\t", &udp_port, pid_path, &udp_buf_size);
    if (n != 3) {
        fclose(stream);
        fputs("ERROR: readSettings: bad row format\n", stderr);
        return 0;
    }
    fclose(stream);
#ifdef MODE_DEBUG
    printf("readSettings: udp_port: %d, pid_path: %s, udp_buf_size: %d\n", udp_port, pid_path, udp_buf_size);
#endif
    return 1;
}

#define DEVICE_ROW_FORMAT "%d\t%d\t%d\n"
#define DEVICE_FIELD_COUNT 3

#define INDT i*2
#define INDH i*2+1

int initDevice(DeviceList *list, DItemList *dl) {
    FILE* stream = fopen(DEVICE_FILE, "r");
    if (stream == NULL) {
        fputs("ERROR: initDevice: fopen\n", stderr);
        return 0;
    }
    int rnum = 0;
    while (1) {
        int n = 0, x1, x2, x3;
        n = fscanf(stream, DEVICE_ROW_FORMAT, &x1, &x2, &x3);
        if (n != DEVICE_FIELD_COUNT) {
            break;
        }
#ifdef MODE_DEBUG
        printf("initDevice: count: pin = %d, t_id = %d, h_id = %d\n", x1, x2, x3);
#endif
        rnum++;

    }
    rewind(stream);
size_t i;
    list->length = rnum;
    if (list->length > 0) {
        list->item = (Device *) malloc(list->length * sizeof *(list->item));
        if (list->item == NULL) {
            list->length = 0;
            fputs("ERROR: initDevice: failed to allocate memory for pins\n", stderr);
            fclose(stream);
            return 0;
        }
        int done = 1;
        FORL{
            int n;
            n = fscanf(stream, DEVICE_ROW_FORMAT,
            &LIi.pin,
            &LIi.t_id,
            &LIi.h_id
            );
            if (n != DEVICE_FIELD_COUNT) {
                done = 0;
            }
#ifdef MODE_DEBUG
            printf("initDevice: read: pin = %d, t_id = %d, h_id = %d\n", LIi.pin, LIi.t_id, LIi.h_id);
#endif
            LIi.tm.tv_sec = 0;
            LIi.tm.tv_nsec = 0;
        }
        if (!done) {
            fclose(stream);
            fputs("ERROR: initDevice: failure while reading rows\n", stderr);
            return 0;
        }
    }
    dl->length = list->length * 2;
    dl->item = (DItem *) malloc(dl->length * sizeof *(dl->item));
    if (dl->item == NULL) {
        dl->length = 0;
        fputs("initDevice: failed to allocate memory for DItem\n", stderr);
        fclose(stream);
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
    fclose(stream);
    return 1;
}
