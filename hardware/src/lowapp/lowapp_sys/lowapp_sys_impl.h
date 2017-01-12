/**
 * @file lowapp_sys_impl.h
 * @brief Implementation of the system level functions for LoWAPP
 *
 * @author Nathan Olff
 * @date October 20, 2016
 */
#ifndef LOWAPP_SYS_IMPL_H_
#define LOWAPP_SYS_IMPL_H_

#include "lowapp_inc.h"
#include "lowapp_sys_uart.h"
#include "lowapp_sys_timer.h"
#include "lowapp_sys_eeprom.h"
#include "lowapp_sys_radio.h"
#include "sx1272_ex.h"

/**
 * @addtogroup lowapp_hardware_sys
 * @{
 */
/**
 * @brief Structure containing the configuration variables of the end device
 */
typedef struct {
	uint8_t deviceId;			/**< End point device id */
	uint16_t groupId;			/**< Group id */
	uint32_t gwMask;			/**< Gateway mask */
	uint8_t rchanId;			/**< Radio channel id */
	uint8_t rsf;				/**< Radio spreading factor */
	uint16_t preambleTime;		/**< Preamble time (in ms) */
	uint8_t encKey[16];			/**< Encryption key : 256 bit AES */
} ConfigNode_t;

/** @} */

void register_sys_functions(LOWAPP_SYS_IF_T *lowappSys);

#endif
