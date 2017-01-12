/**
 * @file lowapp_msg.c
 * @brief LoWAPP Message and frame related functions
 *
 * Defines functions used to build and parse frame from message structures.
 *
 * @author Nathan Olff
 * @date August 22, 2016
 */
#include "lowapp_inc.h"
#include "lowapp_utils_crc.h"
#include "utilities.h"
#include <math.h>

extern QFIXED_T _rx_pkt_list;
extern const uint32_t bandwidthValues[];

/* Static function prototypes */
static void encodeInPlace(uint8_t _appKey[], uint16_t nonce,
		uint8_t* startEncode, uint16_t sizeToEncode);
static void decodeInPlace(uint8_t _appKey[], uint16_t nonce,
		uint8_t* encBuf, uint16_t sizeToEncode);
static uint16_t buildJson(uint8_t **frameBuffer, MSG_RX_APP_T *msg_rx);


/**
 * @name LoWAPP JSON response string literals
 * @brief Strings used to return received messages to the application
 * @{
 */
/** Source id string for the json field */
const uint8_t jsonSrcId[] = "\"srcId\":";
/** Destination id string for json field */
const uint8_t jsonDestId[] = "\"destId\":";
/** Duplicate flag string for json field */
const uint8_t jsonDuplicate[] = "\"duplicateFrames\":";
/** Missing frame string for json field */
const uint8_t jsonMissingFrame[] = "\"missingFrames\":";
/** RSSI string for json field */
const uint8_t jsonRssi[] = "\"rssi\":";
/** Payload string for json field */
const uint8_t jsonPayload[] = "\"payload\":\"";
/** Hex prefix string for json field */
const uint8_t jsonHexPrefix[] = "0x";
/** End of string characters for json field */
const uint8_t jsonEndPayload[] = "\"}";

/** Missing Ack string for json field */
const uint8_t jsonMissingAck[] = "OK TX {\"missingAck\":";

/** Prefix for OK json response */
const uint8_t jsonPrefixOk[] = "OK {\"";
/** Prefix for OK TX json response */
const uint8_t jsonPrefixOkTx[] = "OK TX {\"";

/** Suffix for json object */
const uint8_t jsonSuffix[] = "\"}";
/** Delimiter for json string field */
const uint8_t jsonKeyValDelimiter[] = "\":\"";
/** Delimiter between json elements */
const uint8_t jsonFieldDelimiter[] = "\",\"";

/** Prefix for NOK error json */
const uint8_t jsonPrefixError[] = "NOK {\"errno\":\"";
/** Delimiter between error code and error string for json */
const uint8_t jsonDelimiterErrorCodeString[] = "\", \"errstr\":\"";

/** Invalid configuration error message */
const uint8_t errorMsgMissingConfiguration[] = "\", \"errstr\":\"Missing or incomplete configuration\"}";
/** AT command invalid size error message */
const uint8_t errorMsgAtCmdInvalidSize[] = "AT COMMAND TOO LONG";

/** Prefix for NOK TX retry json */
const uint8_t jsonPrefixNokTxRetry[] = "NOK TX {\"retry\":\"";
/** Max retry reached error message */
const uint8_t jsonErrorMaxRetry[] = "NOK TX {\"retry\":\"MAX\"}";
/** TX Faile error message json */
const uint8_t jsonErrorTxFail[] = "NOK TX {\"status\":\"FAILED\"}";

/** NOK TX json response */
const uint8_t jsonNokTx[] = "NOK TX";
/** NOK for RXERROR json response */
const uint8_t jsonNokTxRxError[] = "NOK TX {\"status\":\"RXERROR\"}";
/** NOK for RXTIMEOUT json response */
const uint8_t jsonNokTxRxTimeout[] = "NOK TX {\"status\":\"RXTIMEOUT\"}";

/** Statistics for AT+WHO command prefix for json response */
const uint8_t jsonWhoPrefix[] = "OK {\"wholist\":[";
/** Device id for WHO statistics json response */
const uint8_t jsonWhoDevice[] = "{\"deviceId\":";
/** Last RSSI for WHO statistics json response */
const uint8_t jsonWhoLastRssi[] = ",\"lastRssi\":";
/** Last seen time for WHO statistics json response */
const uint8_t jsonWhoLastSeen[] = ",\"lastSeen\":\"";
/** Suffix for WHO statistics json response */
const uint8_t jsonWhoSuffix[] = "]}";


/** @} */

/**
 * Generate a 2-bytes random number for encryption
 * @return A random number
 */
uint16_t makeNonce() {
	return (uint16_t)randr(0, 65535);
}

/**
 * @addtogroup lowapp_core LoWAPP Core
 * @{
 */
/**
 * @addtogroup lowapp_core_messages LoWAPP Core Messages
 * @brief Message structures and processing functions
 * @{
 */

/**
 * Encode the message directly inside the buffer
 * @param _appKey Encryption key
 * @param randomValueForNonce Nonce used for encryption
 * @param startEncode Pointer to the start of the buffer to encode
 * @param sizeToEncode Size of the data to encode
 */
static void encodeInPlace(uint8_t _appKey[], uint16_t randomValueForNonce, uint8_t* startEncode, uint16_t sizeToEncode) {
	/* Temporary encryption buffer */
	uint8_t encryptedBuffer[256] = {0};
	/* Actual key used for AES encryption (currently on 128 bits) */
	uint8_t actualKey[ENCKEY_SIZE] = {0};
	uint32_t actualNonce;
	/* Set nonce as concatenation of groupId and randomValue from the frame */
	actualNonce = (_groupId << 16) | randomValueForNonce;
	/* Compute actual key using both _encryptionKey and nonce */
	uint8_t i;
	for(i = 0; i < ENCKEY_SIZE; ++i) {
		/* XOR between _encryptionKey and the actualNonce variable */
		actualKey[i] |= _encryptionKey[i] ^ ((actualNonce >> (4*(i/4))) & 0xFF);
	}
	/* Encode into a buffer */
	LoRaMacPayloadEncrypt(startEncode, sizeToEncode, _appKey, 0, 0, 0, encryptedBuffer);
	memcpy(startEncode, encryptedBuffer, sizeToEncode);
}

/**
 * Decode the message directly inside the buffer
 * @param _appKey Encryption key
 * @param randomValueForNonce Nonce used for encryption
 * @param encBuf Pointer to the start of the buffer to decode
 * @param sizeToEncode Size of the data to decode
 */
static void decodeInPlace(uint8_t _appKey[], uint16_t randomValueForNonce, uint8_t* encBuf, uint16_t sizeToEncode) {
	uint8_t decBuffer[256] = {0};
	/* Actual key used for AES encryption (currently on 128 bits) */
	uint8_t actualKey[ENCKEY_SIZE] = {0};
	uint32_t actualNonce;
	/* Set nonce as concatenation of groupId and randomValue from the frame */
	actualNonce = (_groupId << 16) | randomValueForNonce;
	/* Compute actual key using both _encryptionKey and nonce */
	uint8_t i;
	for(i = 0; i < ENCKEY_SIZE; ++i) {
		/* XOR between a pair of bytes from the 32-bytes _encryptionKey and the actualNonce variable */
		actualKey[i] |= _encryptionKey[i] ^ ((actualNonce >> (4*(i/4))) & 0xFF);
	}
	/* Decode into a buffer */
	LoRaMacPayloadDecrypt(encBuf, sizeToEncode, _appKey, 0, 0, 0, decBuffer);
	memcpy(encBuf, decBuffer, sizeToEncode);
}

/**
 * Get the size of the frame in bytes
 * @param msg Message to transmit
 * @return Number of bytes of the frame
 */
uint8_t frameSize(MSG_T *msg) {
	uint16_t packetSize;
	/* Check frame type */
	switch(msg->hdr.type) {
	case TYPE_STDMSG:
		packetSize = sizeof(LORA_HDR_T)
				+ 2	// Nonce
				+ 3	// Standard type
				+ msg->hdr.payloadLength
				+ 2;	// CRC
		break;
	case TYPE_ACK:
		packetSize = sizeof(LORA_HDR_T)
				+ 2	// Nonce
				+ sizeof(ACKMSG_T)
				+ 2; // CRC
		break;
	default:
		packetSize = 0;
		break;
	}
	return packetSize;
}

/**
 * Build frame from the message structure
 * @param[out] frameBuffer Output frame buffer
 * @param[in] msg Message to transform into frame
 * @return The total size of the frame
 */
uint16_t buildFrame(uint8_t *frameBuffer, MSG_T *msg) {
	uint8_t* ptrBuf;
	uint16_t packetSize;
	uint16_t crc;
	/* Check frame type */
	switch(msg->hdr.type) {
	case TYPE_STDMSG:
		ptrBuf = frameBuffer;
		*ptrBuf = (msg->hdr.version << 4) | (msg->hdr.type);
		ptrBuf++;
		wrap_byte(&ptrBuf, msg->hdr.payloadLength);
		wrap_short(&ptrBuf, msg->hdr.rfu);
		wrap_short(&ptrBuf, makeNonce());
		wrap_byte(&ptrBuf, msg->content.std.destId);
		wrap_byte(&ptrBuf, msg->content.std.srcId);
		wrap_byte(&ptrBuf, msg->content.std.txSeq);
		memcpy(ptrBuf, msg->content.std.payload, msg->hdr.payloadLength);
		ptrBuf += msg->hdr.payloadLength;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
		/* Compute CRC on full frame and add it to the end */
		crc = PacketComputeCrc(frameBuffer, ptrBuf-frameBuffer, POLYNOMIAL_IBM);
#pragma GCC diagnostic pop
		wrap_short(&ptrBuf, crc);

		/* Encode */
		encodeInPlace(_encryptionKey, *((uint16_t*)(frameBuffer+4)), frameBuffer+6, msg->hdr.payloadLength+8);

		return ptrBuf-frameBuffer;
	case TYPE_ACK:
		packetSize = 0;
		packetSize += sizeof(LORA_HDR_T);
		packetSize += 2;	// Nonce
		packetSize += sizeof(ACKMSG_T);
		packetSize += 2;	// CRC

		ptrBuf = frameBuffer;
		*ptrBuf = (msg->hdr.version << 4) | (msg->hdr.type);
		ptrBuf++;
		wrap_byte(&ptrBuf, msg->hdr.payloadLength);
		wrap_short(&ptrBuf, msg->hdr.rfu);
		wrap_short(&ptrBuf, makeNonce());
		wrap_byte(&ptrBuf, msg->content.ack.destId);
		wrap_byte(&ptrBuf, msg->content.ack.srcId);
		wrap_byte(&ptrBuf, msg->content.ack.rxdSeq);
		wrap_byte(&ptrBuf, msg->content.ack.expectedSeq);

		/* Compute CRC on full frame and add it to the end */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
		crc = PacketComputeCrc(frameBuffer, ptrBuf-frameBuffer, POLYNOMIAL_IBM);
#pragma GCC diagnostic pop
		LOG(LOG_DBG, "CRC ACK = %u", crc);
		wrap_short(&ptrBuf, crc);

		/* Encode */
		encodeInPlace(_encryptionKey, *((uint16_t*)(frameBuffer+4)), frameBuffer+6, 6);

		return ptrBuf-frameBuffer;
	default:
		return 0;
	}
}


/**
 * Retrieve a message structure from a frame
 * @param[out] msg Message to be filled from the buffer
 * @param[in] frameBuffer Input frame buffer
 * @retval 0 If the message was filled successfully
 * @retval -1 If the message type was unknown
 * @retval -2 If the packet was not destined to me
 * @retval -3 If the CRC check failed
 * @retval -4 If the version of the protocol is not the current version
 */
int8_t retrieveMessage(MSG_T *msg, uint8_t *frameBuffer) {
	uint8_t* ptrBuf = frameBuffer;
	uint16_t nonce;
	uint16_t crcComputed, crcRetrieved;

	/* Copy header from the frame buffer */
	msg->hdr.version = *ptrBuf >> 4;
	msg->hdr.type = *ptrBuf & 0xF;
	ptrBuf++;
	msg->hdr.payloadLength = parse_byte(&ptrBuf);
	msg->hdr.rfu = parse_short(&ptrBuf);
	nonce = parse_short(&ptrBuf);

	/* Check protocol version */
	if(msg->hdr.version != LOWAPP_CURRENT_VERSION) {
		return -4;
	}

	/* Check message type from header */
	switch(msg->hdr.type) {
	case TYPE_STDMSG:
		/* Decode message */
		decodeInPlace(_encryptionKey, nonce, ptrBuf, msg->hdr.payloadLength+8);
		/* Copy message content */
		msg->content.std.destId = parse_byte(&ptrBuf);
		msg->content.std.srcId = parse_byte(&ptrBuf);

		/* Check CRC */
		crcRetrieved = get_short(ptrBuf+1+msg->hdr.payloadLength);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
		crcComputed = PacketComputeCrc(frameBuffer, ptrBuf+1+msg->hdr.payloadLength-frameBuffer, POLYNOMIAL_IBM);
#pragma GCC diagnostic pop
		if(crcComputed != crcRetrieved) {
			/* Wrong AES key assumed */
			return -3;
		}

		/* Check destination */
		if(msg->content.std.destId == _deviceId || msg->content.std.destId == LOWAPP_ID_BROADCAST) {
			msg->content.std.txSeq = parse_byte(&ptrBuf);
			memcpy(msg->content.std.payload, ptrBuf, msg->hdr.payloadLength);
			ptrBuf += msg->hdr.payloadLength;
			return 0;
		}
		else {
			return -2;
		}
	case TYPE_ACK:
		/* Decode message */
		decodeInPlace(_encryptionKey, nonce, ptrBuf, 6);
		/* Copy message content */
		msg->content.ack.destId = parse_byte(&ptrBuf);
		msg->content.ack.srcId = parse_byte(&ptrBuf);

		/* Check CRC */
		crcRetrieved = get_short(ptrBuf+2);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
		crcComputed = PacketComputeCrc(frameBuffer, ptrBuf+2-frameBuffer, POLYNOMIAL_IBM);
#pragma GCC diagnostic pop
		if(crcComputed != crcRetrieved) {
			/* Wrong AES key assumed */
			return -3;
		}

		/* Check destination */
		if(msg->content.ack.destId == _deviceId) {
			msg->content.ack.rxdSeq = parse_byte(&ptrBuf);
			msg->content.ack.expectedSeq = parse_byte(&ptrBuf);
			return 0;
		}
		else {
			return -2;
		}
	default:
		return -1;
	}
}

/**
 * Build JSON object from the message structure
 * @param[out] frameBuffer Output JSON buffer
 * @param[in] msg_rx Message to transform into JSON
 * @return The total size of the JSON buffer
 */
static uint16_t buildJson(uint8_t **frameBuffer, MSG_RX_APP_T *msg_rx) {
	uint8_t* ptrBuf;
	uint16_t bufferSize;
	MSG_T *msg = msg_rx->msg;
	/* Check frame type */
	switch(msg->hdr.type) {
	case TYPE_STDMSG:
		bufferSize = 2+
					+sizeof(jsonSrcId)-1+4
					+sizeof(jsonDestId)-1+4
					+sizeof(jsonRssi)-1+3	/* Add memory for rssi value (3 characters) */
					+sizeof(jsonDuplicate)-1+4	/* Add memory for duplicate even though we might not need it */
					+sizeof(jsonMissingFrame)-1+4	/* Add memory for missing frames even though we might not need it */
					+sizeof(jsonPayload)-1+msg->hdr.payloadLength+sizeof(jsonEndPayload)-1;
		*frameBuffer = (uint8_t*) malloc(bufferSize*sizeof(uint8_t));
		ptrBuf = *frameBuffer;
		if(*frameBuffer == NULL) {
			LOG(LOG_ERR, "Memory allocation error");
			return 0;
		}
		memset(ptrBuf, 0, bufferSize*sizeof(uint8_t));
		*ptrBuf = '{';
		ptrBuf++;
		/* Add source id */
		memcpy(ptrBuf, jsonSrcId, sizeof(jsonSrcId)-1);
		ptrBuf += sizeof(jsonSrcId)-1;
		ptrBuf += FillBuffer8_t(ptrBuf, 0, &(msg->content.std.srcId), 1, false);
		*ptrBuf = ',';
		ptrBuf++;
		/* Add destination id */
		memcpy(ptrBuf, jsonDestId, sizeof(jsonDestId)-1);
		ptrBuf += sizeof(jsonDestId)-1;
		ptrBuf += FillBuffer8_t(ptrBuf, 0, &(msg->content.std.destId), 1, false);
		*ptrBuf = ',';
		ptrBuf++;
		/* Add RSSI value */
		memcpy(ptrBuf, jsonRssi, sizeof(jsonRssi)-1);
		ptrBuf += sizeof(jsonRssi)-1;
		uint8_t rssiReversed = -msg_rx->rssi;
		ptrBuf += FillBuffer8_t(ptrBuf, 0, &rssiReversed, 1, false);
		*ptrBuf = ',';
		ptrBuf++;
		/* Look for state and add if necessary */
		/* Look for duplicate state */
		if(msg_rx->state.duplicate_flag) {
			memcpy(ptrBuf, jsonDuplicate, sizeof(jsonDuplicate)-1);
			ptrBuf += sizeof(jsonDuplicate)-1;
			ptrBuf += FillBuffer8_t(ptrBuf, 0, &(msg_rx->state.duplicate_flag), 1, false);
			*ptrBuf = ',';
			ptrBuf++;
		}
		/* Look for missing frame */
		if(msg_rx->state.missing_frames) {
			memcpy(ptrBuf, jsonMissingFrame, sizeof(jsonMissingFrame)-1);
			ptrBuf += sizeof(jsonMissingFrame)-1;
			ptrBuf += FillBuffer8_t(ptrBuf, 0, &(msg_rx->state.missing_frames), 1, false);
			*ptrBuf = ',';
			ptrBuf++;
		}
		/* Add payload */
		memcpy(ptrBuf, jsonPayload, sizeof(jsonPayload)-1);
		ptrBuf += sizeof(jsonPayload)-1;
		memcpy(ptrBuf, msg->content.std.payload, msg->hdr.payloadLength);
		ptrBuf += msg->hdr.payloadLength;
		memcpy(ptrBuf, jsonEndPayload, sizeof(jsonEndPayload)-1);
		ptrBuf += sizeof(jsonEndPayload)-1;
		return ptrBuf-*frameBuffer;
	}
	return 0;
}

/**
 * Display received packets as JSON
 *
 * Returns all received packets since last check as JSON formatted string through SYS_cmdResponse
 */
void response_rx_packets() {
	uint8_t* buffer = NULL;
	uint8_t* totalBuffer = NULL;
	void* bufMsgRx = NULL;
	uint16_t length;
	uint16_t totalLength;
#ifdef LOWAPP_MSG_FORMAT_CLASSIC
	/* Check the rx queue is not empty */
	if(queue_size(&_rx_pkt_list) > 0) {
		totalLength = 14;

		totalBuffer = (uint8_t*) malloc(sizeof(uint8_t)*totalLength);
		strncpy((char*)totalBuffer, "OK {\"rxpkts\":[", totalLength);

		MSG_T* msg;
		MSG_RX_APP_T *msg_rx_app;
		while(queue_size(&_rx_pkt_list) > 0) {
			/* Retrieve element of the RX queue */
			get_from_queue(&_rx_pkt_list, &bufMsgRx, &length);
			msg_rx_app = (MSG_RX_APP_T*) bufMsgRx;
			msg = msg_rx_app->msg;
			/* Build JSON object for each message */
			length = buildJson(&buffer, msg_rx_app);
			/* Realloc enough memory to store ]}\0 */
			totalBuffer = realloc(totalBuffer, totalLength + length + 3);
			/* Copy JSON formatted message to the final buffer */
			strncpy((char*)(totalBuffer+totalLength), (char*)buffer, length);
			totalLength += length;
			/* Add seperator for next message */
			strncpy((char*)(totalBuffer+totalLength), ",", 1);
			totalLength++;

			/* Free JSON temporary buffer */
			free(buffer);
			buffer = NULL;
			/* Free MSG retrieved from queue */
			free(msg);
			msg = NULL;
			free(bufMsgRx);
			bufMsgRx = NULL;
			msg = NULL;
		}
		totalLength--;	/* Remove trailing ',' */
		strncpy((char*)(totalBuffer+totalLength), "]}\0", 3);
		totalLength += 2;
		_sys->SYS_cmdResponse(totalBuffer, totalLength);

		/* Free allocation buffer */
		free(totalBuffer);
		totalBuffer = NULL;
	}
	else {
		_sys->SYS_cmdResponse((uint8_t*)"OK {\"rxpkts\":[]}", 16);
	}
#elif (defined(LOWAPP_MSG_FORMAT_GPSAPP) || defined(LOWAPP_MSG_FORMAT_GPSAPP_RSSI))
	/*
	 * When using the special GPSAPP format, messages are forwarded to the application
	 * using the following binary format :
	 *  B0-B1: 45 02
	 *  B2: Number of GPS elements following
	 *  For each received message :
	 *    B(2+nx): Device Id of the source device
	 *    B(2+nx+1)-B(2+nx+4): GPS latitude
	 *    B(2+nx+5)-B(2+nx+8): GPS longitude
	 *  Then, again for each received message:
	 *    B(2+9n): Length of the message, including length byte and source id byte
	 *    B(2+9n+1): deviceId of the source of the message
	 *    B(2+9n+2)-BN…: Text message received
	 *    Example of received messages :
	 *    4502
	 *     02
	 *       03 34352E31 352E3730
	 *       01 00000001 00000002
	 *     06 01 41424344  (ABCD)
	 *     07 03 3132333435 (12345)
	 */
	uint8_t rxSize = queue_size(&_rx_pkt_list);
	uint8_t offset = 0;
	uint8_t offsetTxtMessage = 0;
	uint8_t i;
	uint8_t messageLength;
	totalLength = 3+9*rxSize;

	totalBuffer = (uint8_t*) malloc(sizeof(uint8_t)*totalLength);
	/* Set prefix for GPS format response */
	*(totalBuffer+(offset++)) = 0x45;
	*(totalBuffer+(offset++)) = 0x02;
	*(totalBuffer+(offset++)) = rxSize;

	/* Offset used to set text messages */
	offsetTxtMessage = offset + rxSize*9;
	MSG_T* msg;
	MSG_RX_APP_T *msg_rx_app;
	for(i = 0; i < rxSize; ++i) {
		/* Retrieve element of the RX queue */
		get_from_queue(&_rx_pkt_list, &bufMsgRx, &length);
		msg_rx_app = (MSG_RX_APP_T*) bufMsgRx;
		msg = msg_rx_app->msg;
		/* Check duplicate flag */
//		if(msg_rx_app->state.duplicate_flag == 0) {

			/* Copy text message */
#ifdef LOWAPP_MSG_FORMAT_GPSAPP_RSSI
			uint8_t bufferRssiString[5] = ",";
			uint8_t rssiReversed;
			uint8_t rssiStringLength;
			rssiReversed = -msg_rx_app->rssi;
			rssiStringLength = FillBuffer8_t(bufferRssiString, 1, &rssiReversed, 1, false);
			/*
			 * Actual message length is 8 less than payloadLength
			 * because of the GPS coordinates
			 */
			messageLength = msg->hdr.payloadLength-8;
			/* Realloc enough memory */
			totalLength += 2 + messageLength + rssiStringLength;
			totalBuffer = realloc(totalBuffer, totalLength);
			/* Copy message source device id to the buffer */
			*(totalBuffer+offset+(i*9)) = msg->content.std.srcId;
			/* Copy GPS coordinates to the buffer */
			memcpy((char*)(totalBuffer+offset+(i*9)+1), (char*)msg->content.std.payload, 8);

			/* Length in the message format includes the length value itself and the source id (+2) */
			*(totalBuffer+(offsetTxtMessage++)) = messageLength+2+rssiStringLength;
			*(totalBuffer+(offsetTxtMessage++)) = msg->content.std.srcId;
			memcpy((char*)(totalBuffer+offsetTxtMessage), (char*)msg->content.std.payload+8, messageLength);
			offsetTxtMessage += messageLength;
			memcpy((char*)(totalBuffer+offsetTxtMessage), bufferRssiString, rssiStringLength);
			offsetTxtMessage += rssiStringLength;
#else
			/*
			 * Actual message length is 8 less than payloadLength
			 * because of the GPS coordinates
			 */
			messageLength = msg->hdr.payloadLength-8;
			/* Realloc enough memory */
			totalLength += 2 + messageLength;
			totalBuffer = realloc(totalBuffer, totalLength);
			/* Copy message source device id to the buffer */
			*(totalBuffer+offset+(i*9)) = msg->content.std.srcId;
			/* Copy GPS coordinates to the buffer */
			memcpy((char*)(totalBuffer+offset+(i*9)+1), (char*)msg->content.std.payload, 8);
			/* Length in the message format includes the length value itself and the source id (+2) */
			*(totalBuffer+(offsetTxtMessage++)) = messageLength+2;
			*(totalBuffer+(offsetTxtMessage++)) = msg->content.std.srcId;
			memcpy((char*)(totalBuffer+offsetTxtMessage), (char*)msg->content.std.payload+8, messageLength);
			offsetTxtMessage += messageLength;
#endif
//		}

		/* Free MSG retrieved from queue */
		free(msg);
		msg = NULL;
		free(bufMsgRx);
		bufMsgRx = NULL;
		msg = NULL;
	}
	totalBuffer = realloc(totalBuffer, totalLength+2);
	_sys->SYS_cmdResponse(totalBuffer, totalLength);

	/* Free allocation buffer */
	free(totalBuffer);
	totalBuffer = NULL;
#endif
}

/**
 * Get the symbol time for current SF and bandwidth
 * @return The duration of a single symbol in seconds
 */
double get_symbol_time() {
	/* Compute duration of one symbol in seconds */
	return ((1 << _rsf)/((double)bandwidthValues[_bandwidth]));
}

/**
 * Convert preamble time from us to symbols
 * @param preambleTime Duration of the preamble in us
 * @return The number of corresponding symbols
 */
uint16_t preamble_timeus_to_symbols(uint32_t preambleTime) {
	uint16_t pLen;
	/* Get number of symbols for the whole preambleTime */
	pLen = floor(preambleTime/get_symbol_time() - 4.25);
	return pLen;
}

/**
 * Convert preamble time from ms to symbols
 * @param preambleTime Duration of the preamble in ms
 * @return The number of corresponding symbols
 */
uint16_t preamble_timems_to_symbols(uint16_t preambleTime) {
	uint16_t pLen;
	/* Get number of symbols for the whole preambleTime */
	pLen = floor((preambleTime/1000.0)/get_symbol_time() - 4.25);
	return pLen;
}

/**
 * Convert preamble from number of symbols to ms
 * @param preambleLen Preamble in number of symbols
 * @return The preamble duration in us
 */
uint32_t preamble_symbols_to_timeus(uint16_t preambleLen) {
	double pTime;
	/* Get number of symbols for the whole preambleTime */
	pTime = (preambleLen+4.25)*get_symbol_time();
	return floor(pTime * 1e6);
}

/**
 * Convert preamble from number of symbols to ms
 * @param preambleLen Preamble in number of symbols
 * @return The preamble duration in ms
 */
uint32_t preamble_symbols_to_timems(uint16_t preambleLen) {
	double pTime;
	/* Get number of symbols for the whole preambleTime */
	pTime = (preambleLen+4.25)*get_symbol_time();
	return floor(pTime * 1e3);
}

/** @} */
/** @} */
