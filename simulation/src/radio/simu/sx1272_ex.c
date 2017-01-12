/**
 * @file sx1272_ex.c
 * @brief Extension for SX1272
 *
 * Additional functions used to set some configuration
 * values for Rx and Tx without doing a complete SetRx
 * or SetTx
 *
 * @author Nathan Olff
 * @date October 31, 2016
 */

#include "sx1272_ex.h"
#include "radio-simu.h"

extern RadioEvents_t *RadioEvents;
extern RadioSettings_t Settings;

/**
 * Set implicit header value for Tx (only in LoRa mode)
 *
 * @param fixLen If the packet has a fixed length
 */
void setTxFixLen(bool fixLen) {
    Settings.LoRa.FixLen = fixLen;
}

/**
 * Set implicit header value for Rx (only in LoRa mode)
 *
 * @param fixLen If the packet has a fixed length
 * @param payloadLen Size of the expected packet
 */
void setRxFixLen(bool fixLen, uint8_t payloadLen) {
	Settings.LoRa.FixLen = fixLen;
	Settings.LoRa.PayloadLen = payloadLen;
}


/**
 * Set preamble length (only in LoRa mode)
 *
 * @param preambleLen Preamble length (in symbols)
 */
void setPreambleLength(uint16_t preambleLen) {
    Settings.LoRa.PreambleLen = preambleLen;
}

/**
 * Set TX timeout
 *
 * @param timeout Timeout for transmission (in ms)
 */
void setTxTimeout(uint32_t timeout) {
    Settings.LoRa.TxTimeout = timeout;
}

/**
 * Set RX continuous
 *
 * @param rxContinuous Continuous mode for RX
 */
void setRxContinuous(bool rxContinuous) {
    Settings.LoRa.RxContinuous = rxContinuous;
}

/**
 * Set radio callbacks
 *
 * @param events Set of radio callbacks
 */
void setRadioCallbacks( RadioEvents_t *events ) {
	RadioEvents = events;
}
