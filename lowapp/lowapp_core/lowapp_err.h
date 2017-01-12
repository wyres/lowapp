/**
 * @file lowapp_err.h
 * @brief LoWAPP error codes
 *
 * @author Brian Wyld
 * @author Nathan Olff
 * @date June 29, 2016
 */
#ifndef LOWAPP_CORE_ERR_H_
#define LOWAPP_CORE_ERR_H_

/** Invalid value error */
#define LOWAPP_ERR_INVAL -1
/** Queue full error */
#define LOWAPP_ERR_QFULL -2
/** No ACK received */
#define LOWAPP_ERR_NOACK -3
/** Persistent memory not available */
#define LOWAPP_ERR_NOMEM -4

/** Persistent memory error */
#define LOWAPP_ERR_PERSISTMEM	-10
/** Error while loading configuration */
#define LOWAPP_ERR_LOADCFG	-11
/** Error while setting a configuration value */
#define LOWAPP_ERR_SETATTR	-12
/** Message payload error */
#define LOWAPP_ERR_PAYLOAD	-13
/** Destination id error */
#define LOWAPP_ERR_DESTID	-14

/** Invalid size for AT command */
#define LOWAPP_ERR_ATSIZE	-15

/** Disconnected mode */
#define LOWAPP_ERR_DISCONNECT	-16

/** Variable not initialise error */
#define LOWAPP_ERR_NOTINIT -100
/** Serial peripheral not available */
#define LOWAPP_ERR_NOSERIAL -101
/** LoWAPP broken error */
#define LOWAPP_ERR_BROKEN -102
/** Error, functionality not implemented */
#define LOWAPP_ERR_NOTIMPL -103


#endif
