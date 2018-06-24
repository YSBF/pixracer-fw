#include <ch.h>
#include <hal.h>
#include <serial-datagram/serial_datagram.h>
#include <chprintf.h>
#include "thread_prio.h"
#include "main.h"

void sd_write(void *arg, const void *buf, size_t len)
{
    if (len == 0) {
        return;
    }

    sdWrite((SerialDriver *)arg, buf, len);
}

static THD_WORKING_AREA(telemetry_thread, 1000);
static THD_FUNCTION(telemetry_main, arg)
{
    (void)arg;
    chRegSetThreadName("telemetry");
    while (true) {
        static char msg[100];
        int len;
        len = chsnprintf(msg, sizeof(msg), "hello world! systime: %09lu\n", (unsigned long)chVTGetSystemTime());
        serial_datagram_send(msg, len, sd_write, TELEM1);
        palTogglePad(GPIOB, GPIOB_FMU_LED_BLUE);
        chThdSleepMilliseconds(10);
    }
}

void telemetry_start(void)
{
    chThdCreateStatic(telemetry_thread, sizeof(telemetry_thread), THD_PRIO_TELEMETRY, telemetry_main, NULL);
}
