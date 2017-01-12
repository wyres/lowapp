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
#include "fifo.h"

uint8_t EndChar=13;	// \r
uint8_t EndChar2=10;	// \n

static uint16_t FifoNext( Fifo_t *fifo, uint16_t index )
{
    return ( index + 1 ) % fifo->Size;
}

void FifoInit( Fifo_t *fifo, uint8_t *buffer, uint16_t size )
{
    fifo->Begin = 0;
    fifo->End = 0;
    fifo->Data = buffer;
    fifo->Size = size;
}

void FifoPush( Fifo_t *fifo, uint8_t data )
{
	fifo->Data[fifo->End] = data;
	fifo->End = FifoNext( fifo, fifo->End );
}

uint8_t FifoPop( Fifo_t *fifo )
{
    uint8_t data = fifo->Data[FifoNext( fifo, fifo->Begin )];

    fifo->Begin = FifoNext( fifo, fifo->Begin );
    return data;
}

void FifoFlush( Fifo_t *fifo )
{
    fifo->Begin = 0;
    fifo->End = 0;
}

bool IsFifoEmpty( Fifo_t *fifo )
{
    return ( fifo->Begin == fifo->End );
}

bool IsFifoFull( Fifo_t *fifo )
{
    return ( FifoNext( fifo, fifo->End ) == fifo->Begin );
}

bool CompleteStringInFiFo(Fifo_t *fifo)
{
//	if(fifo->Data[fifo->End] == EndChar)
	if(fifo->Data[(fifo->End-2)%fifo->Size] == EndChar &&
			fifo->Data[(fifo->End-1)%fifo->Size] == EndChar2)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

bool CompleteIbeaconStringInFiFo(Fifo_t *fifo)
{
	if(fifo->Data[fifo->End] == 0x04)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
void CopyFiFoInString(uint8_t *buffer, Fifo_t *fifo)

{
	int16_t i=0;
	i=fifo->End;
	while(i>=0)
	{
		buffer[i]=fifo->Data[i+1];
		i--;
	}
}

void CopyFiFoInString2(uint8_t *buffer, Fifo_t *fifo, uint8_t Offset)
{
	memcpy(buffer,fifo->Data+1+Offset,fifo->End);
}

void SetEndChar (uint8_t StringEndChar)
{
	EndChar=StringEndChar;
}

uint8_t GetEndChar ( void )
{
	return EndChar;
}

uint8_t GetEndChar2 ( void )
{
	return EndChar2;
}

