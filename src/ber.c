/*
 * Basic Encoding Rules (BER)
 *
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#include "ber.h"

int ber_word16_encode(uint16_t d, bercode_t *ber)
{
	uint8_t *p = ber->buf;

	if (d < 128) {
		*p = d;
		ber->len = 1;
	} else if (d < 256) {
		*p++ = 0x81;
		*p = d;
		ber->len = 2;
	} else {
		*p++ = 0x82;
		*p++ = d >> 8;
		*p = d;
		ber->len = 3;
	}

	return 0;
}

int ber_word16_decode(uint16_t *d, bercode_t *ber)
{
	switch (ber->len) {
	case 1:
		*d = ber->buf[0];
		break;
	case 2:
		*d = ber->buf[1];
		break;
	case 3:
		*d = ber->buf[1] << 8 | ber->buf[2];
		break;
	default:
		break;
	}

	return 0;
}
