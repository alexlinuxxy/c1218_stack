/*
 * C12.18 data link layer
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#ifndef __C1218_DL__
#define __C1218_DL__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "c1218_time.h"

#define CHANNEL_TRAFFIC_TIMEOUT	6	//seconds
#define RESPONSE_TIMEOUT	2	//seconds
#define TURN_AROUND_DELAY	175	//microseconds

#define DEF_PACKETSIZE	64
#define DEF_PACKET_NBR	1

typedef enum {
	ACK = 0x6,
	NAK = 0x15,
} ack_code_t;

//<packet> ::= <stp><identity><ctrl><seq-nbr><length><data><crc>
struct packet {
	uint8_t stp;
	uint8_t identity;
	uint8_t ctrl;
	uint8_t seq_nbr;
	uint16_t length;
	uint8_t data[0];
} __attribute__((aligned(1)));

struct packet_chain {
	struct packet *head;
	struct packet *tail;
	int16_t	nr_packets;
	uint16_t len;
	uint16_t max_len;
};

struct c1218_pl;
typedef struct c1218_dl {
	struct c1218_pl *pl;

	struct packet_chain send_chain;
	struct packet_chain recv_chain;

	uint16_t packet_size;
	uint8_t nr_packets;

	uint8_t nr_retries;
} c1218_dl_t;

extern int c1218_dl_init(c1218_dl_t *dl);
extern int c1218_dl_run(c1218_dl_t *dl);
extern int16_t c1218_dl_send(c1218_dl_t *dl, uint8_t *buf, int16_t len);
extern int16_t c1218_dl_recv(c1218_dl_t *dl, uint8_t *buf, int16_t len);

#endif
