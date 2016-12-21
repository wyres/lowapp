/**
 * @file lowapp_sys_storage.c
 * @brief Implementation of the storage related functions for LoWAPP
 *
 * @author Nathan Olff
 * @date December 5, 2016
 */


#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include "lowapp_log.h"
#include "configuration.h"

#include "lowapp_sys_storage.h"
#include "radio-simu.h"

/** Path to the configuration file of this node */
char configFile[100];

/** Node subdirectory */
char nodeSubdir[] = "Nodes/";

/** Arguments for argp parsing */
struct arguments arguments;

extern const uint8_t strDeviceId[];
extern const uint8_t strGroupId[];
extern const uint8_t strGwMask[];
extern const uint8_t strEncKey[];
extern const uint8_t strRchanId[];
extern const uint8_t strRsf[];
extern const uint8_t strPreambleTime[];

/**
 * @addtogroup lowapp_simu
 * @{
 */
/**
 * @addtogroup lowapp_simu_config
 * @{
 */

/**
 * @brief Save current configuration into persistent memory
 *
 * Save the current configuration into a device specific file
 * in the Nodes/ folder, using the uuid as filename.
 *
 * @retval 0 If the configuration was saved
 * @retval -1 If the file could not be opened
 */
int8_t save_configuration() {
	FILE *fp;
	uint8_t value[256];
	fp = fopen(configFile, "w");
	if(fp == NULL) {
		// File opening failed
		return -1;
	}
	// Write data from myConfig
	get_config(strDeviceId, value);
	fprintf(fp, "%s:%s\r\n", strDeviceId, value);
	get_config(strGroupId, value);
	fprintf(fp, "%s:%s\r\n", strGroupId, value);
	get_config(strRchanId, value);
	fprintf(fp, "%s:%s\r\n", strRchanId, value);
	get_config(strRsf, value);
	fprintf(fp, "%s:%s\r\n", strRsf, value);
	get_config(strPreambleTime, value);
	fprintf(fp, "%s:%s\r\n", strPreambleTime, value);
	get_config(strGwMask, value);
	fprintf(fp, "%s:%s\r\n", strGwMask, value);
	get_config(strEncKey, value);
	fprintf(fp, "%s:%s\r\n", strEncKey, value);
	/* Do not save max retry LBT and max payload size */
	fclose(fp);
	return 0;
}


/**
 * @brief Read configuration from the device specific file
 *
 * The configuration file is stored in the Nodes/ directory. with the
 * uuid as its file name.
 *
 * @retval 0 If the file was opened and parsed
 * @retval -1 If the file could not be opened
 */
int8_t read_configuration() {
	FILE *fp;
	char line[256];
	/* Check that the configuration file exists. If not, create it. */
	if(access( configFile, F_OK ) == -1) {
		LOG(LOG_INFO, "The configuration file (%s) did not exist", configFile);
		fp = fopen(configFile, "w");
		fclose(fp);
		return -1;
	}

	fp = fopen(configFile, "r");
	if(fp == NULL) {
		LOG(LOG_ERR, "The file could not be opened");
		return -1;
	}
	/* Read data from file to myConfig */
	while (fgets(line, sizeof(line), fp)) {
		int size = strlen(line);
		/* Remove new line characters */
		if(line[size-1] == '\n' || line[size-1] == '\r') {
			line[--size] = '\0';
		}
		if(line[size-1] == '\n' || line[size-1] == '\r') {
			line[--size] = '\0';
		}
		parse_line(line);
	}
	fclose(fp);
	return 0;
}



/** @} */
/** @} */
