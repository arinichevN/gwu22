
#ifndef GWU22_H
#define GWU22_H

#include "lib/app.h"
#include "lib/gpio.h"
#include "lib/timef.h"
#include "lib/dht22.h"
#include "lib/acp/main.h"
#include "lib/acp/app.h"
#include "lib/udp.h"

#define APP_NAME gwu22
#define APP_NAME_STR TOSTRING(APP_NAME)

#ifdef MODE_FULL
#define CONF_DIR "/etc/controller/" APP_NAME_STR "/"
#endif
#ifndef MODE_FULL
#define CONF_DIR "./"
#endif

#define DEVICE_FILE "" CONF_DIR "device.tsv"
#define CONFIG_FILE "" CONF_DIR "config.tsv"

#define RETRY_NUM 5

#define FLOAT_NUM "%.3f"

enum {
    ON = 1,
    OFF,
    DO,
    INIT,
    WTIME
} StateAPP;

struct device_st {
    int pin;
    int t_id;
    int h_id;
    struct ditem_st *t;
    struct ditem_st *h;
    struct timespec tm; //measurement time
};

struct ditem_st {
    int id;
    struct device_st *device;
    float value;
    int value_state; //0 if reading value from device failed
};

typedef struct device_st Device;
typedef struct ditem_st DItem;

DEF_LIST(Device)
DEF_LIST(DItem)


extern int readSettings();

extern int initDevice(DeviceList *list, DItemList *dl);

extern int checkDevice(DeviceList *list, DItemList *ilist);

extern void readDevice(Device *item);

extern void serverRun(int *state, int init_state);

extern void initApp();

extern int initData();

extern void freeData();

extern void freeApp();

extern void exit_nicely();

extern void exit_nicely_e(char *s);

#endif

