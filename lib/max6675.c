
#include "max6675.h"

int max6675_init(int sclk, int cs, int miso) {
    pinModeOut(cs);
    pinModeOut(sclk);
    pinModeIn(miso);
    pinHigh(cs);
    return 1;
}

int max6675_read(float *result, int sclk, int cs, int miso) {
    uint16_t v;
    pinLow(cs);
    delayUsBusy(1000);
    {
        int i;
        for (i = 15; i >= 0; i--) {
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
        fputs("max6675_read: thermocouple input is open\n", stderr);
#endif
        return 0;
    }
    v >>= 3;
    *result = v * 0.25;
    return 1;
}

