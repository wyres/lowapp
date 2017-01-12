/**
 * @file lowapp_sys_eeprom.h
 * @brief Implementation of the EEPROM related functions for LoWAPP
 *
 * @author Nathan Olff
 * @date October 21, 2016
 */
#ifndef LOWAPP_SYS_EEPROM_H_
#define LOWAPP_SYS_EEPROM_H_

#include "board.h"
#include "lowapp_sys.h"
#include "stm32l1xx_hal_flash.h"
#include "stm32l1xx_hal_flash_ex.h"

/** EEPROM base address in memory */
#define EEPROM_BASE					0x08081800
/** Device ID address location in EEPROM */
#define EEPROM_DEVICEID_START		0x08081800
/** Radio channel ID address location in EEPROM */
#define EEPROM_RCHANID_START		0x08081801
/** Spreadinf factor address location in EEPROM */
#define EEPROM_RSF_START			0x08081802
/** Group ID address location in EEPROM */
#define EEPROM_GROUPID_START		0x08081804
/** Preamble time address location in EEPROM */
#define EEPROM_PTIME_START			0x08081806
/** Gateway mask address location in EEPROM */
#define EEPROM_GWMASK_START			0x08081808
/** Encryption key address location in EEPROM */
#define EEPROM_ENCKEY_START			0x0808180C

int8_t read_configuration();
int8_t save_configuration();


#endif
