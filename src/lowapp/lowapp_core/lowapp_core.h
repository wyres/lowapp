/**
 * @file lowapp_core.h
 * @brief Internal core messages, events, structures
 * @date June 29, 2016
 *
 * @author Brian Wyld
 * @author Nathan Olff
 */

#ifndef LOWAPP_CORE_CORE_H_
#define LOWAPP_CORE_CORE_H_

#include "lowapp_types.h"
#include "lowapp_msg.h"

/**
 * LoWAPP Message types
 *
 * @anchor lowapp_message_types
 */
typedef enum {
	TYPE_STDMSG = 0x1, /**< Standard type LoWAPP message */
	TYPE_ACK = 0x2, /**< Acknowledge type LoWAPP message */
	TYPE_GWOUT = 0x3, /**< Gateway out type LoWAPP message */
	TYPE_GWIN = 0x4 /**< Gateway in type LoWAPP message */
} MSG_TYPE;

/**
 * @addtogroup lowapp_core
 * @{
 */
/**
 * Current version of the LoWAPP protocol
 *
 * This is used for the version field in the header of the frames.
 */
#define LOWAPP_CURRENT_VERSION	0x1

/**
 * @addtogroup lowapp_core_config
 * @{
 */
/**
 * @name LoWAPP default values related to general working of the protocol
 * @{
 */

/**
 * Radio coding rate
 *
 * @see #_coderate Corresponding attribute
 */
#define LOWAPP_CODING_RATE	1

/**
 * Radio spreading factor
 *
 * @see #_rsf Corresponding attribute
 */
#define LOWAPP_SPREADING_FACTOR	7

/**
 * Radio power (in dBm)
 *
 * @see #_power Corresponding attribute
 */
#define LOWAPP_TX_POWER	14

/**
 * Radio channel id
 *
 * @see #_rchanId Corresponding attribute
 */
#define LOWAPP_CHANNEL	0

/**
 * Radio bandwidth
 *
 * @see #_bandwidth Corresponding attribute
 */
#define LOWAPP_BANDWIDTH	0

/**
 * Radio preamble time (in ms)
 *
 * This is converted into a number of symbols
 *
 * @see #_preambleLen Corresponding attribute
 */
#define LOWAPP_PREAMBLE_TIME	1000

/**
 * CAD duration (in ms)
 *
 * @see #_cad_duration Corresponding attribute
 */
#define LOWAPP_CAD_DURATION	100

/**
 * CAD interval (in ms)
 *
 * @see #_cad_interval Corresponding attribute
 */
#define LOWAPP_CAD_INTERVAL	500

/** @} */
/** @} */


/**
 * @addtogroup lowapp_core_sm
 * @{
 */

/**
 * Event type used for the state machine
 */
typedef enum {
	STATE_ENTER, /**< Entering a state */
	STATE_EXIT, /**< Exiting a state */
	TXREQ, /**< The application requested to send a message */
	TXDONE, /**< End of transmission */
	RXMSG, /**< Incoming message has been received */
	RXERROR, /**< An error occurred during reception */
	RXAT, /**< An AT command is waiting to the processed */
	CADDONE, /**< Channel activity detection has terminated */
	CADTIMEOUT, /**< Periodic timeout for launching CAD */
	TIMEOUT, /**< Generic timer timed out (usually safeguard timer) */
	RXTIMEOUT, /**< Timeout occurred at the radio reception level */
	TXTIMEOUT, /**< Timeout occurred at the radio transmission level */
	TXUNBLOCK	/**< Unblock transmission when timer ends */
} EVENTS;

/**
 * States of the state machine
 */
typedef enum {
	/** Idle state @see state_idle Corresponding state's function */
	IDLE,
	/** Receiving state @see state_rxing Corresponding state's function */
	RXING,
	/** Skipping Ack state @see state_skipping_ack Corresponding state's function */
	SKIPPING_ACK,
	/** Wait window for tx Ack state @see state_wait_window_tx_ack */
	WAIT_SLOT_TX_ACK,
	/** Transmitting Ack state @see state_txingack Corresponding state's function */
	TXING_ACK,
	/** Transmitting state @see state_txing Corresponding state's function */
	TXING,
	/** Waiting before listening for Ack state @see state_wait_before_ack Corresponding state's function */
	WAIT_BEFORE_LISTENING_FOR_ACK,
	/** Receiving Ack state @see state_wait_ack Corresponding state's function */
	RXING_ACK,
	/** CAD state @see state_cad Corresponding state's function */
	CAD,
	/** Restart state @see state_restart Corresponding state's function */
	RESTART
} STATES;

/** @} */
/** @} */

/**
 * Sequence number for a specific group member
 */
typedef struct PEER {
	uint8_t out_txseq; /**< TX Sequence number */
	uint8_t out_rxseq; /**< Last RX sequence number from the receiver */
	uint8_t in_expected; /**< Next sequence number expected for incomming transmission */
} PEER_T;

/** Lower threshold for sequence numbers, used to assume rollover of the counter */
#define SEQ_ROLLOVER_LOW_THRESHOLD	30
/** Higher threshold for sequence numbers, used to assume rollover of the counter */
#define SEQ_ROLLOVER_HIGH_THRESHOLD	230

/** Duty cycle window (in ms) */
#define DUTY_CYCLE_WINDOW	3600000
/** Duty cycle 1% equivalent (in ms) */
#define DUTY_CYCLE_ALLOWED	36000

/**
 * Statistics element for AT+WHO
 */
typedef struct STAT {
	uint8_t deviceId;	/**< Device id */
	int16_t lastRssi;		/**< RSSI of the last message received from this node */
	uint64_t lastSeen;	/**< Time in ms of the last message received */
} STAT_T;

/**
 * Operation modes available for the nodes
 */
enum NODE_MODE {
	/**
	 * @brief Pull mode (default)
	 *
	 * In pull mode, the application must explicitly issue an AT+POLLRX
	 * command to get the received packets
	 */
	PULL,
	/**
	 * @brief Push mode
	 *
	 * In push mode, the core sends directly the received packets to the application
	 */
	PUSH
};

/** Operation mode type definition */
typedef enum NODE_MODE NODE_MODE_T;

void update_safeguard_timers();
void set_default_values();
bool check_configuration();
bool check_attribute(const uint8_t* key, const uint8_t* val);
void core_radio_init();
void core_init();
int8_t lowapp_tx(MSG_T* msg);
uint8_t sm_run();
void clean_queues(void);
void process_ack(MSG_T* msg);

void timeoutCB();
void timeoutCB2();
void cadTimeoutCB();

#endif // LOWAPP_INC_LOWAPP_CORE_H_
