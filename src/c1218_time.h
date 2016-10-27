#ifndef __C1218_TIME__
#define __C1218_TIME__

#include <stdint.h>

typedef uint32_t usec_t;
typedef uint32_t msec_t;

extern usec_t get_current_time(void);
extern void msleep(msec_t msec);
extern void nsleep(usec_t usec);

#endif
