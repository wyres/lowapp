/**
 * @file lowapp_sys_eeprom.c
 * @brief Implementation of the EEPROM related functions for LoWAPP
 *
 * @author Nathan Olff
 * @date October 21, 2016
 */
#include "lowapp_sys_impl.h"
#include "lowapp_sys_eeprom.h"

extern ConfigNode_t myConfig;

/**
 * @addtogroup lowapp_hardware_sys
 * @{
 */
/**
 * @addtogroup lowapp_hardware_sys_eeprom LoWAPP Hardware System EEPROM Interface
 * @brief EEPROM Middle Layer For Persistant Memory Access
 * @{
 */

/**
 * @brief Save current configuration into persistent memory
 *
 * @retval 0 If the configuration was saved
 * @retval >0 If an error occurred
 */
int8_t save_configuration() {
	uint8_t i;
	volatile HAL_StatusTypeDef Status=0;
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_WRPERR);
	uint8_t EepromIndex=0;
	HAL_FLASHEx_DATAEEPROM_Unlock();

	//Erase the sector
	while(EepromIndex<100)
	{
		HAL_FLASHEx_DATAEEPROM_Erase(FLASH_TYPEERASEDATA_BYTE, EEPROM_BASE+EepromIndex);
		EepromIndex++;
	}
	EepromIndex=0;

	HAL_FLASHEx_DATAEEPROM_Unlock();

	Status |= HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE, (EEPROM_DEVICEID_START),myConfig.deviceId);
	Status |= HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE,(EEPROM_RCHANID_START),myConfig.rchanId);
	Status |= HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE,(EEPROM_RSF_START),myConfig.rsf);

	Status |= HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE,(EEPROM_GROUPID_START),myConfig.groupId);
	Status |= HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE,(EEPROM_GROUPID_START+1),myConfig.groupId>>8);
	Status |= HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE,(EEPROM_PTIME_START),myConfig.preambleTime);
	Status |= HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE,(EEPROM_PTIME_START+1),myConfig.preambleTime>>8);

	Status |= HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE,(EEPROM_GWMASK_START),myConfig.gwMask);
	Status |= HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE,(EEPROM_GWMASK_START+1),myConfig.gwMask>>8);
	Status |= HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE,(EEPROM_GWMASK_START+2),myConfig.gwMask>>16);
	Status |= HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE,(EEPROM_GWMASK_START+3),myConfig.gwMask>>24);

	for(i = 0; i < 16; ++i) {
		Status |= HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_FASTBYTE,(EEPROM_ENCKEY_START+i),myConfig.encKey[i]);
	}

	HAL_FLASHEx_DATAEEPROM_Lock();

	return Status;
}

/**
 * Read one byte in EEPROM
 *
 * @param address Address of the byte to read
 * @return The value stored in EEPROM
 */
uint8_t readEEPROMByte(uint32_t address) {
    uint8_t tmp = 0;
    tmp = *(__IO uint32_t*)address;
    return tmp;
}

/**
 * @brief Read configuration from the device specific file
 *
 * @retval 0
 */
int8_t read_configuration() {
	uint8_t i;

	memset(&myConfig, 0, sizeof(ConfigNode_t));

	myConfig.deviceId = readEEPROMByte(EEPROM_DEVICEID_START);
	myConfig.rchanId = readEEPROMByte(EEPROM_RCHANID_START);
	myConfig.rsf = readEEPROMByte(EEPROM_RSF_START);
	myConfig.groupId = readEEPROMByte(EEPROM_GROUPID_START) |
			readEEPROMByte(EEPROM_GROUPID_START+1) << 8;
	myConfig.preambleTime = readEEPROMByte(EEPROM_PTIME_START) |
					readEEPROMByte(EEPROM_PTIME_START+1) << 8;
	myConfig.gwMask = readEEPROMByte(EEPROM_GWMASK_START) |
			readEEPROMByte(EEPROM_GWMASK_START+1) << 8 |
			readEEPROMByte(EEPROM_GWMASK_START+2) << 16 |
			readEEPROMByte(EEPROM_GWMASK_START+3) << 24;
	for(i = 0; i < 16; ++i) {
		myConfig.encKey[i] = readEEPROMByte(EEPROM_ENCKEY_START+i);
	}
	return 0;
}

/** @} */
/** @} */
