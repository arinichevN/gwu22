
#ifndef LIBPAS_MAX6675_H
#define LIBPAS_MAX6675_H

#include "gpio.h"

extern int max6675_init(int sclk, int cs, int miso);

extern int max6675_read(float *result, int sclk, int cs, int miso );

#endif 

