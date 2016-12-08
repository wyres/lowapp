/**
 * @file lowapp_log.c
 * @brief LoWAPP multi level log system
 *
 * Define functions and globals used for multilevel logging
 *
 * @author Nathan Olff
 * @date August 23, 2016
 */

#include "lowapp_log.h"


#ifdef SIMU

#include <string.h>

/**
 * @addtogroup lowapp_core
 * @{
 */
/**
 * @addtogroup lowapp_core_log LoWAPP Core Log System
 * @brief Multi level logging system
 * @{
 */

/**
 * Stream to which the logs are sent
 */
FILE *dbgstream;
/**
 * Current log level
 *
 * We only send to the dbgstream the messages with a level lower or equal to the
 * current debug_level.
 */
int  debug_level;

/**
 * Log buffer, used when we are not sure if we want to display the logging message
 */
char logBuffer[LOG_BUFFER_SIZE];

/**
 * Initialise multi level logging
 */
void init_log() {
	dbgstream = stdout;
	debug_level = LOG_DBG;
	flush_log_buffer();
}

/**
 * Set the current log level
 * @param level New log level
 */
void set_log_level(int level) {
	debug_level = level;
}

/**
 * Flush the log buffer after printing
 */
void flush_log_buffer(void) {
	memset(logBuffer, 0, LOG_BUFFER_SIZE);
}

/** @} */
/** @} */

#endif
