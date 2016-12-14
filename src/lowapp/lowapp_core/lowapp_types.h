/**
 * @file lowapp_types.h
 * @brief LoWAPP macros and type definitions for system level functions
 *
 * Defines LoWAPP constants related to timing functionality and identifiers.
 * Declares types for the system level functions and a general structure
 * for storing all system level functions.
 *
 * @author Brian Wyld
 * @author Nathan Olff
 * @date March 22, 2016
 */

#ifndef LOWAPP_CORE_TYPES_H_
#define LOWAPP_CORE_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @name LoWAPP Message format type
 * @{
 */
/** Classic format message */
//#define LOWAPP_MSG_FORMAT_CLASSIC
/** BLE Application format message */
//#define LOWAPP_MSG_FORMAT_GPSAPP
//#define LOWAPP_MSG_FORMAT_GPSAPP_RSSI

/**
 * Verify that a message format is selected
 *
 * LOWAPP_MSG_FORMAT_CLASSIC : Standard messages, sent at ASCII text with AT+SEND,
 * returned as JSON format
 * LOWAPP_MSG_FORMAT_GPSAPP : Special format of messages for communication with
 * an Android application
 * LOWAPP_MSG_FORMAT_GPSAPP_RSSI : Special format of messages for communication with
 * an Android application. This adds the RSSI values to the message payload
 */
#if (defined(LOWAPP_MSG_FORMAT_CLASSIC) && defined(LOWAPP_MSG_FORMAT_GPSAPP)) \
	|| (defined(LOWAPP_MSG_FORMAT_GPSAPP) && defined(LOWAPP_MSG_FORMAT_GPSAPP_RSSI)) \
	|| (defined(LOWAPP_MSG_FORMAT_CLASSIC) && defined(LOWAPP_MSG_FORMAT_GPSAPP_RSSI)) \
	|| (!defined(LOWAPP_MSG_FORMAT_CLASSIC) && !defined(LOWAPP_MSG_FORMAT_GPSAPP) \
			&& !defined(LOWAPP_MSG_FORMAT_GPSAPP_RSSI))
	#error "Please select one message format"
#endif
/** @} */

/**
 * @name LoWAPP Special device ID field values
 * Constants used for special destination ID in messages
 * @{*/
/** ID field for broadcast messages */
#define LOWAPP_ID_BROADCAST 0xFF
/** RFU device id */
#define LOWAPP_ID_ADDR_RES_1 	0xFE
/** RFU device id */
#define LOWAPP_ID_ADDR_RES_2 	0xFD
/** RFU device id */
#define LOWAPP_ID_ADDR_RES_3	0xFC
/** RFU device id */
#define LOWAPP_ID_ADDR_REQ_4	0xFB
/** ID field fo gateway messages */
#define LOWAPP_ID_GATEWAY		0x00
/** @}*/

/**
 * @name LoWAPP State Machine return values
 * Constants used to specify if the device should go in shallow
 * or deep sleep mode
 * @{
 */
/**
 * @brief SM return code for shallow sleep mode
 *
 * This is used when we are in any state of the state machine
 * different from Idle
 */
#define LOWAPP_SM_SHALLOW_SLEEP		0
/**
 * @brief SM return code for deep sleep mode
 *
 * This is used when we are in sleep mode. Only CAD request and
 * AT commands should wake the device.
 */
#define LOWAPP_SM_DEEP_SLEEP	1
/**
 * @brief SM return code for transmission in progress
 *
 * This is used to notify the application that we are transmitting.
 * It is up to the application developer to decide if the application
 * should do something, go to a shallow sleep mode or simply looping.
 */
#define LOWAPP_SM_TX	2
/**
 * @brief SM return code for reception in progress
 *
 * This is used to notify the application that we are receiving a packet.
 * It is up to the application developer to decide if the application
 * should do something, go to a shallow sleep mode or simply looping.
 */
#define LOWAPP_SM_RX	3

/** @} */

/**
 * @name LoRa Bandwidth values
 * @{
 */
#define LORA_BANDWIDTH_0	125000
#define LORA_BANDWIDTH_1	250000
#define LORA_BANDWIDTH_2	500000
/** @} */

/**
 * @name LoRa Channel Frequencies
 * @{
 */
#define LORA_CHANID_0	863125000
#define LORA_CHANID_1	863425000
#define LORA_CHANID_2	863725000
#define LORA_CHANID_3	864025000
#define LORA_CHANID_4	864325000
#define LORA_CHANID_5	864625000
#define LORA_CHANID_6	864925000
#define LORA_CHANID_7	865225000
#define LORA_CHANID_8	865525000
#define LORA_CHANID_9	865825000
#define LORA_CHANID_10	866125000
#define LORA_CHANID_11	866425000
#define LORA_CHANID_12	866725000
#define LORA_CHANID_13	867025000
#define LORA_CHANID_14	867325000
#define LORA_CHANID_15	867625000

/** @} */

/**
 * @name Default times for transmission, reception and safeguard timers (in ms)
 * @{
 */
/** Beginning of the Ack slot */
#define TIMER_ACK_SLOT_START	1000
/**
 * Time between RX Done and TX Ack start
 *
 * In theory, this should be put at the middle of the ACK slot
 * SLOT_START+(SLOT_START+SLOT_LENGTH)/2. In practice, after the timer, the
 * transmission process starts by checking the channel for CHAN_FREE_TIMEOUT.
 * We therefore need to move the start of tx ack to SLOT_START
 *
 */
#define TIMER_ACK_SLOT_TX		TIMER_ACK_SLOT_START+(TIMER_ACK_SLOT_LENGTH/2)-TIMER_CHANNEL_FREE_INTERVAL
/** Length of the Ack slot */
#define TIMER_ACK_SLOT_LENGTH	1000
/** Timer before next channel free for tx check */
#define TIMER_CHANNEL_FREE_INTERVAL	10
/** Preamble time for ACK */
#define PREAMBLE_ACK		8 //2
/** Timer for retry when TX fail */
#define TIMER_TX_FAIL_RETRY		1000
/**@}*/

/**
 * @name Bounds for configuration variables
 * @{
 */
/** Minimum authorized device ID */
#define MIN_DEVICE_ID	1
/** Maximum authorized device ID */
#define MAX_DEVICE_ID	250

/** Maximum radio channel value */
#define MAX_RCHAN_ID	15

/** Minimum spreading factor */
#define MIN_SPREADINGFACTOR	7u
/** Maximum spreading factor */
#define MAX_SPREADINGFACTOR	12u

/** Size of the encryption key */
#define ENCKEY_SIZE		16

/* @} */

/**
 * @name Prototypes for interface between system and core
 * @{
 */
/** Timer callback */
typedef void (*LOWAPP_TIMER_CB_T)(void);
/** LoRa reception callback */
typedef void (*LOWAPP_LORARX_CB_T)(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
/** LoRa RX error callback */
typedef void (*LOWAPP_LORARXERROR_CB_T)(void);
/** LoRa RX timeout callback */
typedef void (*LOWAPP_LORARXTIMEOUT_CB_T)(void);
/** LoRa transmission callback */
typedef void (*LOWAPP_LORATX_CB_T)(void);
/** LoRa TX timeout callback */
typedef void (*LOWAPP_LORATXTIMEOUT_CB_T)(void);
/** LoRA CAD done callback */
typedef void (*LOWAPP_LORACAD_CB_T)(bool channelActivityDetected);

/** Command response function */
typedef void (*LOWAPP_CMDRX_CB_T)(void);

/**
 * Get the current clock time in ms
 * @see LOWAPP_SYS_IF#SYS_getTimeMs
 */
typedef uint64_t (*LOWAPP_GETTIMEMS_T)();

/**
 * Initialise a timer
 * @see LOWAPP_SYS_IF#SYS_initTimer
 */
typedef void (*LOWAPP_INITTIMER_T)(LOWAPP_TIMER_CB_T callback);
/**
 * Request to set a callback to be called in timems
 * @see LOWAPP_SYS_IF#SYS_setTimer
 * @see LOWAPP_SYS_IF#SYS_setRepetitiveTimer
 */
typedef void (*LOWAPP_SETTIMER_T)(uint32_t timems);
/**
 * Disarm the timer to avoid unexpected timeout later on
 * @see LOWAPP_SYS_IF#SYS_cancelTimer
 */
typedef void (*LOWAPP_CANCELTIMER_T)(void);
/**
 * Blocking delay in ms
 * @see LOWAPP_SYS_IF_T#SYS_delayMs
 */
typedef void (*LOWAPP_DELAYMS_T)(uint32_t delay);
/**
 * Send ascii data back to the application
 * @see LOWAPP_SYS_IF#SYS_cmdResponse
 */
typedef int8_t (*LOWAPP_CMDRESPONSE_T)(uint8_t* data, uint16_t length);

/**
 * Get a string configuration value from persistent memory
 * @see LOWAPP_SYS_IF#SYS_getConfig
 */
typedef int8_t (*LOWAPP_GETCONFIG_T)(const uint8_t* key, uint8_t* value);

/**
 * Set a configuration value to persistent memory
 * @see LOWAPP_SYS_IF#SYS_getConfig
 */
typedef int8_t (*LOWAPP_SETCONFIG_T)(const uint8_t* key, const uint8_t* value);

/**
 * Register callback for LoRa reception
 * @see LOWAPP_SYS_IF#SYS_register_rx_done
 */
typedef void (*LOWAPP_REGISTERLORARX_T)(LOWAPP_LORARX_CB_T callback);
/**
 * Register callback for LoRa reception error
 * @see LOWAPP_SYS_IF#SYS_register_rx_error
 */
typedef void (*LOWAPP_REGISTERLORARXERROR_T)(LOWAPP_LORARXERROR_CB_T callback);
/**
 * Register callback for LoRa reception timeout
 * @see LOWAPP_SYS_IF#SYS_register_rx_timeout
 */
typedef void (*LOWAPP_REGISTERLORARXTIMEOUT_T)(LOWAPP_LORARXTIMEOUT_CB_T callback);
/**
 * Register callback for LoRa transmission
 * @see LOWAPP_SYS_IF#SYS_register_tx_done
 */
typedef void (*LOWAPP_REGISTERLORATX_T)(LOWAPP_LORATX_CB_T callback);
/**
 * Register callback for LoRa transmission timeout
 * @see LOWAPP_SYS_IF#SYS_register_tx_timeout
 */
typedef void (*LOWAPP_REGISTERLORATXTIMEOUT_T)(LOWAPP_LORATXTIMEOUT_CB_T callback);
/**
 * Register callback for LoRa CAD
 * @see LOWAPP_SYS_IF#SYS_register_cad
 */
typedef void (*LOWAPP_REGISTERLORACAD_T)(LOWAPP_LORACAD_CB_T callback);
/**
 * Request transmission of a LoRa frame
 * @see LOWAPP_SYS_IF#SYS_loraTx
 */
typedef void (*LOWAPP_RADIO_TX_T)(uint8_t* data, uint8_t dlen);
/**
 * Start reception of a LoRa frame
 * @see LOWAPP_SYS_IF#SYS_loraRx
 */
typedef void (*LOWAPP_RADIO_RX_T)(uint32_t timeout);

/**
 * Request LoRa CAD (Channel Activity Detection)
 * @see LOWAPP_SYS_IF#SYS_checkLoraActivityForRx
 */
typedef void (*LOWAPP_RADIO_CAD_T)();
/**
 * Check LoRa channel availability before transmission
 * @see LOWAPP_SYS_IF#SYS_checkChanFreeForTx
 */
typedef bool (*LOWAPP_RADIO_LBT_T)(uint8_t chan);

/**
 * Write configuration into persistent memory
 * @see LOWAPP_SYS_IF#SYS_writeConfig
 */
typedef int8_t (*LOWAPP_WRITECONFIG_T)(void);
/**
 * Write configuration from persistent memory
 * @see LOWAPP_SYS_IF#SYS_readConfig
 */
typedef int8_t (*LOWAPP_READCONFIG_T)(void);

/**
 * Radio driver callback functions
 */
typedef struct
{
	/** LoRa transmission callback */
	LOWAPP_LORATX_CB_T TxDone;
	/** LoRa TX timeout callback */
	LOWAPP_LORATXTIMEOUT_CB_T TxTimeout;
	/** LoRa reception callback */
	LOWAPP_LORARX_CB_T RxDone;
	/** LoRa RX timeout callback */
    LOWAPP_LORARXTIMEOUT_CB_T RxTimeout;
	/** LoRa RX error callback */
    LOWAPP_LORARXERROR_CB_T RxError;
    //void ( *FhssChangeChannel )( uint8_t currentChannel );
	/** LoRA CAD done callback */
    LOWAPP_LORACAD_CB_T CadDone;
}Lowapp_RadioEvents_t;


/**
 * Initialise radio
 * @see LOWAPP_SYS_IF#SYS_radioInit
 */
typedef void (*LOWAPP_RADIO_INIT_T)(Lowapp_RadioEvents_t *evt);

/**
 * Configure TX parameters
 * @see LOWAPP_SYS_IF#SYS_radioSetTx
 */
typedef void (*LOWAPP_RADIO_SETTXCONFIG_T)(int8_t power, uint8_t bandwidth, uint8_t datarate,
		uint8_t coderate, uint16_t preambleLen, uint32_t timeout, bool fixLen);

/**
 * Configure RX parameters
 * @see LOWAPP_SYS_IF#SYS_radioSetRx
 */
typedef void (*LOWAPP_RADIO_SETRXCONFIG_T)(uint8_t bandwidth, uint8_t datarate, uint8_t coderate,
		uint16_t preambleLen, bool fixLen, uint8_t payloadLen, bool rxContinuous);

/**
 * Compute the packet time on air in us
 * @see LOWAPP_SYS_IF#SYS_radioTimeOnAir
 */
typedef uint32_t (*LOWAPP_RADIO_TIMEONAIR_T)(uint8_t pktLen);

/**
 * Set radio channel
 * @see LOWAPP_SYS_IF#SYS_radioSetChannel
 */
typedef void ( *LOWAPP_RADIO_SETCHANNEL_T )( uint32_t freq );

/**
 * Generate random value based on RSSI readins
 * @see LOWAPP_SYS_IF#SYS_radioRandom
 */
typedef uint32_t (*LOWAPP_RANDOM_T)(void);

/**
 * Set radio in sleep mode
 * @see LOWAPP_SYS_IF#SYS_radioSleep
 */
typedef void (*LOWAPP_RADIO_SLEEP_T)(void);

/**
 * Set radio preamble length (without full SetTx/SetRx)
 * @see LOWAPP_SYS_IF#SYS_radioSetPreamble
 */
typedef void (*LOWAPP_RADIO_SETPREAMBLE_T)(uint16_t preambleLen);

/**
 * Set fix length packet for Tx (without full SetTx/SetRx)
 * @see LOWAPP_SYS_IF#SYS_radioSetTxFixLen
 */
typedef void (*LOWAPP_RADIO_SETTXFIXLEN_T)(bool fixLen);

/**
 * Set fix length packet for Rx (without full SetTx/SetRx)
 * @see LOWAPP_SYS_IF#SYS_radioSetRxFixLen
 */
typedef void (*LOWAPP_RADIO_SETRXFIXLEN_T)(bool fixLen, uint8_t payloadLen);

/**
 * Set TX timeout (without full SetTx)
 * @see LOWAPP_SYS_IF_T#SYS_radioSetTxTimeout
 */
typedef void (*LOWAPP_RADIO_SETTXTIMEOUT_T)(uint32_t timeout);

/**
 * Set RX continuous
 * @see LOWAPP_SYS_IF#SYS_radioSetRxContinuous
 */
typedef void (*LOWAPP_RADIO_SETRXCONTINUOUS_T)(bool rxContinuous);

/**
 * Set radio callbacks
 * @see LOWAPP_SYS_IF#SYS_radioSetCallbacks
 */
typedef void (*LOWAPP_RADIO_SETRADIOCB_T)(Lowapp_RadioEvents_t* events);

/**@} */


#endif /* LOWAPP_CORE_TYPES_H_ */
