/*
 * C12.18 phcisal layer
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#ifndef __C1218_LINUX_PL__
#define __C1218_LINUX_PL__

#include "c1218_pl.h"

#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern int c1218_linux_pl_set_args(const char *tty);
extern struct c1218_pl_ops c1218_linux_pl_ops;

#endif
