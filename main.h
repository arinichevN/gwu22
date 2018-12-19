
#ifndef GWU22_H
#define GWU22_H

#include "lib/app.h"
#include "lib/gpio.h"
#include "lib/timef.h"
#include "lib/device/dht22.h"
#include "lib/tsv.h"
#include "lib/lcorrection.h"
#include "lib/acp/main.h"
#include "lib/acp/app.h"
#include "lib/udp.h"

#define APP_NAME gwu22
#define APP_NAME_STR TOSTRING(APP_NAME)

#ifdef MODE_FULL
#define CONF_DIR "/etc/controller/" APP_NAME_STR "/"
#endif
#ifndef MODE_FULL
#define CONF_DIR "./config/"
#endif

#define CONF_MAIN_FILE CONF_DIR "main.tsv"
#define CONF_DEVICE_FILE CONF_DIR "device.tsv"
#define CONF_LCORRECTION_FILE CONF_DIR "lcorrection.tsv"
#define CONF_MOD_MAPPING_FILE CONF_DIR "mod_mapping.tsv"

#define SET_READ_INTERVAL(V) V.tv_sec=1; V.tv_nsec=0;
#define READ_DELAY_US 400000


struct device_st {
    int pin;
    int temp_id;
    int hum_id;
    struct ditem_st *t;
    struct ditem_st *h;
    struct timespec tm; //measurement time
    
    //we will read from this device with a certain frequency
    Ton_ts read_tmr;
    struct timespec read_interval;
    
    int retry_count;
};

struct ditem_st {
    int id;
    struct device_st *device;
    double value;
    int value_state; //0 if reading value from device failed
    LCorrection *lcorrection;
};

typedef struct device_st Device;
typedef struct ditem_st DItem;

DEC_LIST(Device)
DEC_LIST(DItem)


extern int readSettings();

extern void serverRun(int *state, int init_state);

extern int initApp();

extern int initData();

extern void freeData();

extern void freeApp();

extern void exit_nicely();

#endif

