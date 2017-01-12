/**
 * @file lowapp_radio_evt.c
 * @brief LoWAPP radio event
 *
 * Defines the functions that are to be called from the radio layer.
 *
 * @author Nathan Olff
 * @date August 22, 2016
 */

#include "lowapp_inc.h"

extern QEVENT_T _eventQ;

/** Safeguard timer for receiving standard message */
uint32_t timer_safeguard_rxing_std;
/** Safeguard timer for receiving ACK */
uint32_t timer_safeguard_rxing_ack;
/** Safeguard timer for transmission */
uint32_t timer_safeguard_txing_std;
/** Safeguard timer for transmission of ACK */
uint32_t timer_safeguard_txing_ack;

/** Set of radio callbacks */
Lowapp_RadioEvents_t radio_callbacks;

/**
 * Bitmask used as flags for radio callbacks
 *
 * This is used when we are out of the state machine,
 * for example when sending a ping with AT+PING
 */
volatile uint8_t radioFlags = 0;

/**
 * Buffer used to store the received message when not in state machine
 */
MSG_RXDONE_T msgReceived;

/**
 * @addtogroup lowapp_core
 * @{
 */
/**
 * @addtogroup lowapp_core_radio_evt LoWAPP Core Radio Events
 * @brief Radio event functions to be called by the Radio layer
 * @{
 */
/**
 * Called when the radio CAD ends
 *
 * When radio CAD ends, this function posts a CADDONE event to the state machine's
 * event queue with a boolean value indicating if a preamble was detected as data.
 *
 * @param channelActivityDetected A boolean indicating if a preamble was detected
 * on the radio channel
 */
void cadDone ( bool channelActivityDetected ) {
	/* Put radio in sleep mode after continuous RX */
	_sys->SYS_radioSleep();
	LOG_LATER(LOG_RADIO, "CAD done callback");
	LOG_LATER(LOG_PARSER, "CAD result = %d", channelActivityDetected);	/* Used by log parse */
	lock_eventQ();

	/*
	 * Ignore gcc warning for this cast because we did want to pass a
	 * uint8_t as a value to the event
	 */
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	add_event(&_eventQ, CADDONE, (void*)channelActivityDetected, 1);
	#pragma GCC diagnostic pop

	unlock_eventQ();
#ifdef SIMU
	/* Print log buffer only if CAD successfull */
	if(channelActivityDetected) {
		LOG_BUFFER();
	}
	else {
		flush_log_buffer();
	}
#endif
}

/**
 * Called when a radio reception ends successfully
 *
 * When radio RX ends successfully, this function posts a RXMSG with the message
 * as data to the state machine's main event queue.
 *
 * @param payload Buffer of the received message
 * @param size Size of the buffer
 * @param rssi Radio Signal Strength Indicator for the received signal
 * @param snr Signal to Noise ratio of the received signal
 */
void rxDone ( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr ){
	/* Put radio in sleep mode */
	_sys->SYS_radioSleep();

	LOG(LOG_PARSER, "RX Done callback, received %d bytes", size);
	/* Build a structure to store all informations */
	MSG_RXDONE_T* rxDoneMessage;
	rxDoneMessage = malloc(sizeof(MSG_RXDONE_T));
	rxDoneMessage->data = payload;
	rxDoneMessage->length = size;
	rxDoneMessage->rssi = rssi;
	rxDoneMessage->snr = snr;
	lock_eventQ();
	add_event(&_eventQ, RXMSG, rxDoneMessage, sizeof(MSG_RXDONE_T));
	unlock_eventQ();
}

/**
 * Called when an error occurs during radio reception
 *
 * When radio RX fails, this function posts a RXERROR event to the state machine's
 * main event queue.
 */
void rxError(void){
	/* Put radio in sleep mode */
	_sys->SYS_radioSleep();

	LOG(LOG_PARSER, "RX Error callback");
	lock_eventQ();
	add_simple_event(&_eventQ, RXERROR);
	unlock_eventQ();
}

/**
 * Called when a radio reception does not end within a certain time slot
 *
 * When radio RX times out, this function posts a TIMEOUT event to the state
 * machine's main event queue.
 */
void rxTimeout(void){
	/* Put radio in sleep mode */
	_sys->SYS_radioSleep();

	LOG(LOG_PARSER, "RX Timeout callback");
	lock_eventQ();
	add_simple_event(&_eventQ, RXTIMEOUT);
	unlock_eventQ();
}

/**
 * Called when a radio transmission ends successfully
 *
 * When radio TX ends successfully, this function posts a TXDONE event to the
 * state machine's main event queue.
 */
void txDone(void){
	/* Put radio in sleep mode */
	_sys->SYS_radioSleep();

	LOG(LOG_PARSER, "TX Done callback");
	lock_eventQ();
	add_simple_event(&_eventQ, TXDONE);
	unlock_eventQ();
}

/**
 * Called when a radio transmission does not end within a certain time slot
 *
 * When radio TX times out, this function posts a TIMEOUT event to the state
 * machine's main event queue.
 */
void txTimeout(void){
	/* Put radio in sleep mode */
	_sys->SYS_radioSleep();

	LOG(LOG_PARSER, "TX Timeout callback");
	lock_eventQ();
	add_simple_event(&_eventQ, TXTIMEOUT);
	unlock_eventQ();
}

/**
 * @addtogroup lowapp_core_radio_evt_nosm LoWAPP Core Radio Events Out of the State Machine
 * @brief Radio event functions to be called by the Radio layer when not in the state machine
 * @{
 */

/**
 * Called when a radio reception ends successfully
 *
 * When radio RX ends successfully, this function sets a flag in radioFlags
 *
 * @param payload Buffer of the received message
 * @param size Size of the buffer
 * @param rssi Radio Signal Strength Indicator for the received signal
 * @param snr Signal to Noise ratio of the received signal
 */
void noSmRxDone ( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr ){
	/* Put radio in sleep mode */
	_sys->SYS_radioSleep();
	LOG(LOG_PARSER, "RX Done callback, received %d bytes", size);
	msgReceived.data = payload;
	msgReceived.length = size;
	msgReceived.rssi = rssi;
	msgReceived.snr = snr;
	radioFlags |= RADIOFLAGS_RXDONE;
}

/**
 * Called when an error occurs during radio reception
 *
 * When radio RX fails, this function sets a flag in radioFlags
 */
void noSmRxError(void){
	/* Put radio in sleep mode */
	_sys->SYS_radioSleep();
	LOG(LOG_PARSER, "RX Error callback");
	radioFlags |= RADIOFLAGS_RXERROR;
}

/**
 * Called when a radio reception does not end within a certain time slot
 *
 * When radio RX times out, this function sets a flag in radioFlags
 */
void noSmRxTimeout(void){
	/* Put radio in sleep mode */
	_sys->SYS_radioSleep();
	LOG(LOG_PARSER, "RX Timeout callback");
	radioFlags |= RADIOFLAGS_RXERROR;
}

/**
 * Called when a radio transmission ends successfully
 *
 * When radio TX ends successfully, this function sets a flag in radioFlags
 */
void noSmTxDone(void){
	/* Put radio in sleep mode */
	_sys->SYS_radioSleep();
	LOG(LOG_PARSER, "TX Done callback");
	radioFlags |= RADIOFLAGS_TXDONE;
}

/**
 * Called when a radio transmission does not end within a certain time slot
 *
 * When radio TX times out, this function sets a flag in radioFlags
 */
void noSmTxTimeout(void){
	/* Put radio in sleep mode */
	_sys->SYS_radioSleep();
	LOG(LOG_PARSER, "TX Timeout callback");
	radioFlags |= RADIOFLAGS_TXTIMEOUT;
}



/** @} */

/** @} */

/** @} */
