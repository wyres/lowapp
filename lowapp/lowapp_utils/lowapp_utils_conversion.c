/**
 * @file lowapp_utils_conversion.c
 * @brief Conversion function for hex, ascii and decimals
 * @date August 16, 2016
 *
 * @author Benjamin Boulet
 * @author Nathan Olff
 */
#include "lowapp_utils_conversion.h"
#include <math.h>

/**
 * @addtogroup lowapp_core
 * @{
 */
/**
 * @addtogroup lowapp_core_utils LoWAPP Core Utility
 * @brief Utility functions used by the LoWAPP Core
 * @{
 */
/**
 * @addtogroup lowapp_core_utils_conversion LoWAPP Core Utility Conversion
 * @brief Conversion functions for ASCII, decimals and binary
 * @{
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

DecimalAsciiConversion_t 	D2AC;
HexaAsciiConversion_t		H2AC;

/* Static functions prototypes */
static void Reset_D2AC( void );
static void Reset_H2AC( void );
static uint8_t AddEndOfString(uint8_t *buf, uint8_t offset);



void HexaAsciiConversion (uint8_t Hexa)
{
	int LSB, MSB;
	LSB = Hexa & 0x0F;
	MSB = (Hexa & 0xF0)>>4;

	Reset_H2AC();
	//Now convert this into ASCII by ORing it with 0x30
	if(LSB >= 0xA)
	{
		H2AC.LSB = LSB + 55;
	}
	else
	{
		H2AC.LSB = LSB | 0x30;
	}
	if(MSB >= 0xA)
	{
		H2AC.MSB = MSB + 55;
	}
	else
	{
		H2AC.MSB = MSB | 0x30;
	}
}


uint8_t DecimalAsciiConversion (uint16_t decimal)
{
	int hundreds, tens, ones, thousands, thousandsdecades;
	ones = decimal%10;
	decimal = decimal/10;
	tens = decimal%10;
	decimal = decimal/10;
	hundreds = decimal%10;
	decimal = decimal/10;
	thousands = decimal%10;
	decimal = decimal/10;
	thousandsdecades = decimal%10;

	Reset_D2AC();
	//Now convert this into ASCII by ORing it with 0x30
	D2AC.unit = ones | 0x30;
	D2AC.decade = (uint8_t)tens | 0x30;
	D2AC.hundred = (uint8_t)hundreds | 0x30;
	D2AC.thousand = (uint8_t)thousands | 0x30;
	D2AC.thousanddecade = (uint8_t)thousandsdecades | 0x30;

	// return the number of digits useful
	if(D2AC.thousanddecade!=48)
	{
		return 5;
	}
	if(D2AC.thousand!=48)
	{
		return 4;
	}
	if(D2AC.hundred!=48)
	{
		return 3;
	}
	if(D2AC.decade!=48)
	{
		return 2;
	}
return 1;
}

/* Fill a buffer with a data array converted into hex (big endian) characters (as ascii) */
uint8_t FillBufferHexBI8_t(uint8_t *buffer,uint8_t BufferOffset,uint8_t *Data, uint8_t DataSize, bool addEndChar)
{
	uint8_t i=DataSize,index=0;

	while(i>0)
	{
		HexaAsciiConversion(Data[i-1]);
		buffer[BufferOffset+index]=H2AC.MSB;
		buffer[BufferOffset+index+1]=H2AC.LSB;
		index+=2;
		i--;
	}
	/* Add end char if asked */
	if(addEndChar) {
		index += AddEndOfString(buffer, BufferOffset+index);
	}

	return BufferOffset+index;
}

uint8_t FillBufferHexLI8_t(uint8_t *buffer,uint8_t BufferOffset,uint8_t *Data, uint8_t DataSize, bool addEndChar)
{
	uint8_t i=0,index=0;

	while(i<DataSize)
	{
		HexaAsciiConversion(Data[i]);
		buffer[BufferOffset+index]=H2AC.MSB;
		buffer[BufferOffset+index+1]=H2AC.LSB;
		index+=2;
		i++;
	}
	/* Add end char if asked */
	if(addEndChar) {
		index += AddEndOfString(buffer, BufferOffset+index);
	}
	return BufferOffset+index;
}

/*
 * Fill a buffer with the conversion into decimal of a integer array,
 * seperated by dots. This is useful for displaying non integer values.
 */
uint8_t FillBuffer8_t(uint8_t *buffer,uint8_t BufferOffset,uint8_t *Data, uint8_t DataSize, bool addEndChar)
{
	uint8_t i=0,UsefulDigit,index=0;

	while(i<DataSize)
	{
		UsefulDigit=DecimalAsciiConversion((uint16_t)Data[i]);

		if(UsefulDigit==3)
		{
			buffer[BufferOffset+index]=D2AC.hundred;
			buffer[BufferOffset+index+1]=D2AC.decade;
			buffer[BufferOffset+index+2]=D2AC.unit;
			index+=UsefulDigit;
			UsefulDigit=0;
		}
		if(UsefulDigit==2)
		{
			buffer[BufferOffset+index]=D2AC.decade;
			buffer[BufferOffset+index+1]=D2AC.unit;
			index+=UsefulDigit;
			UsefulDigit=0;
		}
		if(UsefulDigit==1)
		{
			buffer[BufferOffset+index]=D2AC.unit;
			index+=UsefulDigit;
			UsefulDigit=0;
		}

		i++;
	}
	/* Add end character if necessary */
	if(addEndChar) {
		index += AddEndOfString(buffer, BufferOffset+index);
	}
	return BufferOffset+index;
}

uint8_t FillBuffer16_t(uint8_t *buffer,uint8_t BufferOffset,uint16_t *Data, uint8_t DataSize, bool addEndChar)
{
	uint8_t i=0,UsefulDigit,index=0;

	while(i<DataSize)
	{
		UsefulDigit=DecimalAsciiConversion(Data[i]);
		if(UsefulDigit==5)
		{
			buffer[BufferOffset+index]=D2AC.thousanddecade;
			buffer[BufferOffset+index+1]=D2AC.thousand;
			buffer[BufferOffset+index+2]=D2AC.hundred;
			buffer[BufferOffset+index+3]=D2AC.decade;
			buffer[BufferOffset+index+4]=D2AC.unit;
			index+=UsefulDigit;
			UsefulDigit=0;
		}
		if(UsefulDigit==4)
		{
			buffer[BufferOffset+index]=D2AC.thousand;
			buffer[BufferOffset+index+1]=D2AC.hundred;
			buffer[BufferOffset+index+2]=D2AC.decade;
			buffer[BufferOffset+index+3]=D2AC.unit;
			index+=UsefulDigit;
			UsefulDigit=0;
		}
		if(UsefulDigit==3)
		{
			buffer[BufferOffset+index]=D2AC.hundred;
			buffer[BufferOffset+index+1]=D2AC.decade;
			buffer[BufferOffset+index+2]=D2AC.unit;
			index+=UsefulDigit;
			UsefulDigit=0;
		}
		if(UsefulDigit==2)
		{
			buffer[BufferOffset+index]=D2AC.decade;
			buffer[BufferOffset+index+1]=D2AC.unit;
			index+=UsefulDigit;
			UsefulDigit=0;
		}
		if(UsefulDigit==1)
		{
			buffer[BufferOffset+index]=D2AC.unit;
			index+=UsefulDigit;
			UsefulDigit=0;
		}

		i++;
	}
	/* Add end character if necessary */
	if(addEndChar) {
		index += AddEndOfString(buffer, BufferOffset+index);
	}
	return BufferOffset+index;
}

/*
 * Convert ascii string into an integer.
 * The string is read as a decimal string
 *
 * @param InBuffer Input ascii values
 * @param BufSise Size of InBuffer
 * @return The buffer converted as one decimal value
 */
uint16_t AsciiDecStringConversion_t( const uint8_t *InBuffer , uint8_t BufSize)
{
	uint8_t i=0;
	uint16_t val=0;

	/* Convert first digit between 0 and 9 */
	if((InBuffer[i]>=48)&&InBuffer[i]<=57)
	{
		//Convert the Ascii number (0-9) in real number
		val += (InBuffer[i]-ASCII_DEC_OFFSET);
	}
	i++;
	while(i<BufSize)
	{
		/* Convert digit between 0 and 9 */
		if((InBuffer[i]>=48)&&InBuffer[i]<=57)
		{
			/* Shift the decimal numbers */
			val *= 10;
			/* Add the digit converted from Ascii number (0-9) */
			val += (InBuffer[i]-ASCII_DEC_OFFSET);
		}
		else {
			return 0;
		}
		i++;
	}
	return val;
}

/*
 * Convert ascii string into an integer array.
 * The string is read as little endian
 * Arg :
 * 		OutBuffer : output binary value(s)
 * 		InBuffer : input ascii value(s) as little endian
 * 		BufSize is the size of the InBuffer
 */
uint8_t AsciiHexStringConversionLI8_t( uint8_t *OutBuffer, const uint8_t *InBuffer , uint8_t BufSize)
{
	uint8_t i=0,j=0,k=0;

	while(k<(BufSize/2))
	{
		OutBuffer[k]=0;
		for(j=0;j<2;j++)
		{
			if((InBuffer[i]>=48)&&InBuffer[i]<=57)
			{
				if(j==1)
				{
					//Convert the Ascii number (0-9) in real number (unit)
					OutBuffer[k]+=(InBuffer[i]-ASCII_DEC_OFFSET);
				}
				if(j==0)
				{
					//Convert the Ascii number (0-9) in real number (tens)
					OutBuffer[k]+=((InBuffer[i]-ASCII_DEC_OFFSET)*16);
				}
			}
			else if((InBuffer[i]>=65)&&InBuffer[i]<=70)
			{
				if(j==1)
				{
					//Convert the Ascii number hex (A-F) in real number (unit)
					OutBuffer[k]+=(InBuffer[i]-55);
				}
				if(j==0)
				{
					//Convert the Ascii number hex (A-F) in real number (tens)
					OutBuffer[k]+=((InBuffer[i]-55)*16);
				}
			}
			else {
				return 0;
			}
			i++;
		}
		k++;
	}
	return 1;
}

/*
 * Convert ascii string into an integer array.
 * The string is read as big endian
 * Arg :
 * 		OutBuffer : output binary value(s)
 * 		InBuffer : input ascii value(s) as big endian
 * 		BufSize is the size of the InBuffer
 */
uint8_t AsciiHexStringConversionBI8_t( uint8_t *OutBuffer, const uint8_t *InBuffer , uint8_t BufSize)
{
	uint8_t i=0,j=0;
	int8_t k=0;
	k = BufSize/2-1;
	while(k>=0)
	{
		OutBuffer[k]=0;
		for(j=0;j<2;j++)
		{
			if((InBuffer[i]>=48)&&InBuffer[i]<=57)
			{
				if(j==1)
				{
					//Convert the Ascii number (0-9) in real number (unit)
					OutBuffer[k]+=(InBuffer[i]-ASCII_DEC_OFFSET);
				}
				if(j==0)
				{
					//Convert the Ascii number (0-9) in real number (tens)
					OutBuffer[k]+=((InBuffer[i]-ASCII_DEC_OFFSET)*16);
				}
			}
			else if((InBuffer[i]>=65)&&InBuffer[i]<=70)
			{
				if(j==1)
				{
					//Convert the Ascii number hex (A-F) in real number (unit)
					OutBuffer[k]+=(InBuffer[i]-55);
				}
				if(j==0)
				{
					//Convert the Ascii number hex (A-F) in real number (tens)
					OutBuffer[k]+=((InBuffer[i]-55)*16);
				}
			}
			else {
				return 0;
			}
			i++;
		}
		k--;
	}
	return 1;
}

/*
 * Convert ascii string into an integer.
 * The string is read as big endian
 * Arg :
 * 		OutBuffer : output binary value
 * 		InBuffer : input ascii value as big endian
 */
uint8_t AsciiHexConversionOneValueBI8_t( uint8_t *OutBuffer, const uint8_t *InBuffer)
{
	uint8_t temp[2];
	if(!((InBuffer[1]>=48)&&InBuffer[1]<=57) && !((InBuffer[1]>=65)&&InBuffer[1]<=70)) {
		temp[0] = '0';
		temp[1] = InBuffer[0];
		return AsciiHexStringConversionBI8_t(OutBuffer, temp, 2);
	}
	else {
		return AsciiHexStringConversionBI8_t(OutBuffer, InBuffer, 2);
	}
}

static uint8_t AddEndOfString(uint8_t *buf, uint8_t offset) {
//	buf[offset] = '\r';
//	buf[offset+1] = '\n';
	buf[offset] = '\0';
	return 1;
}

/* Reset global variables used for conversion process */
void Reset_D2AC( void )
{
	D2AC.unit = 0;
	D2AC.decade = 0;
	D2AC.hundred = 0;
	D2AC.thousand = 0;
	D2AC.thousanddecade = 0;
}

/* Reset global variables used for conversion process */
static void Reset_H2AC( void )
{
	H2AC.LSB = 0;
	H2AC.MSB = 0;
}

#endif

/**
 * @brief Wrap uint16_t value into a uin8_t* buffer as big endian
 *
 * @param buf : buffer in which to add the value. The pointer is moved
 * two bytes after to point to the next location
 * @param value : value to be copied to the buffer
 */
void wrap_short(uint8_t** buf, uint16_t value) {
	// Stored as Big Endin
	wrap_byte(buf, (uint8_t)(value >> 8));
	wrap_byte(buf, (uint8_t)(value & 0xFF));
}

/**
 * @brief Wrap uint8_t value into a uin8_t* buffer
 *
 * @param buf : buffer in which to add the value. The pointer is moved
 * one byte after to point to the next location
 * @param value : value to be copied to the buffer
 */
void wrap_byte(uint8_t** buf, uint8_t value) {
	memcpy(*buf, &value, sizeof(uint8_t));
	(*buf)++;
}

/**
 * @brief Parse a uint16_t from a uint8_t* buffer
 *
 * This functions moves the pointer after the short value was parsed
 * @param buf : pointer to a uint16_t stored in a buffer
 * @return The uint16_t value parsed from the buffer
 */
uint16_t parse_short(uint8_t** buf) {
	uint16_t data;
	/* Store the uint16_t from big endian form */
	data = parse_byte(buf) << 8;
	data |= parse_byte(buf);
	return data;
}

/**
 * @brief Parse a uint16_t from a uint8_t* buffer
 *
 * This function does not move the pointer
 * @param buf : pointer to a uint16_t stored in a buffer
 * @return The uint16_t value parsed from the buffer
 */
uint16_t get_short(uint8_t* buf) {
	uint16_t data;
	/* Store the uint16_t from big endian form */
	data = get_byte(buf) << 8;
	data |= get_byte(buf+1);
	return data;
}

/**
 * @brief Parse a uint8_t from a uint8_t* buffer
 *
 * This functions moves the pointer after the short value was parsed
 *
 * @param buf : pointer to a uint8_t stored in a buffer
 * @return The uint16_t value parsed from the buffer
 */
uint8_t parse_byte(uint8_t** buf) {
	uint8_t data = get_byte(*buf);
	(*buf)++;
	return data;
}

/**
 * @brief Parse a uint8_t from a uint8_t* buffer
 *
 * This function does not move the pointer
 * @param buf : pointer to a uint8_t stored in a buffer
 * @return The uint16_t value parsed from the buffer
 */
uint8_t get_byte(uint8_t* buf) {
	uint8_t data;
	memcpy(&data, buf, sizeof(uint8_t));
	return data;
}

/** @} */
/** @} */
/** @} */
