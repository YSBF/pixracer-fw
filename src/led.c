#include <ch.h>
#include <hal.h>
#include "thread_prio.h"
#include "led.h"

static THD_WORKING_AREA(led_thread, 128);
static THD_FUNCTION(led_main, arg)
{
    (void)arg;
    chRegSetThreadName("heartbeat");
    while (true) {
        palClearPad(GPIOB, GPIOB_FMU_LED_GREEN);
        chThdSleepMilliseconds(80);
        palSetPad(GPIOB, GPIOB_FMU_LED_GREEN);
        chThdSleepMilliseconds(80);
        palClearPad(GPIOB, GPIOB_FMU_LED_GREEN);
        chThdSleepMilliseconds(80);
        palSetPad(GPIOB, GPIOB_FMU_LED_GREEN);
        chThdSleepMilliseconds(760);
    }
}

void led_init(void)
{
    chThdCreateStatic(led_thread, sizeof(led_thread), THD_PRIO_LED, led_main, NULL);
}
