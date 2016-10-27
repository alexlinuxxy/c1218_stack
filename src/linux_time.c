/*
 * C12.18 phcisal layer
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#include <time.h>
#include "c1218_time.h"
#include <unistd.h>

extern int usleep(usec_t usec);

usec_t get_current_time(void)
{
	return time(NULL) * 1000 * 1000;
}

void msleep(msec_t msec)
{
	usleep(msec * 1000);
}

void nsleep(usec_t usec)
{
	usleep(usec);
}
