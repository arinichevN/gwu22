
#include "max31855.h"

int max31855_init(int sclk, int cs, int miso) {
    pinModeOut(cs);
    pinModeOut(sclk);
    pinModeIn(miso);
    pinHigh(cs);
    return 1;
}

int max31855_read(float *result, int sclk, int cs, int miso) {
    uint32_t v;
    pinLow(cs);
    delayUsBusy(1000);
    {
        int i;
        for (i = 31; i >= 0; i--) {
            pinLow(sclk);
            delayUsBusy(1000);
            if (pinRead(miso)) {
                v |= (1 << i);
            }
            pinHigh(sclk);
            delayUsBusy(1000);
        }
    }
    pinHigh(cs);
    if (v & 0x4) {
#ifdef MODE_DEBUG
        fprintf(stderr, "max31855_read: thermocouple is short-circuited to VCC where sclk=%d and cs=%d and miso=%d\n", sclk, cs, miso);
#endif
        return 0;
    }
    if (v & 0x2) {
#ifdef MODE_DEBUG
        fprintf(stderr, "max31855_read: thermocouple is short-circuited to GND where sclk=%d and cs=%d and miso=%d\n", sclk, cs, miso);
#endif
        return 0;
    }
    if (v & 0x1) {
#ifdef MODE_DEBUG
        fprintf(stderr, "max31855_read: thermocouple input is open where sclk=%d and cs=%d and miso=%d\n", sclk, cs, miso);
#endif
        return 0;
    }
    if (v & 0x8000) {
#ifdef MODE_DEBUG
        fprintf(stderr, "max31855_read: fault expected but not found where sclk=%d and cs=%d and miso=%d\n", sclk, cs, miso);
#endif
        return 0;
    }
    if (v & 0x80000000) {
        v = 0xFFFFC000 | ((v >> 18) & 0x00003FFFF);
    } else {
        v >>= 18;
    }
    *result = *result * 0.25;
    return 1;
}

