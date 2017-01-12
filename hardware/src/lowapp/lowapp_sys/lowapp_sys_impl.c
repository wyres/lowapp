/**
 * @file lowapp_sys_impl.c
 * @brief Implementation of the system level functions for LoWAPP
 *
 * @author Nathan Olff
 * @date October 20, 2016
 */
#include "lowapp_sys_impl.h"

/**
 * @addtogroup lowapp_hardware_sys LoWAPP Hardware System Implementation
 * @brief LoWAPP Hardware System Implementation
 * @{
 */

/**
 * @addtogroup lowapp_hardware_sys_config LoWAPP Hardware System Configuration
 * @brief Hardware System Configuration
 * @{
 */

/**
 * Current configuration of the radio
 */
ConfigNode_t myConfig;

/**
 * @brief Get a configuration value using its key
 *
 * The variable's value is stored as a string in
 * the value buffer sent as parameter.
 *
 * @param key Key of the expected configuration variable (as a string)
 * @param value Buffer in which to store the value (as a string)
 */
int8_t get_config(const uint8_t* key, uint8_t* value) {
	const char* keyChar = (const char*) key;
	if(strcmp(keyChar, (const char*)strGwMask) == 0) {
		return FillBufferHexBI8_t((uint8_t*)value, 0, (uint8_t*)&(myConfig.gwMask), 4, true);
	}
	else if(strcmp(keyChar, (const char*)strDeviceId) == 0) {
		return FillBufferHexBI8_t((uint8_t*)value, 0, &(myConfig.deviceId), 1, true);
	}
	else if(strcmp(keyChar, (const char*)strGroupId) == 0) {
		return FillBufferHexBI8_t((uint8_t*)value, 0, (uint8_t*)&(myConfig.groupId), 2, true);
	}
	else if(strcmp(keyChar, (const char*)strRchanId) == 0) {
		return FillBufferHexBI8_t((uint8_t*)value, 0, &(myConfig.rchanId), 1, true);
	}
	else if(strcmp(keyChar, (const char*)strRsf) == 0) {
		return FillBufferHexBI8_t((uint8_t*)value, 0, &(myConfig.rsf), 1, true);
	}
	else if(strcmp(keyChar, (const char*)strPreambleTime) == 0) {
		return FillBuffer16_t((uint8_t*)value, 0, &(myConfig.preambleTime), 1, true);
	}
	else if(strcmp(keyChar, (const char*)strEncKey) == 0) {
		return FillBufferHexBI8_t((uint8_t*)value, 0, myConfig.encKey, 16, true);
	}
	else {
		return -1;
	}
	return 0;
}

/**
 * @brief Set a configuration variable using its key
 *
 * @param key Key of the configuration variable to set (as a string)
 * @param val New value of the configuration variable
 * @retval 0 If the key was found and the value set
 * @retval -1 If the key was not found
 */
int8_t set_config(const uint8_t* key, const uint8_t* val) {
	const char* keyChar = (const char*) key;
	if(strcmp(keyChar, (const char*)strGwMask) == 0) {
		AsciiHexStringConversionBI8_t((uint8_t*)&(myConfig.gwMask), val, 8);
	}
	else if(strcmp(keyChar, (const char*)strDeviceId) == 0) {
		AsciiHexStringConversionBI8_t(&(myConfig.deviceId), val, 2);
	}
	else if(strcmp(keyChar, (const char*)strGroupId) == 0) {
		AsciiHexStringConversionBI8_t((uint8_t*)&(myConfig.groupId), val, 4);
	}
	else if(strcmp(keyChar, (const char*)strRchanId) == 0) {
		AsciiHexConversionOneValueBI8_t((uint8_t*)&(myConfig.rchanId), val);
	}
	else if(strcmp(keyChar, (const char*)strRsf) == 0) {
		AsciiHexConversionOneValueBI8_t((uint8_t*)&(myConfig.rsf), val);
	}
	else if(strcmp(keyChar, (const char*)strPreambleTime) == 0) {
		myConfig.preambleTime = AsciiDecStringConversion_t(val, strlen((char*)val));
	}
	else if(strcmp(keyChar, (const char*)strEncKey) == 0) {
		AsciiHexStringConversionBI8_t(myConfig.encKey, (const uint8_t*)val, 32);
	}
	else {
		return -1;
	}
	return 0;
}

/** @} */


/**
 * Register system level functions for the LoWAPP core
 *
 * @param lowappSys Set of system level functions required by the core
 */
void register_sys_functions(LOWAPP_SYS_IF_T *lowappSys) {
	lowappSys->SYS_getTimeMs = get_time_ms;
	lowappSys->SYS_initTimer = init_timer1;
	lowappSys->SYS_setTimer = set_timer1;
	lowappSys->SYS_cancelTimer = cancel_timer1;
	lowappSys->SYS_initTimer2 = init_timer2;
	lowappSys->SYS_setTimer2 = set_timer2;
	lowappSys->SYS_cancelTimer2 = cancel_timer2;
	lowappSys->SYS_initRepetitiveTimer = init_timer_cad;
	lowappSys->SYS_setRepetitiveTimer = set_timer_cad;
	lowappSys->SYS_cancelRepetitiveTimer = cancel_timer_cad;
	lowappSys->SYS_delayMs = DelayMs;
	lowappSys->SYS_getConfig = get_config;
	lowappSys->SYS_setConfig = set_config;
	lowappSys->SYS_writeConfig = save_configuration;
	lowappSys->SYS_readConfig = read_configuration;
	lowappSys->SYS_random = radio_random;
	lowappSys->SYS_cmdResponse = cmd_response;
	lowappSys->SYS_radioTx = radio_send;
	lowappSys->SYS_radioCAD = radio_cad;
	lowappSys->SYS_radioLBT = radio_lbt;
	lowappSys->SYS_radioRx = radio_rx;
	lowappSys->SYS_radioInit = radio_init;
	lowappSys->SYS_radioSetTxConfig = radio_setTxConfig;
	lowappSys->SYS_radioSetRxConfig = radio_setRxConfig;
	lowappSys->SYS_radioTimeOnAir = radio_timeOnAir;
	lowappSys->SYS_radioSetChannel = radio_setChannel;
	lowappSys->SYS_radioSleep = radio_sleep;
	lowappSys->SYS_radioSetPreamble = setPreambleLength;
	lowappSys->SYS_radioSetRxFixLen = setRxFixLen;
	lowappSys->SYS_radioSetTxFixLen = setTxFixLen;
	lowappSys->SYS_radioSetTxTimeout = setTxTimeout;
	lowappSys->SYS_radioSetRxContinuous = setRxContinuous;
	lowappSys->SYS_radioSetCallbacks = radio_setCallbacks;
}

/** @} */
