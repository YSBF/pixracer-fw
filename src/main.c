#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <shell.h>
#include <shell_cmd.h>
#include <stdlib.h>

#include "usbcfg.h"
#include "thread_prio.h"
#include "msgbus/msgbus.h"
#include "ts/type_print.h"
#include "types/dummy.h"

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

static void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[])
{
    static const char *states[] = {CH_STATE_NAMES};
    thread_t *tp;

    (void)argv;
    if (argc > 0) {
        shellUsage(chp, "threads");
        return;
    }
    chprintf(chp,
             "stklimit    stack     addr refs prio     state       time         name\r\n"SHELL_NEWLINE_STR);
    tp = chRegFirstThread();
    do {
#if (CH_DBG_ENABLE_STACK_CHECK == TRUE) || (CH_CFG_USE_DYNAMIC == TRUE)
        uint32_t stklimit = (uint32_t)tp->wabase;
#else
        uint32_t stklimit = 0U;
#endif
        chprintf(chp, "%08lx %08lx %08lx %4lu %4lu %9s %10lu %12s"SHELL_NEWLINE_STR,
                 stklimit, (uint32_t)tp->ctx.sp, (uint32_t)tp,
                 (uint32_t)tp->refs - 1, (uint32_t)tp->prio, states[tp->state],
                 (uint32_t)tp->time, tp->name == NULL ? "" : tp->name);
        tp = chRegNextThread(tp);
    } while (tp != NULL);
    chprintf(chp, "[sytick %d @ %d Hz]\n", chVTGetSystemTime(), CH_CFG_ST_FREQUENCY);
}

static void cmd_topic_print(BaseSequentialStream *stream, int argc, char *argv[]) {
    if (argc != 1) {
        chprintf(stream, "usage: topic_print name\n");
        return;
    }
    msgbus_subscriber_t sub;
    if (msgbus_topic_subscribe(&sub, &bus, argv[0], MSGBUS_TIMEOUT_IMMEDIATE)) {
        if (msgbus_subscriber_topic_is_valid(&sub)) {
            msgbus_topic_t *topic = msgbus_subscriber_get_topic(&sub);
            const ts_type_definition_t *type = msgbus_topic_get_type(topic);
            void *buf = malloc(type->struct_size);
            if (buf == NULL) {
                chprintf(stream, "malloc failed\n");
                return;
            }
            msgbus_subscriber_read(&sub, buf);
            ts_print_type((void (*)(void *, const char *, ...))chprintf,
                              stream, type, buf);
            free(buf);
        } else {
            chprintf(stream, "topic not published yet\n");
            return;
        }
    } else {
        chprintf(stream, "topic doesn't exist\n");
        return;
    }
}

static void cmd_topic_list(BaseSequentialStream *stream, int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    msgbus_topic_t *topic = msgbus_iterate_topics(&bus);
    while (topic != NULL) {
        chprintf(stream, "%s\n", msgbus_topic_get_name(topic));
        topic = msgbus_iterate_topics_next(topic);
    }
}

const ShellCommand commands[] = {
    {"topic_print", cmd_topic_print},
    {"topics", cmd_topic_list},
    {"threads", cmd_threads},
    {NULL, NULL}
};

static THD_WORKING_AREA(shell_wa, 2048);
static thread_t *shelltp = NULL;

#if (SHELL_USE_HISTORY == TRUE)
char sc_histbuf[256];
#endif
#if (SHELL_USE_COMPLETION == TRUE)
// size = # help + # default commands + # custom commands + 1 for NULL termination
#define SC_COMPLETION_SIZE (sizeof(commands) / sizeof(ShellCommand) + 8)
char *sc_completion[SC_COMPLETION_SIZE];
#endif

static ShellConfig shell_cfg = {
    NULL,
    commands,
#if (SHELL_USE_HISTORY == TRUE)
    sc_histbuf,
    sizeof(sc_histbuf),
#endif
#if (SHELL_USE_COMPLETION == TRUE)
    sc_completion
#endif
};

void shell_spawn(BaseSequentialStream *stream)
{
    if (!shelltp) {
        shell_cfg.sc_channel = stream;
        shelltp = chThdCreateStatic(&shell_wa, sizeof(shell_wa), THD_PRIO_SHELL,
                                    shellThread, (void *)&shell_cfg);
        chRegSetThreadNameX(shelltp, "shell");
    } else if (chThdTerminatedX(shelltp)) {
        chThdRelease(shelltp);    /* Recovers memory of the previous shell.   */
        shelltp = NULL;           /* Triggers spawning of a new shell.        */
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
