/*
 * C12.18 application layer
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#ifndef __C1218_AL__
#define __C1218_AL__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "c1218_time.h"
#include "c1219_tbl.h"

typedef	enum {
	OK = 0,
	ERR,
	SNS,
	ISC,
	ONP,
	IAR,
	BSY,
	DNR,
	DLK,
	RNO,
	ISSS,
} psem_res_t;

struct c1218_dl;
struct c1218_al_ops;
typedef struct c1218_al {
	struct c1218_dl *dl;

	struct c1219_tbl tbl;
	struct c1218_al_ops *ops;
} c1218_al_t;

typedef struct c1218_al_read_req {
	uint8_t *buf;
	uint16_t size;
	uint16_t rwsize;
	uint16_t table_id;
	uint16_t *index_tbl;
	uint8_t nr_indexs;
	uint32_t offset;
	uint16_t element_octet_count;
} read_req_t;

struct c1218_al_ops {
	int (*ident)(struct c1218_al *al);
	int (*ident_r)(struct c1218_al *al);
	int (*negotiate)(struct c1218_al *al);
	int (*negotiate_r)(struct c1218_al *al, uint8_t *buf, int16_t len);
	int (*logon)(struct c1218_al *al);
	int (*logon_r)(struct c1218_al *al, uint8_t *buf, int16_t len);
	int (*logoff)(struct c1218_al *al);
	int (*logoff_r)(struct c1218_al *al);
	int (*terminate)(struct c1218_al *al);
	int (*terminate_r)(struct c1218_al *al);
	int (*full_read)(struct c1218_al *al, read_req_t *req);
	int (*full_read_r)(struct c1218_al *al, uint8_t *req, int16_t len);
	int (*pread_index)(struct c1218_al *al, read_req_t *req);
	int (*pread_index_r)(struct c1218_al *al, uint8_t *req, int16_t len);
	int (*pread_offset)(struct c1218_al *al, read_req_t *req);
	int (*pread_offset_r)(struct c1218_al *al, uint8_t *req, int16_t len);
	int (*pread_default)(struct c1218_al *al, read_req_t *req);
	int (*pread_default_r)(struct c1218_al *al, uint8_t *req, int16_t len);
};

extern int c1218_al_init(c1218_al_t *al);
extern int c1218_al_run(c1218_al_t *al);

#endif
