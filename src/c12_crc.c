/*
 * c12_crc.c
 *
 *  Created on: Aug 4, 2016
 *      Author: mingtan1
 *
 *  Copy form C12.22 Annex H - CRC Examples.
 *  This is not the same with CCITT CRC.
 */

#include "c12_crc.h"

unsigned short crc16(unsigned char octet, unsigned short crc)
{
	int i;

	for (i = 8; i; i--)
	{
		if (crc & 0x0001)
		{
			crc >>= 1;
			if (octet & 0x01)
				crc |= 0x8000;
			crc = crc ^ 0x8408;   /* 0x1021 inverted = 1000 0100 0000 1000 */
			octet >>= 1;
		}
		else
		{
			crc >>= 1;
			if (octet & 0x01)
				crc |= 0x8000;
			octet >>= 1;
		}
	}

	return crc;
}

unsigned short c12_crc(int size, unsigned char *packet)
{
	int i;
	unsigned short crc;

	crc = (~packet[1] << 8) | (~packet[0] & 0xFF);

	for (i=2 ; i<size; i++)
		crc = crc16(packet[i], crc);

	crc = crc16(0x00, crc);
	crc = crc16(0x00, crc);
	crc = ~crc;

	crc = crc >> 8 | crc << 8; /* SWAP the bytes for Little Endian presentation per HDLC */

	return crc;
}
