/*
 * C12.18 stack
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#include "c1218_stack.h"
#include <string.h>
#include "log.h"

int c1218_stack_init(c1218_stack_t *stack, char *serial_device, int is_client)
{
	memset(stack, 0, sizeof(*stack));

	strcpy(stack->s_dev, serial_device);
	stack->is_client = !!is_client;
	stack->is_device = !is_client;
	memcpy(stack->user, "commdevice", 10);

	c1218_al_init(&stack->al);
	c1218_dl_init(&stack->dl);
	c1218_pl_init(&stack->pl);

	return 0;
}

int c1218_stack_run(c1218_stack_t *stack)
{
	/* just PL, DL testing */
	//c1218_pl_run(&stack->pl);
	//c1218_dl_run(&stack->dl);
	/* end of testing */

	c1218_al_run(&stack->al);

	return 0;
}
