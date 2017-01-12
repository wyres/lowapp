/**
 * @file lowapp_api.c
 * @brief Interface functions of the LoWAPP core callable from the application
 * @date June 29, 2016
 *
 * This file contains the configuration variables, the strings
 * corresponding to these variables and the functions callable from
 * the application.
 *
 * @author Brian Wyld
 * @author Nathan Olff
 */

#include "lowapp_inc.h"

/**
 * @addtogroup lowapp_core
 * @{
 */
/**
 * @addtogroup lowapp_core_config LoWAPP Core Device Configuration
 * @brief Device configuration variables
 * @{
 */
/**
 * @name LoWAPP Configuration variables
 *
 * Configuration values of the device
 * @{
 */
/**
 * Radio channel currently used by the device
 * Invalid value set by default for checking load config
 */
uint8_t _rchanId = 255;
/**
 * Radio spreading factor / datarate currently used by the device
 *
 * Values (chips) :
 * 		6: 64
 * 		7: 128
 * 		8: 256
 * 		9: 512
 *      10: 1024
 *      11: 2048
 *      12: 4096
 */
uint8_t _rsf;

/**
 * Radio bandwidth
 *
 * Values :
 * 		0: 125 kHz
 * 		1: 250 kHz
 * 		2: 500 kHz
 * 		3: Reserved
 */
uint8_t _bandwidth = LOWAPP_BANDWIDTH;

/**
 * Radio coding rate
 *
 * Values :
 * 		1: 4/5
 * 		2: 4/6
 * 		3: 4/7
 * 		4: 4/8
 */
uint8_t _coderate = LOWAPP_CODING_RATE;
/** Radio power (in dBm) */
int8_t _power = LOWAPP_TX_POWER;
/**
 * @brief Bitfield used for gateway type
 *
 * It is displayed as Big Endian when needed.<br />
 * Each bit of this variable correspond to a type of gateway messages:
 * - Bit 0 : LoWAPP intergroup message
 * - Bit 1 : LoRaWAN public network
 * - Bit 2 : IPv4 network with UDP addressing
 * - Bit 3-31 : Reserved for future use
 */
uint32_t _gwMask;
/** End point device id */
uint8_t _deviceId;
/** Group id */
uint16_t _groupId;
/** Preamble length in symbols */
uint16_t _preambleLen;
/** Encryption key : 128 bit AES, displayed as Little Endian */
uint8_t _encryptionKey[16];
/** Operation mode of the node */
NODE_MODE_T _opMode = PULL;
/** Connected flag */
bool _connected = false;

/**
 * Preamble time in ms
 * Invalid value (0) set by default for checking load config
 */
uint16_t preambleTime = 0;

/** CAD duration in ms */
uint16_t _cad_duration = LOWAPP_CAD_DURATION;
/** Interval between two CAD in ms */
uint32_t _cad_interval;

/** @} */

/**
 * @name LoWAPP Configuration variables string literals
 * @brief Strings used to store variables or to modify them through AT commands
 * @{
 */
/**
 * Radio channel key string
 * @see _rchanId configuration variable
 */
const uint8_t strRchanId[] = "chanId";
/**
 * Radio spreading factor key string
 * @see _rsf configuration variable
 */
const uint8_t strRsf[] = "txDatarate";
/**
 * Coding rate key string
 * @see _coderate configuration variable
 */
const uint8_t strCoderate[] = "coderate";
/**
 * Bandwidth key string
 * @see _bandwidth configuration variable
 */
const uint8_t strBandwidth[] = "bandwidth";
/**
 * Power key string
 * @see _power configuration variable
 */
const uint8_t strPower[] = "power";
/**
 * Gateway type key string
 * @see _gwMask configuration variable
 */
const uint8_t strGwMask[] = "gwMask";
/**
 * Device id key string
 * @see _deviceId configuration variable
 */
const uint8_t strDeviceId[] = "deviceId";
/**
 * Group id key string
 * @see _groupId configuration variable
 */
const uint8_t strGroupId[] = "groupId";
/**
 * Preamble time key string
 * @see preambleTime variable
 */
const uint8_t strPreambleTime[] = "pTime";
/**
 * Preamble length key string
 * @see _preambleLength configuration variable
 */
const uint8_t strPreambleLength[] = "pLen";
/**
 * Encryption key string
 * @see _appKey configuration variable
 */
const uint8_t strEncKey[] = "encKey";
/**
 * Operation mode key string
 * @see _opMode configuration variable
 */
const uint8_t strOpMode[] = "opMode";
/**
 * Maximum retry for LBT key string
 * @see _maxRetryLBT
 */
const uint8_t strMaxRetryLBT[] = "maxRetryLBT";
/** @} */

/** @} */
/** @} */

/** Store all bandwidth values */
const uint32_t bandwidthValues[4] = {
		LORA_BANDWIDTH_0,
		LORA_BANDWIDTH_1,
		LORA_BANDWIDTH_2,
		0
};

/** Store frequency for each channel id */
const uint32_t channelFrequencies[] = {
	LORA_CHANID_0,
	LORA_CHANID_1,
	LORA_CHANID_2,
	LORA_CHANID_3,
	LORA_CHANID_4,
	LORA_CHANID_5,
	LORA_CHANID_6,
	LORA_CHANID_7,
	LORA_CHANID_8,
	LORA_CHANID_9,
	LORA_CHANID_10,
	LORA_CHANID_11,
	LORA_CHANID_12,
	LORA_CHANID_13,
	LORA_CHANID_14,
	LORA_CHANID_15
};

/**
 * System level functions
 */
LOWAPP_SYS_IF_T* _sys = NULL;

extern QFIXED_T _atcmd_list;
extern QEVENT_T _coldEventQ;

extern uint8_t cad_flag;

extern const uint8_t jsonPrefixError[];
extern const uint8_t errorMsgMissingConfiguration[];

/**
 * @addtogroup lowapp_core LoWAPP Core
 * @{
 */
/**
 * @addtogroup lowapp_core_interface LoWAPP Core Public Interface
 * @brief Application Programming Interface
 * @{
 */

/**
 * @brief Initialise the LoWAPP core
 *
 * @param sys_fns : set of system level functions
 * @retval 0 If the LoWAPP core was successfully initialised
 * @retval -1 If an error happened
 */
int8_t lowapp_init(LOWAPP_SYS_IF_T* sys_fns) {
	LOG(LOG_INFO, "Initialise LoWAPP Core");
	_sys = sys_fns;
	core_radio_init();
#ifdef SIMU
	set_default_values(); /* Set default values before reading from memory */
#endif
	_sys->SYS_readConfig(); /* Retrieve config from persistent memory */
	/* Init timers */
	_sys->SYS_initTimer(timeoutCB);
	_sys->SYS_initTimer2(timeoutCB2);
	_sys->SYS_initRepetitiveTimer(cadTimeoutCB);

	if(load_full_config() < 0 || !check_configuration()) { 	/* Get all config values from system */
		LOG(LOG_FATAL, "Missing configuration values, could not start");
		uint8_t jerr[200] = "";
		/* Format message for answer to the UART */
		/* Size of the current string to add to the answer */
		uint8_t sizeStr = 0;
		uint8_t offset = 0;
		/* Build the JSON message */
		sizeStr = strlen((char*)jsonPrefixError);
		memcpy(jerr, jsonPrefixError, sizeStr);
		offset += sizeStr;
		uint8_t errorCode = LOWAPP_ERR_LOADCFG;
		offset = FillBuffer8_t(jerr, offset, &errorCode, 1, false);
		sizeStr = strlen((char*)errorMsgMissingConfiguration);
		memcpy(jerr+offset, errorMsgMissingConfiguration, sizeStr);
		offset += sizeStr;

		_sys->SYS_cmdResponse(jerr, offset);
		core_init(); /* Init LoWAPP core */
		return -1;
	}
	else {
		LOG(LOG_INFO, "Device ID=%u", _deviceId);
		core_init(); /* Init LoWAPP core */

		/* Initialise radio as connected */
		_connected = true;

		/* Initialise CAD timer */
		cad_flag = 0;
		_sys->SYS_setRepetitiveTimer(_cad_interval);

		_sys->SYS_cmdResponse((uint8_t*)"BOOT OK", 7);
		return 0;
	}
}

/**
 * @brief Execute the state machine
 *
 * This is the function that runs the LoWAPP state machine and manages the whole protocol.<br />
 * It returns when the state machine no longer has any event to process.<br />
 * It needs to be called from the application periodically (or every time an interrupt occurs).
 *
 * @return When there are no event to be processed in the event queues.
 * The return value specify whether the device should go in shallow sleep
 * mode or in deep sleep mode.
 */
uint8_t lowapp_process() {
	return sm_run();
}

/**
 * @brief Prepare an AT commands to be process by the LoWAPP core
 *
 * Adds the command to the list of AT commands to process.<br />
 * Notify the core that an AT command is waiting to be processed through the cold
 * event queue.
 *
 * @param cmdrequest AT command as a string, forwarded from the application or
 * from the UART driver
 * @param sizeCommand Size of the command (not counting end of char/line)
 * @retval -1 If the parameter was NULL
 * @retval 0 If the command was added to the queue of AT commands to process
 */
int8_t lowapp_atcmd(uint8_t* cmdrequest, uint16_t sizeCommand) {
	if (cmdrequest == NULL) {
		return LOWAPP_ERR_INVAL;
	}
	uint8_t* command = NULL;
	/*
	 * Copy cmdrequest to a hex buffer to avoid allocation issue with calling
	 * application code
	 */
	command = (uint8_t*) calloc(sizeCommand+1, sizeof(uint8_t));
	if (command == NULL) {
		LOG(LOG_ERR, "Error allocating memory");
		return -1;
	}
	memcpy(command, cmdrequest, sizeCommand);
	lock_atcmd();
	/* Add AT command to the list */
	if (add_to_queue(&_atcmd_list, command, sizeCommand) == -1) {
		unlock_atcmd();
		LOG(LOG_ERR, "The AT cmd queue was full");
		/* Free command buffer */
		free(command);
		command = NULL;
		return -1;
	} else {
		unlock_atcmd();
	}

	lock_coldEventQ();
	/* Notify the core that an AT command has been received */
	add_simple_event(&_coldEventQ, RXAT);
	unlock_coldEventQ();

	return 0;
}

/**
 * Add a null command to the list of AT command to notify the application that
 * an error occurred.
 */
void lowapp_atcmderror() {
	lock_atcmd();
	/* Add AT command to the list */
	if (add_to_queue(&_atcmd_list, NULL, 0) == -1) {
		unlock_atcmd();
		LOG(LOG_ERR, "The AT cmd queue was full");
	} else {
		unlock_atcmd();
	}

	lock_coldEventQ();
	/* Notify the core that an AT command has been received */
	add_simple_event(&_coldEventQ, RXAT);
	unlock_coldEventQ();
}

/** @} */
/** @} */

