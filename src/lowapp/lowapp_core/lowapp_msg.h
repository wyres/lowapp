/**
 * @file lowapp_msg.h
 * @brief LoWAPP Message and frame related functions
 *
 * Defines functions used to build and parse frame from message structures.
 *
 * @author Nathan Olff
 * @date August 22, 2016
 */

#ifndef LOWAPP_CORE_MSG_H_
#define LOWAPP_CORE_MSG_H_

/* Include LoRaMacCrypto header */
#include <LoRaMacCrypto.h>

/* Simulation specific includes */
#ifdef SIMU
	#include <stdint.h>
	#include <stdlib.h>
#endif

/**
 * @addtogroup lowapp_core LoWAPP Core
 * @{
 */
/**
 * @addtogroup lowapp_core_messages LoWAPP Core Messages
 * @{
 */

/** General maximum frame size */
#define MAX_FRAME_SIZE	255

/**
 * Maximum payload size of standard message
 *
 * Takes into account the maximum frame size (255), the size of the LoRa header (4),
 * the nonce (2) and the content of a standard frame (3)
 */
#define MAX_PAYLOAD_STD_SIZE	MAX_FRAME_SIZE-4-2-3
/**
 * Maximum payload size of a gateway out message
 */
#define MAX_PAYLOAD_GWOUT_SIZE	MAX_FRAME_SIZE-4-2-23
/**
 * Maximum payload size of a gateway in message
 */
#define MAX_PAYLOAD_GWIN_SIZE	MAX_FRAME_SIZE-4-2-23

/** Number of retry in case of tx failure */
#define MAX_TX_FAIL_RETRY	2

/** Size of an ACK frame in bytes */
#define ACK_FRAME_LENGTH	12

/** Maximum number of retry for txFrame */
#define MAX_TX_FRAME_RETRY	3

/** Random value min for blocking tx */
#define RANDOM_BLOCK_TX_MIN	0
/** Random value max for blocing tx */
#define RANDOM_BLOCK_TX_MAX	1000

/** Standard message */
struct STDMSG {
	uint8_t destId;		/**< Destination device id */
	uint8_t srcId;		/**< Source device id */
	uint8_t txSeq;		/**< Sequence number */
	uint8_t payload[MAX_PAYLOAD_STD_SIZE];	/**< Payload of the message */
};

/** Acknowledge message */
struct ACKMSG {
	uint8_t destId;		/**< Destination device id */
	uint8_t srcId;		/**< Source device id */
	uint8_t rxdSeq;		/**< Received sequence number */
	uint8_t expectedSeq;	/**< Expected sequence number */
};

/** Gateway out message */
struct GWOUTMSG {
	uint8_t srcId;			/**< Source device id */
	uint8_t txSeq;			/**< Tx sequence number */
	uint32_t netType;		/**< Network type */
	uint8_t netAddrLen;		/**< Network address length */
	uint8_t netAddr[16];	/**< Network address */
	uint8_t payload[MAX_PAYLOAD_GWOUT_SIZE];	/**< Payload of the message */
};

/** Gateway in message */
struct GWINMSG {
	uint8_t destId;			/**< Destination device id */
	uint8_t txSeq;			/**< Tx sequence number */
	uint32_t netType;		/**< Network type */
	uint8_t netAddrLen;		/**< Network address length */
	uint8_t netAddr[16];	/**< Network address */
	uint8_t payload[MAX_PAYLOAD_GWIN_SIZE];	/**< Payload of the message */
};

/** Generic frame content */
union FMSG {
	struct STDMSG std;		/**< Standard message */
	struct ACKMSG ack;		/**< Acknowledge message */
	struct GWOUTMSG gwout;	/**< Gateway out message */
	struct GWINMSG gwin;		/**< Gateway in message */
};

/**
 * LoRa header
 *
 * The 4 bytes header contains:
 * - Version value on 4-bits
 * - Type value on 4-bits @see #MSG_TYPE
 * - Payload length on 1 byte
 * - Reserved for future use on 2 bytes
 */
struct LORA_HDR {
	uint8_t version:4;	/**< Version of the protocol (4 bits) */
	uint8_t type:4;		/**< Type of the message (4 bits) @see @ref lowapp_message_types */
	uint8_t payloadLength;	/**< Payload length (1 byte) */
	uint16_t rfu;			/**< Reserved for future use (2 bytes) */
};

/**
 * Generic message
 *
 * A generic message contains a LoRA header as well as a type specific content.
 */
struct MSG {
	struct LORA_HDR hdr;	/**< LoRa header */
	union FMSG content;	/**< Content of the frame */
};

/**
 * Informations about duplicate or missing frames for received packets
 */
struct MSG_RX_STATE {
	/** Flag for duplicate frames detection */
	uint8_t duplicate_flag;
	/** Number of missing frames since the previous received message */
	uint8_t missing_frames;
};

/**
 * Message received, packaged for forwarding to the application
 */
struct MSG_RX_APP {
	/** State of received messages (duplicate and/or missing frames) */
	struct MSG_RX_STATE state;
	/** Pointer to the actual received message */
	struct MSG *msg;
	/** RSSI of the data signal */
	int16_t rssi;
	/** SNR of the message received */
	int8_t snr;
};

/**
 * Message received by the radio, packaged for forwarding to the core
 *
 * This is used to pass all informations from the RxDone event to the radio
 * in the RXMSG event
 */
struct MSG_RXDONE {
	/** Data received on the radio */
	uint8_t* data;
	/** Length in bytes of the received data */
	uint16_t length;
	/** RSSI of the data signal */
	int16_t rssi;
	/** SNR of the message received */
	int8_t snr;
};

/**@} */
/**@} */

/* Typedef for message structures */
/** Standard message type definition */
typedef struct STDMSG STDMSG_T;
/** Ack message type definition */
typedef struct ACKMSG ACKMSG_T;
/** Gateway out message type definition */
typedef struct GWOUTMSG GWOUTMSG_T;
/** Gateway in message type definition */
typedef struct GWINMSG GWINMSG_T;
/** Generic frame content type definition */
typedef union FMSG FMSG_T;
/** LoRa header type definition */
typedef struct LORA_HDR LORA_HDR_T;
/** Generic message type definition */
typedef struct MSG MSG_T;
/** Receive message state type definition */
typedef struct MSG_RX_STATE MSG_RX_STATE_T;
/** Receive message with state type definition */
typedef struct MSG_RX_APP MSG_RX_APP_T;
/** Receive message with RSSI and SNR */
typedef struct MSG_RXDONE MSG_RXDONE_T;

uint16_t buildFrame(uint8_t *frameBuffer, MSG_T *msg);
int8_t retrieveMessage(MSG_T *msg, uint8_t *frameBuffer);

uint8_t frameSize(MSG_T *msg);

void response_rx_packets();
double get_symbol_time();
uint16_t preamble_timems_to_symbols(uint16_t preambleTime);
uint32_t preamble_symbols_to_timems(uint16_t preambleLen);

#endif /* LOWAPP_CORE_MSG_H_ */
