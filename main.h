
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
#define LCORRECTION_FILE "" CONF_DIR "lcorrection.tsv"

#define SET_READ_INTERVAL(V) V.tv_sec=1; V.tv_nsec=0;

enum {
    ON = 1,
    OFF,
    DO,
    INIT,
    WTIME
} StateAPP;

typedef struct {
    int active;
    float factor;
    float delta;
} LCORRECTION;

struct device_st {
    int pin;
    int t_id;
    int h_id;
    struct ditem_st *t;
    struct ditem_st *h;
    struct timespec tm; //measurement time
    
    //we will read from this device with a certain frequency
    Ton_ts read_tmr;
    struct timespec read_interval;
    
    unsigned int retry_count;
};

struct ditem_st {
    int id;
    struct device_st *device;
    float value;
    int value_state; //0 if reading value from device failed
    LCORRECTION lcorrection;
};

typedef struct device_st Device;
typedef struct ditem_st DItem;

DEC_LIST(Device)
DEC_LIST(DItem)


extern int readSettings();

extern int initDevice(DeviceList *list, DItemList *dl, unsigned int retry_count);

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

