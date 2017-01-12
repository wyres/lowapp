/**
 * @file sx1272_ex.h
 * @brief Extension for SX1272
 *
 * Additional functions used to set some configuration
 * values for Rx and Tx without doing a complete SetRx
 * or SetTx
 *
 * @author Nathan Olff
 * @date October 31, 2016
 */

#ifndef __SX1272_EX_H__
#define __SX1272_EX_H__

#include "board.h"
#include "radio.h"
#include "sx1272.h"
#include "sx1272-board.h"

void setTxFixLen(bool fixLen);
void setRxFixLen(bool fixLen, uint8_t payloadLen);
void setPreambleLength(uint16_t preambleLen);
void setTxTimeout(uint32_t timeout);
void setRxContinuous(bool rxContinuous);
void setRadioCallbacks( RadioEvents_t *events );


#endif
