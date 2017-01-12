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
#include "lowapp_utils_crc.h"


/*
 * CRC algorithm implementation
 *
 * \ param[IN] crc Previous CRC value
 * \ param[IN] data New data to be added to the CRC
 * \ param[IN] polunomial CRC polynomial selection (CCITT, IBM)
 *
 * \retval crc New computed CRC
 */
uint16_t ComputeCrc(uint16_t crc, uint8_t data, uint16_t polynomial)
{
	uint8_t i=0;

	for(i=0; i < 8; i++)
	{
		if((((crc & 0x8000) >> 8) ^ (data & 0x80)) != 0)
		{
			crc <<= 1;			//shift left once
			crc ^= polynomial;	// XOR with polynomial
		}
		else
		{
			crc <<= 1;			//shift left once
		}
		data <<= 1;				//Next data bit
	}
	return crc;
}

/*
 * CRC algorithm implementation
 *
 * \ param[IN] buffet Array containing the data
 * \ param[IN] bufferLength BUffer Legnth
 * \ param[IN] CRC type selection (CCITT, IBM)
 *
 * \retval crc BUffer computed CRC
 */

uint16_t PacketComputeCrc(uint8_t *buffer, uint8_t bufferLength, uint8_t crcType)
{	
	uint8_t i=0;
	uint16_t crc, polynomial;
	
	polynomial = ( crcType == CRC_TYPE_IBM ) ? POLYNOMIAL_IBM : POLYNOMIAL_CCITT;
	crc = ( crcType == POLYNOMIAL_IBM ) ? CRC_IBM_SEED : CRC_CCITT_SEED_XMODEM;
	
	for( i = 0; i < bufferLength; i++)
	{
		crc = ComputeCrc( crc, buffer[i], polynomial );
	}
	
	if( crcType == CRC_TYPE_IBM )
	{
		return ~crc;
	}
	else
	{
		return crc;
	}
}

#define CRC16 0x8408

uint16_t gen_crc16(uint8_t *data, uint16_t size)
{
    uint16_t out = 0;
    int bits_read = 0, bit_flag;

    /* Sanity check: */
    if(data == NULL)
        return 0;

    while(size > 0)
    {
        bit_flag = out >> 15;

        /* Get next bit: */
        out <<= 1;
        out |= (*data >> bits_read) & 1; // item a) work from the least significant bits

        /* Increment bit counter: */
        bits_read++;
        if(bits_read > 7)
        {
            bits_read = 0;
            data++;
            size--;
        }

        /* Cycle check: */
        if(bit_flag)
            out ^= CRC16;

    }

    // item b) "push out" the last 16 bits
    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag)
            out ^= CRC16;
    }

    // item c) reverse the bits
    uint16_t crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out) crc |= j;
    }

    return ~crc;
}
