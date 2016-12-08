/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: CRC functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Boulet Benjamin
*/
#ifndef __CRC_H__
#define __CRC_H__

#include <stdint.h>
#include <stdlib.h>

//CRC types
#define CRC_TYPE_CCITT			0
#define CRC_TYPE_IBM			1

//Polynomial = x^16 + x^12 + x^5 +1
#define POLYNOMIAL_CCITT		0x1021
//Polynomial = x^16 + x^12 + x^2 +1
#define POLYNOMIAL_IBM			0x8005

//Seeds
#define CRC_CCITT_SEED			0x1D0F
#define CRC_CCITT_SEED_XMODEM	0x0000
#define CRC_IBM_SEED			0xFFFF

uint16_t ComputeCrc(uint16_t Crc, uint8_t Data, uint16_t Polynomial);

uint16_t PacketComputeCrc(uint8_t *Buffer, uint8_t BufferLength, uint8_t CrcType);

uint16_t gen_crc16(uint8_t *data, uint16_t size);

#endif // __CRC_H__

