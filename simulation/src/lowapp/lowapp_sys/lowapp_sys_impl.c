/**
 * @file lowapp_sys_impl.c
 * @brief Implementation of the system level functions for LoWAPP
 *
 * @author Nathan Olff
 * @date October 20, 2016
 */
#include "lowapp_sys_impl.h"
#include "lowapp_sys_storage.h"
#include "lowapp_sys_timer.h"
#include "lowapp_sys_io.h"
#include "radio-simu.h"
#include "sx1272_ex.h"

/**
 * @addtogroup lowapp_simu
 * @{
 */
/**
 * @addtogroup lowapp_simu_config
 * @{
 */


/**
 * Dummy function
 *
 * Used for system level functions that does not have any meaning for
 * the simulation
 */
void dummy() {

}

/**
 * Register system level functions for the LoWAPP core
 *
 * @param lowappSys Set of system level functions required by the core
 */
void register_sys_functions(LOWAPP_SYS_IF_T *lowappSys) {
	lowappSys->SYS_getTimeMs = get_time_ms;
	lowappSys->SYS_setTimer = set_timer1;
	lowappSys->SYS_cancelTimer = cancel_timer1;
	lowappSys->SYS_setTimer2 = set_timer2;
	lowappSys->SYS_cancelTimer2 = cancel_timer2;
	lowappSys->SYS_setRepetitiveTimer = set_repet_timer;
	lowappSys->SYS_cancelRepetitiveTimer = cancel_repet_timer;
	lowappSys->SYS_delayMs = simu_delayMs;
	lowappSys->SYS_getConfig = get_config;
	lowappSys->SYS_setConfig = set_config;
	lowappSys->SYS_writeConfig = save_configuration;
	lowappSys->SYS_readConfig = read_configuration;
	lowappSys->SYS_cmdResponse = cmd_response;
	lowappSys->SYS_radioTx = simu_radio_send;
	lowappSys->SYS_radioCAD = simu_radio_cad;
	lowappSys->SYS_radioLBT = simu_radio_lbt;
	lowappSys->SYS_radioRx = simu_radio_rx;
	lowappSys->SYS_radioInit = simu_radio_init;
	lowappSys->SYS_radioSetTxConfig = simu_radio_setTxConfig;
	lowappSys->SYS_radioSetRxConfig = simu_radio_setRxConfig;
	lowappSys->SYS_radioTimeOnAir = simu_radio_timeOnAir;
	lowappSys->SYS_radioSetChannel = simu_radio_setChannel;
	lowappSys->SYS_initTimer = init_timer1;
	lowappSys->SYS_initTimer2 = init_timer2;
	lowappSys->SYS_initRepetitiveTimer = init_repet_timer;
	lowappSys->SYS_random = simu_radio_random;
	lowappSys->SYS_radioSleep = dummy;
	lowappSys->SYS_radioSetPreamble = setPreambleLength;
	lowappSys->SYS_radioSetRxFixLen = setRxFixLen;
	lowappSys->SYS_radioSetTxFixLen = setTxFixLen;
	lowappSys->SYS_radioSetTxTimeout = setTxTimeout;
	lowappSys->SYS_radioSetRxContinuous = setRxContinuous;
	lowappSys->SYS_radioSetCallbacks = simu_radio_setCallbacks;
}

/** @} */
/** @} */

