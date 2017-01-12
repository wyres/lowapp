/**
 * @file lowapp_utils_conversion.h
 * @brief Conversion function for hex, ascii and decimals
 * @date August 16, 2016
 *
 * @author Benjamin Boulet
 * @author Nathan Olff
 */

#ifndef LOWAPP_UTILS_CONVERSION_H_
#define LOWAPP_UTILS_CONVERSION_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/** Ascii value of '0', used to convert ascii digits (0 to 9)*/
#define ASCII_DEC_OFFSET	48
/** Ascii value of 'A', used to convert ascii letters (A to F) */
#define ASCII_HEX_OFFSET	30

/**
 * @brief Structure used to store each digit of a decimal number for conversion purposes
 */
typedef struct {
	uint8_t unit;				/**< Unit digit */
	uint8_t decade;				/**< Decade digit */
	uint8_t	hundred;			/**< Hundred digit */
	uint8_t	thousand;			/**< Thousand digit */
	uint8_t	thousanddecade;		/**< Thousand decade digit */
 }DecimalAsciiConversion_t;

/**
 * @brief Structure used to store 2-bytes values for conversion purposes.
 *
 * Stores a two-bytes value as a MSB and a LSB.
 */
typedef struct {
	 uint8_t LSB;	/**< Least Significant Byte of the value */
	 uint8_t MSB;	/**< Most Significant Byte of the value */
}HexaAsciiConversion_t;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

void HexaAsciiConversion (uint8_t Hexa);
uint8_t DecimalAsciiConversion (uint16_t decimal);
uint16_t AsciiDecStringConversion_t( const uint8_t *InBuffer , uint8_t BufSize);
uint8_t FillBuffer8_t(uint8_t *buffer,uint8_t BufferOffset,uint8_t *Data, uint8_t DataSize, bool addEndChar);
uint8_t FillBuffer16_t(uint8_t *buffer,uint8_t BufferOffset,uint16_t *Data, uint8_t DataSize, bool addEndChar);
uint8_t FillBufferHexBI8_t(uint8_t *buffer,uint8_t BufferOffset,uint8_t *Data, uint8_t DataSize, bool addEndChar);
uint8_t FillBufferHexLI8_t(uint8_t *buffer,uint8_t BufferOffset,uint8_t *Data, uint8_t DataSize, bool addEndChar);
uint8_t AsciiHexStringConversionBI8_t( uint8_t *OutBuffer, const uint8_t *InBuffer , uint8_t BufSize);
uint8_t AsciiHexConversionOneValueBI8_t( uint8_t *OutBuffer, const uint8_t *InBuffer);
uint8_t AsciiHexStringConversionLI8_t( uint8_t *OutBuffer, const uint8_t *InBuffer , uint8_t BufSize);

#endif

void wrap_short(uint8_t** buf, uint16_t bytes);
void wrap_byte(uint8_t** buf, uint8_t byte);
uint16_t parse_short(uint8_t** buf);
uint8_t parse_byte(uint8_t** buf);
uint16_t get_short(uint8_t* buf);
uint8_t get_byte(uint8_t* buf);


#endif
