/**
 * @file lowapp_utils_queue.c
 * @brief Functions to manage FIFO queues
 *
 * @author Nathan Olff
 * @date August 26, 2016
 */
#include "lowapp_utils_queue.h"
#ifdef SIMU
	#include "lowapp_shared_res.h"
#endif

/**
 * @addtogroup lowapp_core
 * @{
 */
/**
 * @addtogroup lowapp_core_utils
 * @{
 */
/**
 * @addtogroup lowapp_core_utils_queue LoWAPP Core Utility Queues
 * @brief FIFO Queue management
 * @{
 */

/**
 * @brief Add element to the head of a queue
 *
 * @param q Queue in which the element should be put
 * @param d Element to put in the queue
 * @param dlen Size of the element d
 *
 * @return The size of the queue once the element was added
 * @retval -1 If the queue was full and the element could not be added
 */
int8_t add_to_queue(volatile QFIXED_T* q, void* d, uint16_t dlen) {
	if (q->count == MAXQSZ) {
		/* The queue is full */
		return -1;
	}
	/* Fill data into head of the queue */
	q->els[q->head].data = d;
	q->els[q->head].datalen = dlen;
	/* Move the head to the next available space in the buffer */
	q->head = (q->head + 1)%MAXQSZ;
	/* Increment the number of element in the queue and return it */
	return ++(q->count);
}

/**
 * @brief Get an element from the tail of a queue
 *
 * @param q Queue from which to extract the element
 * @param d Pointer to the element extracted from the queue
 * @param dlen Size of the element d
 *
 * @return The new size of the queue
 * @retval -1 If the queue was empty
 */
int8_t get_from_queue(volatile QFIXED_T* q, void**d, uint16_t* dlen) {
	if (q->count == 0) {
		/* The queue is empty */
		return -1;
	}
	/* Retrieve data from tail of the queue */
	*d = q->els[q->tail].data;
	*dlen = q->els[q->tail].datalen;
	/* Reset data pointed to by the tail */
	q->els[q->tail].data=NULL;
	q->els[q->tail].datalen=0;
	/* Move the tail */
	q->tail = (q->tail + 1) % MAXQSZ;
	/* Decrease the number of elements in the queue and return it */
	return --(q->count);
}

/**
 * @brief Get the size of the queue
 * @param q Queue to check
 * @return The number of elements in the queue
 */
uint8_t queue_size(volatile QFIXED_T* q) {
	return q->count;
}

/**
 * @brief Check if the queue is full
 * @param q Queue to be checked
 *
 * @retval true If the queue is full
 * @retval false If the queue is not full
 */
bool queue_full(volatile QFIXED_T* q) {
	return q->count == MAXQSZ;
}

/**
 * @brief Get the size of the queue
 * @param q Queue to check
 * @return The number of elements in the queue
 */
uint8_t event_size(volatile QEVENT_T* q) {
	return q->count;
}

/**
 * @brief Add an event to an event queue
 *
 * @param q Queue in which the event should be put
 * @param t Event to put in the queue
 * @param d Data to store along the event
 * @param dlen Size of the data put along the event
 *
 * @return The size of the queue once the event was added
 * @retval -1 If the queue was full and the event could not be added
 */
int8_t add_event(volatile QEVENT_T* q, EVENTS t, void* d, uint16_t dlen) {
	LOG(LOG_STATES, "Add event %u to queue", t);
	if (q->count == MAXQSZ) {
		LOG(LOG_ERR, "The queue was full");
		/* The queue is full */
		return -1;
	}
	/* Fill data into head of the queue */
	q->evts[q->head].type = t;
	q->evts[q->head].data = d;
	q->evts[q->head].datalen = dlen;
	/* Move the head to the next available space in the buffer */
	q->head = (q->head + 1)%MAXQSZ;
	/* Increment the number of element in the queue and return it */
#ifdef SIMU
	wakeup_sm();
#endif
	return ++(q->count);
}

/**
 * @brief Add an event with no data to an event queue.
 *
 * @param q Queue in which the event should be put
 * @param t Event to put in the queue
 * @return The size of the queue once the event was added
 * @retval -1 If the queue was full and the event could not be added
 */
int8_t add_simple_event(volatile QEVENT_T* q, EVENTS t) {
	return add_event(q, t, NULL, 0);
}

// Return (and remove) next event type via params, returns new qsize if ok or -1 if empty
/**
 * @brief Get event from an event queue
 *
 * @param q Queue from which to extract the element
 * @param t Pointer to the event extracted from the queue
 * @param d Pointer to the data associated with the extracted event
 * @param dlen Size of the data associated with the extracted event
 *
 * @return The new size of the event queue
 * @retval -1 If the queue was empty
 */
int8_t get_event(volatile QEVENT_T* q, EVENTS* t, void** d, uint16_t *dlen) {
	if (q->count == 0) {
		/* The queue is empty */
		return -1;
	}
	/* Retrieve event and data from tail of the queue */
	*t = q->evts[q->tail].type;
	*d = q->evts[q->tail].data;
	*dlen = q->evts[q->tail].datalen;
	/* Reset data pointed to by the tail */
	q->evts[q->tail].type = STATE_ENTER;
	q->evts[q->tail].data = NULL;
	q->evts[q->tail].datalen = 0;
	/* Move the tail */
	q->tail = (q->tail + 1) % MAXQSZ;
	/* Decrease the number of elements in the queue and return it */
	return --(q->count);		// new q size
}


/**
 * @brief Add element to a statistics queue
 *
 * @param q Queue in which the element should be put
 * @param d Element to put in the queue
 *
 * @return The size of the queue once the element was added
 */
int8_t add_to_statqueue(volatile QSTAT_T* q, STAT_T d) {
	uint8_t i;
	uint8_t foundAtPos = MAXQSZ;
	uint8_t oldestRecordI = 0;

	/* Search the device using its device id */
	for(i = 0; i < q->count; i++) {
		if(q->els[i].deviceId == d.deviceId) {
			/* Update element */
			q->els[i] = d;
			foundAtPos = i;
		}
		else {
			if(q->els[i].lastSeen < q->els[oldestRecordI].lastSeen) {
				oldestRecordI = i;
			}
		}
	}
	/*
	 * If the device is not in the list, we try to add it.
	 * If the queue is full, we replace the oldest record.
	 */
	if(foundAtPos == MAXQSZ) {
		if (q->count == MAXQSZ) {
			/* Erase oldest record */
			q->els[oldestRecordI] = d;
		}
		else {
			q->els[q->count] = d;
			q->count++;
		}
	}
	return q->count;
}

/**
 * @brief Get the size of the queue
 * @param q Queue to check
 * @return The number of elements in the queue
 */
uint8_t statqueue_size(volatile QSTAT_T* q) {
	return q->count;
}

/** @} */
/** @} */
/** @} */
