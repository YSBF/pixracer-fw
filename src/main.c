#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <shell.h>

#include "usbcfg.h"
#include "thread_prio.h"
#include "commands.h"
#include "led.h"
#include "main.h"

msgbus_t bus;

#define TELEM1 (BaseSequentialStream *)&SD2
#define DEBUG_UART (BaseSequentialStream *)&SD7

void io_setup(void)
{
    static SerialConfig uart_config = {
        SERIAL_DEFAULT_BITRATE, 0,
        USART_CR2_STOP1_BITS, 0
    };

    uart_config.speed = 57600;
    sdStart(DEBUG_UART, &uart_config);

    // Telemetry 1 serial port
    uart_config.speed = 57600;
    sdStart(TELEM1, &uart_config);
}

void usb_start(void)
{
    // Initializes a serial-over-USB CDC driver.
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);

    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(1000);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);
}

int main(void)
{
    halInit();
    chSysInit();
    shellInit();

    led_init();

    io_setup();
    chprintf(DEBUG_UART, "\nboot\n");

    msgbus_init(&bus);

    usb_start();

    while (true) {
        shell_spawn((BaseSequentialStream *)&SDU1);
        chThdSleepMilliseconds(1000);
    }
}
