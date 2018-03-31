#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <shell.h>

#include "usbcfg.h"
#include "thread_prio.h"
#include "types/dummy.h"
#include "commands.h"
#include "main.h"

msgbus_t bus;

static THD_WORKING_AREA(led_thread, 128);
static THD_FUNCTION(led_main, arg)
{
    (void)arg;
    chRegSetThreadName("heartbeat");
    while (true) {
        palClearPad(GPIOB, GPIOB_FMU_LED_GREEN);
        chThdSleepMilliseconds(500);
        palSetPad(GPIOB, GPIOB_FMU_LED_GREEN);
        chThdSleepMilliseconds(500);
    }
}

#define TELEM1 (BaseSequentialStream *)&SD2

int main(void)
{
    halInit();
    chSysInit();

    chThdCreateStatic(led_thread, sizeof(led_thread),
                      THD_PRIO_LED, led_main, NULL);

    msgbus_init(&bus);

    SerialConfig uart_config = {
        SERIAL_DEFAULT_BITRATE, 0,
        USART_CR2_STOP1_BITS, 0
    };
    uart_config.speed = 57600;
    sdStart(&SD7, &uart_config);
    chprintf((BaseSequentialStream *)&SD7, "\nboot\n");

    // Telemetry 1 serial port
    sdStart(&SD2, &uart_config);

    // Shell manager initialization.
    shellInit();

    // Initializes a serial-over-USB CDC driver.
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);

    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(1000);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);

    msgbus_topic_t dummy_topic;
    dummy_t dummy_topic_buf;
    msgbus_topic_create(&dummy_topic, &bus, &dummy_type, &dummy_topic_buf, "/dummy");

    dummy_t dummy = {"hello world", 0};
    while (true) {
        msgbus_topic_publish(&dummy_topic, &dummy);
        dummy.count += 1;

        chprintf(TELEM1, "hello world\n");
        chprintf((BaseSequentialStream *)&SD7, "hello world\n");
        shell_spawn((BaseSequentialStream *)&SDU1);
        chThdSleepMilliseconds(1000);
    }
}
