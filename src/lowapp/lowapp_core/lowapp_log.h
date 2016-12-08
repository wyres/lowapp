/**
 * @file lowapp_log.h
 * @brief LoWAPP multi level log system
 *
 * Defines the multiple levels of log available as well as the logging functions
 * to log a string and to change the log level.
 *
 * @author Nathan Olff
 * @date August 23, 2016
 */

#ifndef LOWAPP_CORE_LOG_H_
#define LOWAPP_CORE_LOG_H_

#include <stdio.h>
#include "lowapp_sys_timer.h"

/** Size of the log buffer */
#define LOG_BUFFER_SIZE	4096

/**
 * @name Log levels
 * @{
 */
/** Fatal error log level */
#define LOG_FATAL    (1)
/** Fatal error log level */
#define LOG_ERR      (2)
/** Warning log level */
#define LOG_WARN     (3)
/** Fatal error log level */
#define LOG_PARSER   (4)
/** Information log level */
#define LOG_INFO     (5)
/** Debug log level */
#define LOG_DBG      (6)
/** Radio related log level */
#define LOG_RADIO	 (7)
/** States related log level */
#define LOG_STATES	 (8)
/** Multithread related log level */
#define LOG_THREAD	 (9)
/**@}*/

#ifdef SIMU
/**
 * Logging macro
 *
 * Currently formatting using printf like format and sending it to a stream.
 * @param level Level of the log message
 * @param ... printf like format with literal string followed by variables
 */
#define LOG(level, ...) do {  \
                            if (level <= debug_level) { \
                                fprintf(dbgstream,"lvl%d:%lu:%s:%d:", level,  get_time_us(), __FILE__, __LINE__); \
                                fprintf(dbgstream, __VA_ARGS__); \
                                fprintf(dbgstream, "\n"); \
                                fflush(dbgstream); \
                            } \
                        } while (0)

/**
 * Format data for logging and put it in the log buffer
 *
 * ! Not thread safe
 * @param level Level of the log message
 * @param ... printf like format with literal string followed by variables
 */
#define LOG_LATER(level, ...) do {  \
                            if (level <= debug_level) { \
                                sprintf(logBuffer+strlen(logBuffer),"lvl%d:%lu:%s:%d:", level,  get_time_us(), __FILE__, __LINE__); \
                                sprintf(logBuffer+strlen(logBuffer), __VA_ARGS__); \
                                sprintf(logBuffer+strlen(logBuffer), "\n"); \
                            } \
                        } while (0)

/**
 * Send the log buffer to the debug stream
 *
 * The log buffer allow us to postpone the printinf of a log message
 * and to cancel a log message if necessary.
 * ! Not thread safe
 */
#define LOG_BUFFER() do {  \
							fprintf(dbgstream,"%s", logBuffer); \
							flush_log_buffer();	\
							fflush(dbgstream); \
						} while (0)



extern FILE *dbgstream;
extern int  debug_level;
extern char logBuffer[];


void init_log();
void set_log_level(int level);
void flush_log_buffer(void);

#else

/**
 * Logging macro
 *
 * Currently formatting using printf like format and sending it to a stream.
 * @param level Level of the log message
 * @param ... printf like format with literal string followed by variables
 */
#define LOG(level, ...)

/**
 * Format data for logging and put it in the log buffer
 *
 * ! Not thread safe
 * @param level Level of the log message
 * @param ... printf like format with literal string followed by variables
 */
#define LOG_LATER(level, ...)

/**
 * Send the log buffer to the debug stream
 */
#define LOG_BUFFER()

#endif

#endif /* LOWAPP_CORE_LOG_H_ */
