/**
 * @file lowapp_atcmd.c
 * @date June 29, 2016
 * @brief LoWAPP AT commands processing
 *
 * @author Brian Wyld
 * @author Nathan Olff
 */

#include "lowapp_inc.h"
#include <ctype.h>


/**
 * @addtogroup lowapp_core
 * @{
 */
/**
 * @addtogroup lowapp_core_atcmd LoWAPP Core AT Commands
 * @brief AT commands definitions and processing functions
 * @{
 */

/**
 * @name LoWAPP AT commands string literals
 * @{
 */
/**
 * AT write configuration command
 *
 * @see cmd_writecfg Corresponding execution function
 */
const uint8_t msgWriteConfig[] 		= "AT&W";
/**
 * AT read configuration command
 *
 * @see cmd_readcfg Corresponding execution function
 */
const uint8_t msgDisplayConfig[] 	= "AT&V";
/**
 * AT set/get gateway type command
 *
 * @see cmd_set AT set execution function
 * @see cmd_get AT get execution function
 */
const uint8_t msgGwMask[]			= "AT+GWMASK";
/**
 * AT set/get device id command
 *
 * @see cmd_set AT set execution function
 * @see cmd_get AT get execution function
 */
const uint8_t msgDeviceId[]			= "AT+DEVICEID";
/**
 * AT set/get group id command
 *
 * @see cmd_set AT set execution function
 * @see cmd_get AT get execution function
 */
const uint8_t msgGroupId[]			= "AT+GROUPID";
/**
 * AT set/get radio channel id command
 *
 * @see cmd_set AT set execution function
 * @see cmd_get AT get execution function
 */
const uint8_t msgChanId[]			= "AT+CHANID";
/**
 * AT set/get radio spreading factor id command
 *
 * @see cmd_set AT set execution function
 * @see cmd_get AT get execution function
 */
const uint8_t msgSf[]			= "AT+TXDR";
/**
 * AT set/get radio preamble time command
 *
 * @see cmd_set AT set execution function
 * @see cmd_get AT get execution function
 */
const uint8_t msgPreambleTime[]	= "AT+PTIME";
/**
 * AT set/get encryption key command
 *
 * @see cmd_set AT set execution function
 * @see cmd_get AT get execution function
 */
const uint8_t msgEncKey[]			= "AT+ENCKEY";
/**
 * AT hardware selftest command
 *
 * @see cmd_selftest Corresponding execution function
 */
const uint8_t msgSelftest[]			= "AT+SELFTEST";
/**
 * AT show statistics command
 *
 * @see cmd_getstats Corresponding execution function
 */
const uint8_t msgStats[]			= "AT+STATS";
/**
 * AT WHO command
 *
 * @see cmd_who Corresponding execution function
 */
const uint8_t msgWho[]				= "AT+WHO";
/**
 * AT PING command
 *
 * @see cmd_ping Corresponding execution function
 */
const uint8_t msgPing[]				= "AT+PING";
/**
 * AT HELLO command
 *
 * @see cmd_hello Corresponding execution function
 */
const uint8_t msgHello[]			= "AT+HELLO";
/**
 * AT SEND command
 *
 * @see cmd_send Corresponding execution function
 */
const uint8_t msgSend[]				= "AT+SEND";
/**
 * AT poll rx command
 *
 * @see cmd_pollrx Corresponding execution function
 */
const uint8_t msgPollRx[]			= "AT+POLLRX";
/**
 * AT push rx command
 *
 * @see cmd_pushrx Corresponding execution function
 */
const uint8_t msgPushRx[]				= "AT+PUSHRX";
/**
 * AT disconnection command
 *
 * @see cmd_disconnect Corresponding execution function
 */
const uint8_t msgDisconnect[]		= "AT+DISCONNECT";
/**
 * AT connection command
 *
 * @see cmd_connect Corresponding execution function
 */
const uint8_t msgConnect[]			= "AT+CONNECT";
/**
 * AT reset command
 *
 * @see cmd_reset Corresponding execution function
 */
const uint8_t msgReset[]			= "ATZ";

#ifdef SIMU
/**
 * AT set log level command
 *
 * @see set_log_level Corresponding execution function
 */
const uint8_t msgLog[]				= "AT+LOG";
#endif

/** @} */

/** Default string for display AT&V response */
const uint8_t defaultDisplayString[] = "OK {\"chanId\":\"00\",\"txDatarate\":\"00\","
		"\"bandwidth\":\"0\",\"coderate\":\"0\",\"power\":\"00\",\"gwMask\":\"00000000\","
		"\"deviceId\":\"00\",\"groupId\":\"0000\",\"pTime\":\"00000\"}";

/** Ping payload */
const uint8_t pingPayload[] = "PING";

extern Lowapp_RadioEvents_t radio_callbacks;
extern MSG_RXDONE_T msgReceived;
extern uint16_t timer_safeguard_rxing_ack;
extern uint8_t radioFlags;

extern QFIXED_T _atcmd_list;
extern QEVENT_T _coldEventQ;

extern bool txBlocked;
extern QFIXED_T _tx_pkt_list;

extern QSTAT_T statisticsWho;

extern PEER_T peers[256];

extern const uint32_t channelFrequencies[];

extern uint8_t jsonPrefixOk[];
extern uint8_t jsonSuffix[];
extern uint8_t jsonKeyValDelimiter[];
extern uint8_t jsonDelimiterErrorCodeString[];
extern uint8_t errorMsgAtCmdInvalidSize[];
extern uint8_t jsonPrefixError[];

extern uint8_t jsonWhoPrefix[];
extern uint8_t jsonWhoDevice[];
extern uint8_t jsonWhoLastRssi[];
extern uint8_t jsonWhoLastSeen[];
extern uint8_t jsonWhoSuffix[];

/* Static functions prototypes */
static int8_t cmd_set(const uint8_t* p1, const uint8_t* p2, uint8_t** err);
static int8_t cmd_get(const uint8_t* p1, uint8_t** err);
static int8_t cmd_writecfg(uint8_t** err);
static int8_t cmd_readcfg(uint8_t** err);
static int8_t cmd_displaycfg(uint8_t** err);
static int8_t cmd_selftest(uint8_t** err);
static int8_t cmd_getstats(uint8_t** err);
static int8_t cmd_who(uint8_t** err);
static int8_t cmd_ping(uint8_t* p1, uint8_t** err);
static int8_t cmd_hello(uint8_t** err);
static int8_t cmd_send(uint8_t* p1, uint8_t* p2, uint8_t** err);
static int8_t cmd_pollrx(uint8_t** err);
static int8_t cmd_pushrx(uint8_t** err);
static int8_t cmd_disconnect(uint8_t** err);
static int8_t cmd_connect(uint8_t** err);
static int8_t cmd_reset(uint8_t** err);
static int8_t at_cmd_process(uint8_t* cmdrequest);
static int8_t at_cmd_interp(uint8_t* cmd, uint8_t* p1, uint8_t* p2, uint8_t** err);
static bool eat_ws(uint8_t** lp);
static int8_t parseATCmd(uint8_t* line, uint8_t** cmd, uint8_t** param1, uint8_t**param2, uint8_t** errstr);

/**
 * @brief Load full configuration from application level memory
 *
 * @retval 0 If all configuration variables were set
 * @retval -1 If at least one value was not found
 * @retval -2 If at least one value was not valid
 */
int8_t load_full_config() {
	uint8_t updateRadioAttr = 0;
	uint8_t value[100];
	int8_t ret = 0;
	/* Retrieve gateway type bitfield */
	if(_sys->SYS_getConfig(strGwMask, value) >= 0) {
		if(AsciiHexStringConversionBI8_t((uint8_t*)&_gwMask, value, 8) != 1) {
			ret = -2;
		}
	}
	else {
		ret = -1;
	}
	/* Retrieve device id value */
	if(_sys->SYS_getConfig(strDeviceId, value) >= 0) {
		if(AsciiHexStringConversionBI8_t((uint8_t*)&_deviceId, value, 2) != 1) {
			ret = -2;
		}
	}
	else {
		ret = -1;
	}
	/* Retrieve group id value */
	if(_sys->SYS_getConfig(strGroupId, value) >= 0) {
		if(AsciiHexStringConversionBI8_t((uint8_t*)&_groupId, value, 4) != 1) {
			ret = -2;
		}
	}
	else {
		ret = -1;
	}
	/* Retrieve radio channel id value */
	if(_sys->SYS_getConfig(strRchanId, value) >= 0) {
		uint8_t newRchanid;
		if(AsciiHexStringConversionBI8_t(&newRchanid, value, 2) != 1) {
			ret = -2;
		}
		else {
			if(_rchanId != newRchanid) {
				/* Change configuration value */
				_rchanId = newRchanid;
				/* Update frequency */
				uint32_t freq = channelFrequencies[_rchanId];
				_sys->SYS_radioSetChannel(freq);
			}
		}
	}
	else {
		ret = -1;
	}
	/* Retrieve radio spreading factor id value */
	if(_sys->SYS_getConfig(strRsf, value) >= 0) {
		uint8_t newRsf = 0;

		if(AsciiHexStringConversionBI8_t(&newRsf, value, 2) != 1) {
			ret = -2;
		}
		else {
			if(_rsf != newRsf) {
				_rsf = newRsf;
				updateRadioAttr++;
			}
		}
	}
	else {
		ret = -1;
	}
	/* Retrieve preamble time value */
	if(_sys->SYS_getConfig(strPreambleTime, value) >= 0) {
		uint16_t newPreamble;
		newPreamble = AsciiDecStringConversion_t(value, strlen((char*)value));
		if(newPreamble == 0) {
			ret = -2;
		}
		else {
			if(newPreamble != preambleTime) {
				/* Set new preamble time */
				preambleTime = newPreamble;
				updateRadioAttr++;
			}
		}
	}
	else {
		ret = -1;
	}
	/* Retrieve encryption key value */
	if(_sys->SYS_getConfig(strEncKey, value) >= 0) {
		if(AsciiHexStringConversionBI8_t((uint8_t*)_encryptionKey, value, 32) != 1) {
			ret = -2;
		}
	}
	else {
		ret = -1;
	}

	/*
	 * If the config is valid and at least one radio related attribute
	 * was changed, we update the cad interval and the timers
	 */
	if(ret == 0 && updateRadioAttr > 0) {
		/* Set preamble length from preamble time */
		_preambleLen = preamble_timems_to_symbols(preambleTime)+10;
		/*
		 * Adapt CAD interval to match actual preamble
		 * This double conversion is necessary because of the
		 * approximation used when converting time to symbols.
		 * Indeed a symbol duration does not always allow to match
		 * the expected preamble duration.
		 */
		_cad_interval = preamble_symbols_to_timems(_preambleLen-10);
		/* Update safeguard timers */
		update_safeguard_timers();
		/* Update CAD timer if the timer is currently running */
		if(_connected) {
			_sys->SYS_setRepetitiveTimer(_cad_interval);
		}
	}

	return ret;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * @name AT command execute functions
 * @{
 */
/**
 * @brief Set an attribute's value
 *
 * @param[in] p1 Key of the configuration variable to set
 * @param[in] p2 New value of the configuration variable
 * @param[out] err Error buffer
 * @retval 0 If the variable was set
 * @retval #LOWAPP_ERR_LOADCFG If the key was not found
 * @retval #LOWAPP_ERR_SETATTR If the variable could not be modified
 * @retval #LOWAPP_ERR_INVAL If a parameter was missing
 */
static int8_t cmd_set(const uint8_t* p1, const uint8_t* p2, uint8_t** err) {
	int8_t confRet;
	/* Back to pull mode */
	_opMode = PULL;
	if (p1!=NULL && p2!=NULL) {
		/* Check the value is valid */
		if(check_attribute(p1, p2)) {
			/* Set new value */
			if(_sys->SYS_setConfig(p1, p2) < 0) {
				*err=(uint8_t*)"Attribute could not be modified";
				return LOWAPP_ERR_SETATTR;
			}
			/* Reload full configuration as required */
			confRet = load_full_config();
			/*
			 * Display OK message even if at least one
			 * configuration value is not valid.
			 * Check for the set parameter is already done above
			 * in check_attribute so this means another attribute
			 * is not right.
			 * The only way for an attribute not to be right is
			 * for the persistent storage to be empty at startup,
			 * so no value is actually read at boot time. In this
			 * case, the device would be disconnected and the
			 * invalid configuration error message would be displayed
			 * when trying to connect.
			 */
			if(confRet == 0 || confRet == -2){
				uint8_t js[200] = "";
				/* Format message for answer to the UART */
				/* Size of the current string to add to the anwser */
				uint8_t sizeStr = 0;
				uint8_t offset = 0;
				/* Build the JSON message */
				sizeStr = strlen((char*)jsonPrefixOk);
				memcpy(js, jsonPrefixOk, sizeStr);
				offset += sizeStr;
				sizeStr = strlen((char*)p1);
				memcpy(js+offset, p1, sizeStr);
				offset += sizeStr;
				sizeStr = strlen((char*)jsonKeyValDelimiter);
				memcpy(js+offset, jsonKeyValDelimiter, sizeStr);
				offset += sizeStr;
				sizeStr = strlen((char*)p2);
				memcpy(js+offset, p2, sizeStr);
				offset += sizeStr;
				sizeStr = strlen((char*)jsonSuffix);
				memcpy(js+offset, jsonSuffix, sizeStr);
				offset += sizeStr;
				_sys->SYS_cmdResponse(js, offset);
				return 0;
			}
			/* At least one attribute was not found */
			else if(confRet == -1) {
				*err=(uint8_t*)"Attribute not found";
				return LOWAPP_ERR_LOADCFG;
			}
			else {
				*err=(uint8_t*)"Unexpected error";
				return LOWAPP_ERR_INVAL;
			}
		}
		else {
			*err=(uint8_t*)"Invalid attribute";
			return LOWAPP_ERR_SETATTR;
		}
	} else {
		/* Lacking params */
		*err=(uint8_t*)"missing params";
		return LOWAPP_ERR_INVAL;
	}
}

/**
 * @brief Get an attribute's value
 *
 * @param[in] p1 Key of the configuration variable needed
 * @param[out] err Error buffer
 * @retval 0 If the value was found. The value is sent back to the application
 * using the SYS_cmdResponse function
 * @retval LOWAPP_ERR_LOADCFG If the attribute was not found
 * @retval LOWAPP_ERR_INVAL If the p1 attribute is missing
 */
static int8_t cmd_get(const uint8_t* p1, uint8_t** err) {
	/* Back to pull mode */
	_opMode = PULL;
	if (p1!=NULL) {
		/* Buffer for sending JSON to the UART */
		uint8_t js[200] = "";
		/* Buffer to store the configuration value retrieved with getConfig */
		uint8_t value[100];
		/* Retrieve configuration value */
		if(_sys->SYS_getConfig(p1, value) >= 0) {
			/* Format message for answer to the UART */
			/* Size of the current string to add to the anwser */
			uint8_t sizeStr = 0;
			/* Pointer to move along the anwser buffer */
			uint8_t *ptrBuff = js;
			/* Build the JSON message */
			sizeStr = strlen((char*)jsonPrefixOk);
			memcpy(ptrBuff, jsonPrefixOk, sizeStr);
			ptrBuff += sizeStr;
			sizeStr = strlen((char*)p1);
			memcpy(ptrBuff, p1, sizeStr);
			ptrBuff += sizeStr;
			sizeStr = strlen((char*)jsonKeyValDelimiter);
			memcpy(ptrBuff, jsonKeyValDelimiter, sizeStr);
			ptrBuff += sizeStr;
			sizeStr = strlen((char*)value);
			memcpy(ptrBuff, value, sizeStr);
			ptrBuff += sizeStr;
			sizeStr = strlen((char*)jsonSuffix);
			memcpy(ptrBuff, jsonSuffix, sizeStr);
			ptrBuff += sizeStr;
			/* Send the value as a JSON string to the application */
			_sys->SYS_cmdResponse(js, ptrBuff-js);
			return 0;
		}
		else {
			*err=(uint8_t*)"Attribute not found";
			return LOWAPP_ERR_LOADCFG;
		}
	} else {
		*err=(uint8_t*)"missing param";
		return LOWAPP_ERR_INVAL;
	}
}


/**
 * @brief Write configuration into persistent memory
 *
 * @param[out] err Error buffer
 * @retval 0 If the configuration was saved in persistent memory
 * @retval #LOWAPP_ERR_PERSISTMEM If an error occurred during writing
 * @see #msgWriteConfig AT command string
 */
static int8_t cmd_writecfg(uint8_t** err) {
	/* Back to pull mode */
	_opMode = PULL;
	if(_sys->SYS_writeConfig() == 0) {
		_sys->SYS_cmdResponse((uint8_t*)"OK WRITECFG", 10);
		return 0;
	}
	else {
		*err=(uint8_t*)"Write configuration not working";
		return LOWAPP_ERR_PERSISTMEM;
	}
}

/**
 * @brief Read configuration from persistent memory
 * @param[out] err Error buffer
 * @retval 0 If the configuration was saved successfully into persistent memory
 * @retval #LOWAPP_ERR_LOADCFG If at least one configuration variable could not be found
 * @retval #LOWAPP_ERR_PERSISTMEM If the persistent memory could not be read
 */
static int8_t cmd_readcfg(uint8_t** err) {
	int confRet;
	/* Back to pull mode */
	_opMode = PULL;
	if(_sys->SYS_readConfig() == 0) {		// Retrieve config from persistent memory
		confRet = load_full_config();	// Get all config values from system
		if(confRet == 0){
			_sys->SYS_cmdResponse((uint8_t*)"OK READCFG", 10);
			return 0;
		}
		else if(confRet == -1) {
			*err=(uint8_t*)"Attribute not found";
			return LOWAPP_ERR_LOADCFG;
		}
		else if(confRet == -2) {
			*err=(uint8_t*)"Invalid attribute found";
			return LOWAPP_ERR_LOADCFG;
		}
	}
	else {
		*err=(uint8_t*)"Read configuration not working";
		return LOWAPP_ERR_PERSISTMEM;
	}
	return 0;
}

/**
 * @brief Display all configuration variables
 *
 * Get all configuration variables and format them for display.
 *
 * @param[out] err Error buffer
 * @retval 0
 * @see #msgDisplayConfig AT command string
 */
static int8_t cmd_displaycfg(uint8_t** err) {
	uint8_t value[256] = "";
	uint8_t tmpString[10] = "";
	uint8_t tmpSize = 0;
	/* Back to pull mode */
	_opMode = PULL;

	/* Copy default output string */
	memcpy(value, defaultDisplayString, sizeof(defaultDisplayString));
	/* Fill the string with values for answer to the UART */
	/* Build the JSON message */
	FillBufferHexBI8_t(value, 14, &_rchanId, 1, false);
	FillBufferHexBI8_t(value, 32, &_rsf, 1, false);
	FillBuffer8_t(value, 49, &_bandwidth, 1, false);
	FillBuffer8_t(value, 64, &_coderate, 1, false);
	/*
	 * Use temporary buffer to adapt the position of the first digit
	 * in the full output string.
	 */
	tmpSize = FillBuffer8_t(tmpString, 0, (uint8_t*)&_power, 1, false);
	memcpy(value+76+(2-tmpSize), tmpString, tmpSize);
	FillBufferHexBI8_t(value, 90, (uint8_t*)&_gwMask, 4, false);
	FillBufferHexBI8_t(value, 112, &_deviceId, 1, false);
	FillBufferHexBI8_t(value, 127, (uint8_t*)&_groupId, 2, false);
	/*
	 * Use temporary buffer to adapt the position of the first digit
	 * in the full output string.
	 */
	tmpSize = FillBuffer16_t(tmpString, 0, &preambleTime, 1, false);
	memcpy(value+142+(5-tmpSize), tmpString, tmpSize);
	_sys->SYS_cmdResponse(value, strlen((char*)value));

	return 0;
}

/**
 * @brief Start hardware selftest
 * @param[out] err Error buffer
 * @retval 0
 * @see #msgSelftest AT command string
 */
static int8_t cmd_selftest(uint8_t** err) {
	/* Back to pull mode */
	_opMode = PULL;
	// TODO
	_sys->SYS_cmdResponse((uint8_t*)"OK SELFTEST", 11);
	return 0;
}

/**
 * @brief Retrieve statistics
 * @param[out] err Error buffer
 * @retval 0
 * @see #msgStats AT command string
 */
static int8_t cmd_getstats(uint8_t** err) {
	/* Back to pull mode */
	_opMode = PULL;
	// TODO
	_sys->SYS_cmdResponse((uint8_t*)"OK GETSTATS", 11);
	return 0;
}

/**
 * @brief Get list of recently seen group members with their RSSIs
 * @param[out] err Error buffer
 * @retval 0
 * @see #msgWho AT command string
 */
static int8_t cmd_who(uint8_t** err) {
	/* Back to pull mode */
	_opMode = PULL;
	uint16_t sizeStr, offset = 0;
	uint8_t *buffer;
	/* Allocate buffer to display all 16 stat elements */
	buffer = calloc(18+statisticsWho.count*62, sizeof(uint8_t));
	sizeStr = strlen((char*)jsonWhoPrefix);
	memcpy(buffer, jsonWhoPrefix, sizeStr);
	offset += sizeStr;

	uint8_t i;
	/* Loop over all elements in the stat queue */
	for(i = 0; i < statisticsWho.count; i++) {
		/* Add device id */
		sizeStr = strlen((char*)jsonWhoDevice);
		memcpy(buffer+offset, jsonWhoDevice, sizeStr);
		offset += sizeStr;
		offset = FillBuffer8_t(buffer, offset, &(statisticsWho.els[i].deviceId), 1, false);
		/* Add last rssi */
		sizeStr = strlen((char*)jsonWhoLastRssi);
		memcpy(buffer+offset, jsonWhoLastRssi, sizeStr);
		offset += sizeStr;
		offset = FillBuffer8_t(buffer, offset, (uint8_t*)&(statisticsWho.els[i].lastRssi), 1, false);
		/* Add last seen */
		sizeStr = strlen((char*)jsonWhoLastSeen);
		memcpy(buffer+offset, jsonWhoLastSeen, sizeStr);
		offset += sizeStr;
		offset = FillBufferHexBI8_t(buffer, offset, (uint8_t*)(&(statisticsWho.els[i].lastSeen)),
				8, false);
		buffer[offset++] = '\"';
		buffer[offset++] = '}';
		buffer[offset++] = ',';
	}
	if(statisticsWho.count > 0) {
		offset--;
	}
	sizeStr = strlen((char*)jsonWhoSuffix);
	memcpy(buffer+offset, jsonWhoSuffix, sizeStr);
	offset += sizeStr;
	_sys->SYS_cmdResponse(buffer, offset);
	free(buffer);
	buffer = NULL;
	return 0;
}

/**
 * @brief Send a ping packet and wait for acknowledge
 *
 * @param[in] p1 Device id to which the ping should be sent
 * @param[out] err Error buffer
 * @retval 0 On Success
 * @retval #LOWAPP_ERR_INVAL If the p1 parameter was missing
 * @see #msgPing AT command string
 */
static int8_t cmd_ping(uint8_t* p1, uint8_t** err) {
	if (p1!=NULL) {
		uint8_t buffer[128] = "";
		MSG_T msgPing;
		MSG_T ackPing;
		uint8_t bufferLength = 0;
		uint8_t destination;
		uint8_t received;
		/* Back to pull mode */
		_opMode = PULL;
		/* New set of callbacks to pause the state machine */
		Lowapp_RadioEvents_t pingEvents;
		pingEvents.CadDone = NULL;
		pingEvents.RxDone = noSmRxDone;
		pingEvents.RxError = noSmRxError;
		pingEvents.RxTimeout = noSmRxTimeout;
		pingEvents.TxDone = noSmTxDone;
		pingEvents.TxTimeout = noSmRxTimeout;
		_sys->SYS_radioSetCallbacks(&pingEvents);
		/* Clear radio callbacks flags */
		radioFlags = 0;

		/* Retrieve destination id */
		AsciiHexConversionOneValueBI8_t(&destination, p1);
		/* Check the destination id */
		if(destination == 0x00) {
			/* Bring back standard radio callbacks */
			_sys->SYS_radioSetCallbacks(&radio_callbacks);
			*err=(uint8_t*)"Gateway functionality not implemented";
			return LOWAPP_ERR_NOTIMPL;
		}
		if(!(destination >= MIN_DEVICE_ID && destination <= MAX_DEVICE_ID)) {
			/* Bring back standard radio callbacks */
			_sys->SYS_radioSetCallbacks(&radio_callbacks);
			*err=(uint8_t*)"Invalid destination id";
			return LOWAPP_ERR_DESTID;
		}
		/* Build PING message */
		msgPing.hdr.type = TYPE_STDMSG;
		msgPing.hdr.version = LOWAPP_CURRENT_VERSION;
		msgPing.hdr.payloadLength = strlen((char*)pingPayload);
		msgPing.content.std.destId = destination;
		msgPing.content.std.srcId = _deviceId;
		msgPing.content.std.txSeq = 0;
		msgPing.content.std.txSeq = peers[msgPing.content.std.destId].out_txseq;

		memcpy(msgPing.content.std.payload, pingPayload, msgPing.hdr.payloadLength);
		/* Fill frame and set flag */
		bufferLength = buildFrame(buffer, &msgPing);
		/* Send the ping */
		_sys->SYS_cmdResponse((uint8_t*)"SEND PING", 9);
		_sys->SYS_radioTx(buffer, bufferLength);
		LOG(LOG_PARSER, "Trying to send (tryTx)");	/* Used by log parser */
		/* Wait for tx done */
		while(radioFlags == 0) { };
		if(radioFlags & RADIOFLAGS_TXDONE) {
			radioFlags = 0;
			/* Increment sequence number when tx done*/
			peers[destination].out_txseq =
				(peers[destination].out_txseq % 255) + 1;
			/* Set RX configuration for ACK */
			_sys->SYS_radioSetRxFixLen(true, ACK_FRAME_LENGTH);
			_sys->SYS_radioSetPreamble(PREAMBLE_ACK);
			_sys->SYS_radioSetRxContinuous(true);
			/* Wait for the ACK slot */
			_sys->SYS_delayMs(TIMER_ACK_SLOT_START);
			/* Start radio reception */
#ifdef SIMU
			/*
			 * Issues were noticed wit inotify using small timeouts of a few
			 * milliseconds on a standard Linux OS. This functions has been
			 * developed to counter these effects by taking care of the whole
			 * reception process at once.
			 */
			simu_radio_rxing_ack(timer_safeguard_rxing_ack);
#else
			/* Direclty start radio reception */
			_sys->SYS_radioRx(timer_safeguard_rxing_ack);
#endif

			while(radioFlags == 0) {}
			if(radioFlags & RADIOFLAGS_RXDONE) {
				radioFlags = 0;
				/* Build MSG_T from message frame */
				received = retrieveMessage(&ackPing, msgReceived.data);
				/* Check destination is this node */
				if (received == 0) {
					process_ack(&ackPing);
				}
				else {
					/* Fail if the message was not received correctly */
					_sys->SYS_cmdResponse((uint8_t*)"NOK TX", 6);
				}
			}
			else {
				/* Fail if no reception occurred */
				_sys->SYS_cmdResponse((uint8_t*)"NOK TX", 6);
			}
		}
		else {
			/* Fail if transmission failed */
			_sys->SYS_cmdResponse((uint8_t*)"NOK TX", 6);
		}

		/* Set RX configuration back to standard */
		_sys->SYS_radioSetRxFixLen(false, 0);
		_sys->SYS_radioSetPreamble(_preambleLen);
		_sys->SYS_radioSetRxContinuous(true);

		/* Bring back standard radio callbacks */
		_sys->SYS_radioSetCallbacks(&radio_callbacks);
		return 0;	// Ok
	} else {
		// lacking params
		*err=(uint8_t*)"missing param";
		return LOWAPP_ERR_INVAL;
	}
}

/**
 * @brief Send a hello packet
 * @param[out] err Error buffer
 * @retval 0
 * @see #msgHello AT command string
 */
static int8_t cmd_hello(uint8_t** err) {
	/* Back to pull mode */
	_opMode = PULL;
	// TODO
	_sys->SYS_cmdResponse((uint8_t*)"OK HELLO", 8);
	return 0;
}

/**
 * @brief Send data to a given device
 *
 * @param[in] p1 Device id of the receiver
 * @param[in] p2 Data to send
 * @param[out] err Error buffer
 * @retval 0 If the data was sent
 * @retval #LOWAPP_ERR_PAYLOAD If the payload was too big for transmission
 * @retval #LOWAPP_ERR_INVAL If a parameter was missing
 * @see #msgSend AT command string
 */
static int8_t cmd_send(uint8_t* p1, uint8_t* p2, uint8_t** err) {
	MSG_T* msg;
#ifdef LOWAPP_MSG_FORMAT_CLASSIC
	if (p1!=NULL && p2!=NULL) {
		int size = 0;
		uint8_t destination;
		/* Retrieve destination id */
		AsciiHexConversionOneValueBI8_t(&destination, p1);

		/* Do not allow send request when disconnected */
		if(!_connected) {
			*err=(uint8_t*)"NOK TX (DISCONNECTED)";
			return LOWAPP_ERR_DISCONNECT;
		}

		/* Check the destination id */
		if(destination == 0x00) {
			*err=(uint8_t*)"Gateway functionality not implemented";
			return LOWAPP_ERR_NOTIMPL;
		}
		if(!((destination >= MIN_DEVICE_ID && destination <= MAX_DEVICE_ID) ||
				destination == LOWAPP_ID_BROADCAST)) {
			*err=(uint8_t*)"Invalid destination id";
			return LOWAPP_ERR_DESTID;
		}

		/* Get size of data (loop until '\0') */
		while(size < MAX_PAYLOAD_STD_SIZE-1 && p2[size] != '\0') {
			size++;
		}
		/* Check payload size is valid */
		if(size == MAX_PAYLOAD_STD_SIZE-1 && p2[size] != '\0') {
			*err=(uint8_t*)"Payload too big for transmission";
			return LOWAPP_ERR_PAYLOAD;
		}

		/* Build a message with the corresponding destination and data */
		msg = (MSG_T*) malloc(sizeof(MSG_T));
		msg->hdr.type = TYPE_STDMSG;
		msg->hdr.version = LOWAPP_CURRENT_VERSION;
		msg->hdr.payloadLength = size;
		msg->content.std.destId = destination;
		msg->content.std.srcId = _deviceId;
		msg->content.std.txSeq = 0;	// #TODO Sequence number
		memcpy(msg->content.std.payload, p2, msg->hdr.payloadLength);
	} else {
		/* Missing params */
		*err=(uint8_t*)"missing params";
		return LOWAPP_ERR_INVAL;
	}
#elif (defined(LOWAPP_MSG_FORMAT_GPSAPP) || defined(LOWAPP_MSG_FORMAT_GPSAPP_RSSI))
	/*
	 * When using the GPS format message, send requests are sent in a special binary
	 * format :
	 * 	B0: 45
	 * 	B1 : Message type:
	 * 		01 : Unicast or broadcast message
	 * 	B2-B6 : GPS latitude
	 * 	B7-B10 : GPS longitude
	 * 	B11 : Destination id, 0xFF for broadcast
	 * 	B12 : Device id of the transmitter
	 * 	B13-BN : Text message to send
	 */

	/* Check the prefix of the GPS APP message */
	if(*p1 == 0x45 && *(p1+1) == 0x01) {
		uint8_t offset = 2;
		uint8_t offsetPayload = 0;

		int size = 0;
		/* Do not allow send request when disconnected */
		if(!_connected) {
			*err=(uint8_t*)"NOK TX (DISCONNECTED)";
			return LOWAPP_ERR_DISCONNECT;
		}

		/* Build a message with the corresponding destination and data */
		msg = (MSG_T*) malloc(sizeof(MSG_T));
		msg->hdr.type = TYPE_STDMSG;
		msg->hdr.version = LOWAPP_CURRENT_VERSION;
		offset = 2;
		/* Store lattitude and longitude at the beginning of the payload */
		memcpy(msg->content.std.payload+offsetPayload, p1+offset, 8);
		offset += 8;
		offsetPayload += 8;
		/* Retrieve destination ID from byte 10 */
		msg->content.std.destId = *(p1+offset);
		offset += 1;
		/* Skip the source id as the device already knows its id */
		offset += 1;
		/* Retrieve text message from end of the message */
		while(size+offsetPayload < MAX_PAYLOAD_STD_SIZE-1 &&
				*(p1+offset+size) != '\0') {
			size++;
		}
		msg->hdr.payloadLength = size+8;
		/* Check payload size is valid */
		if(size+offsetPayload == MAX_PAYLOAD_STD_SIZE-1 &&
				*(p1+offset+size) != '\0') {
			*err=(uint8_t*)"Payload too big for transmission";
			return LOWAPP_ERR_PAYLOAD;
		}
		msg->content.std.srcId = _deviceId;
		msg->content.std.txSeq = 0;	// #TODO Sequence number
		memcpy(msg->content.std.payload+offsetPayload, p1+offset,
				size);
	}
	else {
		*err=(uint8_t*)"Invalid payload for GPS format message";
		return LOWAPP_ERR_PAYLOAD;
	}
#endif
	LOG(LOG_STATES, "Add event TXREQ to cold event queue");

	/* Add message to tx queue */
	if (lowapp_tx(msg) == -1) {
		/* Message is lost */
		LOG(LOG_ERR, "TX queue was full");
		_sys->SYS_cmdResponse((uint8_t*)"NOK TX (QUEUE FULL)", 19);
	}
	else {
		/* We are blocked and therefore cannot send right now */
		if(txBlocked) {
			LOG(LOG_INFO, "Delaying TX");
			_sys->SYS_cmdResponse((uint8_t*)"SEND DELAYED", 12);
		}
		else {
			/*
			 * If there is already an element in the queue, the sending of the packet
			 * will be delayed.
			 */
			if(queue_size(&_tx_pkt_list) > 1) {
				LOG(LOG_INFO, "Delaying TX");
				_sys->SYS_cmdResponse((uint8_t*)"SEND DELAYED", 12);
			}
			else {
				_sys->SYS_cmdResponse((uint8_t*)"SEND REQUEST", 12);
			}
			/* Notify the state machine that a message is waiting to be processed */
			lock_coldEventQ();
			add_simple_event(&_coldEventQ, TXREQ);
			unlock_coldEventQ();
		}
	}
	return 0;
}


/**
 * @brief Check for received packets.
 *
 * Returns all received packets since last check as a JSON formatted string through SYS_cmdResponse
 *
 * @param[out] err Error buffer
 * @retval 0
 * @see #msgPollRx AT command string
 */
static int8_t cmd_pollrx(uint8_t** err) {
	/* Back to pull mode */
	_opMode = PULL;
	/*
	 * Retrieve RX messages
	 * No need to protect resource because only the state machine thread is
	 * touching the _rx_pkt_list
	 */
	response_rx_packets();
	return 0;
}

/**
 * @brief Change to push mode
 *
 * @param[out] err Error buffer
 * @retval 0
 * @see #msgPushRx AT command string
 */
static int8_t cmd_pushrx(uint8_t** err) {
	/* Move to push mode */
	_opMode = PUSH;
	_sys->SYS_cmdResponse((uint8_t*)"OK PUSHRX", 9);
	return 0;
}

/**
 * @brief Stop listening for packets and reject transmissions
 * @param[out] err Error buffer
 * @retval 0
 * @see #msgDisconnect AT command string
 */
static int8_t cmd_disconnect(uint8_t** err) {
	/* Back to pull mode */
	_opMode = PULL;
	if(_connected) {
		_connected = false;
		/* Cancel CAD launcher timer (stop listening) */
		_sys->SYS_cancelRepetitiveTimer();
	}
	_sys->SYS_cmdResponse((uint8_t*)"OK DISCONNECT", 13);
	return 0;
}

/**
 * @brief Start listening for packets and allow transmissions
 * @param[out] err Error buffer
 * @retval 0
 * @see #msgConnect AT command string
 */
static int8_t cmd_connect(uint8_t** err) {
	/* Back to pull mode */
	_opMode = PULL;
	if(!_connected) {
		if(check_configuration()) {
			_connected = true;
			/* Initialise CAD launcher */
			_sys->SYS_setRepetitiveTimer(_cad_interval);
		}
		else {
			*err=(uint8_t*)"Invalid configuration";
			return LOWAPP_ERR_INVAL;	/* #TODO */
		}
	}
	_sys->SYS_cmdResponse((uint8_t*)"OK CONNECT", 10);
	return 0;
}

/**
 * @brief Reset simulated device
 * @param[out] err Error buffer
 * @retval 0
 * @see #msgReset AT command string
 */
static int8_t cmd_reset(uint8_t** err) {
	_sys->SYS_cmdResponse((uint8_t*)"OK RESET", 8);
#ifndef SIMU
	HAL_NVIC_SystemReset();
#endif
	return 0;
}
/** @} */

#pragma GCC diagnostic pop

/**
 * @name AT command processing functions
 * @{
 */

/**
 * @brief Process an AT command
 *
 * Parse the AT command and interpret it.
 *
 * @param cmdrequest AT command to process
 * @return The return value of the corresponding AT command function
 * @retval 0 If the command was an empty string
 * @retval #LOWAPP_ERR_INVAL If the command was not recognised
 */
static int8_t at_cmd_process(uint8_t* cmdrequest) {
	uint8_t* cmd=NULL;
	uint8_t* p1=NULL;
	uint8_t* p2=NULL;
	uint8_t* err=NULL;
	if (cmdrequest==NULL) {
		uint8_t jerr[200] = "";
		/* Format message for answer to the UART */
		/* Size of the current string to add to the anwser */
		uint8_t sizeStr = 0;
		/* Pointer to move along the anwser buffer */
		uint8_t *ptrBuff = jerr;
		/* Build the JSON message */
		sizeStr = strlen((char*)jsonPrefixError);
		memcpy(ptrBuff, jsonPrefixError, sizeStr);
		ptrBuff += sizeStr;
		uint8_t errorCode = LOWAPP_ERR_ATSIZE;
		ptrBuff += FillBuffer8_t(ptrBuff, 0, &errorCode, 1, false);
		sizeStr = strlen((char*)jsonDelimiterErrorCodeString);
		memcpy(ptrBuff, jsonDelimiterErrorCodeString, sizeStr);
		ptrBuff += sizeStr;
		sizeStr = strlen((char*)errorMsgAtCmdInvalidSize);
		memcpy(ptrBuff, errorMsgAtCmdInvalidSize, sizeStr);
		ptrBuff += sizeStr;
		sizeStr = strlen((char*)jsonSuffix);
		memcpy(ptrBuff, jsonSuffix, sizeStr);
		ptrBuff += sizeStr;

		_sys->SYS_cmdResponse(jerr, ptrBuff-jerr);
		return LOWAPP_ERR_INVAL;
	}
	int8_t pret = parseATCmd(cmdrequest, &cmd, &p1, &p2, &err);
	if (pret==0) {
		/* Just blank line */
		return 0;
	}
	if (pret>0) {
		/* Interpret cmd */
		pret = at_cmd_interp(cmd, p1, p2, &err);
	}
	/* Cmd error, we return error response for everyone */
	if (pret<0) {
		uint8_t jerr[200] = "";
		/* Format message for answer to the UART */
		/* Size of the current string to add to the anwser */
		uint8_t sizeStr = 0;
		/* Offset for printing */
		uint8_t offset = 0;
		/* Build the JSON message */
		sizeStr = strlen((char*)jsonPrefixError);
		memcpy(jerr+offset, jsonPrefixError, sizeStr);
		offset += sizeStr;
		offset = FillBuffer8_t(jerr, offset, (uint8_t*)&pret, 1, false);
		sizeStr = strlen((char*)jsonDelimiterErrorCodeString);
		memcpy(jerr+offset, jsonDelimiterErrorCodeString, sizeStr);
		offset += sizeStr;
		sizeStr = strlen((char*)err);
		memcpy(jerr+offset, err, sizeStr);
		offset += sizeStr;
		sizeStr = strlen((char*)jsonSuffix);
		memcpy(jerr+offset, jsonSuffix, sizeStr);
		offset += sizeStr;
		_sys->SYS_cmdResponse(jerr, offset);
	}
	return pret;
}

/**
 * @brief Interpret AT command
 *
 * Compare the AT command with a set of string literals containing
 * valid commands, select the processing function corresponding to to the
 * AT command and execute it.
 *
 * @param[in] cmd AT command
 * @param[in] p1 First parameter of the AT command (after '=')
 * @param[in] p2 Second parameter of the AT command (after '=')
 * @param[out] err Error buffer
 * @return The return value of the corresponding AT command function
 * @retval #LOWAPP_ERR_INVAL If the command was not recognised
 */
static int8_t at_cmd_interp(uint8_t* cmd, uint8_t* p1, uint8_t* p2, uint8_t** err) {
	char* cmdChar = (char*) cmd;
	/* If the command is an AT write config */
	if (strcmp((char*)msgWriteConfig, cmdChar)==0)  {
		/* Execute write config function */
		return cmd_writecfg(err);
	}
	/* If the command is an AT display config */
	else if (strcmp((char*)msgDisplayConfig, cmdChar)==0)  {
		/* Execute display config function */
		return cmd_displaycfg(err);
	}
	/* If the command is an AT command related to the gateway type */
	else if (strcmp((char*)msgGwMask, cmdChar)==0)  {
		/*
		 * If a parameter was sent, we set the config variable.
		 * If no parameter was sent, we get the value of the config variable.
		 */
		if(p1 == NULL) {
			return cmd_get(strGwMask, err);
		}
		else {
			return cmd_set(strGwMask, p1, err);
		}
	}
	/* If the command is an AT command related to the device id */
	else if (strcmp((char*)msgDeviceId, cmdChar)==0)  {
		/*
		 * If a parameter was sent, we set the config variable.
		 * If no parameter was sent, we get the value of the config variable.
		 */
		if(p1 == NULL) {
			return cmd_get(strDeviceId, err);
		}
		else {
			return cmd_set(strDeviceId, p1, err);
		}
	}
	/* If the command is an AT command related to the group id */
	else if (strcmp((char*)msgGroupId, cmdChar)==0)  {
		/*
		 * If a parameter was sent, we set the config variable.
		 * If no parameter was sent, we get the value of the config variable.
		 */
		if(p1 == NULL) {
			return cmd_get(strGroupId, err);
		}
		else {
			return cmd_set(strGroupId, p1, err);
		}
	}
	/* If the command is an AT command related to the encryption key */
	else if (strcmp((char*)msgEncKey, cmdChar)==0)  {
		/*
		 * If a parameter was sent, we set the config variable.
		 * If no parameter was sent, we set an error message as encryption
		 * key should never be displayed over AT commands
		 */
		if(p1 == NULL) {
			*err = (uint8_t*)"ENCKEY cannot be displayed";
			return LOWAPP_ERR_INVAL;
		}
		else {
			return cmd_set(strEncKey, p1, err);
		}
	}
	/* If the command is an AT command related to the radio channel */
	else if (strcmp((char*)msgChanId, cmdChar)==0)  {
		/*
		 * If a parameter was sent, we set the config variable.
		 * If no parameter was sent, we get the value of the config variable.
		 */
		if(p1 == NULL) {
			return cmd_get(strRchanId, err);
		}
		else {
			return cmd_set(strRchanId, p1, err);
		}
	}
	/* If the command is an AT command related to the radio spreading factor */
	else if (strcmp((char*)msgSf, cmdChar)==0)  {
		/*
		 * If a parameter was sent, we set the config variable.
		 * If no parameter was sent, we get the value of the config variable.
		 */
		if(p1 == NULL) {
			return cmd_get(strRsf, err);
		}
		else {
			return cmd_set(strRsf, p1, err);
		}
	}
	/* If the command is an AT command related to the preamble time */
	else if (strcmp((char*)msgPreambleTime, cmdChar)==0)  {
		/*
		 * If a parameter was sent, we set the config variable.
		 * If no parameter was sent, we get the value of the config variable.
		 */
		if(p1 == NULL) {
			return cmd_get(strPreambleTime, err);
		}
		else {
			return cmd_set(strPreambleTime, p1, err);
		}
	}
	/* If the command is a hardware selftest AT command */
	else if (strcmp((char*)msgSelftest,cmdChar)==0)  {
		return cmd_selftest(err);
	}
	/* If the command is a statistics AT command */
	else if (strcmp((char*)msgStats,cmdChar)==0)  {
		return cmd_getstats(err);
	}
	/* If the command is a WHO AT command */
	else if (strcmp((char*)msgWho,cmdChar)==0)  {
		return cmd_who(err);
	}
	/* If the command is a PING AT command */
	else if (strcmp((char*)msgPing,cmdChar)==0)  {
		return cmd_ping(p1,err);
	}
	/* If the command is a HELLO AT command */
	else if (strcmp((char*)msgHello,cmdChar)==0)  {
		return cmd_hello(err);
	}
	/* If the command is a send AT command */
	else if (strcmp((char*)msgSend,cmdChar)==0)  {
		return cmd_send(p1, p2, err);
	}
	/* If the command is a POLLRX AT command */
	else if (strcmp((char*)msgPollRx,cmdChar)==0)  {
		return cmd_pollrx(err);
	}
	/* If the command is a PUSHRX AT command */
	else if (strcmp((char*)msgPushRx,cmdChar)==0)  {
		return cmd_pushrx(err);
	}
	/* If the command is a disconnection AT command */
	else if (strcmp((char*)msgDisconnect,cmdChar)==0)  {
		return cmd_disconnect(err);
	}
	/* If the command is a connection AT command */
	else if (strcmp((char*)msgConnect,cmdChar)==0)  {
		return cmd_connect(err);
	}
	/* If the command is a reset AT command */
	else if (strcmp((char*)msgReset,cmdChar)==0)  {
		return cmd_reset(err);
	}
#ifdef SIMU
	/* If the command is a set log AT command */
	else if(strcmp((char*)msgLog, cmdChar)==0) {
		int log = atoi((char*)p1);
		set_log_level(log);
		_sys->SYS_cmdResponse((uint8_t*)"OK LOG", 6);
		return log;
	}
#endif
	else {
		*err=(uint8_t*)"unknown command";
		return LOWAPP_ERR_INVAL;
	}
}

/**
 * @brief Process next AT command from the queue
 */
void at_queue_process() {
	uint8_t* cmd;
	uint16_t len;
	int8_t getFromQ = 0;
	lock_atcmd();
	getFromQ = get_from_queue(&_atcmd_list, (void**)&cmd, &len);
	unlock_atcmd();
	/* Loop while an AT command is in the queue */
	while(getFromQ != -1) {
		at_cmd_process(cmd);
		/* Free memory */
		free(cmd);
		cmd = NULL;

		lock_atcmd();
		getFromQ = get_from_queue(&_atcmd_list, (void**)&cmd, &len);
		unlock_atcmd();
	}
}

/**
 * @brief Move along a buffer to pass through whitespaces until non-whitespace or
 * end of string character is detected.
 * @param lp Pointer to the buffer storing the string to parse. This pointer
 * is modified by the function.
 * @retval true If EOS is reached, false otherwise
 */
static bool eat_ws(uint8_t** lp) {
	while(isspace(**lp) || **lp=='\r' || **lp=='\n') {
		(*lp)++;
	}
	if (**lp=='\0') {
		return true;
	}
	return false;
}

/**
 * @brief Parse an AT command and move the double pointer parameters to the corresponding
 * place in the line buffer.
 *
 * AT command requests are of format \<atcmd\>{=\<param1\>{,\<param2\>}}<br/>
 * White space is permitted in front of the atcmd, around the '=' and around the ','.
 *
 * Parse along the 'line' buffer, divide into segments at the '=' and ',' points. Each segment is
 * pointed to by the double pointers.
 *
 * @param line Line to parse
 * @param cmd Pointer to the command part of the buffer. The pointer is moved to the
 * right place by the function.
 * @param param1 Pointer to the first parameter of the command. The pointer is moved to the
 * right place by the function (if any).
 * @param param2 Pointer to the second parameter of the command. The pointer is moved to the
 * right place by the function (if any).
 * @param errstr Error buffer
 *
 * @retval 0 If no command has been found
 * @retval 1 If a command has been found without any parameters
 * @retval 2 If a command with one parameter has been found
 * @retval 3 If a command with two parameters has been found
 * @retval -2 If a whitespace has been found in the command
 * @retval -3 If an equal sign with no parameter has been found
 * @retval -4 If a whitespace has been found inside a parameter
 */
static int8_t parseATCmd(uint8_t* line, uint8_t** cmd, uint8_t** param1, uint8_t**param2, uint8_t** errstr) {
	uint8_t* lp = line;
	*cmd = NULL;
	*param1 = NULL;
	*param2 = NULL;

	/* Error buffer filled with no error message by default */
	*errstr=(uint8_t*)"no error";
	if (eat_ws(&lp)) {
		/* Blank line */
		return 0;
	}
	*cmd=lp;
	while(*lp!='=' && isspace(*lp)==false) {
		if (*lp=='\0') {
			/* Command only has been found */
			return 1;
		}
		/* Command part to upper case */
		*lp=toupper(*lp);
		lp++;
	}
	if (eat_ws(&lp)) {
		/* Command only has been found */
		return 1;
	}
	if (*lp!='=') {
		/* Whitespaces in command are not allowed */
		*errstr=(uint8_t*)"WS in cmd name";
		return -2;
	}
	/* Terminate command bit (overwrite the '=') */
	*lp='\0';
	/* Move after the '=' */
	lp++;

#if (defined(LOWAPP_MSG_FORMAT_GPSAPP) || defined(LOWAPP_MSG_FORMAT_GPSAPP_RSSI))
	/*
	 * Special case for GPSAPP message format
	 * We should not look for , because the following bytes
	 * are binary and not ASCII (anything could match the ','
	 * character with not actual meaning.
	 */
	if (strcmp((char*)msgSend,(char*)(*cmd))==0)  {
		*param1=lp;
		return 2;
	}
#endif

	if (eat_ws(&lp)) {
		/* Command with '=' found but no value */
		*errstr=(uint8_t*)"= but no value following";
		return -3;
	}
	*param1=lp;
	/* Check for "," and next param */
	while(*lp!=',' && isspace(*lp)==false) {
		if (*lp=='\0') {
			/* Command with one parameter found */
			return 2;
		}
		lp++;
	}
	if (eat_ws(&lp)) {
		return 2;	/* Command with one parameter found */
	}
	if (*lp!=',') {
		/* Whitespaces in params not allowed */
		*errstr=(uint8_t*)"Whitespace found in parameters";
		return -4;
	}
	/* Terminate param1 bit (overwrite the ',') */
	*lp='\0';
	/* Skip ',' */
	lp++;
	if (eat_ws(&lp)) {
		*errstr=(uint8_t*)", was found but no second parameter";
		return -5;
	}
	/* Up to the end of the line string */
	*param2 = lp;
	/* Command with 2 parameters found */
	return 3;
}

/** @} */

/** @} */
/** @} */

