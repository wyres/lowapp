/**
 * @file lowapp_sm.c
 * @brief Main file of the LoWAPP core
 *
 * This file contains the states' definitions, the definitions of the events and
 * messages queues as well as the state machine itself.
 *
 * @date June 29, 2016
 * @author Brian Wyld
 * @author Nathan Olff
 */
#include "lowapp_inc.h"

#ifdef SIMU
	#include "radio-simu.h"
	#include "activity_stat.h"
#endif
#include <math.h>
#include "utilities.h"

/** Prototype for state's function */
typedef STATES (*SM_PROCESS_T)(EVENT_T evt);

/** Sequence numbers */
PEER_T peers[256];

/**
 * Received messages queue
 *
 * This queue contains the messages received, waiting to be retrieved by the application
 * (or pushed to the application regarding the mode).
 */
volatile QFIXED_T _rx_pkt_list;
/**
 * Transmission messages queue
 *
 * This queue contains the messages waiting to be transmitted.
 */
volatile QFIXED_T _tx_pkt_list;
/**
 * AT command queue, waiting to be process
 */
volatile QFIXED_T _atcmd_list;
/**
 * Standard event queue
 */
volatile QEVENT_T _eventQ;
/**
 * Cold event queue
 *
 * This event queue only contains TXREQ and RXAT events. These are only manage
 * when nothing else is happening, in the idle state.
 */
volatile QEVENT_T _coldEventQ;

/** Statistics queue for AT+WHO */
volatile QSTAT_T statisticsWho;

/** Current state of the state machine */
STATES _currentState = RESTART;

/**
 * CAD flag
 *
 * This is set when a CAD is done, by the radio callback. Normally, the CADTIMEOUT
 * event should be enough. This flag is raised in case the CADTIMEOUT is reached
 * when the state machine is not in idle and is therefore unable to process that
 * event. The flag is read every time we enter the idle state.
 */
volatile uint8_t cad_flag;

/**
 * Ack message used to go through the wait tx ack window and the wait channel ack
 */
MSG_T* ackMsg = NULL;

/**
 * Destination node of the last sent message
 */
uint8_t lastDestination;

/**
 * Current message transmitting
 */
MSG_T* currentTxMsg;

/**
 * Current frame transmitting (used as temporary buffer)
 */
uint8_t currentTxFrame[MAX_FRAME_SIZE];

/**
 * Flag used to indicate that the currentTxFrame buffer is filled with a frame
 */
bool txFrameFilled = false;

/**
 * Frame length in bytes of the currentTxMsg
 */
uint16_t currentTxLength;

/**
 * Number of tx retry done on currentTxFrame
 */
uint8_t retryTxFrame = 0;

/**
 * Flag used to block transmission for some time
 */
volatile bool txBlocked = false;

/**
 * @name LoWAPP State Machine States
 * @{
 */
static STATES state_idle(EVENT_T evt);
static STATES state_rxing(EVENT_T evt);
static STATES state_skipping_ack(EVENT_T evt);
static STATES state_wait_slot_tx_ack(EVENT_T evt);
static STATES state_txing(EVENT_T evt);
static STATES state_rxing_ack(EVENT_T evt);
static STATES state_restart(EVENT_T evt);
static STATES state_txingack(EVENT_T evt);
static STATES state_wait_before_listening_ack(EVENT_T evt);
static STATES state_cad(EVENT_T evt);
/**@} */

/**
 * Array matching each state's function to the state from the enum
 */
SM_PROCESS_T SM[] = { state_idle, state_rxing, state_skipping_ack,
		state_wait_slot_tx_ack, state_txingack,	state_txing,
		state_wait_before_listening_ack, state_rxing_ack, state_cad,
		state_restart };


extern Lowapp_RadioEvents_t radio_callbacks;

/* JSON strings for response to application */
extern const uint8_t jsonMissingAck[];
extern const uint8_t jsonMissingFrame[];
extern const uint8_t jsonSuffix[];
extern const uint8_t jsonPrefixNokTxRetry[];
extern const uint8_t jsonErrorMaxRetry[];
extern const uint8_t jsonErrorTxFail[];
extern const uint8_t jsonPrefixOkTx[];
extern const uint8_t jsonNokTx[];
extern const uint8_t jsonNokTxRxError[];
extern const uint8_t jsonNokTxRxTimeout[];

/* Safeguard timers */
extern uint16_t timer_safeguard_rxing_std;
extern uint16_t timer_safeguard_rxing_ack;
extern uint16_t timer_safeguard_txing_std;
extern uint16_t timer_safeguard_txing_ack;

/* CAD timer and duration */
extern uint16_t _cad_duration;
extern uint32_t _cad_interval;

/* Static functions prototypes */
static STATES tryTxFromQueue();
static STATES tryTxCurrent();
static STATES tryTxFrame();
static void setTimerForUnblockingTx();

/**
 * Initialise the radio core with radio event callbacks
 */
void core_radio_init() {
	/* Register radio events */
	radio_callbacks.CadDone = cadDone;
	radio_callbacks.RxDone = rxDone;
	radio_callbacks.RxError = rxError;
	radio_callbacks.RxTimeout = rxTimeout;
	radio_callbacks.TxDone = txDone;
	radio_callbacks.TxTimeout = txTimeout;

	/* Init radio layer */
	_sys->SYS_radioInit(&radio_callbacks);
}

/**
 * Initialise the LoWAPP core
 */
void core_init() {
	/* Initialise buffers, queues */
	memset((void*)&peers, 0, sizeof(peers));
	memset((void*)&_rx_pkt_list, 0, sizeof(_rx_pkt_list));
	memset((void*)&_tx_pkt_list, 0, sizeof(_tx_pkt_list));
	memset((void*)&_eventQ, 0, sizeof(_eventQ));
	memset((void*)&_coldEventQ, 0, sizeof(_coldEventQ));

	/* By default, lastDestination is the device id */
	lastDestination = _deviceId;

#ifdef SIMU
	/* Initialise multi level log system */
	init_log();
#endif

	/* Reset frame and frame flag */
	memset(currentTxFrame, 0, MAX_FRAME_SIZE);

	/* Start the state machine */
	lock_eventQ();
	add_simple_event(&_eventQ, STATE_ENTER);
	unlock_eventQ();

	/*
	 * Start random number generator from Semtech using a seed
	 * from the Radio
	 */
	srand1(_sys->SYS_random());
}

/**
 * Update the values of safeguard timers according to the current radio configuration
 */
void update_safeguard_timers() {
	/* Set radio Tx configuration for ACK */
	_sys->SYS_radioSetTxConfig(_power, _bandwidth, _rsf, _coderate, PREAMBLE_ACK, timer_safeguard_txing_ack, true);

	/* Set safeguard timer for rxing ack messages */
	timer_safeguard_txing_ack = ceil(_sys->SYS_radioTimeOnAir(ACK_FRAME_LENGTH)*1.2);
	/*
	 * We need to leave enough time after transmission to reach the ACK slot, receive the ACK (whenever it
	 * occured within the slot) and wait for the reception to finish.
	 */
	timer_safeguard_rxing_ack = TIMER_ACK_SLOT_LENGTH+timer_safeguard_txing_ack;

	/* Set radio Rx and Tx configuration to standard messages */
	_sys->SYS_radioSetRxConfig(_bandwidth, _rsf, _coderate, _preambleLen, false, 0, true);
	_sys->SYS_radioSetTxFixLen(false);

	/* Set safeguard timer for rxing standard messages */
	timer_safeguard_rxing_std = ceil(_sys->SYS_radioTimeOnAir(MAX_FRAME_SIZE)*1.2);
	/* Set safeguard timer for txing standard message */
	timer_safeguard_txing_std = timer_safeguard_rxing_std;

	/* Finally update TX configuration to use the new timeout value */
	_sys->SYS_radioSetTxTimeout(timer_safeguard_txing_std);

}

/**
 * Set the default values in the core
 *
 * This is used for configuration variables that are not to be stored in memory
 */
void set_default_values() {
	/* Configuration variables */

	/* Pull mode is the default operation mode */
	_opMode = PULL;

 	/* Set coding rate */
 	_coderate = LOWAPP_CODING_RATE;	/* 1 : 4/5, 2 : 4/6, 3 : 4/7, 4 : 4/8 */

 	/* Set default radio power */
 	_power = LOWAPP_TX_POWER;

	/* Set default bandwidth */
	_bandwidth = LOWAPP_BANDWIDTH;

	/* Set default CAD duration */
	_cad_duration = LOWAPP_CAD_DURATION;

	/* Set default preamble time */
	preambleTime = 500;

	/*
	 * Set invalid value in order to differentiate
	 * initial values from loaded value
	 */
	_rchanId = 255;

	/* Variables related to the state machine */

	/* Initialise radio as disconnected */
	_connected = false;

	/* Initialise tx block flag */
	txBlocked = false;

	/* Initialise retry variable */
	retryTxFrame = 0;
	txFrameFilled = false;
}

/**
 * Check current configuration
 *
 * @retval True If the configuration is valid
 * @retval False If it is not valid
 */
bool check_configuration() {
	uint8_t i = 0, j = 0;
	/* Check AES key */
	for(i = 0; i < ENCKEY_SIZE; ++i) {
		if(_encryptionKey[i] == 0) {
			j++;
		}
	}
	/* Check that the AES key is not null */
	if(j == ENCKEY_SIZE) {
		return false;
	}
	/* Check preamble length not null */
	if(_preambleLen == 0) {
		return false;
	}
	/* Check device id	 */
	if(_deviceId > MAX_DEVICE_ID || _deviceId < MIN_DEVICE_ID) {
		return false;
	}
	/* Check SF */
	if(_rsf < MIN_SPREADINGFACTOR || _rsf > MAX_SPREADINGFACTOR) {
		return false;
	}
	/* Check radio channel */
	if(_rchanId > MAX_RCHAN_ID) {
		return false;
	}
	return true;
}

/**
 * Check attribute value before set for AT commands
 *
 * @param key Key string of the element to set
 * @param val New value of the configuration variable
 * @retval True If the configuration is valid
 * @retval False If it is not valid
 */
bool check_attribute(const uint8_t* key, const uint8_t* val) {
	const char* keyChar = (const char*) key;
	if(strcmp(keyChar, (const char*)strGwMask) == 0) {
		if(strlen((char*)val) != 8) {
			return false;
		}
		uint32_t value;
		if(AsciiHexStringConversionBI8_t((uint8_t*)&value, val, 8) != 1) {
			return false;
		}
	}
	else if(strcmp(keyChar, (const char*)strDeviceId) == 0) {
		if(strlen((char*)val) != 2) {
			return false;
		}
		uint8_t value;
		if(AsciiHexStringConversionBI8_t(&(value), val, 2) != 1) {
			return false;
		}
		if(value > MAX_DEVICE_ID || value < MIN_DEVICE_ID) {
			return false;
		}
	}
	else if(strcmp(keyChar, (const char*)strGroupId) == 0) {
		if(strlen((char*)val) != 4) {
			return false;
		}
		uint16_t value;
		if(AsciiHexStringConversionBI8_t((uint8_t*)&(value), val, 4) != 1) {
			return false;
		}
	}
	else if(strcmp(keyChar, (const char*)strRchanId) == 0) {
		/* Allow half byte value (only one char) */
		if(strlen((char*)val) > 2) {
			return false;
		}
		uint8_t value;
		if(AsciiHexConversionOneValueBI8_t(&(value), val) != 1) {
			return false;
		}
		if(value > MAX_RCHAN_ID) {
			return false;
		}
	}
	else if(strcmp(keyChar, (const char*)strRsf) == 0) {
		/* Allow half byte value (only one char) */
		if(strlen((char*)val) > 2) {
			return false;
		}
		uint8_t value;
		if(AsciiHexConversionOneValueBI8_t(&(value), val) != 1) {
			return false;
		}
		if(value < MIN_SPREADINGFACTOR || value > MAX_SPREADINGFACTOR) {
			return false;
		}
	}
	else if(strcmp(keyChar, (const char*)strPreambleTime) == 0) {
		uint16_t value = AsciiDecStringConversion_t(val, strlen((char*)val));
		if(value == 0) {
			return false;
		}
	}
	else if(strcmp(keyChar, (const char*)strEncKey) == 0) {
		if(strlen((char*)val) != 32) {
			return false;
		}
		uint8_t key[16] = {0};
		if(AsciiHexStringConversionBI8_t(key, (const uint8_t*)val, 32) != 1) {
			return false;
		}
		uint8_t nZeros = 0;
		uint8_t i;
		/* Check that the encryption key is not null */
		for(i = 0; i < 16; ++i) {
			if(key[i] == 0) {
				nZeros++;
			}
		}
		if(nZeros == 16) {
			return false;
		}
	}
	else {
		return false;
	}
	return true;
}

/**
 * Set timer for unblocking TX
 */
static void setTimerForUnblockingTx() {
	/* Block transmission */
	LOG(LOG_DBG, "txBlocked = true");
	txBlocked = true;
	int32_t r = randr(RANDOM_BLOCK_TX_MIN,RANDOM_BLOCK_TX_MAX);
	LOG(LOG_DBG, "Random value = %d", r);
	_sys->SYS_setTimer2(preamble_symbols_to_timems(_preambleLen)+r);
}

/**
 * Post a CAD timeout event periodically
 *
 * Add CADTIMEOUT event to the standard event queue
 */
void cadTimeoutCB() {
	lock_eventQ();
	add_simple_event(&_eventQ, CADTIMEOUT);
	cad_flag = 1;
	unlock_eventQ();
	_sys->SYS_setRepetitiveTimer(_cad_interval);	// Rearm
}

/**
 * Post a Timeout event when the timer reaches its end
 */
void timeoutCB() {
	LOG(LOG_STATES, "Timeout event occurred");
	lock_eventQ();
	if (add_event(&_eventQ, TIMEOUT, NULL, 0) < 0) {
		LOG(LOG_ERR, "Event queue was full");
	}
	unlock_eventQ();
}

/**
 * Unblock transmission after some time
 */
void timeoutCB2() {
	LOG(LOG_INFO, "Timeout 2 event occurred (unblocking tx)");
	lock_eventQ();
	txBlocked = false;
	if (add_event(&_eventQ, TXUNBLOCK, NULL, 0) < 0) {
		LOG(LOG_ERR, "Event queue was full");
	}
	unlock_eventQ();
}

/**
 * Schedule a message for transmission
 *
 * Puts the message on the tx queue.
 *
 * @param msg Message to transmit
 * @retval 0 If the message was added to the queue
 * @retval -1 If the queue was full
 */
int8_t lowapp_tx(MSG_T* msg) {
	/* Try adding to the message to the TX queue */
	if (add_to_queue(&_tx_pkt_list, msg, sizeof(MSG_T)) == -1) {
		/* Queue was full, buffer was freed by the add_to_queue function */
		LOG(LOG_ERR, "Event queue was full");
		/* Free buffer */
		free(msg);
		msg = NULL;
		return -1;
	}
	else {
		LOG(LOG_STATES, "Add message to TX queue");
		return 0;
	}
}

/**
 * Try sending message from currentTxFrame temporary variable
 * @return The new state to run after trying to send the message
 */
static STATES tryTxFrame() {
	LOG(LOG_PARSER, "Trying to send (tryTx)");	/* Used by log parser */
	/* Used by log parser */
	if(currentTxMsg == NULL) {
		LOG(LOG_ERR, "Current message has been cleaned before the frame was sent");
	}
	else {
		LOG(LOG_PARSER, "Sending frame of %u bytes to node %u", currentTxLength, currentTxMsg->content.std.destId);
	}

	/* Listen before talk */
	if (_sys->SYS_radioLBT(_rchanId)) {
		/* Block tx while transmitting to let time for the radio to finish the transmission */
		LOG(LOG_DBG, "txBlocked = true");
		txBlocked = true;
		/* Send frame */
		_sys->SYS_radioTx(currentTxFrame, currentTxLength);
		return TXING;
	}
	else {
		retryTxFrame++;

		if(retryTxFrame < MAX_TX_FRAME_RETRY) {
			LOG(LOG_INFO, "LBT found something, try to go to RX mode");

			/* Set timer for unblocking tx */
			txBlocked = true;
			int32_t r = ceil(_sys->SYS_radioTimeOnAir(MAX_FRAME_SIZE))		// preamble time + max transmission
							+randr(RANDOM_BLOCK_TX_MIN,RANDOM_BLOCK_TX_MAX)	// + random
							+TIMER_ACK_SLOT_START							// + time for transmitting ACK
							+TIMER_ACK_SLOT_LENGTH;
			LOG(LOG_DBG, "Set block timer to %d ms", r);
			_sys->SYS_setTimer2(r);

			uint8_t bufCmd[50] = "";

			/* Format message for answer to the UART */
			/* Size of the current string to add to the anwser */
			uint8_t sizeStr = 0;
			uint8_t offset = 0;
			/* Build the JSON message */
			sizeStr = strlen((char*)jsonPrefixNokTxRetry);
			memcpy(bufCmd+offset, jsonPrefixNokTxRetry, sizeStr);
			offset += sizeStr;

			offset = FillBuffer8_t(bufCmd, offset, &retryTxFrame, 1, false);

			sizeStr = strlen((char*)jsonSuffix);
			memcpy(bufCmd+offset, jsonSuffix, sizeStr);
			offset += sizeStr;
			_sys->SYS_cmdResponse(bufCmd, offset);
			return RXING;
		}
		else {
			LOG(LOG_ERR, "Maximum number of retry reached, canceling TX");
			/* Reset txFrame */
			memset(currentTxFrame, 0, MAX_FRAME_SIZE);
			txFrameFilled = false;
			_sys->SYS_cmdResponse((uint8_t*)jsonErrorMaxRetry, strlen((char*)jsonErrorMaxRetry));
			return RXING;
		}
	}
}

/**
 * Try sending message from the currentTxMsg temporary variable
 *
 * @return The new state to run after trying to send the message
 * @retval #WAIT_CHANNEL If the channel is not available for transmission
 * @retval #TXING If the message is a standard message and if the channel is
 * available for transmission
 * @retval #TXING_ACK If the message is an acknowledge and if the channel is
 * available for transmission
 * @retval #IDLE If the message type is unkown or cannot be handled
 */
static STATES tryTxCurrent() {
	/* Check the type of message from its header */
	switch (currentTxMsg->hdr.type) {
	case TYPE_STDMSG:

		/* Compute frame size */
		currentTxLength = frameSize(currentTxMsg);
		LOG(LOG_INFO, "Channel free for TX");

		LOG(LOG_DBG, "peers[out_tx]=%u\tpeers[out_rx]=%u\tpeers[in_expected]=%u", peers[currentTxMsg->content.std.destId].out_txseq, peers[currentTxMsg->content.std.destId].out_rxseq, peers[currentTxMsg->content.std.destId].in_expected);

		currentTxMsg->content.std.txSeq = peers[currentTxMsg->content.std.destId].out_txseq;

		lastDestination = currentTxMsg->content.std.destId;

		/* Fill frame and set flag */
		currentTxLength = buildFrame(currentTxFrame, currentTxMsg);
		txFrameFilled = true;

		retryTxFrame = 0;

		return tryTxFrame();
	case TYPE_ACK:
		/* For ACK, do not use the currentTxFrame buffer ! */
		LOG(LOG_PARSER, "Trying to send ACK (tryTxAck)");	/* Used by log parser */
		uint8_t frameBuffer[ACK_FRAME_LENGTH] = {0};
		/* ackMsg should only contain acknowledge type message */
		currentTxLength = buildFrame(frameBuffer, currentTxMsg);
		LOG(LOG_PARSER, "Sending frame of %u bytes to node %u", currentTxLength, currentTxMsg->content.ack.destId);

		LOG(LOG_INFO, "ack from %u to %u, rx %u, expect %u", currentTxMsg->content.ack.srcId, currentTxMsg->content.ack.destId, currentTxMsg->content.ack.rxdSeq, currentTxMsg->content.ack.expectedSeq);

		/* Set radio TX configuration for ACK */
		_sys->SYS_radioSetTxFixLen(true);
		_sys->SYS_radioSetPreamble(PREAMBLE_ACK);
		_sys->SYS_radioSetTxTimeout(timer_safeguard_txing_ack);

		/* Start transmission */
		_sys->SYS_radioTx(frameBuffer, currentTxLength);

		LOG(LOG_DBG, "Time on air computer : %u us", _sys->SYS_radioTimeOnAir(currentTxLength));

		/* Free the message buffer */
		free(currentTxMsg);
		currentTxMsg = NULL;
		return TXING_ACK;
	default:
		/* Free message buffer */
		free(currentTxMsg);
		currentTxMsg = NULL;
		/* #TODO Manage other message types */
		LOG(LOG_ERR, "Unknown message type received");
		return IDLE;
	}
}


/**
 * Try sending message from the TX queue
 *
 * @return The new state to run after trying to send the message
 * @retval #WAIT_CHANNEL If the channel is not available for transmission
 * @retval #TXING If the message is a standard message and if the channel is
 * available for transmission
 * @retval #TXING_ACK If the message is an acknowledge and if the channel is
 * available for transmission
 * @retval #IDLE If the message type is unkown or cannot be handled
 */
static STATES tryTxFromQueue() {
	get_from_queue(&_tx_pkt_list, (void**) &currentTxMsg, &currentTxLength);
	return tryTxCurrent();
}

/**
 * @addtogroup lowapp_core
 * @{
 */
/**
 * @addtogroup lowapp_core_sm LoWAPP Core State Machine
 * @brief State machine and events
 * @{
 */

/**
 * State machine 'run' method
 *
 * This is the running method of the state machine. It retrieves the events
 * from the two event queues, execute the states' functions with those events
 * and manages the transitions from one state to another by using STATE_EXIT
 * and STATE_ENTER respectively on the previous and new state's function.
 *
 * @return When there are no event to be processed in the event queues.
 * The return value specify whether the device should go in shallow sleep
 * mode or in deep sleep mode.
 */
uint8_t sm_run() {
	while (true) {
		EVENT_T evt;
		STATES newState;
		LOG(LOG_STATES, "Currently %u events in the queue", event_size(&_eventQ));
		lock_eventQ();
		/* Retrieve event from standard event queue */
		int qs = get_event(&_eventQ, &evt.type, &evt.data, &evt.datalen);
		unlock_eventQ();
		/* If no event in the queue */
		if (qs < 0) {
			/*
			 * Look at cold queue only if we are in idle mode and
			 * did not have any standard queue event to process
			 */
			if (_currentState == IDLE) {
				lock_coldEventQ();
				qs = get_event(&_coldEventQ, &evt.type, &evt.data,
						&evt.datalen);
				unlock_coldEventQ();
				/* If no event either, return */
				if (qs < 0)
					return LOWAPP_SM_DEEP_SLEEP;
				LOG(LOG_STATES, "Get event %u from cold event queue", evt.type);
			}
			else {
				switch(_currentState) {
				case TXING:
				case TXING_ACK:
					return LOWAPP_SM_TX;
				case RXING:
				case RXING_ACK:
					return LOWAPP_SM_RX;
				default:
					/* Nothing to process */
					return LOWAPP_SM_SHALLOW_SLEEP;
				}
			}
		}
		else {
			LOG(LOG_STATES, "Get event %u from standard event queue (forwarded to state %u)", evt.type, _currentState);
		}

		/*
		 * Execute current state's function with incoming event and
		 * store return value for transition
		 */
		newState = (SM[_currentState])(evt);
		/* Manage the transition from a state to another state */
		while (newState != _currentState) {
			evt.type = STATE_EXIT;
			(SM[_currentState])(evt);
			evt.type = STATE_ENTER;
			_currentState = newState;
			/* STATE_ENTER can also change state */
			newState = (SM[newState])(evt);
			if(newState != _currentState) {
				LOG(LOG_DBG, "Loop again over state change !!!");
			}
		}

	}
}

/**
 * Idle state execution function
 *
 * When entering the state, we first check if there is any AT command waiting
 * to be processed. If so, we process one of them.<br />
 * Then we check the cad flag. If the flag is raised, we move over to the CAD state.
 *
 * When a RXAT event occurs, we check the size of the AT command queue and process
 * one message.
 *
 * When a TXREQ event occurs, we add the message to the TX queue using #lowapp_tx,
 * and try sending one element of the TX queue with #tryTxFromQueue .
 *
 * When a CADTIMEOUT event occurs, we move other to CAD state.
 *
 * @param evt Event to process by this state
 * @return Next state for the state machine
 */
static STATES state_idle(EVENT_T evt) {
	switch (evt.type) {
	case STATE_ENTER:
		LOG(LOG_STATES, "Entering Idle state");
		/* Check AT command queue */
		lock_atcmd();
		if (queue_size(&_atcmd_list) > 0) {
			unlock_atcmd();
			at_queue_process();
		}
		else {
			unlock_atcmd();
		}

		/* Manage push mode */
		if(_opMode == PUSH && queue_size(&_rx_pkt_list) > 0) {
			response_rx_packets();
		}

		/* Check if tx is blocked */
		if(!txBlocked) {
			/* Unblock signal occured, try to send frame */
			if(txFrameFilled) {
				return tryTxFrame();
			}
			else {
				/* Check tx queue */
				if (queue_size(&_tx_pkt_list) > 0) {
					return tryTxFromQueue();
				}
			}
		}
		return _currentState;
	case TXUNBLOCK:
		/* Unblock signal occured, try to send frame */
		if(txFrameFilled) {
			return tryTxFrame();
		}
		else {
			/* Check tx queue */
			if (queue_size(&_tx_pkt_list) > 0) {
				return tryTxFromQueue();
			}
		}
		return _currentState;
	case RXAT:
		LOG(LOG_STATES, "RXAT");
		/* Check AT command queue */
		lock_atcmd();
		if (queue_size(&_atcmd_list) > 0) {
			unlock_atcmd();
			at_queue_process();
		}
		else {
			unlock_atcmd();
		}
		return _currentState;
	case TXREQ:
		LOG(LOG_DBG, "Processing of TXREQ");
		/* Check if tx is blocked */
		if(!txBlocked) {
			/* Unblock signal occured, try to send frame */
			if(txFrameFilled) {
				return tryTxFrame();
			}
			else {
				/* Check tx queue */
				if (queue_size(&_tx_pkt_list) > 0) {
					return tryTxFromQueue();
				}
			}
		}
		return _currentState;
	case CADTIMEOUT:
		return CAD;
	default:
		return _currentState;		// Ignore event and stay here
	}
	return 0;
}

/**
 * Receiving state execution function
 *
 * When entering the state, we start reception on the radio. The radio then
 * either posts a RXMSG event if a message was successfully received, a RXERROR
 * event if an error occurred or a TIMEOUT event if nothing happened.
 *
 * When a RXMSG event occurs, we build a corresponding MSG_T event from the
 * frame buffer, add it to the RX queue, build a ACK message and move to Wait slot
 * tx ack state.
 *
 * @param evt Event to process by this state
 * @return Next state for the state machine
 */
static STATES state_rxing(EVENT_T evt) {
	MSG_T* msg = NULL;
	MSG_RX_APP_T *msg_rx_app = NULL;
	int8_t received;
	MSG_RXDONE_T* rxDoneMessage = NULL;
	switch (evt.type) {
	case STATE_ENTER:
		LOG(LOG_PARSER, "Entering RXING state");
		LOG(LOG_DBG, "Timer safeguard at %u", timer_safeguard_rxing_std);
		/* Start radio reception */
		_sys->SYS_radioRx(timer_safeguard_rxing_std);
		return _currentState;
	case RXMSG:
		rxDoneMessage = (MSG_RXDONE_T*) evt.data;
		LOG(LOG_STATES, "Processing RXMSG event");
		if (rxDoneMessage == NULL || rxDoneMessage->data == NULL) {
			LOG(LOG_ERR, "No data received with RXMSG event");
			return IDLE;
		}
		/* Build MSG_T from message frame */
		msg = malloc(sizeof(MSG_T));
		received = retrieveMessage(msg, rxDoneMessage->data);
		/* Check destination */
		if (received == 0) {
			/* Create message for app with the state included */
			msg_rx_app = (MSG_RX_APP_T*) malloc(sizeof(MSG_RX_APP_T));
			memset(msg_rx_app, 0, sizeof(MSG_RX_APP_T));
			msg_rx_app->msg = msg;
			msg_rx_app->rssi = rxDoneMessage->rssi;
			msg_rx_app->snr = rxDoneMessage->snr;
			/* Free the memory for the rx done message structure */
			free(rxDoneMessage);
			rxDoneMessage = NULL;
			/* Add message with state to the rx_pkt_list fifo */
			received = add_to_queue(&_rx_pkt_list, msg_rx_app, sizeof(MSG_RX_APP_T));
			/* Add to the statistics */
			STAT_T stat;
			stat.deviceId = msg->content.std.srcId;
			stat.lastRssi = msg_rx_app->rssi;
			stat.lastSeen = _sys->SYS_getTimeMs();
			add_to_statqueue(&statisticsWho, stat);

			/* Check the message was added to the queue (queue not full) */
			if(received != -1) {
				LOG(LOG_PARSER, "Received message from %u", msg->content.std.srcId);
				LOG(LOG_DBG, "peers[out_tx]=%u\tpeers[out_rx]=%u\tpeers[in_expected]=%u", peers[msg->content.std.srcId].out_txseq, peers[msg->content.std.srcId].out_rxseq, peers[msg->content.std.srcId].in_expected);

				/* Manage broadcast */
				if(msg->content.std.destId == LOWAPP_ID_BROADCAST) {
					LOG(LOG_INFO, "Broadcast received");
					return IDLE;
				}
				else {

					/* Prepare ACK message */
					currentTxMsg = (MSG_T*) malloc(sizeof(MSG_T));

					currentTxMsg->hdr.payloadLength = 0;
					currentTxMsg->hdr.type = TYPE_ACK;
					currentTxMsg->hdr.version = LOWAPP_CURRENT_VERSION;
					currentTxMsg->content.ack.destId = msg->content.std.srcId;
					currentTxMsg->content.ack.srcId = _deviceId;
					/* Sequence number is 0 if the sender node has been re-initialised */
					if(msg->content.std.txSeq == 0 && peers[msg->content.std.srcId].in_expected != 0) {
						LOG(LOG_INFO, "Sender's node got initialised");
						peers[msg->content.std.srcId].in_expected = 0;
						peers[msg->content.std.srcId].out_txseq = 0;
						peers[msg->content.std.srcId].out_rxseq = 0;
					}

					/* Fill ACK sequence numbers */
					currentTxMsg->content.ack.expectedSeq = peers[msg->content.std.srcId].in_expected;
					currentTxMsg->content.ack.rxdSeq = msg->content.std.txSeq;

					/* Send ACK as of now */

					/* If the sequence number is the one we were expecting */
					if(msg->content.std.txSeq == peers[msg->content.std.srcId].in_expected) {
						LOG(LOG_INFO, "Received seq = expected seq");
						/* Update sequence number */
						peers[msg->content.std.srcId].in_expected = (peers[msg->content.std.srcId].in_expected % 255) + 1;
					}
					/*
					 * If the sequence number from the message is bigger than what we were expecting.
					 * Take into account rollover of the variable using two thresholds.
					 */
					else if(msg->content.std.txSeq > peers[msg->content.std.srcId].in_expected ||
								(msg->content.std.txSeq < SEQ_ROLLOVER_LOW_THRESHOLD
									&& peers[msg->content.std.srcId].in_expected > SEQ_ROLLOVER_HIGH_THRESHOLD)) {
						LOG(LOG_INFO, "Received seq > expected seq");
						LOG(LOG_WARN, "%u missing frames !", msg->content.std.txSeq - peers[msg->content.std.srcId].in_expected);
						/* Notify app by setting state for message added to _rx_pkt */
						msg_rx_app->state.missing_frames = msg->content.std.txSeq - peers[msg->content.std.srcId].in_expected;
						/* Catch up with the actual received sequence number */
						peers[msg->content.std.srcId].in_expected = (msg->content.std.txSeq % 255) + 1;
					}
					/*
					 * Duplicate frame is detected if the txSeq of the message is slightly lower than
					 * the expected sequence number.
					 */
					else if((msg->content.std.txSeq < peers[msg->content.std.srcId].in_expected
							 || (msg->content.std.txSeq > SEQ_ROLLOVER_HIGH_THRESHOLD &&
									 peers[msg->content.std.srcId].in_expected < SEQ_ROLLOVER_LOW_THRESHOLD))
							&& (peers[msg->content.std.srcId].in_expected - msg->content.std.txSeq) < 10) {
						LOG(LOG_INFO, "Received seq < expected seq");
						LOG(LOG_WARN, "Duplicate frame detected !");
						msg_rx_app->state.duplicate_flag = 1;
					}
					else {
						LOG(LOG_ERR, "Unexpected difference found between txSeq (%u) and peers[%u].in_expected (%u)",
								msg->content.std.txSeq, msg->content.std.srcId, peers[msg->content.std.srcId].in_expected);
					}

					LOG(LOG_INFO, "Sequence number received");

					LOG(LOG_DBG, "peers[out_tx]=%u\tpeers[out_rx]=%u\tpeers[in_expected]=%u", peers[msg->content.std.srcId].out_txseq, peers[msg->content.std.srcId].out_rxseq, peers[msg->content.std.srcId].in_expected);

					/* Slot before sending Ack */
					return WAIT_SLOT_TX_ACK;
				}
			}
			else {
				/* An error occurred during radio reception */
				LOG(LOG_ERR, "RX queue was full");
				/* Free buffer */
				free(msg);
				msg = NULL;
				return IDLE;
			}
		}
		else {
			/* Free the memory for the rx done message structure */
			free(rxDoneMessage);
			rxDoneMessage = NULL;
			/* If the packet was destined to someone else, log a message */
			if(received == -2 && msg->hdr.type == TYPE_STDMSG) {
				LOG(LOG_PARSER, "Received message from %u not for me", msg->content.std.srcId);
				/* Free message if it was not destined to us */
				free(msg);
				msg = NULL;
				return SKIPPING_ACK;
			}
			/* If the CRC check failed */
			else if(received == -3 && msg->hdr.type == TYPE_STDMSG) {
				LOG(LOG_PARSER, "CRC check failed");
			}

			if(msg != NULL) {
				/* Free message if it was not destined to us */
				free(msg);
				msg = NULL;
			}
			return IDLE;
		}
	case RXERROR:
	case RXTIMEOUT:
	case TIMEOUT:
		/* An error occurred during radio reception */
		return IDLE;
	default:
		return _currentState;		// Ignore event and stay here
	}
}

/**
 * Skipping ACK state execution function
 *
 * Waiting for the actual destination of the packet received to send its ACK for
 * one ACK slot.
 *
 * @param evt Event to process by this state
 * @return Next state for the state machine
 */
static STATES state_skipping_ack(EVENT_T evt) {
	switch (evt.type) {
	case STATE_ENTER:
		LOG(LOG_INFO, "Skipping ACK window");
		_sys->SYS_setTimer(TIMER_ACK_SLOT_START+TIMER_ACK_SLOT_LENGTH);
		return _currentState;
	case TIMEOUT:
		LOG(LOG_INFO, "Skipping timeout");
		return IDLE;
	default:
		return _currentState;		// Ignore event and stay here
	}
}

/**
 * Wait slot for tx ACK execution function
 *
 * The original sender of a message will listen for an ACK in a time window (slot).
 * By default it will be between 1s and 2s after transmission.
 * This state waits for the beginning of that ACK slot.
 *
 * When the TIMEOUT event is received, we call tryTxAck.
 *
 * @param evt Event to process by this state
 * @return Next state for the state machine
 */
static STATES state_wait_slot_tx_ack(EVENT_T evt) {
	switch(evt.type) {
	case STATE_ENTER:
		LOG(LOG_STATES, "Entering Wait slot TX ACK state");
		_sys->SYS_setTimer(TIMER_ACK_SLOT_TX);
		return _currentState;
	case TIMEOUT:
		return tryTxCurrent();
	default:
		return _currentState;
	}
}

/**
 * Transmitting ACK state execution function
 *
 * @param evt Event to process by this state
 * @return Next state for the state machine
 */
static STATES state_txingack(EVENT_T evt) {
	switch (evt.type) {
	case STATE_ENTER:
		LOG(LOG_STATES, "Entering TXING ACK state");
		return _currentState;
	case TXDONE:
		/* Back to standard radio TX configuration */
		_sys->SYS_radioSetTxFixLen(false);
		_sys->SYS_radioSetPreamble(_preambleLen);
		_sys->SYS_radioSetTxTimeout(timer_safeguard_txing_std);

		LOG(LOG_INFO, "ACK transmitted");
		return IDLE;
	case TIMEOUT:
	case TXTIMEOUT:
		LOG(LOG_ERR, "Transmission of ACK timed out");

		/* Back to standard radio TX configuration */
		_sys->SYS_radioSetTxFixLen(false);
		_sys->SYS_radioSetPreamble(_preambleLen);
		_sys->SYS_radioSetTxTimeout(timer_safeguard_txing_std);

		return IDLE;
	default:
		return _currentState;		// Ignore event and stay here
	}
}

/**
 * Transmitting state execution function
 *
 * Transmission is started before this state. Here, we wait for the transmission
 * to finish (TXDONE) or to time out.
 *
 * When entering the state, we set a safeguard timer in order to recover in case
 * the radio does not post an event.
 *
 * When a TXDONE event occurs, we move over to WAIT_BEFORE_LISTENING_FOR_ACK.
 *
 * @param evt Event to process by this state
 * @return Next state for the state machine
 */
static STATES state_txing(EVENT_T evt) {
	switch (evt.type) {
	case STATE_ENTER:
		LOG(LOG_STATES, "Entering TXING state (Transmitting message)");
		return _currentState;
	case TXDONE:
		/* Increment sequence number when tx done*/
		peers[lastDestination].out_txseq =
			(peers[lastDestination].out_txseq % 255) + 1;
		LOG(LOG_STATES, "peers[out_tx]=%u\tpeers[out_rx]=%u\tpeers[in_expected]=%u", peers[lastDestination].out_txseq, peers[lastDestination].out_rxseq, peers[lastDestination].in_expected);

		/* Block transmissions for the duration of one preamble */
		LOG(LOG_INFO, "Blocking TX for one preamble duration");
		LOG(LOG_DBG, "txBlocked = true");
		txFrameFilled = false;

		/* Check if an ACK is expected */
		if(currentTxMsg->content.std.destId == LOWAPP_ID_BROADCAST) {
			setTimerForUnblockingTx();
			/* Free message buffer */
			free(currentTxMsg);
			currentTxMsg = NULL;
			return IDLE;
		}
		else {
			txBlocked = true;

			/* Free message buffer */
			free(currentTxMsg);
			currentTxMsg = NULL;
			return WAIT_BEFORE_LISTENING_FOR_ACK;
		}
	case TIMEOUT:
	case TXTIMEOUT:
		setTimerForUnblockingTx();

		/* Increment retry */
		retryTxFrame++;

		/* If we can still retry transmission */
		if(retryTxFrame < MAX_TX_FRAME_RETRY) {
			LOG(LOG_ERR, "TX Timeout (retry %u)", retryTxFrame);
			uint8_t bufCmd[50] = "";
			/* Format message for answer to the UART */
			/* Size of the current string to add to the anwser */
			uint8_t sizeStr = 0;
			uint8_t offset = 0;
			/* Build the JSON message */
			sizeStr = strlen((char*)jsonPrefixNokTxRetry);
			memcpy(bufCmd, jsonPrefixNokTxRetry, sizeStr);
			offset += sizeStr;

			offset = FillBuffer8_t(bufCmd, offset, &retryTxFrame, 1, false);

			sizeStr = strlen((char*)jsonSuffix);
			memcpy(bufCmd+offset, jsonSuffix, sizeStr);
			offset += sizeStr;
			_sys->SYS_cmdResponse(bufCmd, offset);
			return IDLE;
		}
		else {
			LOG(LOG_ERR, "TX Timeout");
			_sys->SYS_cmdResponse((uint8_t*)jsonErrorTxFail, strlen((char*)jsonErrorTxFail));

			txFrameFilled = false;

			/* Free message buffer */
			free(currentTxMsg);
			currentTxMsg = NULL;
			return IDLE;
		}
	default:
		return _currentState;		// Ignore event and stay here
	}
}

/**
 * Waiting before listening for ACK state execution function
 *
 * When we enter the timer, we set a timer to wait a certain time before we
 * listen for the Ack response. This is done to avoid loosing energy.
 *
 * @param evt Event to process by this state
 * @return Next state for the state machine
 */
static STATES state_wait_before_listening_ack(EVENT_T evt) {
	switch (evt.type) {
	case STATE_ENTER:
		LOG(LOG_STATES, "Entering Wait before listening for ACK state");
		_sys->SYS_setTimer(TIMER_ACK_SLOT_START);
		return _currentState;
	case TIMEOUT:
		/* Directly go into receive mode, no CAD */
		return RXING_ACK;
	default:
		return _currentState;
	}
}

/**
 * Process ACK and compare sequence numbers
 *
 * @param msg Message received (ack)
 */
void process_ack(MSG_T* msg) {
	uint8_t bufferResponse[50] = "";

	LOG(LOG_DBG, "peers[out_tx]=%u\tpeers[out_rx]=%u\tpeers[in_expected]=%u", peers[msg->content.ack.srcId].out_txseq, peers[msg->content.ack.srcId].out_rxseq, peers[msg->content.ack.srcId].in_expected);

	LOG(LOG_PARSER, "ACK retrieved from %u", msg->content.ack.srcId);
	/* Check for re-initialisation of the communication if expectedSeq == 0 */
	if(msg->content.ack.expectedSeq == 0 && msg->content.ack.rxdSeq != 0) {
		LOG(LOG_INFO, "Re-initialising communication (sequence numbers set to 0)");
		/*
		 * We assume the message sent was the first of the new connection
		 * and the received rxd was 0, so we move txseq to 1 and rxseq to 1
		 * (next rx expected to be 1).
		 */
		peers[msg->content.ack.srcId].out_txseq = 1;
		peers[msg->content.ack.srcId].out_rxseq = 1;
		peers[msg->content.ack.srcId].in_expected = 0;
		_sys->SYS_cmdResponse((uint8_t*)"OK TX", 5);
	}
	/* Check the ACK sequence number #TODO signal duplicate or missing frames */
	else if(msg->content.ack.rxdSeq == msg->content.ack.expectedSeq) {
		LOG(LOG_PARSER, "ACK received OK");	/* Use by log parser */
		/* Check that the expected sequence number of the receiver matches with the last ACK we got */
		if(peers[msg->content.ack.srcId].out_rxseq == msg->content.ack.expectedSeq) {
			LOG(LOG_INFO, "Expected sequence number from ACK matches with record");
			/* Update sequence number from the receiver */
			peers[msg->content.ack.srcId].out_rxseq = (peers[msg->content.ack.srcId].out_rxseq % 255) + 1;
			_sys->SYS_cmdResponse((uint8_t*)"OK TX", 5);
		}
		/*
		 * If the last recorded value for rxSeq is lower than the expected sequence number
		 * from the Ack, it means that some ACK sent by the receiver were not received by the
		 * transmitter (current node).
		 */
		else if(peers[msg->content.ack.srcId].out_rxseq < msg->content.ack.expectedSeq
				|| (peers[msg->content.ack.srcId].out_rxseq > SEQ_ROLLOVER_HIGH_THRESHOLD
						&& msg->content.ack.expectedSeq < SEQ_ROLLOVER_LOW_THRESHOLD)){
			LOG(LOG_INFO, "Expected sequence number from ACK > record");
			LOG(LOG_INFO, "Looks like a previous ACK was sent by the receiver but not received by this node.");
			/* Format message for answer to the UART */
			/* Size of the current string to add to the anwser */
			uint8_t sizeStr = 0;
			uint8_t offset = 0;
			/* Build the JSON message */
			sizeStr = strlen((char*)jsonMissingAck);
			memcpy(bufferResponse, jsonMissingAck, sizeStr);
			offset += sizeStr;

			offset = FillBuffer8_t(bufferResponse, offset, &retryTxFrame, 1, false);

			sizeStr = strlen((char*)jsonSuffix);
			memcpy(bufferResponse+offset, jsonSuffix, sizeStr);
			offset += sizeStr;
			/* Notify application of missing ACKs */
			_sys->SYS_cmdResponse(bufferResponse, offset);
			/* Catch up with sequence number from the receiver */
			peers[msg->content.ack.srcId].out_rxseq = (msg->content.ack.expectedSeq % 255) + 1;
		}
		else if(peers[msg->content.ack.srcId].out_rxseq > msg->content.ack.expectedSeq
				|| (msg->content.ack.expectedSeq > SEQ_ROLLOVER_HIGH_THRESHOLD
												&& peers[msg->content.ack.srcId].out_rxseq < SEQ_ROLLOVER_LOW_THRESHOLD)) {
			LOG(LOG_ERR, "Expected sequence number from ACK < record ! This should not happen");
			/*
			 * The expected sequence number should be upated on the receiver side to match the
			 * last received sequence number. A simple increment of the record should be enough
			 * to match the next time.
			 */
			_sys->SYS_cmdResponse((uint8_t*)jsonNokTx, strlen((char*)jsonNokTx));
		}
		else{
			LOG(LOG_ERR, "Unexpected difference found between peers[%u].out_rxseq (%u) and ack.expected (%u)",
					msg->content.ack.srcId, peers[msg->content.ack.srcId].out_rxseq, msg->content.ack.expectedSeq);
			_sys->SYS_cmdResponse((uint8_t*)jsonNokTx, strlen((char*)jsonNokTx));
		}
	}
	else {
		LOG(LOG_PARSER, "ACK received NOK (from %u to %u : %u / %u)", msg->content.ack.srcId, msg->content.ack.destId,
				msg->content.ack.rxdSeq, msg->content.ack.expectedSeq);
		/* Check that the expected sequence number of the receiver matches with the last ACK we got */
		if(peers[msg->content.ack.srcId].out_rxseq == msg->content.ack.expectedSeq) {
			LOG(LOG_INFO, "All ACK sent have been received, but the receiver missed some messages");
			/* Format message for answer to the UART */
			/* Size of the current string to add to the anwser */
			uint8_t sizeStr = 0;
			uint8_t offset = 0;
			/* Build the JSON message */
			sizeStr = strlen((char*)jsonPrefixOkTx);
			memcpy(bufferResponse, jsonPrefixOkTx, sizeStr);
			offset += sizeStr;

			sizeStr = strlen((char*)jsonMissingFrame);
			memcpy(bufferResponse+offset, jsonMissingFrame, sizeStr);
			offset += sizeStr;

			uint8_t nMissing = msg->content.ack.rxdSeq - msg->content.ack.expectedSeq;
			offset = FillBuffer8_t(bufferResponse, offset, &nMissing, 1, false);

			sizeStr = strlen((char*)jsonSuffix);
			memcpy(bufferResponse+offset, jsonSuffix, sizeStr);
			offset += sizeStr;
			_sys->SYS_cmdResponse(bufferResponse, offset);
			/* Update sequence number from the receiver */
			peers[msg->content.ack.srcId].out_rxseq = (peers[msg->content.ack.srcId].out_rxseq % 255) + 1;
		}
		/*
		 * If the last recorded value for rxSeq is lower than the expected sequence number
		 * from the Ack, it means that some ACK sent by the receiver were not received by the
		 * transmitter (current node).
		 */
		else if(peers[msg->content.ack.srcId].out_rxseq < msg->content.ack.expectedSeq
				|| (peers[msg->content.ack.srcId].out_rxseq > SEQ_ROLLOVER_HIGH_THRESHOLD
						&& msg->content.ack.expectedSeq < SEQ_ROLLOVER_LOW_THRESHOLD)){
			LOG(LOG_INFO, "Expected sequence number from ACK > record");
			LOG(LOG_INFO, "Looks like a previous ACK was sent by the receiver but not received by this node.");

			/* Format message for answer to the UART */
			/* Size of the current string to add to the anwser */
			uint8_t sizeStr = 0;
			uint8_t offset = 0;
			/* Build the JSON message */
			sizeStr = strlen((char*)jsonMissingAck);
			memcpy(bufferResponse, jsonMissingAck, sizeStr);
			offset += sizeStr;

			uint8_t nMissing = msg->content.ack.expectedSeq-peers[msg->content.ack.srcId].out_rxseq;
			offset = FillBuffer8_t(bufferResponse, offset, &nMissing, 1, false);

			sizeStr = strlen((char*)jsonSuffix);
			memcpy(bufferResponse+offset, jsonSuffix, sizeStr);
			offset += sizeStr;
			/* Notify application of missing ACKs */
			_sys->SYS_cmdResponse(bufferResponse, offset);
		}
		else if(peers[msg->content.ack.srcId].out_rxseq > msg->content.ack.expectedSeq
				|| (msg->content.ack.expectedSeq > SEQ_ROLLOVER_HIGH_THRESHOLD
						&& peers[msg->content.ack.srcId].out_rxseq < SEQ_ROLLOVER_LOW_THRESHOLD)) {
			LOG(LOG_ERR, "Expected sequence number from ACK < record ! This should not happen");
			/*
			 * The expected sequence number should be upated on the receiver side to match the
			 * last received sequence number. A simple increment of the record should be enough
			 * to match the next time.
			 */
			_sys->SYS_cmdResponse((uint8_t*)jsonNokTx, strlen((char*)jsonNokTx));
		}
		else{
			LOG(LOG_ERR, "Unexpected difference found between peers[%u].out_rxseq (%u) and ack.expected (%u)",
					msg->content.ack.srcId, peers[msg->content.ack.srcId].out_rxseq, msg->content.ack.expectedSeq);
			_sys->SYS_cmdResponse((uint8_t*)jsonNokTx, strlen((char*)jsonNokTx));
		}

		/*
		 * If ack.rxd > ack.expected
		 * Catch up with sequence number from the receiver.
		 * As the ACK expected and received sequence number were different,
		 * the record needs to be updated if rxSeq > expected
		 */
		if(msg->content.ack.rxdSeq > msg->content.ack.expectedSeq) {
			LOG(LOG_INFO, "Update record out_rxseq");
			peers[msg->content.ack.srcId].out_rxseq = (msg->content.ack.rxdSeq % 255) + 1;
		}
	}

	LOG(LOG_DBG, "peers[out_tx]=%u\tpeers[out_rx]=%u\tpeers[in_expected]=%u", peers[msg->content.ack.srcId].out_txseq, peers[msg->content.ack.srcId].out_rxseq, peers[msg->content.ack.srcId].in_expected);
}

/**
 * Waiting for ACK state execution function
 *
 * After the CAD_ACK state, we start listening for the Ack frame.
 *
 * When a RXMSG event occurs, we check that it is the Ack we were expecting.
 *
 * If an error or timeout occurs, we log a message and go back to idle state
 *
 * @param evt Event to process by this state
 * @return Next state for the state machine
 */
static STATES state_rxing_ack(EVENT_T evt) {
	MSG_T* msg = NULL;
	MSG_RXDONE_T* rxDoneMessage = NULL;
	int8_t received;
	switch (evt.type) {
	case STATE_ENTER:
		/* Set RX configuration for ACK */
		_sys->SYS_radioSetRxFixLen(true, ACK_FRAME_LENGTH);
		_sys->SYS_radioSetPreamble(PREAMBLE_ACK);
		_sys->SYS_radioSetRxContinuous(true);
		/* Used by log parser */
		LOG(LOG_PARSER, "Entering RXING ACK state (Receiving ACK)");


#ifdef SIMU
		uint8_t fail_generator = rand() % 100;
		/* Simulate errors on reception of ACK */
		if(fail_generator >= FAILURE_RANDOM_START_RX) {
			simu_radio_rxing_ack(timer_safeguard_rxing_ack);
		}
#else
		/* Direclty start radio reception */
		_sys->SYS_radioRx(timer_safeguard_rxing_ack);
#endif
		return _currentState;
	case RXMSG:
		rxDoneMessage = (MSG_RXDONE_T*) evt.data;
		/* Set RX configuration back to standard */
		_sys->SYS_radioSetRxFixLen(false, 0);
		_sys->SYS_radioSetPreamble(_preambleLen);
		_sys->SYS_radioSetRxContinuous(true);

		if (rxDoneMessage == NULL || rxDoneMessage->data == NULL) {
			LOG(LOG_ERR, "No data received with RXMSG event");
			return IDLE;
		}
		/* Build MSG_T from message frame */
		msg = malloc(sizeof(MSG_T));
		received = retrieveMessage(msg, rxDoneMessage->data);
		/* Free the memory for the rx done message structure */
		free(rxDoneMessage);
		rxDoneMessage = NULL;
		if (received == 0 && msg->hdr.type == TYPE_ACK) {
			process_ack(msg);
			/* Free ack message received */
			free(msg);
			msg = NULL;
		}
		else {
			if(msg->hdr.type != TYPE_ACK) {
				LOG(LOG_PARSER, "Messages received was not an ACK");
			}
			/* If the packet was destined to someone else, log a message */
			else if(received == -2) {
				LOG(LOG_PARSER, "Received ACK from %u not for me", msg->content.ack.srcId);
			}
			/* If the CRC check failed */
			else if(received == -3) {
				LOG(LOG_PARSER, "CRC check failed");
			}
			_sys->SYS_cmdResponse(jsonNokTx, strlen((char*)jsonNokTx));
			if(msg != NULL) {
				/* Free ack message received */
				free(msg);
				msg = NULL;
			}
		}

		setTimerForUnblockingTx();

		return IDLE;
	case RXERROR:
		setTimerForUnblockingTx();

		/* Set RX configuration back to standard */
		_sys->SYS_radioSetRxFixLen(false, 0);
		_sys->SYS_radioSetPreamble(_preambleLen);
		_sys->SYS_radioSetRxContinuous(true);

		/* Nothing was received by the radio */
		LOG(LOG_PARSER, "No ACK");
		_sys->SYS_cmdResponse(jsonNokTxRxError, strlen((char*)jsonNokTxRxError));
		return IDLE;
	case RXTIMEOUT:
		setTimerForUnblockingTx();

		/* Set RX configuration back to standard */
		_sys->SYS_radioSetRxFixLen(false, 0);
		_sys->SYS_radioSetPreamble(_preambleLen);
		_sys->SYS_radioSetRxContinuous(true);

		/* Nothing was received by the radio */
		LOG(LOG_PARSER, "No ACK");
		_sys->SYS_cmdResponse(jsonNokTxRxTimeout, strlen((char*)jsonNokTxRxTimeout));
		return IDLE;
	case TIMEOUT:
		setTimerForUnblockingTx();

		/* Set RX configuration back to standard */
		_sys->SYS_radioSetRxFixLen(false, 0);
		_sys->SYS_radioSetPreamble(_preambleLen);
		_sys->SYS_radioSetRxContinuous(true);

		/* Nothing was received by the radio */
		LOG(LOG_PARSER, "No ACK");
		_sys->SYS_cmdResponse(jsonNokTx, strlen((char*)jsonNokTx));
		return IDLE;
	default:
		return _currentState;		// Ignore event and stay here
	}
}

/**
 * CAD state execution function
 *
 * When entering the state, we start the radio CAD (Channel activity detection).
 *
 * When the CAD is done, the radio event function CadDone posts a CADDONE event
 * with a boolean as data to notify if a preamble was detected or not on the
 * channel. We then move either to RXING or IDLE.
 *
 * @param evt Event to process by this state
 * @return Next state for the state machine
 */
static STATES state_cad(EVENT_T evt) {
	uint8_t res;
	switch (evt.type) {
	case STATE_ENTER:
		LOG_LATER(LOG_PARSER, "Entering CAD state");	/* Used by log parser */

		/* Reset CAD flag */
		cad_flag = 0;
		/* Check for the CAD preamble for one symbol */
		_sys->SYS_radioCAD();
		return _currentState;
	case CADDONE:
		LOG(LOG_STATES, "CAD DONE event received");
		/*
		 * Ignore gcc warning for this cast because we did want to pass a
		 * uint8_t as a value to the event
		 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
		/* Retrieve boolean from event data */
		res = (uint8_t) evt.data;
#pragma GCC diagnostic pop
		/*
		 * If a preamble was detected, we move over to RXING state, if not, we
		 * log a message and move back to IDLE.
		 */
		if (res == 1)
			return RXING;
		else
			return IDLE;
	default:
//		LOG(LOG_INFO, "Event unknown (%u)", evt.type);
		return _currentState;
	}
}

/**
 * Restart state execution function
 *
 * @param evt Event to process by this state
 * @return Next state for the state machine
 */
static STATES state_restart(EVENT_T evt) {
	switch (evt.type) {
	case STATE_ENTER:
		return IDLE;
		break;
	default:
		return _currentState;
	}
}

/**
 * Clean resources used in the state machine
 */
void clean_queues(void) {
	MSG_T* msg = NULL;
	MSG_RX_APP_T* msg_rx_app = NULL;
	void *buf = NULL;
	EVENTS evt;
	uint16_t length;
	/* Clear rx packets */
	while(queue_size(&_rx_pkt_list) > 0) {
		get_from_queue(&_rx_pkt_list, &buf, &length);
		msg_rx_app = (MSG_RX_APP_T*) buf;
		msg = msg_rx_app->msg;
		free(msg);
		msg = NULL;
		free(msg_rx_app);
		msg = NULL;
	}
	/* Clear tx packets */
	while(queue_size(&_tx_pkt_list) > 0) {
		get_from_queue(&_tx_pkt_list, &buf, &length);
		msg = (MSG_T*) buf;
		free(msg);
		msg = NULL;
	}
	/* Clear atcmd packets */
	while(queue_size(&_atcmd_list) > 0) {
		get_from_queue(&_atcmd_list, &buf, &length);
		free(buf);
		buf = NULL;
	}
	/* Clean event queue */
	while(event_size(&_eventQ) > 0) {
		get_event(&_eventQ, &evt, &buf, &length);
		if(buf != NULL) {
			free(buf);
			buf = NULL;
		}
	}
	/* Clean cold event queue */
	while(event_size(&_coldEventQ) > 0) {
		get_event(&_coldEventQ, &evt, &buf, &length);
		if(buf != NULL) {
			free(buf);
			buf = NULL;
		}
	}
}

/** @} */
/** @} */
