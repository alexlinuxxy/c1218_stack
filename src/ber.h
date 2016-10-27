/*
 * Basic Encoding Rules (BER)
 *
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#ifndef __BER__
#define __BER__

#include <stdint.h>

typedef struct bercode {
	uint8_t buf[5];	//max int32 len
	uint8_t len;
} bercode_t;

extern int ber_word16_encode(uint16_t d, bercode_t *ber);
extern int ber_word16_decode(uint16_t *d, bercode_t *ber);

#endif
