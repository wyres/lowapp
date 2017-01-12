/**
 * @file lowapp_utils_queue.h
 * @brief Functions to manage FIFO queues
 *
 * @author Nathan Olff
 * @date August 26, 2016
 */

#ifndef LOWAPP_UTILS_QUEUE_H_
#define LOWAPP_UTILS_QUEUE_H_

#include "lowapp_core.h"
#include "lowapp_log.h"

/**
 * @addtogroup lowapp_core
 * @{
 */
/**
 * @addtogroup lowapp_core_utils
 * @{
 */
/**
 * @addtogroup lowapp_core_utils_queue
 * @{
 */

/** Size of the FIFO queues (< 128) */
#define MAXQSZ	16

/** Generic element for FIFO queue */
typedef struct QEL {
	void* data;			/**< Pointer to the data of the element */
	uint16_t datalen;	/**< Size in bytes of the corresponding data */
} QEL_T;

/**
 * FIFO represented as a ring buffer of generic elements
 *
 * Elements are added in the head of the queue and retrieved from the tail.
 */
typedef struct QFIXED {
	uint8_t head;		/**< Head of the FIFO queue */
	uint8_t tail;		/**< Tail of the FIFO queue */
	uint8_t count;		/**< Number of elements in the queue */
	QEL_T els[MAXQSZ];	/**< Array used for ring buffer */
} QFIXED_T;

/**
 * Event elements of the event queues
 *
 * An event element contains an event type and, optionally, a data and a data size.<br />
 * Data is used to store a message content for a TXREQ type event for example.
 */
typedef struct EVENT {
	EVENTS type;		/**< Type of the event */
	void* data;			/**< Data related to the event (optional) */
	uint16_t datalen;	/**< Size in bytes of the event */
} EVENT_T;

/**
 * FIFO represented as a ring buffer of event elements
 *
 * Elements are added in the head of the queue and retrieved from the tail.
 */
typedef struct QEVENT {
	uint8_t head;		/**< Head of the FIFO queue */
	uint8_t tail;		/**< Tail of the FIFO queue */
	uint8_t count;		/**< Number of elements in the queue */
	EVENT_T evts[MAXQSZ];	/**< Array used for ring buffer */
} QEVENT_T;

/**
 * FIFO represented as a ring buffer of generic elements
 *
 * Elements are added in the head of the queue and retrieved from the tail.
 */
typedef struct QSTAT {
	uint8_t count;		/**< Number of elements in the queue */
	STAT_T els[MAXQSZ];	/**< Array used for ring buffer */
} QSTAT_T;

/** @} */
/** @} */
/** @} */

uint8_t event_size(volatile QEVENT_T* q);
int8_t add_event(volatile QEVENT_T* q, EVENTS t, void* d, uint16_t dlen);
int8_t add_simple_event(volatile QEVENT_T* q, EVENTS t);
int8_t get_event(volatile QEVENT_T* q, EVENTS* t, void** d, uint16_t *dlen);

int8_t add_to_queue(volatile QFIXED_T* q, void* d, uint16_t dlen);
int8_t get_from_queue(volatile QFIXED_T* q, void**d, uint16_t* dlen);
uint8_t queue_size(volatile QFIXED_T* q);
bool queue_full(volatile QFIXED_T* q);

int8_t add_to_statqueue(volatile QSTAT_T* q, STAT_T d);
uint8_t statqueue_size(volatile QSTAT_T* q);

#endif /* LOWAPP_UTILS_QUEUE_H_ */
