#ifndef COMMANDS_H
#define COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal.h>

void shell_spawn(BaseSequentialStream *stream);

#ifdef __cplusplus
}
#endif

#endif /* COMMANDS_H */
