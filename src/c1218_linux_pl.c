/*
 * C12.18 phcisal layer
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#include <stdio.h>
#include "c1218_linux_pl.h"
#include "log.h"

static int fd = -1;
static char def_tty[] = "/dev/ttyS0";
static struct termios old_tio = { 0 };

static int c1218_linux_pl_init(uint8_t baudrate,
			       const char *tty);
static int16_t c1218_linux_pl_write(uint8_t *buf, uint16_t size);
static int16_t c1218_linux_pl_read(uint8_t *buf, uint16_t size);
static int c1218_linux_pl_release();
static int c1218_linux_set_baudrate(uint8_t baudrate);
static int c1218_linux_pl_flush();

static speed_t baudrates[20] = {
	0,
	B300,
	B600,
	B1200,
	B2400,
	B4800,
	B9600,
	0,
	B19200,
	0,
	B57600,
	B38400,
	B115200,
	0,
	0,
};

struct c1218_pl_ops c1218_linux_pl_ops = {
	.init = c1218_linux_pl_init,
	.read = c1218_linux_pl_read,
	.write = c1218_linux_pl_write,
	.release = c1218_linux_pl_release,
	.set_baudrate = c1218_linux_set_baudrate,
	.flush = c1218_linux_pl_flush,
};

static int c1218_linux_pl_init(uint8_t baudrate, const char *tty)
{
	struct termios new_tio = { 0 };
	const char *dev;

	dev = *tty ? tty : def_tty;

	LOGD("C12.18 PL initialing, baudrate: 0x%x, tty device: %s\n",
	     baudrate, dev);

	fd = open(dev, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		perror(dev);
		return -1;
	}

	tcgetattr(fd, &old_tio);

	new_tio.c_iflag = IGNPAR;
	new_tio.c_oflag = 0;
	new_tio.c_cflag = CS8 | CREAD | CLOCAL;
	cfsetispeed(&new_tio, baudrates[baudrate]);
	cfsetospeed(&new_tio, baudrates[baudrate]);
	new_tio.c_cc[VMIN] = 1;
	new_tio.c_cc[VTIME] = 5; //INTER-CHARACTER TIME-OUT:  500 milliseconds
	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, &new_tio);

	return 0;
}

static int c1218_linux_pl_flush(void)
{
	/* flush all of input out */
	tcflush(fd, TCIFLUSH);

	return 0;
}

static int c1218_linux_pl_release(void)
{
	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, &old_tio);
	close(fd);

	return 0;
}

static int c1218_linux_set_baudrate(uint8_t baudrate)
{
	struct termios tio;
	tcgetattr(fd, &tio);

	LOGD("PL set baudrate: 0x%x\n", baudrate);
	cfsetispeed(&tio, baudrates[baudrate]);
	cfsetospeed(&tio, baudrates[baudrate]);
	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, &tio);

	return 0;
}

static int16_t c1218_linux_pl_read(uint8_t *buf, uint16_t size)
{
	return read(fd, buf, size);
}

static int16_t c1218_linux_pl_write(uint8_t *buf, uint16_t size)
{
#if 1
	ssize_t wsize;

	wsize = write(fd, buf, size);
	fsync(fd);

	return wsize;
#else
	return write(fd, buf, size);
#endif
}
