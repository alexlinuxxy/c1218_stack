/*
 * C12.18 stack
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#ifndef __C1218_STACK__
#define __C1218_STACK__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "c1218_al.h"
#include "c1218_dl.h"
#include "c1218_pl.h"

typedef enum {
	Base_State = 1,
	ID_State,
	Session_State,
} c1218_sss_t;

#define BUFFCACHE_SIZE	1024

struct c1218_stack_ops;
typedef struct c1218_stack {
	struct c1218_al al;
	struct c1218_dl dl;
	struct c1218_pl pl;

	c1218_sss_t state;	// Service Sequence State Control
	char is_client:1;	// device type
	char is_device:1;
	char s_dev[20];		// serial device
	char user[10];		// user identification

	struct c1218_stack_ops *ops;
} c1218_stack_t;

#define offset_of(type, member) &(((type *)0)->member)
#define container_of(addr, type, member) \
	((void *)addr - (void *)offset_of(type, member))

static inline c1218_stack_t *pl_to_stack(void *pl) {
	return (c1218_stack_t *)container_of(pl, struct c1218_stack, pl);
}

static inline c1218_stack_t *dl_to_stack(void *dl) {
	return (c1218_stack_t *)container_of(dl, struct c1218_stack, dl);
}

static inline c1218_stack_t *al_to_stack(void *al) {
	return (c1218_stack_t *)container_of(al, struct c1218_stack, al);
}

extern int c1218_stack_init(c1218_stack_t *stack, char *serial_device,
			    int is_client);
extern int c1218_stack_run(c1218_stack_t *stack);

#endif
