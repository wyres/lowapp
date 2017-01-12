/**
 * @file lowapp_sys_radio.h
 * @brief Implementation of the Radio related functions for LoWAPP
 *
 * @author Nathan Olff
 * @date October 24, 2016
 */
#ifndef LOWAPP_SYS_RADIO_H_
#define LOWAPP_SYS_RADIO_H_

#include "lowapp_sys.h"
#include "timer.h"

/**
 * CRC parameter for SetTx and SetRx
 *
 * Enable or disable CRC for the SX1272
 */
#define LOWAPP_SYS_RADIO_CRC	1

/**
 * Threshold used for LBT
 */
#define LOWAPP_SYS_RADIO_RSSI	-85

/**
 * LoWapp symbol timeout for RxConfig
 *
 * Minimum time for the modem to acquire lock = 4
 */
#define LOWAPP_SYMBOL_TIMEOUT        4


void radio_init(Lowapp_RadioEvents_t *evt);
void radio_setTxConfig(int8_t power, uint8_t bandwidth, uint8_t datarate,
		uint8_t coderate, uint16_t preambleLen, uint32_t timeout, bool fixLen);
void radio_setRxConfig(uint8_t bandwidth, uint8_t datarate, uint8_t coderate,
		uint16_t preambleLen, bool fixLen, uint8_t payloadLen, bool rxContinuous);
void radio_setChannel(uint32_t chan);
uint32_t radio_timeOnAir(uint8_t pktLen);
bool radio_lbt(uint8_t chan);
void radio_send(uint8_t *data, uint8_t dlen);
void radio_rx(uint32_t timeout);
void radio_cad();
void radio_sleep();
void radio_setCallbacks(Lowapp_RadioEvents_t *evt);
uint32_t radio_random( void );

#endif
