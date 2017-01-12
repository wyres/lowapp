/**
 * @file radio-simu.h
 * @brief Simulation of the radio transmissions
 *
 * @author Nathan Olff
 * @date August 10, 2016
 */

#ifndef LOWAPP_SIMU_RADIO_H_
#define LOWAPP_SIMU_RADIO_H_

#include "lowapp_types.h"
#include "radio.h"
#include <pthread.h>


/** Size of the reception buffer */
#define BUFFER_RX_SIZE	512
/** Signal number used for preamble timers */
#define	SIG_PREAMBLE	SIGRTMIN+2
/** Signal number used for transmission timers */
#define	SIG_TX	SIGRTMIN+3

/** Time to wait for the channel to stay free before starting a transmission */
#define	CHAN_FREE_TIMEOUT	1

/**
 * CRC parameter for SetTx and SetRx
 *
 * Enable or disable CRC for the SX1272
 */
#define LOWAPP_SYS_RADIO_CRC	1

/**
 * Threshold used for LBT
 */
#define LOWAPP_SYS_RADIO_RSSI	-80

/**
 * LoWapp symbol timeout for RxConfig
 */
#define LOWAPP_SYMBOL_TIMEOUT        5


/** CAD duration (in ms) */
#define CAD_DURATION	2

/** Random failure percentage for simulating error while starting a transmission */
#define FAILURE_RANDOM_START_TX 0
/** Random failure percentage for simulating error while starting a reception */
#define FAILURE_RANDOM_START_RX	0

/** Timer for end of ACK preamble, used for simu_block */
#define TIMER_BLOCK_PREAMBLE_TIME_ACK	50


/**
 * Radio LoRa modem parameters
 */
typedef struct
{
	/**
	 * Radio power for transmission (in dBm)
	 *
	 * @see #_power LoWAPP core attribute
	 */
    int8_t   Power;
    /**
     * Radio bandwidth
     *
     * @see #_bandwidth LoWAPP core attribute
     */
    uint32_t Bandwidth;
    /**
     * Radio datarate / spreading factor
     *
     * @see #_rsf LoWAPP core attribute
     */
    uint32_t Datarate;
    /**
     * Radio low datarate optimize
     *
     * Not used in LoWAPP, set to false
     */
    bool     LowDatarateOptimize;
    /**
     * Radio coding rate
     *
     * @see #_coderate LoWAPP core attribute
     */
    uint8_t  Coderate;
    /**
     * Preamble length
     *
     * @see #_preambleLen LoWAPP core attribute
     */
    uint16_t PreambleLen;
    /**
     * Fixed length for implicit header
     *
     * This is used to sending or receiving frame with implicit header
     * and a predefined length (used for ACK messages).
     *
     * Values :
     * 		0: variable
     * 		1: fixed
     */
    bool     FixLen;
    /**
     * Sets payload length when fixed length is used
     */
    uint8_t  PayloadLen;
    /**
     * Enable CRC at the end of the frame
     *
     * This is enabled for all messages within LoWAPP
     */
    bool     CrcOn;
    /**
     * Enables/disables the intra-packet frequency hopping
     *
     * Not used in LoWAPP, set to false
     */
    bool     FreqHopOn;
    /**
     * Number of symbols bewteen each hop
     *
     * Not used in LoWAPP, set to false
     */
    uint8_t  HopPeriod;
    /**
     * Inverts IQ signals
     *l
     * Not used in LoWAPP, set to false
     */
    bool     IqInverted;
    /**
     * Sets the reception in continuous mode
     *
     * Not used in LoWAPP, set to false
     *
     * Values :
     * 		false : single mode
     * 		true : continuous mode
     */
    bool     RxContinuous;
    /**
     * Transmission timeout in us
     */
    uint32_t TxTimeout;

    /**
     * Custom attribute used for CAD timeouts in simulation (in ms)
     */
    uint16_t symbTimeout;
}RadioLoRaSettings_t;

/**
 * Radio LoRa packet handler state
 */
//typedef struct
//{
//    int8_t SnrValue;
//    int16_t RssiValue;
//    uint8_t Size;
//}RadioLoRaPacketHandler_t;

/**
 * Radio Settings
 */
typedef struct
{
	/** Current state of the radio */
    RadioState_t             State;
    /**
     * Current mode (FSK / LORA) of the radio
     *
     * Not used in LoWAPP, set to LoRa
     */
    RadioModems_t            Modem;
    /**
     * Radio channel
     */
    uint32_t                 Channel;
//    RadioFskSettings_t       Fsk;
//    RadioFskPacketHandler_t  FskPacketHandler;
    /**
     * LoRa specific settings
     */
    RadioLoRaSettings_t      LoRa;
    /**
     * LoRa packet handler
     */
//    RadioLoRaPacketHandler_t LoRaPacketHandler;
}RadioSettings_t;



/**
 * Structure storing argument for starting transmission threads
 *
 * This is necessary because a Linux pthread takes a void* arg. To send
 * the parameters we need, we use this structure, fill it with data and pass
 * a pointer to this structure to the thread at its creation.
 */
typedef struct {
	/** Preamble length */
	uint32_t plen;
	/** Data buffer */
	void* data;
	/** Size of the data */
	uint32_t dlen;
}RadioThreadArg_t;

/**
 * Structure storing argument for starting CAD threads
 *
 * This is necessary because a Linux pthread takes a void* arg. To send
 * the parameters we need, we use this structure, fill it with data and pass
 * a pointer to this structure to the thread at its creation.
 */
typedef struct {
	/** Radio channel id */
	uint8_t chan;
	/** Spreading factor */
	uint8_t sf;
	/** Timeout (in ms) */
	uint16_t timeoutms;
}CADThreadArg_t;

void* thread_continuous_radio(void *arg);
void stop_radio_thread();

int radio_tx_preamble();
int handle_events(int fd, int wd, char* toWatch, char* fileToCheck);
int inotify_create(uint32_t chan, uint8_t sf, uint16_t timeoutms);

int cad_for_standard_rx();
bool simu_radio_lbt(uint8_t chan);

int radio_read(int size, uint32_t timeoutms);
int radio_rx(uint32_t timeoutms);

void radio_tx_write(void* data, uint32_t dlen);
void radio_tx_eof();
void simu_delayMs(uint32_t timems);
int radio_processing_sleep(uint32_t timems);

int start_generic_thread(pthread_t *th, void*(*handler)(void *arg), void *arg);
void simu_radio_cad();
void simu_radio_rx(uint32_t timeout);

int16_t get_file_size(const char* filename);
bool file_exists(char* path);

int simu_blocking_cad_for_rx_ack(uint32_t chan, uint8_t sf, uint16_t timeoutStartMs, uint16_t timeoutPreMs);

int inotify_create2(uint32_t chan, uint8_t sf, uint16_t timeout1ms, uint16_t timeout2ms);

bool check_radio_file_exists();

void simu_radio_init(Lowapp_RadioEvents_t *evt);
void simu_radio_setTxConfig(int8_t power, uint8_t bandwidth, uint8_t datarate,
		uint8_t coderate, uint16_t preambleLen, uint32_t timeout, bool fixLen);
void simu_radio_setRxConfig(uint8_t bandwidth, uint8_t datarate, uint8_t coderate,
		uint16_t preambleLen, bool fixLen, uint8_t payloadLen, bool rxContinuous);
void simu_radio_send(uint8_t *data, uint8_t dlen);
uint32_t simu_radio_timeOnAir(uint8_t pktLen);
double simu_radio_transmissionTimePreamble();
double simu_radio_transmissionTimePayload(uint16_t pktLen);
int8_t simu_radio_rxing_ack(uint32_t timeoutms);
void simu_radio_setChannel(uint32_t chan);
void simu_radio_setCallbacks(Lowapp_RadioEvents_t *evt);
void rx_ack(uint32_t timeoutms);

uint32_t simu_radio_random(void);

#endif /* LOWAPP_SIMU_RADIO_H_ */
