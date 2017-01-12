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

extern RadioEvents_t *RadioEvents;

/**
 * Set implicit header value for Tx (only in LoRa mode)
 *
 * @param fixLen If the packet has a fixed length
 */
void setTxFixLen(bool fixLen) {
    SX1272.Settings.LoRa.FixLen = fixLen;
    /* Write config to the radio */
    SX1272Write( REG_LR_MODEMCONFIG1,
		 ( SX1272Read( REG_LR_MODEMCONFIG1 ) &
		   RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
		   ( fixLen << 2 ) );
}

/**
 * Set implicit header value for Rx (only in LoRa mode)
 *
 * @param fixLen If the packet has a fixed length
 * @param payloadLen Size of the expected packet
 */
void setRxFixLen(bool fixLen, uint8_t payloadLen) {
	SX1272.Settings.LoRa.FixLen = fixLen;
	SX1272.Settings.LoRa.PayloadLen = payloadLen;
	/* Write config to the radio */
	SX1272Write( REG_LR_MODEMCONFIG1,
			 ( SX1272Read( REG_LR_MODEMCONFIG1 ) &
			   RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) |
			   ( fixLen << 2 ) );
	if( fixLen == 1 )
	{
		SX1272Write( REG_LR_PAYLOADLENGTH, payloadLen );
	}
}


/**
 * Set preamble length (only in LoRa mode)
 *
 * @param preambleLen Preamble length (in symbols)
 */
void setPreambleLength(uint16_t preambleLen) {
    SX1272.Settings.LoRa.PreambleLen = preambleLen;
    /* Write preamble length to the radio */
    SX1272Write( REG_LR_PREAMBLEMSB, ( uint8_t )( ( preambleLen >> 8 ) & 0xFF ) );
	SX1272Write( REG_LR_PREAMBLELSB, ( uint8_t )( preambleLen & 0xFF ) );
}

/**
 * Set TX timeout
 *
 * @param timeout Timeout for transmission
 */
void setTxTimeout(uint32_t timeout) {
    SX1272.Settings.LoRa.TxTimeout = timeout;
}

/**
 * Set RX continuous
 *
 * @param rxContinuous Continuous mode for RX
 */
void setRxContinuous(bool rxContinuous) {
    SX1272.Settings.LoRa.RxContinuous = rxContinuous;
}

/**
 * Set radio callbacks
 *
 * @param events Set of radio callbacks
 */
void setRadioCallbacks( RadioEvents_t *events ) {
	RadioEvents = events;
}
