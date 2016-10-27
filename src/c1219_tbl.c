/*
 * C12.19 table, sample, fixed table
 * author: huang shuang
 * email: nikshuang@sina.com
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "c1219_tbl.h"
#include "c1218_errno.h"
#include "ber.h"

c1219_tbl_t c1219_tbl;

int c1219_tbl_init(c1219_tbl_t *tbl)
{
	bercode_t ber;

	ber_word16_encode((uint16_t)55555, &ber);
	memcpy(tbl->relative_uid, ber.buf, ber.len);
	tbl->relative_uid_len = ber.len;
	tbl->identity_char_format = 2;
	// 1-2.345
	tbl->identification_len = 8;
	tbl->identification[0] = 0;
	tbl->identification[1] = 1;
	tbl->identification[2] = 0xa;
	tbl->identification[3] = 2;
	tbl->identification[4] = 0xd;
	tbl->identification[5] = 3;
	tbl->identification[6] = 4;
	tbl->identification[7] = 5;

	tbl->user_id = 6666;

	return 0;
}

int c1219_full_read(uint8_t *buf, int32_t size, uint16_t table_id)
{
	return 0;
}

int32_t c1219_pread_default(uint8_t *buf, int32_t size)
{
#if 1
	int fd;
	int32_t rsize;

	fd = open(RAW_DATA, O_RDONLY);
	if (fd < 0) {
		perror(RAW_DATA);
		return IO_ERR;
	}

	rsize = read(fd, buf, size);
	if (rsize < 0) {
		perror(RAW_DATA);
		return IO_ERR;
	}

	close(fd);

	return rsize;
#endif
}

int c1219_pread_offset(uint8_t *buf, int32_t size, uint16_t table_id,
			      uint32_t offset, uint16_t octet_count)
{
	return 0;
}

int c1219_pread_index(uint8_t *buf, int32_t size, uint16_t table_id,
			     uint16_t *index, uint16_t element_count)
{
	return 0;
}
