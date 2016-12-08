/**
 * @file configuration.h
 * @brief Node management for LoWAPP simulation
 *
 * @date August 9, 2016
 * @author Nathan Olff
 */
#ifndef LOWAPP_SIMU_CONFIGURATION_H_
#define LOWAPP_SIMU_CONFIGURATION_H_

#include "board.h"

/**
 * @addtogroup lowapp_simu LoWAPP Linux Simulation
 * @{
 */
/**
 * @addtogroup lowapp_simu_config LoWAPP Simulation Device Configuration
 * @brief Application level device configuration
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
	uint8_t encKey[32];			/**< Encryption key : 256 bit AES */
} ConfigNode_t;

/** @} */
/** @} */

/**
 * Structure used to retrieve arguments
 *
 * It is used by main to communicate with parse_opt
 */
struct arguments
{
  char *directory;	/**< Root directory for the simulation (defaults to ./) */
  char *uuid;		/**< UUID of the node. Config file is in root's Nodes subdirectory */
  char *config;  	/**< Configuration file, relative to root directory */
};

int8_t get_uuid(int argc, char* argv[]);
int8_t parse_line(char* line);
int8_t node_init(struct arguments *args);
bool file_exists(char* path);
int8_t get_config(const uint8_t* key, uint8_t* value);
int8_t set_config(const uint8_t* key, const uint8_t* val);


#endif
