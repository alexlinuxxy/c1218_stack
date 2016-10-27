/*
 * C12.19 table, sample, fixed table
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#ifndef __C1219_TBL__
#define __C1219_TBL__

#include <stdint.h>

typedef struct c1219_tbl {
	// ident
	uint8_t ver;
	uint8_t rev;
	uint8_t relative_uid_len;
	uint8_t relative_uid[4];
	uint8_t identity_char_format;
	uint8_t identification_len;
	uint8_t identification[10];

	uint16_t user_id;
} c1219_tbl_t;

#define RAW_DATA	"raw_data"

extern int c1219_tbl_init(c1219_tbl_t *tbl);
extern int c1219_full_read(uint8_t *buf, int32_t size, uint16_t table_id);
extern int c1219_pread_default(uint8_t *buf, int32_t size);
extern int c1219_pread_offset(uint8_t *buf, int32_t size, uint16_t table_id,
			      uint32_t offset, uint16_t octet_count);
extern int c1219_pread_index(uint8_t *buf, int32_t size, uint16_t table_id,
			     uint16_t *index, uint16_t element_count);
#endif
