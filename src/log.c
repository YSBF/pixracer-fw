#include <hal.h>
#include <chprintf.h>
#include <string.h>
#include <stdarg.h>
#include <error/error.h>
#include "memstreams.h"
#include "main.h"

MUTEX_DECL(log_lock);

int chvsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
    MemoryStream ms;
    BaseSequentialStream *chp;
    size_t size_wo_nul;
    int retval;

    if (size > 0) {
        size_wo_nul = size - 1;
    } else {
        size_wo_nul = 0;
    }

    /* Memory stream object to be used as a string writer, reserving one
       byte for the final zero.*/
    msObjectInit(&ms, (uint8_t *)str, size_wo_nul, 0);

    /* Performing the print operation using the common code.*/
    chp = (BaseSequentialStream *)(void *)&ms;
    retval = chvprintf(chp, fmt, ap);

    /* Terminate with a zero, unless size==0.*/
    if (ms.eos < size) {
        str[ms.eos] = 0;
    }

    /* Return number of bytes that would have been written.*/
    return retval;
}

static const char *get_thread_name(void)
{
    const char *thread_name;

    thread_name = chRegGetThreadNameX(chThdGetSelfX());
    if (thread_name == NULL) {
        thread_name = "unknown";
    }

    return thread_name;
}


static void panic_message(struct error *e, ...)
{
    va_list ap;
    static char msg[256];

    va_start(ap, e);
    chvsnprintf(msg, sizeof(msg), e->text, ap);
    va_end(ap);

    msg[sizeof(msg) - 1] = 0; // null terminate

    chSysHalt(msg);
}

static void log_message(struct error *e, ...)
{
    va_list va;
    chMtxLock(&log_lock);

    /* Print time */
    systime_t ts = chVTGetSystemTime();
    chprintf(DEBUG_UART, "[%08u]\t", ts);

    /* Print location. */
    chprintf(DEBUG_UART, "%s:%d\t", strrchr(e->file, '/') + 1, e->line);

    /* Print current thread */
    chprintf(DEBUG_UART, "%s\t", get_thread_name());

    /* Print severity message */
    chprintf(DEBUG_UART, "%s\t", error_severity_get_name(e->severity));

    /* Print message */
    va_start(va, e);
    chvprintf(DEBUG_UART, e->text, va);
    va_end(va);

    chprintf(DEBUG_UART, "\n");

    chMtxUnlock(&log_lock);
}

void log_init(void)
{
    error_register_error(panic_message);
    error_register_warning(log_message);
    error_register_notice(log_message);
    error_register_debug(log_message);
}
