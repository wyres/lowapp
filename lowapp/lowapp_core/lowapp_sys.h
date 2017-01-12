/**
 * @file lowapp_sys.h
 * @brief LoWAPP system interface
 *
 * Defines the structure containing all system level functions required by the core
 *
 * @author Nathan Olff
 * @date August 30, 2016
 */
#ifndef LOWAPP_CORE_SYS_H_
#define LOWAPP_CORE_SYS_H_

#include "lowapp_types.h"

/**
 * @addtogroup lowapp_core
 * @{
 */
/**
 * @addtogroup lowapp_core_sys_fct LoWAPP Core System Level Functions
 * @brief System level functions that need to be implemented and sent to the
 * LoWAPP initialise function.
 * @{
 */
/**
 * Structure containing all system level functions required by the LoWAPP core
 */
struct LOWAPP_SYS_IF {
	/**
	 * Get current time in ms
	 *
	 * Clock time in ms since epoch (to the best knowledge of the system)
	 * @return The number of ms since epoch
	 */
	LOWAPP_GETTIMEMS_T SYS_getTimeMs;
	/**
	 * Initialise timer 1
	 *
	 * @param callback Callback to be called after timems
	 */
	LOWAPP_INITTIMER_T SYS_initTimer;
	/**
	 * Initialise timer 2
	 *
	 * @param callback Callback to be called after timems
	 */
	LOWAPP_INITTIMER_T SYS_initTimer2;
	/**
	 * Initialise retitive timer
	 *
	 * @param callback Callback to be called after timems
	 */
	LOWAPP_INITTIMER_T SYS_initRepetitiveTimer;
	/**
	 * Request to set a callback to be called in timems milleseconds from now
	 *
	 * @param timems Time in ms
	 * @retval 0 On success
	 * @retval -1 If an error occurred
	 */
	LOWAPP_SETTIMER_T SYS_setTimer;
	/**
	 * Request to set a callback to be called in timems milleseconds from now
	 * using a second timer
	 *
	 * @param timems Time in ms
	 * @param callback Callback to be called after timems
	 * @retval 0 On success
	 * @retval -1 If an error occurred
	 */
	LOWAPP_SETTIMER_T SYS_setTimer2;
	/**
	 * Request to set a callback to be called every timems milleseconds
	 *
	 * @param timems Time in ms
	 * @param callback Callback to be called after timems
	 * @retval 0 On success
	 * @retval -1 If an error occurred
	 */
	LOWAPP_SETTIMER_T SYS_setRepetitiveTimer;
	/**
	 * Disarm the timer to avoid unexpected timeout later on
	 * @retval 0 On success
	 * @retval -1 If an error occurred
	 */
	LOWAPP_CANCELTIMER_T SYS_cancelTimer;
	/**
	 * Disarm the second timer
	 * @retval 0 On success
	 * @retval -1 If an error occurred
	 */
	LOWAPP_CANCELTIMER_T SYS_cancelTimer2;
	/**
	 * Disarm the repetitive timer to avoid unexpected cad timeout later on
	 * @retval 0 On success
	 * @retval -1 If an error occurred
	 */
	LOWAPP_CANCELTIMER_T SYS_cancelRepetitiveTimer;
	/**
	 * Blocking delay function
	 * @param delay Time to wait in ms
	 */
	LOWAPP_DELAYMS_T SYS_delayMs;

	/**
	 * Send data back to the application
	 *
	 * On the hardware, we will be using UART peripheral, in simulation
	 * the console standard output stream.
	 *
	 * @param data Ascii data to send
	 * @param length Length of the data to send
	 * @retval 0 On success
	 * @retval -1 If an error occurred
	 */
	LOWAPP_CMDRESPONSE_T SYS_cmdResponse;
	/**
	 * Get a string configuration value from persistent memory
	 *
	 * @param[in] key String corresponding to the configuration variable expected
	 * @param[out] value Value of the configuration variable (as a string)
	 * @return The size of the value in bytes
	 * @retval -1 If failed
	 */
	LOWAPP_GETCONFIG_T SYS_getConfig;
	/**
	 * Set a configuration value to persistent memory
	 * @param key String key of the configuration variable to change
	 * @param value New value for the given key (as a string)
	 * @retval 0 If the variable was set
	 * @retval -1 If the variable could not be set
	 */
	LOWAPP_SETCONFIG_T SYS_setConfig;
	/**
	 * Write configuration into persistent memory
	 * @retval 0 On success
	 * @retval -1 If configuration could not be saved
	 */
	LOWAPP_WRITECONFIG_T SYS_writeConfig;
	/**
	 * Read configuration from persistent memory
	 * @retval 0 On success
	 * @retval -1 If configuration could not be read
	 */
	LOWAPP_READCONFIG_T SYS_readConfig;
	/**
	 * Generate random value to use as a seed for
	 * custom random generator
	 */
	LOWAPP_RANDOM_T SYS_random;
	/**
	 * Request transmission of a LoRa frame
	 *
	 * @param data Data frame buffer
	 * @param dlen Size of the data buffer
	 */
	LOWAPP_RADIO_TX_T SYS_radioTx;
	/**
	 * Start reception of a LoRa frame
	 * @param timeout Timeout (in ms) for reception
	 * @retval 0 If the reception was started
	 * @retval -1 If the reception could not be started
	 */
	LOWAPP_RADIO_RX_T SYS_radioRx;
	/**
	 * Request LoRa CAD (Channel Activity Detection)
	 *
	 * @retval 0 If the CAD was started
	 * @retval -1 If the CAD could not be started
	 */
	LOWAPP_RADIO_CAD_T SYS_radioCAD;
	/**
	 * LoRA LBT (Listen before talk)
	 *
	 * Check LoRa channel availability before transmission
	 * This function blocks for timeoutms.
	 *
	 * @param chan Radio channel to check
	 * @retval 0 If the channel is free for transmission
	 * @retval 1 If the channel is used by another device
	 */
	LOWAPP_RADIO_LBT_T SYS_radioLBT;

	/**
	 * Initialise the radio
	 *
	 * @param events Structure containing the driver callback functions
	 */
	LOWAPP_RADIO_INIT_T SYS_radioInit;

	/**
	 * Sets the transmission parameters
	 *
	 * @param power Sets the output power [dBm]
	 * @param bandwidth Sets the bandwidth
	 *                          [0: 125 kHz, 1: 250 kHz,
	 *                          2: 500 kHz, 3: Reserved]
	 * @param datarate Sets the Datarate
	 *                          [6: 64, 7: 128, 8: 256, 9: 512,
	 *                          10: 1024, 11: 2048, 12: 4096  chips]
	 * @param coderate Sets the coding rate
	 *                          [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
	 * @param preambleLen Sets the preamble length
	 * @param timeout Transmission timeout [ms]
	 * @param fixLen Fixed length packets [0: variable, 1: fixed]
	 */
	LOWAPP_RADIO_SETTXCONFIG_T SYS_radioSetTxConfig;

	/**
	 * Sets the reception parameters
	 *
	 * @param bandwidth Sets the bandwidth
	 *                          [0: 125 kHz, 1: 250 kHz,
	 *                          2: 500 kHz, 3: Reserved]
	 * @param datarate Sets the Datarate
	 *                          [6: 64, 7: 128, 8: 256, 9: 512,
	 *                          10: 1024, 11: 2048, 12: 4096  chips]
	 * @param coderate Sets the coding rate
	 *                          [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
	 * @param preambleLen Sets the preamble length
	 * @param fixLen Fixed length packets [0: variable, 1: fixed]
     * @param payloadLen Sets payload length when fixed length is used
     * @param rxContinuous Sets the reception in continuous mode
     * 							[false: single mode, true: continuous mode]
	 */
	LOWAPP_RADIO_SETRXCONFIG_T SYS_radioSetRxConfig;

	/**
	 * Computes the packet time on air in us for the given payload
	 *
	 * @param pktLen Packet payload length
	 * @return airTime Computed airTime (ms) for the given packet payload length
	 */
	LOWAPP_RADIO_TIMEONAIR_T SYS_radioTimeOnAir;

	/**
	 * Set radio channel
	 *
	 * @param freq Radio channel
	 */
	LOWAPP_RADIO_SETCHANNEL_T SYS_radioSetChannel;

	/**
	 * Set radio in sleep mode
	 */
	LOWAPP_RADIO_SLEEP_T SYS_radioSleep;

	/**
	 * Set radio preamble length (without full SetTx/SetRx
	 * @param preambleLen Length of the preamble (in symbols)
	 */
	LOWAPP_RADIO_SETPREAMBLE_T SYS_radioSetPreamble;

	/**
	 * Set fix length packet for Tx (without full SetTx)
	 * @param fixLen If the packet has a fixed length
	 */
	LOWAPP_RADIO_SETTXFIXLEN_T SYS_radioSetTxFixLen;

	/**
	 * Set fix length packet for Rx (without full SetRx)
	 * @param fixLen If the packet has a fixed length
	 * @param payloadLen Size of the expected packet
	 */
	LOWAPP_RADIO_SETRXFIXLEN_T SYS_radioSetRxFixLen;

	/**
	 * Set TX timeout (without full SetTx)
	 * @param timeout Timeout for transmission (in ms)
	 */
	LOWAPP_RADIO_SETTXTIMEOUT_T SYS_radioSetTxTimeout;

	/**
	 * Set RX continuous mode
	 * @param rxContinuous Continuous mode for RX
	 */
	LOWAPP_RADIO_SETRXCONTINUOUS_T SYS_radioSetRxContinuous;

	/**
	 * Set radio callbacks
	 * @param events Radio callbacks
	 */
	LOWAPP_RADIO_SETRADIOCB_T SYS_radioSetCallbacks;
};

/** @} */
/** @} */

/** System level functions for LoWAPP */
typedef struct LOWAPP_SYS_IF LOWAPP_SYS_IF_T;

#endif
