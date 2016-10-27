/*
 * C12.18 error number and msg
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#ifndef __C1218_ERRNO__
#define __C1218_ERRNO__

typedef enum {
	GENERAL_ERR = -1,
	IO_ERR = -2,
	DATA_ERR = -3,
	ARG_ERR = -4,
	MEM_ERR = -5,
	RES_TIMEOUT_ERR = -6,
	PACKET_TRANSMIT_ERR = -7,
	TRAFFIC_TIMEOUT_ERR = -8,
	RANGE_ERR = -9,
	CREATE_PACKET_ERR = -10,
	ASSEMBLE_PACKET_ERR = -11,
	DUPLICATE_PACKET_ERR = -12,
} c1218_errno_t;

#endif
