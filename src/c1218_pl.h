/*
 * C12.18 phcisal layer
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#ifndef __C1218_PL__
#define __C1218_PL__

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "c1218_dl.h"

struct c1218_pl_ops {
	int (*init)(uint8_t baudrate, const char *dev);
	int16_t (*read)(uint8_t *buf, uint16_t size);
	int16_t (*write)(uint8_t *buf, uint16_t size);
	int (*release)();
	int (*set_baudrate)(uint8_t baudrate);
	int (*flush)();
};

typedef struct c1218_pl {
	struct c1218_dl *dl;

	char devname[20];	// serial port
	uint8_t baudrate;

	struct c1218_pl_ops *ops;
} c1218_pl_t;

#define DEF_BAUDRATE	6	//9600

extern int c1218_pl_init(c1218_pl_t *pl);
extern int c1218_pl_run(c1218_pl_t *pl);
extern int16_t c1218_pl_write(c1218_pl_t *pl, uint8_t *buf, uint16_t size);
extern int16_t c1218_pl_read(c1218_pl_t *pl, uint8_t *buf, uint16_t size);
extern int c1218_pl_release(c1218_pl_t *pl);
extern int c1218_pl_iflush(c1218_pl_t *pl);
extern int c1218_pl_set_baudrate(c1218_pl_t *pl, uint8_t baudrate);

#endif
