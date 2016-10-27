/*
 * C12.18 data link layer
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#include "c1218_stack.h"
#include "c1218_dl.h"
#include "c1218_time.h"
#include "c1218_errno.h"
#include "c12_crc.h"
#include "log.h"
#include <unistd.h>

static uint8_t send_buf[BUFFCACHE_SIZE] = { 0 };
static uint8_t recv_buf[BUFFCACHE_SIZE] = { 0 };
static int8_t duplicate_toggle_bit = 0;
static int8_t last_duplicate_toggle_bit = 0;

int c1218_dl_init(c1218_dl_t *dl)
{
	c1218_stack_t *stack = dl_to_stack(dl);

	dl->pl = &stack->pl;
	dl->nr_retries = 3;
	dl->nr_packets = 1;
	dl->packet_size = DEF_PACKETSIZE;

	dl->send_chain.head = dl->send_chain.tail = (struct packet *)send_buf;
	dl->send_chain.max_len = sizeof(send_buf);
	dl->recv_chain.head = dl->recv_chain.tail = (struct packet *)recv_buf;
	dl->recv_chain.max_len = sizeof(recv_buf);

	return 0;
}

int c1218_dl_run(c1218_dl_t *dl)
{
	c1218_stack_t *stack = dl_to_stack(dl);
	int16_t rsize, wsize;

	dl->nr_packets = BUFFCACHE_SIZE / dl->packet_size;

	if (stack->is_client) {
		uint8_t buf2[BUFFCACHE_SIZE] = { 0 };
		int16_t rsize2;

		rsize2 = c1219_pread_default(buf2, sizeof(buf2));
		LOGD("c1219_pread_default size: %d\n", rsize2);

		wsize = c1218_dl_send(dl, buf2, rsize2);
		if (wsize > 0)
			LOGD(">> %d bytes\n", wsize);
		else
			LOGD("c1218_dl_recv() failed, errno: %d\n",
			     wsize);
	} else {
		char buf[BUFFCACHE_SIZE] = { 0 };

		while (1) {
			rsize = c1218_dl_recv(dl, (uint8_t *)buf, sizeof(buf));
			if (rsize > 0)
				LOGD("<< %d bytes.\n", rsize);
#if 1
			else
				LOGD("c1218_dl_recv() failed, errno: %d\n",
				     rsize);
#endif
			msleep(500);
		}
	}

	return 0;
}

static int wait_for_response(c1218_dl_t *dl)
{
	uint8_t ack;
	usec_t start, end;

	start = get_current_time();

	while (1) {
		if (c1218_pl_read(dl->pl, &ack, 1) == 1)
			break;
		msleep(300);
		end = get_current_time();
		if ((end - start) / 1000000 >= RESPONSE_TIMEOUT)
			return RES_TIMEOUT_ERR;
	}

	return ack;
}

static int send_packet(c1218_dl_t *dl, uint8_t *buf, uint16_t len)
{
	int16_t wsize;
	ack_code_t ack = 0;
	int nr_retries = dl->nr_retries;

	while (nr_retries--) {
		LOGD("DL >> packet, %d bytes, retries: %d.\n", len, nr_retries);
		wsize = c1218_pl_write(dl->pl, buf, len);
		if (wsize < len)
			return IO_ERR;

		nsleep(TURN_AROUND_DELAY);

		ack = wait_for_response(dl);
		if (ack == ACK) {
			LOGD("DL << ack.\n");
			goto success;
		}
	}

	LOGD("DL >> packet failed.\n");

	return PACKET_TRANSMIT_ERR;

success:
	return 0;
}

static inline int calc_packet_max_data_len(c1218_dl_t *dl)
{
	return dl->packet_size - sizeof(struct packet) - 2;
}

static inline int calc_packet_size(struct packet *p)
{
	return (sizeof(*p) + p->length + 2);
}

static struct packet *next_packet(struct packet *p)
{
	return (struct packet *)((uint8_t *)p + calc_packet_size(p));
}

static int packet_verify_crc(struct packet *packet)
{
	uint16_t crc = c12_crc(sizeof(*packet) + packet->length,
			      (uint8_t *)packet);
	uint8_t *p = packet->data + packet->length;

	return !((*p++ == (crc & 0xff)) && (*p == crc >> 8));
}

static int append_crc(struct packet *packet)
{
	uint16_t crc = c12_crc(sizeof(*packet) + packet->length,
			      (uint8_t *)packet);
	uint8_t *p = packet->data + packet->length;

	*p++ = crc;
	*p = crc >> 8;

	return 0;
}

static struct packet *chain_append_packet(struct packet_chain *chain,
	uint8_t identity, uint8_t *data, uint16_t len,
	int is_single, int is_first_packet, uint8_t seq_nbr)
{
	struct packet *p = chain->tail;

	// check region
	if (chain->len + sizeof(*p) + len + 2 > chain->max_len)
		return NULL;

	p->stp = 0xee;
	p->identity = identity;
	duplicate_toggle_bit = !duplicate_toggle_bit;
	if (is_single) {
		p->ctrl = duplicate_toggle_bit << 5;
	} else {
		if (is_first_packet)
			p->ctrl = 1 << 7 | 1 << 6 | duplicate_toggle_bit << 5;
		else
			p->ctrl = 1 << 7 | duplicate_toggle_bit << 5;
	}
	p->length = len;
	p->seq_nbr = seq_nbr;
	memcpy(p->data, data, len);
	if (append_crc(p))
		return NULL;

	chain->len += calc_packet_size(p);
	chain->tail = next_packet(p);
	chain->nr_packets++;

	return p;
}

static int16_t send_chain(c1218_dl_t *dl, struct packet_chain *chain)
{
	struct packet *p = chain->head;
	uint16_t plen, len;
	c1218_errno_t errno;
	int16_t nr_packets;

	len = chain->len;
	nr_packets = chain->nr_packets;

	while (len && nr_packets--) {
		if (*(uint8_t *)p != 0xee) {
			errno = DATA_ERR;
			goto err;
		}

		LOGD("DL >> packet, toggle bit: %d, seq number: %d\n",
		     (p->ctrl >> 5) & 1, p->seq_nbr);

		plen = calc_packet_size(p);
		// fixme dumppacket()
		if ((errno = send_packet(dl, (uint8_t *)p, plen)) < 0)
			goto err;
		len -= plen;
		p = next_packet(p);
	}

	return chain->len - len;

err:
	return errno;
}

static int init_chain(struct packet_chain *chain)
{
	chain->tail = chain->head;
	chain->len = 0;
	chain->nr_packets = 0;

	return 0;
}

static int reset_chain(struct packet_chain *chain)
{
	return init_chain(chain);
}

int16_t c1218_dl_send(c1218_dl_t *dl, uint8_t *buf, int16_t len)
{
	int nr_packets;
	uint8_t identity = 0;
	struct packet *p;
	struct packet_chain *chain = &dl->send_chain;
	c1218_errno_t errno;

	if (!len)
		return 0;

	nr_packets = (len + dl->packet_size - sizeof(struct packet) - 2 - 1)
		/ (dl->packet_size - sizeof(struct packet) - 2);

	if (nr_packets > dl->nr_packets) {
		LOGE("DL segment packets failed: data size: %d, "
		     "max number of packets: %d\n", len, dl->nr_packets);
		errno = RANGE_ERR;
		goto err;
	}

	init_chain(chain);

	if (nr_packets == 1) {	// unsegmented packet
		p = chain_append_packet(chain, identity, buf, len, 1, 0, 0);
		if (!p) {
			errno = CREATE_PACKET_ERR;
			goto err;
		}
	} else {	// segmented packet
		int first_packet = 1;
		int16_t dlen = calc_packet_max_data_len(dl);

		while (nr_packets-- > 0) {
			if (first_packet) {
				p = chain_append_packet(chain, identity, buf,
							dlen, 0, 1, nr_packets);
				if (!p) {
					errno = CREATE_PACKET_ERR;
					goto err;
				}
				first_packet = 0;
			} else {
				p = chain_append_packet(chain, identity, buf,
							dlen, 0, 0, nr_packets);
				if (!p) {
					errno = CREATE_PACKET_ERR;
					goto err;
				}
			}
			buf = buf + dlen;
			len -= p->length;
			if (len < dlen)
				dlen = len;
		}
	}

	errno = send_chain(dl, chain);
	reset_chain(chain);

err:
	return errno;
}

static int send_response(c1218_dl_t *dl, ack_code_t ack)
{
	uint8_t buf[1];
	buf[0] = (uint8_t)ack;

	LOGD("DL >> ack\n");
	return !(c1218_pl_write(dl->pl, buf, 1) == 1);
}

static int is_duplicate_packet(struct packet *p)
{
	static int8_t last_seq_nbr = 0;
	int8_t toggle_bit = (p->ctrl >> 5) & 0x1;

	if (last_duplicate_toggle_bit == toggle_bit &&
	    last_seq_nbr == p->seq_nbr)
		return 1;

	last_duplicate_toggle_bit = toggle_bit;
	last_seq_nbr = p->seq_nbr;

	return 0;
}

static int recv_packet(c1218_dl_t *dl, struct packet *p)
{
	int16_t rsize;
	ack_code_t ack = ACK;
	int errno;

recv_header:
	LOGD("DL << packet header.\n");
	rsize = c1218_pl_read(dl->pl, (uint8_t *)p, sizeof(*p));
	if (rsize != sizeof(*p)) {
		errno = IO_ERR;
		goto err;
	}

	/* get rid of the rest input data */
	if (p->stp != 0xee) {
		c1218_pl_iflush(dl->pl);
		msleep(300);
		goto recv_header;
	}

	LOGD("DL << packet seq number: %d\n", p->seq_nbr);
	LOGD("DL << packet data + CRC.\n");

	rsize = c1218_pl_read(dl->pl, p->data, p->length + 2);
	if (rsize != p->length + 2) {
		errno = IO_ERR;
		goto err;
	}

	/* not C12.18 packet */
	if (p->ctrl & 0x3)
		ack = NAK;

	/* invalid packet */
	if (packet_verify_crc(p))
		ack = NAK;

	nsleep(TURN_AROUND_DELAY);

	if (send_response(dl, ack)) {
		errno = IO_ERR;
		goto err;
	}

	if (is_duplicate_packet(p)) {
		LOGD("DL << duplicate packet, ignore it.\n");
		goto recv_header;
	}

	return 0;
err:
	return errno;
}

static int wait_for_packet(c1218_dl_t *dl, struct packet *p)
{
	usec_t start, end;

	start = get_current_time();

	while (1) {
		if (!recv_packet(dl, p))
			break;

		end = get_current_time();
		if ((end - start) / 1000000 >= CHANNEL_TRAFFIC_TIMEOUT)
			return TRAFFIC_TIMEOUT_ERR;

		msleep(200);
	}

	return 0;
}

int16_t c1218_dl_recv(c1218_dl_t *dl, uint8_t *buf, int16_t len)
{
	int16_t rsize = 0;
	struct packet *p;
	struct packet_chain *chain;
	int wait_for_seq_nbr = 0;
	int errno;

	chain = &dl->recv_chain;
	p = chain->head;

	init_chain(chain);

	if (!len)
		return 0;

	// recv first packet
	if ((errno = recv_packet(dl, p)))
		goto err;

	LOGD("first packet: ctrl: 0x%x\n", p->ctrl);

	if (!(p->ctrl & 1 << 7)) {	// unsegmented packet
		rsize = len > p->length ? p->length : len;
		memcpy(buf, p->data, rsize);
	} else {	// segmented packets
		chain->nr_packets = 1;

		// first packet
		if (p->ctrl & 1 << 6) {
			wait_for_seq_nbr = p->seq_nbr - 1;
		} else {
			errno = ASSEMBLE_PACKET_ERR;
			goto err;
		}

		// the rest packets
		while (wait_for_seq_nbr >= 0) {
			p = next_packet(p);

			if ((errno = wait_for_packet(dl, p)))
				goto err;

			if (wait_for_seq_nbr != p->seq_nbr) {
				errno = ASSEMBLE_PACKET_ERR;
				goto err;
			}

			wait_for_seq_nbr--;
			chain->nr_packets++;
		}

		// copy data to buf
		uint8_t *d = buf;
		int16_t psize;
		p = chain->head;

		while (len > 0 && chain->nr_packets--) {
			if (p->stp != 0xee) {
				errno = DATA_ERR;
				goto err;
			}

			psize = len > p->length ? p->length : len;
			memcpy(d, p->data, psize);
			d = d + psize;
			rsize += psize;
			len -= psize;
			p = next_packet(p);
		}
	}

	reset_chain(chain);

	return rsize;

err:
	return errno;
}
