/*
 * C12.18 phcisal layer
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#include "c1218_stack.h"
#include "c1218_pl.h"
#include "log.h"
#include "c1218_errno.h"

#ifdef __LINUX__
#include <errno.h>
#include "c1218_linux_pl.h"
#endif

int c1218_pl_init(c1218_pl_t *pl)
{
	c1218_stack_t *stack = pl_to_stack(pl);
	pl->baudrate = DEF_BAUDRATE;

#ifdef __LINUX__
	pl->ops = &c1218_linux_pl_ops;
#endif

	return pl->ops->init(pl->baudrate, stack->s_dev);
}

int c1218_pl_release(c1218_pl_t *pl)
{
	return pl->ops->release();
}

int c1218_pl_iflush(c1218_pl_t *pl)
{
	return pl->ops->flush();
}

int16_t c1218_pl_write(c1218_pl_t *pl, uint8_t *buf, uint16_t size)
{
	int16_t wsize = pl->ops->write(buf, size);

	if (wsize < 0) {
#ifdef __LINUX__
		LOGD("PL encounter write I/O error, libc errno: %d\n", errno);
#else
		LOGD("PL encounter write I/O error\n");
#endif
	} else {
		LOGD("PL >> %d bytes.\n", wsize);
	}

	return wsize;
}

int16_t c1218_pl_read(c1218_pl_t *pl, uint8_t *buf, uint16_t size)
{
	int16_t rsize = pl->ops->read(buf, size);

	if (rsize < 0) {
#ifdef __LINUX__
		LOGD("PL encounter read I/O error, libc errno: %d\n", errno);
#else
		LOGD("PL encounter read I/O error\n");
#endif
	} else {
		LOGD("PL << %d bytes.\n", rsize);
	}

	return rsize;
}

/* Just for testing */
int c1218_pl_run(c1218_pl_t *pl)
{
	char *msg;
	char buf[BUFFCACHE_SIZE] = { 0 };
	ssize_t rsize, wsize;
	c1218_stack_t *stack = pl_to_stack(pl);

	if (!stack->is_client && !stack->is_device)
		return -1;

	if (stack->is_client)
		msg = "hello C12.18 Device!!!";
	else
		msg = "hello C12.18 Client!!!";

	while (1) {
		wsize = pl->ops->write((uint8_t *)msg, strlen(msg) + 1);
		if (wsize < strlen(msg) + 1) {
			LOGE("PL: fail to send testing msg.\n");
			return -1;
		}
		LOGI(">> %s\n", msg);
		rsize = pl->ops->read((uint8_t *)buf, (uint16_t)sizeof(buf));
		if (rsize < 0) {
			perror("read");
		}
		LOGI("<< %s\n", buf);
		LOGI("sleep 1s...\n");
		sleep(1);
	}

	return 0;
}

int c1218_pl_set_baudrate(c1218_pl_t *pl, uint8_t baudrate)
{
	if (pl->baudrate == baudrate)
		return 0;

	pl->baudrate = baudrate;
	return pl->ops->set_baudrate(baudrate);
}
