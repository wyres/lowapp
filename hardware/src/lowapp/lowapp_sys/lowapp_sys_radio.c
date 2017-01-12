/**
 * @file lowapp_sys_radio.c
 * @brief Implementation of the Radio related functions for LoWAPP
 *
 * @author Nathan Olff
 * @date October 24, 2016
 */
#include "lowapp_sys_radio.h"
#include "sx1272_ex.h"
#include "board.h"

/**
 * @addtogroup lowapp_hardware_sys
 * @{
 */
/**
 * @addtogroup lowapp_hardware_sys_radio LoWAPP Hardware System Radio Interface
 * @brief System Radio Middle Layer With The SX1272
 * @{
 */

/**
 * Set of radio events used for initialising the SX1272
 */
RadioEvents_t events;

extern const uint32_t channelFrequencies[];

/**
 * Initialise the radio
 */
void radio_init(Lowapp_RadioEvents_t *evt) {
	/* Set events structure for radio init */
	events.CadDone = evt->CadDone;
	events.RxDone = evt->RxDone;
	events.FhssChangeChannel = NULL;
	events.RxError = evt->RxError;
	events.RxTimeout = evt->RxTimeout;
	events.TxDone = evt->TxDone;
	events.TxTimeout = evt->TxTimeout;
	Radio.Init(&events);
}

/**
 * Initialise the radio
 */
void radio_setCallbacks(Lowapp_RadioEvents_t *evt) {
	/* Set events structure for radio init */
	events.CadDone = evt->CadDone;
	events.RxDone = evt->RxDone;
	events.FhssChangeChannel = NULL;
	events.RxError = evt->RxError;
	events.RxTimeout = evt->RxTimeout;
	events.TxDone = evt->TxDone;
	events.TxTimeout = evt->TxTimeout;
	setRadioCallbacks(&events);
}

/**
 * Send function for the radio
 */
void radio_send(uint8_t *data, uint8_t dlen) {
	Radio.Send(data, dlen);
}


/**
 * Set radio TX config
 */
void radio_setTxConfig(int8_t power, uint8_t bandwidth, uint8_t datarate,
		uint8_t coderate, uint16_t preambleLen, uint32_t timeout, bool fixLen) {
	Radio.SetTxConfig(MODEM_LORA, power, 0, bandwidth, datarate, coderate, preambleLen,
			fixLen, LOWAPP_SYS_RADIO_CRC, 0, 0, 0, timeout);
}

/**
 * Set radio RX config
 */
void radio_setRxConfig(uint8_t bandwidth, uint8_t datarate, uint8_t coderate,
		uint16_t preambleLen, bool fixLen, uint8_t payloadLen, bool rxContinuous) {
	Radio.SetRxConfig(MODEM_LORA, bandwidth, datarate, coderate, 0, preambleLen,
			LOWAPP_SYMBOL_TIMEOUT, fixLen, payloadLen, LOWAPP_SYS_RADIO_CRC, 0, 0, 0, rxContinuous);
}

/**
 * Set radio channel
 */
void radio_setChannel(uint32_t freq) {
	if(Radio.CheckRfFrequency(freq)) {
		Radio.SetChannel(freq);
	}
}

/**
 * Set radio to sleep mode
 */
void radio_sleep() {
	Radio.Sleep();
}

/**
 * Computes the packet time on air in us for the given payload
 *
 * @param pktLen Packet length
 * @return Computed air time in us for the given packet length
 */
uint32_t radio_timeOnAir(uint8_t pktLen) {
	return Radio.TimeOnAir(MODEM_LORA, pktLen);
}



/**
 * Listen Before Talk
 *
 * Check if the radio channel is free for a transmission
 *
 * @param chan Radio channel to check
 * @retval True If the channel is free
 * @retval False If the channel is not free
 */
bool radio_lbt(uint8_t chan) {
	return Radio.IsChannelFree(MODEM_LORA, channelFrequencies[chan], LOWAPP_SYS_RADIO_RSSI);
}

/**
 * Signal radio thread to start CAD process
 */
void radio_cad() {
	Radio.StartCad();
}

/**
 * Signal radio thread to start reception process
 *
 * @param timeout Reception timeout (ms)
 */
void radio_rx(uint32_t timeout) {
	/* TODO Change return types of most of those radio functions */
	Radio.Rx(timeout);
}

/**
 * Generate 32 bit random value based in the RSSI readings
 */
uint32_t radio_random( void ) {
	return Radio.Random();
}

/** @} */
/** @} */
