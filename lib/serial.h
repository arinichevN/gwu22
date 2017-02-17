
#ifndef LIBPAS_SERIAL_H
#define LIBPAS_SERIAL_H

extern int serialOpen(const char *device, const int baud);

extern void serialFlush(const int fd);

extern void serialClose(const int fd);

extern int serialAvailable(const int fd);

#endif

