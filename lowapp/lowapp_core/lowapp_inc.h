/**
 * @file lowapp_inc.h
 * @brief LoWAPP general include file
 *
 * Includes all LoWAPP core header files and defines access to external
 * configuration variables.
 *
 * @author Brian Wyld
 * @author Nathan Olff
 * @date June 29, 2016
 */
#ifndef LOWAPP_CORE_INC_H_
#define LOWAPP_CORE_INC_H_

/* Include LoWAPP core headers */
#include "lowapp_if.h"
#include "lowapp_err.h"
#include "lowapp_core.h"
#include "lowapp_msg.h"
#include "lowapp_log.h"
#include "lowapp_radio_evt.h"
#include "lowapp_atcmd.h"
/* Include LoWAPP util headers */
#include "lowapp_utils_queue.h"
#include "lowapp_utils_conversion.h"

#include "lowapp_shared_res.h"

/* Externs for all globals */
extern uint8_t _rchanId;
extern uint8_t _rsf;
extern uint8_t _bandwidth;
extern uint8_t _coderate;
extern int8_t _power;
extern uint32_t _gwMask;
extern uint8_t _deviceId;
extern uint16_t _groupId;
extern uint16_t _preambleLen;
extern uint8_t _encryptionKey[16];
extern NODE_MODE_T _opMode;
extern bool _connected;

extern uint32_t _cad_interval;

extern uint16_t preambleTime;

extern const uint8_t strRchanId[];
extern const uint8_t strRsf[];
extern const uint8_t strGwMask[];
extern const uint8_t strDeviceId[];
extern const uint8_t strGroupId[];
extern const uint8_t strPreambleTime[];
extern const uint8_t strPreambleLength[];
extern const uint8_t strEncKey[];
extern const uint8_t strMaxRetryLBT[];
extern const uint8_t strCoderate[];
extern const uint8_t strBandwidth[];
extern const uint8_t strPower[];


/* System callbacks */
extern LOWAPP_SYS_IF_T* _sys;

#endif
