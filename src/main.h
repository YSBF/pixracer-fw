#ifndef MAIN_H
#define MAIN_H

#include <hal.h>
#include <msgbus/msgbus.h>

#define TELEM1 (BaseSequentialStream *)&SD2
#define DEBUG_UART (BaseSequentialStream *)&SD7

extern msgbus_t bus;

#endif /* MAIN_H */
