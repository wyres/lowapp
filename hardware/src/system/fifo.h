/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Implements a FIFO buffer

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#ifndef __FIFO_H__
#define __FIFO_H__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*!
 * FIFO structure
 */
typedef struct Fifo_s
{
    uint16_t Begin;
    uint16_t End;
    uint8_t *Data;
    uint16_t Size;
}Fifo_t;

/*!
 * Initializes the FIFO structure
 *
 * \param [IN] fifo   Pointer to the FIFO object
 * \param [IN] buffer Buffer to be used as FIFO
 * \param [IN] size   Size of the buffer
 */
void FifoInit( Fifo_t *fifo, uint8_t *buffer, uint16_t size );

/*!
 * Pushes data to the FIFO
 *
 * \param [IN] fifo Pointer to the FIFO object
 * \param [IN] data Data to be pushed into the FIFO
 */
void FifoPush( Fifo_t *fifo, uint8_t data );

/*!
 * Pops data from the FIFO
 *
 * \param [IN] fifo Pointer to the FIFO object
 * \retval data     Data popped from the FIFO
 */
uint8_t FifoPop( Fifo_t *fifo );

/*!
 * Flushes the FIFO
 *
 * \param [IN] fifo   Pointer to the FIFO object
 */
void FifoFlush( Fifo_t *fifo );

/*!
 * Checks if the FIFO is empty
 *
 * \param [IN] fifo   Pointer to the FIFO object
 * \retval isEmpty    true: FIFO is empty, false FIFO is not empty
 */
bool IsFifoEmpty( Fifo_t *fifo );

/*!
 * Checks if the FIFO is full
 *
 * \param [IN] fifo   Pointer to the FIFO object
 * \retval isFull     true: FIFO is full, false FIFO is not full
 */
bool IsFifoFull( Fifo_t *fifo );

/*!
 * Checks if there is a complete string in the FiFo by detecting the\0
 *
 * \param [IN] fifo   Pointer to the FIFO object
 * \retval isFull     true: complete string in FiFo, not complete string in FiFo
 */
bool CompleteStringInFiFo(Fifo_t *fifo);

/*!
 * Checks if there is a complete string in the FiFo by detecting the EOT
 *
 * \param [IN] fifo   Pointer to the FIFO object
 * \retval isFull     true: complete string in FiFo, not complete string in FiFo
 */
bool CompleteIbeaconStringInFiFo(Fifo_t *fifo);

/*!
 * Checks if there is a complete string in the FiFo by detecting the\0
 *
 * \param [IN] fifo   Pointer to the FIFO object / buffer Pointer to the output buffer
 * \retval none
 */
void CopyFiFoInString(uint8_t *buffer, Fifo_t *fifo);
void CopyFiFoInString2(uint8_t *buffer, Fifo_t *fifo, uint8_t Offset);

/*!
 * Set the string end char need to detect a complete string
 *
 * \param [IN] the String End Char (ex : \n , \r, EOT etc...)
 * \retval none
 */
void SetEndChar (uint8_t StringEndChar);

/*!
 * Get the string end char used to detect a complete string
 *
 * \param [IN] None
 * \retval the String End Char (ex : \n , \r, EOT etc...)
 */
uint8_t GetEndChar ( void );

/*!
 * Get the string end char used to detect a complete string
 *
 * \param [IN] None
 * \retval the String End Char (ex : \n , \r, EOT etc...)
 */
uint8_t GetEndChar2 ( void );



#endif // __FIFO_H__
