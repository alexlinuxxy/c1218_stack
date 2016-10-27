/*
 * C12.18 application layer
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#include "c1218_stack.h"
#include "c1218_time.h"
#include "c1218_errno.h"
#include "log.h"

static int c1218_al_ident(struct c1218_al *al);
static int c1218_al_ident_r(struct c1218_al *al);
static int c1218_al_negotiate(struct c1218_al *al);
static int c1218_al_negotiate_r(struct c1218_al *al, uint8_t *buf, int16_t len);
static int c1218_al_terminate(struct c1218_al *al);
static int c1218_al_terminate_r(struct c1218_al *al);
static int c1218_al_logoff(struct c1218_al *al);
static int c1218_al_logoff_r(struct c1218_al *al);
static int c1218_al_logon(struct c1218_al *al);
static int c1218_al_logon_r(struct c1218_al *al, uint8_t *req, int16_t len);
static int c1218_al_full_read(struct c1218_al *al, read_req_t *req);
static int c1218_al_full_read_r(struct c1218_al *al, uint8_t *req, int16_t len);
static int c1218_al_pread_index(struct c1218_al *al, read_req_t *req);
static int c1218_al_pread_index_r(struct c1218_al *al, uint8_t *req,
				  int16_t len);
static int c1218_al_pread_offset(struct c1218_al *al, read_req_t *req);
static int c1218_al_pread_offset_r(struct c1218_al *al, uint8_t *req,
				   int16_t len);
static int c1218_al_pread_default(struct c1218_al *al, read_req_t *req);
static int c1218_al_pread_default_r(struct c1218_al *al, uint8_t *req,
				    int16_t len);
static int16_t send_request_response(c1218_al_t *al, uint8_t *buf,
				     uint16_t len);
static int16_t wait_for_response(c1218_al_t *al, uint8_t *buf, uint16_t len);
static int c1218_al_reset_config(struct c1218_al *al);

struct c1218_al_ops c1218_al_ops = {
	.ident = c1218_al_ident,
	.ident_r = c1218_al_ident_r,
	.negotiate = c1218_al_negotiate,
	.negotiate_r = c1218_al_negotiate_r,
	.terminate = c1218_al_terminate,
	.terminate_r = c1218_al_terminate_r,
	.logon = c1218_al_logon,
	.logon_r = c1218_al_logon_r,
	.logoff = c1218_al_logoff,
	.logoff_r = c1218_al_logoff_r,
	.full_read = c1218_al_full_read,
	.full_read_r = c1218_al_full_read_r,
	.pread_index = c1218_al_pread_index,
	.pread_index_r = c1218_al_pread_index_r,
	.pread_offset = c1218_al_pread_offset,
	.pread_offset_r = c1218_al_pread_offset_r,
	.pread_default = c1218_al_pread_default,
	.pread_default_r = c1218_al_pread_default_r,
};

static psem_res_t response_code(c1218_errno_t errno)
{
	psem_res_t res;

	switch (errno)
	{
	case RANGE_ERR:
	case DATA_ERR:
		res = ONP;
		break;
	default:
		res = ERR;
		break;
	}

	LOGD("response_code(): errno: %d, res_code: %d\n", errno, res);

	return res;
}

static uint8_t cksum(uint8_t *buf, uint8_t len)
{
	uint8_t sum = 0;
	uint8_t *p = buf;

	while (len--)
	{
		sum += *p++;
	}

	return sum;
}

static int c1218_al_pread_index(struct c1218_al *al, read_req_t *req)
{
	uint8_t buf[BUFFCACHE_SIZE], *p;
	int16_t wsize;
	int32_t rsize;
	c1218_errno_t errno;
	int i;

	p = buf;
	*p++ = 0x30 | req->nr_indexs;
	*p++ = req->table_id << 8;
	*p++ = req->table_id & 0xff;
	for (i = 0; i < req->nr_indexs; i++) {
		*p++ = req->index_tbl[i] >> 16;
		*p++ = req->index_tbl[i] >> 8;
	}
	*p++ = req->element_octet_count >> 8;
	*p++ = req->element_octet_count & 0xff;

	// send request
	LOGD("AL >> pread-index\n");
	wsize = send_request_response(al, buf, p - buf);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	// receive PSEM response
	LOGD("AL << pread-index_response\n");
	rsize = wait_for_response(al, buf, sizeof(buf));
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	if (*buf)
		return *buf;

	p = buf + 1;
	rsize = p[0] << 8 | p[1];
	p += 2;
	if (p[rsize] != cksum(p, rsize)) {
		errno = DATA_ERR;
		goto err;
	}

	if (rsize > req->size)
		rsize = req->size;
	memcpy(req->buf, buf, rsize);

	return rsize;
err:
	return errno;
}

static int c1218_al_pread_index_r(struct c1218_al *al, uint8_t *req,
				  int16_t len)
{
	uint8_t buf[BUFFCACHE_SIZE], *p;
	int16_t wsize;
	int32_t rsize;
	c1218_errno_t errno;
	int i, nr_indexs;
	uint16_t index[9] = { 0 };
	uint16_t table_id;
	uint16_t element_count;

	p = req;
	nr_indexs = *p % 0x30;
	table_id = p[0] << 8 | p[1];
	p = p + 2;
	for (i = 0; i < nr_indexs; i++)
		index[i] = *(uint16_t *)p++;
	element_count = p[0] << 8 | p[1];

	rsize = c1219_pread_index(buf, sizeof(buf), table_id,
				   index, element_count);
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	LOGD("AL >> pread-index_response\n");
	p = buf;
	*p++ = OK;
	*p++ = rsize >> 8;
	*p++ = rsize & 0xff;
	memcpy(p, buf, rsize);
	p = p + rsize;
	*p++ = cksum(buf, rsize);

	wsize = send_request_response(al, buf, p - buf);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	return 0;
err:
	buf[0] = response_code(errno);

	send_request_response(al, buf, 1);

	return buf[0];
}

static int c1218_al_pread_offset(struct c1218_al *al, read_req_t *req)
{
	uint8_t buf[BUFFCACHE_SIZE], *p;
	int16_t wsize;
	int32_t rsize;
	c1218_errno_t errno;

	p = buf;
	*p++ = 0x3f;
	*p++ = req->table_id << 8;
	*p++ = req->table_id & 0xff;
	*p++ = req->offset >> 16;
	*p++ = req->offset >> 8;
	*p++ = req->offset & 0xff;
	*p++ = req->element_octet_count >> 8;
	*p++ = req->element_octet_count & 0xff;

	// send request
	LOGD("AL >> pread-offset\n");
	wsize = send_request_response(al, buf, p - buf);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	// receive PSEM response
	LOGD("AL << pread-offset_response\n");
	rsize = wait_for_response(al, buf, sizeof(buf));
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	if (*buf)
		return *buf;

	p = buf + 1;
	rsize = p[0] << 8 | p[1];
	p += 2;
	if (p[rsize] != cksum(p, rsize)) {
		errno = DATA_ERR;
		goto err;
	}

	if (rsize > req->size)
		rsize = req->size;
	memcpy(req->buf, buf, rsize);

	return rsize;
err:
	return errno;
}

static int c1218_al_pread_offset_r(struct c1218_al *al, uint8_t *req,
				   int16_t len)
{
	uint8_t buf[BUFFCACHE_SIZE], *p;
	int16_t wsize;
	int32_t rsize;
	c1218_errno_t errno;
	uint16_t table_id;
	uint32_t offset;
	uint16_t octet_count;

	p = req + 1;
	table_id = p[0] << 8 | p[1];
	p = p + 2;
	offset = p[0] << 16 | p[1] << 8 | p[2];
	p = p + 3;
	octet_count = p[0] << 8 | p[1];

	rsize = c1219_pread_offset(buf, sizeof(buf), table_id,
				   offset, octet_count);
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	LOGD("AL >> pread-offset_response\n");
	p = buf;
	*p++ = OK;
	*p++ = rsize >> 8;
	*p++ = rsize & 0xff;
	memcpy(p, buf, rsize);
	p = p + rsize;
	*p++ = cksum(buf, rsize);

	wsize = send_request_response(al, buf, p - buf);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	return 0;
err:
	buf[0] = response_code(errno);

	send_request_response(al, buf, 1);

	return buf[0];
}

static int c1218_al_pread_default(struct c1218_al *al, read_req_t *req)
{
	uint8_t buf[BUFFCACHE_SIZE], *p;
	int16_t wsize;
	int32_t rsize;
	c1218_errno_t errno;

	buf[0] = 0x3e;

	// send request
	LOGD("AL >> pread-default\n");
	wsize = send_request_response(al, buf, 1);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	// receive PSEM response
	LOGD("AL << pread-default_response\n");
	rsize = wait_for_response(al, buf, sizeof(buf));
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	// <nok>
	if (*buf)
		return *buf;

	// <ok>...
	p = buf + 1;
	rsize = p[0] << 8 | p[1];
	LOGD("AL << pread-default_response %d bytes.\n", rsize);
	p = p + 2;
	LOGD("AL << pread-default_response cksum: 0x%x, p[rsize]: 0x%x.\n",
	     cksum(p, rsize), p[rsize]);

	if (p[rsize] != cksum(p, rsize)) {
		LOGD("AL pread-default_response cksum failed.\n");
		errno = DATA_ERR;
		goto err;
	}

	if (rsize > req->size)
		rsize = req->size;
	memcpy(req->buf, p, rsize);

	return rsize;
err:
	return errno;
}

static int c1218_al_pread_default_r(struct c1218_al *al, uint8_t *req,
				    int16_t len)
{
	uint8_t buf[BUFFCACHE_SIZE], *p;
	int16_t wsize;
	int32_t rsize;
	c1218_errno_t errno;

	rsize = c1219_pread_default(buf, sizeof(buf));
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	LOGD("AL >> pread-default_response\n");
	p = buf;
	*p++ = OK;
	*p++ = rsize >> 8;
	*p++ = rsize & 0xff;
	LOGD("AL >> pread-default_response %d bytes.\n", rsize);
	memcpy(p, buf, rsize);
	p = p + rsize;
	LOGD("AL >> pread-default_response cksum: 0x%x.\n",
	     cksum(buf, rsize));
	*p++ = cksum(buf, rsize);

	wsize = send_request_response(al, buf, p - buf);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	return 0;
err:
	buf[0] = response_code(errno);

	send_request_response(al, buf, 1);

	return buf[0];
}

static int32_t c1218_al_full_read(struct c1218_al *al, read_req_t *req)
{
	uint8_t buf[BUFFCACHE_SIZE], *p;
	int16_t wsize;
	int32_t rsize;
	c1218_errno_t errno;

	buf[0] = 0x30;
	buf[1] = req->table_id >> 8;
	buf[2] = req->table_id & 0xff;

	// send request
	LOGD("AL >> full-read\n");
	wsize = send_request_response(al, buf, 3);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	// receive PSEM response
	LOGD("AL << full-read_response\n");
	rsize = wait_for_response(al, buf, sizeof(buf));
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	if (*buf)
		return *buf;

	p = buf + 1;
	rsize = p[0] << 8 | p[1];
	p += 2;
	if (p[rsize] != cksum(p, rsize)) {
		errno = DATA_ERR;
		goto err;
	}

	if (rsize > req->size)
		rsize = req->size;
	memcpy(req->buf, buf, rsize);

	return rsize;
err:
	return errno;
}

static int c1218_al_full_read_r(struct c1218_al *al, uint8_t *req, int16_t len)
{
	uint8_t buf[BUFFCACHE_SIZE], *p;
	int16_t wsize;
	int32_t rsize;
	c1218_errno_t errno;
	uint16_t table_id;

	table_id = req[1] << 8 | req[2];

	rsize = c1219_full_read(buf, sizeof(buf), table_id);
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	LOGD("AL >> full-read_response\n");
	p = buf;
	*p++ = OK;
	*p++ = rsize >> 8;
	*p++ = rsize & 0xff;
	memcpy(p, buf, rsize);
	p = p + rsize;
	*p++ = cksum(buf, rsize);

	wsize = send_request_response(al, buf, p - buf);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	return 0;
err:
	buf[0] = response_code(errno);

	send_request_response(al, buf, 1);

	return buf[0];
}

static int c1218_al_logoff(struct c1218_al *al)
{
	uint8_t buf[1];
	int16_t wsize, rsize;
	c1218_errno_t errno;

	buf[0] = 0x52;

	// send request
	LOGD("AL >> logoff\n");
	wsize = send_request_response(al, buf, 1);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	// receive PSEM response
	LOGD("AL << logoff_response\n");
	rsize = wait_for_response(al, buf, sizeof(buf));
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	return *buf;
err:
	return errno;
}

static int c1218_al_logoff_r(struct c1218_al *al)
{
	uint8_t buf[1];
	int16_t wsize;
	c1218_errno_t errno;

	c1218_al_reset_config(al);

	LOGD("AL >> terminate_response\n");
	buf[0] = OK;
	wsize = send_request_response(al, buf, 1);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	return 0;
err:
	return errno;
}

static int c1218_al_logon(struct c1218_al *al)
{
	uint8_t buf[13];
	int16_t wsize, rsize;
	c1218_errno_t errno;
	int16_t user_id = 6666;
	c1218_stack_t *stack = al_to_stack(al);

	buf[0] = 0x50;
	buf[1] = user_id >> 8;
	buf[2] = user_id;
	memcpy(buf + 3, stack->user, 10);

	// send request
	LOGD("AL >> logon\n");
	wsize = send_request_response(al, buf, 13);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	// receive PSEM response
	LOGD("AL << logon_response\n");
	rsize = wait_for_response(al, buf, sizeof(buf));
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	return *buf;
err:
	return errno;
}

static int c1218_al_logon_r(struct c1218_al *al, uint8_t *req, int16_t len)
{
	uint8_t buf[1];
	int16_t wsize;
	c1218_errno_t errno;
	c1218_stack_t *stack = al_to_stack(al);

	al->tbl.user_id = req[1] << 8 | req[2];
	memcpy(stack->user, req + 3, 10);

	// send request
	LOGD("AL >> logon response\n");
	buf[0] = OK;
	wsize = send_request_response(al, buf, 1);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	return 0;
err:
	return errno;
}

static int c1218_al_terminate(struct c1218_al *al)
{
	uint8_t buf[1];
	int16_t wsize, rsize;
	c1218_errno_t errno;

	buf[0] = 0x21;

	// send request
	LOGD("AL >> terminate\n");
	wsize = send_request_response(al, buf, 1);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	// receive PSEM response
	LOGD("AL << terminate_response\n");
	rsize = wait_for_response(al, buf, sizeof(buf));
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	return *buf;
err:
	return errno;
}

static int c1218_al_terminate_r(struct c1218_al *al)
{
	uint8_t buf[1];
	int16_t wsize;
	c1218_errno_t errno;

	c1218_al_reset_config(al);

	LOGD("AL >> terminate_response\n");
	buf[0] = OK;
	wsize = send_request_response(al, buf, 1);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	return 0;
err:
	return errno;
}

static int parse_ident_response(struct c1218_al *al, uint8_t *buf, int16_t len)
{
	uint8_t *p = buf;

	if (*p)
		return *p;

	if (*++p)	// <std>
		goto err;
	al->tbl.ver = *p++;
	al->tbl.rev = *p++;

	while (*p) {	// end of 0x0
		if (*p == 0x6) {
			p++;

			if (*p == 0xd) {	// relative uid
				al->tbl.relative_uid_len = *++p;
				p++;
				memcpy(al->tbl.relative_uid, p,
				       al->tbl.relative_uid_len);
				p += al->tbl.relative_uid_len;
			}
			else if (*p == 0x6) {	// absoulute uid

			} else {
				goto err;
			}
		} else if (*p == 0x7) {
			al->tbl.identification_len = *++p - 1;
			al->tbl.identity_char_format = *++p;
			p++;
			memcpy(al->tbl.identification, p,
			       al->tbl.identification_len);
			p += al->tbl.identification_len;
		} else {
			goto err;
		}
	}

	return 0;

err:
	return DATA_ERR;
}

static int16_t send_request_response(c1218_al_t *al, uint8_t *buf, uint16_t len)
{
	return c1218_dl_send(al->dl, buf, len);
}

static int16_t wait_for_response(c1218_al_t *al, uint8_t *buf, uint16_t len)
{
	uint16_t rsize;
	usec_t start, end;

	start = get_current_time();

	while (1) {
		rsize = c1218_dl_recv(al->dl, buf, sizeof(buf));
		if (rsize > 0)
			break;

		end = get_current_time();
		if ((end - start) / 1000000 >= CHANNEL_TRAFFIC_TIMEOUT)
			return TRAFFIC_TIMEOUT_ERR;

		msleep(300);
	}

	return rsize;
}

static int c1218_al_negotiate(struct c1218_al *al)
{
	c1218_pl_t *pl = al->dl->pl;
	uint8_t buf[15];
	int16_t wsize, rsize;
	c1218_errno_t errno;
	psem_res_t res;

	/* baudrate: 115200, packetsize: 64, packets: BUFFCACHE_SIZE / 64 */
	buf[0] = 0x61;
	buf[1] = 0;
	buf[2] = 64;
	buf[3] = BUFFCACHE_SIZE / 64;
	buf[4] = 0xc;

	// send request
	LOGD("AL >> negotiate\n");
	wsize = send_request_response(al, buf, 5);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	// receive PSEM response
	LOGD("AL << negotiate_response\n");
	rsize = wait_for_response(al, buf, sizeof(buf));
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	res = *buf;
	if (res)
		return res;

	al->dl->packet_size = buf[1] << 8 | buf[2];
	al->dl->nr_packets = buf[3];
	c1218_pl_set_baudrate(pl, buf[4]);

	LOGD("negotiate: set packsize: %d\n", al->dl->packet_size);
	LOGD("negotiate: number of packets: %d\n", al->dl->nr_packets);

	return 0;
err:
	return errno;
}

// The obset of negotiate
static int c1218_al_reset_config(struct c1218_al *al)
{
	al->dl->nr_packets = DEF_PACKET_NBR;
	al->dl->packet_size = DEF_PACKETSIZE;
	c1218_pl_set_baudrate(al->dl->pl, DEF_BAUDRATE);

	return 0;
}

static int c1218_al_negotiate_r(struct c1218_al *al, uint8_t *req, int16_t len)
{
	c1218_pl_t *pl = al->dl->pl;
	uint8_t buf[5];
	int16_t wsize;
	c1218_errno_t errno;
	int nr_baudrates;

	nr_baudrates = req[0] % 0x60;
	al->dl->packet_size = req[1] << 8 | req[2];
	LOGD("Negotiate: set packetsize: %d\n", al->dl->packet_size);
	al->dl->nr_packets = req[3];
	LOGD("Negotiate: set number of packets: %d\n", al->dl->nr_packets);
	if (nr_baudrates > 0) {
		uint8_t baudrate = 0;
		uint8_t *p = req + 4;
		while (nr_baudrates--) {
			if (*p > baudrate)
				baudrate = *p;
			p++;
		}
		c1218_pl_set_baudrate(pl, baudrate);
	}

	buf[0] = OK;
	buf[1] = al->dl->packet_size >> 8;
	buf[2] = al->dl->packet_size & 0xff;
	buf[3] = al->dl->nr_packets;
	buf[4] = pl->baudrate;

	// send request
	LOGD("AL >> negotiate_response\n");
	wsize = send_request_response(al, buf, 5);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	return 0;
err:
	return errno;
}

/*
 * return value: 0, c1218_errno or psem_res_code
 */
static int c1218_al_ident(struct c1218_al *al)
{
	uint8_t buf[128];	// larger than default packetsize(64)
	int16_t wsize, rsize;
	c1218_errno_t errno;
	psem_res_t res;

	buf[0] = 0x20;

	// send request
	LOGD("AL >> ident\n");
	wsize = send_request_response(al, buf, 1);
	if (wsize < 0) {
		errno = wsize;
		goto err;
	}

	// receive PSEM response
	LOGD("AL << ident_response\n");
	rsize = wait_for_response(al, buf, sizeof(buf));
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}

	res = *buf;
	if (res)
		return res;

	return parse_ident_response(al, buf, rsize);

err:
	return errno;
}

/*
 * return value: 0, c1218_errno or psem_res_code
 */
static int c1218_al_ident_r(struct c1218_al *al)
{
	/*
	 * <ident-r> ::= <isss> | <bsy> | <err> |
	 * <ok><std><ver><rev><feature> * <end-of-list>
	 */
	uint8_t buf[128], *p;	// larger than default packetsize(64)
	int16_t wsize;
	c1218_errno_t errno;
	int has_dev_class = 0;

	// get data from C12.19 Table first.
	// response <err>, <busy>, if nessary

	p = buf;
	*p++ = OK;
	*p++ = 0;
	*p++ = 1;
	*p++ = 0;

	/* device class */
	has_dev_class = 1;
	*p++ = 0x6;
	*p++ = 0xd;
	*p++ = al->tbl.relative_uid_len;
	memcpy(p, al->tbl.relative_uid, al->tbl.relative_uid_len);
	p += al->tbl.relative_uid_len;

	/* identification */
	*p++ = 0x7;
	*p++ = 1 + al->tbl.identification_len;
	if (has_dev_class)
		*p++ = al->tbl.identity_char_format;
	else
		*p++ = 1;
	memcpy(p, al->tbl.identification, al->tbl.identification_len);
	p += al->tbl.identification_len;

	/* end of list */
	*p++ = 0;

	LOGD("AL >> ident_response\n");
	wsize = send_request_response(al, buf, p - buf);
	if (wsize < 0) {
		errno = wsize;
		LOGD("%s(): Fail to send response, errno: %d\n",
		     __func__, errno);
		goto err;
	}

	return 0;
err:
	return errno;
}

int c1218_al_init(c1218_al_t *al)
{
	c1218_stack_t *stack = al_to_stack(al);

	al->dl = &stack->dl;
	al->ops = &c1218_al_ops;
	if (stack->is_device)
		c1219_tbl_init(&al->tbl);

	return 0;
}

static int process_requests(c1218_al_t *al)
{
	c1218_stack_t *stack = al_to_stack(al);
	uint8_t buf[128] = { 0 };	// larger than default packetsize(64)
	uint16_t rsize;
	int argc;
	int errno;

	// Service Sequence State Control
	stack->state = Base_State;

	while (1) {
		if ((rsize = c1218_dl_recv(al->dl, buf, sizeof(buf))) > 0) {
			switch (*buf) {
			case 0x20:	// ident
				// double check, ident or other
				if (rsize != 1)
					break;
				// Service Sequence State Control
				if (stack->state != Base_State) {
					buf[0] = ISSS;
					c1218_dl_send(al->dl, buf, 1);
					break;
				}

				errno = al->ops->ident_r(al);
				LOGD("processed ident request, errno: %d\n",
				     errno);
				if (!errno)
					stack->state = ID_State;
				break;
			case 0x21:	// terminate
				// double check, terminate or other
				if (rsize != 1)
					break;
				errno = al->ops->terminate_r(al);
				LOGD("processed terminate request, "
				     "errno: %d\n", errno);
				if (!errno)
					stack->state = Base_State;
				break;
			case 0x30:	// full-read
				// double check, full-read or other
				if (rsize != 3)
					break;

				// Service Sequence State Control
				if (stack->state != Session_State) {
					buf[0] = ISSS;
					c1218_dl_send(al->dl, buf, 1);
					break;
				}

				errno = al->ops->full_read_r(al, buf, rsize);
				LOGD("processed full-read request, "
				     "errno: %d\n", errno);
				break;
			case 0x31:	// pread-index
			case 0x32:
				// double check, negotiate or other
				argc = *buf % 0x30;
				if (rsize != (1 + 2 + 2 * argc + 2))
					break;
				// Service Sequence State Control
				if (stack->state != Session_State) {
					buf[0] = ISSS;
					c1218_dl_send(al->dl, buf, 1);
					break;
				}

				errno = al->ops->pread_index_r(al, buf, rsize);
				LOGD("processed pread-index request, "
				     "errno: %d\n", errno);
				break;
			case 0x3e:	// pread-default
				if (rsize != 1)
					break;
				// Service Sequence State Control
				if (stack->state != Session_State) {
					buf[0] = ISSS;
					c1218_dl_send(al->dl, buf, 1);
					break;
				}

				errno = al->ops->pread_default_r(al, buf,
								 rsize);
				LOGD("processed pread-default request, "
				     "errno: %d\n", errno);
				break;
			case 0x3f:	// pread-offset
				if (rsize != 8)
					break;
				// Service Sequence State Control
				if (stack->state != Session_State) {
					buf[0] = ISSS;
					c1218_dl_send(al->dl, buf, 1);
					break;
				}

				errno = al->ops->pread_offset_r(al, buf, rsize);
				LOGD("processed pread-offset request, "
				     "errno: %d\n", errno);
				break;
			case 0x50:	// logon
				// double check, logon or other
				if (rsize != 13)
					break;
				// Service Sequence State Control
				if (stack->state != ID_State) {
					buf[0] = ISSS;
					c1218_dl_send(al->dl, buf, 1);
					break;
				}
				errno = al->ops->logon_r(al, buf, rsize);
				LOGD("processed logon request, errno: %d\n",
				     errno);
				if (!errno)
					stack->state = Session_State;
				break;
			case 0x52:	// logoff
				// double check, logoff or other
				if (rsize != 1)
					break;
				// Service Sequence State Control
				if (stack->state != Session_State) {
					buf[0] = ISSS;
					c1218_dl_send(al->dl, buf, 1);
					break;
				}
				errno = al->ops->logoff_r(al);
				LOGD("processed logoff request, errno: %d\n",
				     errno);
				if (!errno)
					stack->state = ID_State;
				break;
			case 0x60:	// negotiate
			case 0x61:
			case 0x62:
			case 0x63:
				// double check, negotiate or other
				if (rsize != (4 + *buf % 0x60))
					break;
				// Service Sequence State Control
				if (stack->state != ID_State) {
					buf[0] = ISSS;
					c1218_dl_send(al->dl, buf, 1);
					break;
				}

				errno = al->ops->negotiate_r(al, buf, rsize);
				LOGD("processed negotiate request, "
				     "errno: %d\n", errno);
				break;
			default:
				/*
				if (*buf != ACK || *buf != NAK) {
					buf[0] = SNS;
					c1218_dl_send(al->dl, buf, 1);
				}
				*/
				break;
			}
			memset(buf, 0, sizeof(buf));
		}
	}

	return 0;
}

static int pread_default_testing(c1218_al_t *al)
{
	uint8_t buf[BUFFCACHE_SIZE] = { 0 };
	uint8_t buf2[BUFFCACHE_SIZE] = { 0 };
	int16_t rsize, rsize2;
	read_req_t req = { 0 };
	req.buf = buf;
	req.size = sizeof(buf);
	c1218_errno_t errno;

	rsize = al->ops->pread_default(al, &req);
	if (rsize < 0) {
		errno = rsize;
		goto err;
	}
	LOGD("pread-default size: %d\n", rsize);

	if (rsize == 1)
		LOGD("pread default response code: %d\n", req.buf[0]);

	rsize2 = c1219_pread_default(buf2, sizeof(buf2));
	LOGD("c1219_pread_default size: %d\n", rsize2);

	if (rsize != rsize2 || memcmp(req.buf, buf2, rsize)) {
		errno = DATA_ERR;
		goto err;
	}

	return 0;
err:
	return errno;
}

int c1218_al_run(c1218_al_t *al)
{
	c1218_stack_t *stack = al_to_stack(al);
	c1218_errno_t errno;

	if (stack->is_device) {	// C12.18 Device
		process_requests(al);
	} else {		// C12.18 Client
		if ((errno = al->ops->ident(al)))
			LOGD("Fail to request ident, errno: %d\n", errno);

		if ((errno = al->ops->negotiate(al)))
			LOGD("Fail to request negotiate, errno: %d\n", errno);

		if ((errno = al->ops->logon(al)))
			LOGD("Fail to request negotiate, errno: %d\n", errno);

		if ((errno = pread_default_testing(al)))
			LOGD("Fail to pread-default, errno: %d\n", errno);
	}

	return 0;
}
