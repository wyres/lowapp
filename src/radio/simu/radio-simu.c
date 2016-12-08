/**
 * @file radio-simu.c
 * @brief Simulation of the radio transmissions
 *
 * @author Nathan Olff
 * @date August 10, 2016
 */
#include "radio-simu.h"
#include "lowapp_msg.h"
#include "lowapp_log.h"
#include "activity_stat.h"
#include "configuration.h"
#include "sx1272_ex.h"

/* Inotify and thread related includes */
#include <sys/inotify.h>
#include <errno.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <math.h>

#ifdef SIMU
	#include "lowapp_sys_timer.h"
#else
	#include "lowapp_sys.h"
	extern LOWAPP_SYS_IF_T* _sys;
#endif

/**
 * @addtogroup lowapp_simu
 * @{
 */
/**
 * @addtogroup lowapp_simi_radio LoWAPP Simulation Radio
 * @brief Filesystem simulating radio transmissions
 * @{
 */

/** Name of the file used to simulate radio transmission */
char radioFile[50];
/** Name of the directory in which the radio files are */
const char radioSubdir[] = "Radio/";
/** Full path to the radio sub directory */
char radioDir[100] = {};

/**
 * Flag used to differentiate standard CAD to CAD for ACK
 *
 * This is done because the reception process for ACK is different in the
 * simulation to the reception of standard messages
 */
volatile bool cadAckFlag = false;

/** Radio Settings */
RadioSettings_t Settings;

/** Program's arguments */
extern struct arguments arguments;

/**
 * Data to be transmitted
 *
 * Shared variable between the state machine calling function (start_radio_tx)
 * and the radio thread
 */
void* tx_data;
/**
 * Size of the data to be transmitted
 *
 * Shared variable between the state machine calling function (start_radio_tx)
 * and the radio thread
 */
uint32_t tx_dlen;
/**
 * Preamble time
 *
 * Shared variable between the state machine calling function (start_radio_tx)
 * and the radio thread
 */
uint32_t tx_plen;

/** Default transmission time */
int transmission_duration = 100;

/** Radio thread */
pthread_t th_radio;

/** Timeout for current radio activity */
uint32_t radio_timeout;

/** Flag used to close properly the radio thread */
bool th_radio_running;

/**
 * @addtogroup lowapp_simu_radio_tx LoWAPP Simulation Radio Transmission
 * @brief Filesystem writes simulating radio transmissions
 * @{
 */

/** Pthread condition used to signal the radio thread */
pthread_cond_t cond_radio;
/** Radio mutex used in conjonction with cond_radio for signalling the radio thread */
pthread_mutex_t mutex_radio;

/** Radio callbacks */
RadioEvents_t events;

/** Pointer to the radio callbacks used in the radio driver files */
RadioEvents_t *RadioEvents;

/** Actual bandwidth values */
extern const uint32_t bandwidthValues[];

extern const uint32_t channelFrequencies[];


/**
 * Initialise the radio simulation and start its thread
 */
void simu_radio_init(Lowapp_RadioEvents_t *evt) {
	/* Set events structure for radio init */
	events.CadDone = evt->CadDone;
	events.RxDone = evt->RxDone;
	events.FhssChangeChannel = NULL;
	events.RxError = evt->RxError;
	events.RxTimeout = evt->RxTimeout;
	events.TxDone = evt->TxDone;
	events.TxTimeout = evt->TxTimeout;

	RadioEvents = &events;

	/* Initialise state at OFF */
	Settings.State = RF_IDLE;
	th_radio_running = true;
	/* Fill Settings structure */
	Settings.Modem = MODEM_LORA;
	Settings.LoRa.CrcOn = true;
	Settings.LoRa.FreqHopOn = false;
	Settings.LoRa.HopPeriod = false;
	Settings.LoRa.IqInverted = false;
	Settings.LoRa.LowDatarateOptimize = false;
	Settings.LoRa.RxContinuous = false;
	Settings.LoRa.FixLen = false;

	/* Set radio directory within root directory */
	strcpy(radioDir, arguments.directory);
	strcat(radioDir, radioSubdir);

	/* Create the radio folder in case it doesn't exist */
	mkdir(radioDir, 0777);

	start_generic_thread(&th_radio, thread_continuous_radio, NULL);
}

/**
 * Set the radio callbacks for the radio layer
 */
void simu_radio_setCallbacks(Lowapp_RadioEvents_t *evt) {
	/* Set events structure for radio init */
	events.CadDone = evt->CadDone;
	events.RxDone = evt->RxDone;
	events.FhssChangeChannel = NULL;
	events.RxError = evt->RxError;
	events.RxTimeout = evt->RxTimeout;
	events.TxDone = evt->TxDone;
	events.TxTimeout = evt->TxTimeout;
	setRadioCallbacks(&events);
}

/**
 * Update the path to the radio file
 */
void update_radio_file() {
	sprintf(radioFile, "%schannel-%lu", radioDir, Settings.Channel);
}

/**
 * Set radio channel
 */
void simu_radio_setChannel(uint32_t chan) {
	Settings.Channel = chan;
}



/**
 * Set radio RX config
 */
void simu_radio_setRxConfig(uint8_t bandwidth, uint8_t datarate, uint8_t coderate,
		uint16_t preambleLen, bool fixLen, uint8_t payloadLen, bool rxContinuous) {
	Settings.LoRa.Bandwidth = bandwidth;
	Settings.LoRa.Datarate = datarate;
	Settings.LoRa.Coderate = coderate;
	Settings.LoRa.PreambleLen = preambleLen;
	Settings.LoRa.symbTimeout = LOWAPP_SYMBOL_TIMEOUT;
	Settings.LoRa.FixLen = fixLen;
	Settings.LoRa.PayloadLen = payloadLen;
}

/**
 * Set radio TX config
 */
void simu_radio_setTxConfig(int8_t power, uint8_t bandwidth, uint8_t datarate,
		uint8_t coderate, uint16_t preambleLen, uint32_t timeout, bool fixLen) {
	Settings.LoRa.Power = power;
	Settings.LoRa.Bandwidth = bandwidth;
	Settings.LoRa.Datarate = datarate;
	Settings.LoRa.Coderate = coderate;
	Settings.LoRa.PreambleLen = preambleLen;
	Settings.LoRa.TxTimeout = timeout;
	Settings.LoRa.FixLen = fixLen;
}

/**
 * Send function for the radio (similar to SX1272)
 */
void simu_radio_send(uint8_t *data, uint8_t dlen) {
	LOG(LOG_DBG, "Start thread for radio transmission");
	/* Take the radio mutex to set shared variables */
	pthread_mutex_lock(&mutex_radio);
	setRadioActivity(RADIO_TX);
	/* Copy data, preamble and size to global variables */
	tx_data = (uint8_t*) calloc(dlen, sizeof(uint8_t));
	memcpy(tx_data, data, dlen);
	tx_dlen = dlen;
	/* Set state for the radio thread to TX */
	Settings.State = RF_TX_RUNNING;
	/* Signal the radio thread to wake up */
	pthread_cond_signal(&cond_radio);
	/* Release radio mutex to let the radio thread execute */
	pthread_mutex_unlock(&mutex_radio);
}

/**
 * Computes the time on air of the preamble
 *
 * @return Computed air time in s
 */
double simu_radio_transmissionTimePreamble() {
	double bw = bandwidthValues[Settings.LoRa.Bandwidth];
	 // Symbol rate : time for one symbol (secs)
	double rs = bw / ( 1 << Settings.LoRa.Datarate );
	double ts = 1 / rs;
	// time of preamble
	double tPreamble = ( Settings.LoRa.PreambleLen + 4.25 ) * ts;
	return tPreamble;
}

/**
 * Computes the packet time on air in us for the given payload
 *
 * @param pktLen Packet length
 * @return Computed air time in s for the given packet length
 */
double simu_radio_transmissionTimePayload(uint16_t pktLen) {
	double bw = bandwidthValues[Settings.LoRa.Bandwidth];

	// Symbol rate : time for one symbol (secs)
	double rs = bw / ( 1 << Settings.LoRa.Datarate );
	double ts = 1 / rs;

	// Symbol length of payload and time
	double tmp = ceil( ( 8 * pktLen - 4 * Settings.LoRa.Datarate +
						 28 + 16 * Settings.LoRa.CrcOn -
						 ( Settings.LoRa.FixLen ? 20 : 0 ) ) /
						 ( double )( 4 * Settings.LoRa.Datarate -
						 ( ( Settings.LoRa.LowDatarateOptimize > 0 ) ? 8 : 0 ) ) ) *
						 ( Settings.LoRa.Coderate + 4 );
	double nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );
	double tPayload = nPayload * ts;
	// Time on air
	return tPayload;
}

/**
 * Computes the packet time on air in us for the given payload
 *
 * @param pktLen Packet length
 * @return Computed air time in ms for the given packet length
 */
uint32_t simu_radio_timeOnAir(uint8_t pktLen) {
	double tPreamble, tPayload;
	tPreamble = simu_radio_transmissionTimePreamble();
	tPayload = simu_radio_transmissionTimePayload(pktLen);
	double tOnAir = tPreamble + tPayload;
	// return us secs
	double airTime = floor( tOnAir * 1e3 + 0.999 );
	return airTime;
}

/**
 * Radio thread, running continuously to simulate the LoRa radio
 *
 * The thread spend most of its time waiting on a pthread_cond. It is signaled
 * for wake up by the system level function for loraTx, loraRx and CAD.
 *
 * @param arg Thread arg (not used)
 */
void* thread_continuous_radio(void *arg) {
	/* Generate random number to simulate transmission failures */
	uint8_t fail_generator;
	while(th_radio_running) {
		/* Wait on condition */
	    pthread_mutex_lock(&mutex_radio);
	    while(Settings.State == RF_IDLE) {
	    	pthread_cond_wait(&cond_radio, &mutex_radio);
	    }
	    fail_generator = rand() % 100;
	    switch(Settings.State) {
	    case RF_TX_RUNNING:
    		if(fail_generator >= FAILURE_RANDOM_START_TX) {
				if(tx_data == NULL) {
					LOG(LOG_ERR, "Data not set");
					break;
				}
				LOG(LOG_INFO, "Radio thread transmitting...");
				/* Preamble */
				radio_tx_preamble();
				/* Write */
				radio_tx_write(tx_data, tx_dlen);
				/* End of transmission */
				radio_tx_eof();
    		}
    		else {
    			LOG(LOG_INFO, "Simulating TX failure");
    		}
			/* Free frame buffer */
			free(tx_data);
			tx_data = NULL;
	    	break;
	    case RF_RX_RUNNING:
    		if(fail_generator >= FAILURE_RANDOM_START_RX) {
    			radio_rx(radio_timeout);
    		}
    		else {
    			LOG(LOG_INFO, "Simulating RX failure");
    		}
	    	break;
	    case RF_CAD:
	    	/*
	    	 * CAD and RX are done at once for ACK because the preamble is smaller
	    	 */
	    	if(cadAckFlag) {
		    	rx_ack(radio_timeout);
	    	}
	    	else {
	    		cad_for_standard_rx();
	    	}
	    	break;
	    default:
	    	break;
	    }
		Settings.State = RF_IDLE;
    	setRadioActivity(RADIO_OFF);
    	writeRadioActivity();
		pthread_mutex_unlock(&mutex_radio);
	}

	pthread_exit(NULL);
}


/**
 * Start transmission process
 *
 * 	1. <b>We create an empty file and set a timer for preamble</b>
 * 	2. We write the data and set a timer for transmission time
 * 	3. We remove the file
 *
 * @retval 0 On success
 * @retval -1 Otherwise
 */
int radio_tx_preamble() {
	LOG(LOG_PARSER, "Start transmission process (radio_tx)");
	FILE *fp;
	update_radio_file();
	/* Create empty file */
	fp = fopen(radioFile, "w");
	if(fp == NULL) {
		return -1;
	}
	fclose(fp);

	LOG(LOG_RADIO, "Set preamble timer for %u ms", (uint16_t)floor(simu_radio_transmissionTimePreamble()*1000));
	radio_processing_sleep((uint16_t)floor(simu_radio_transmissionTimePreamble()*1000));	/* Set preamble timer */
	return 0;
}

/**
 * End of preamble handler, writing data to the radio file
 *
 * Transmission process:
 *
 * 	1. We create an empty file and set a timer for preamble
 * 	2. <b>We write the data and set a timer for transmission time</b>
 * 	3. We remove the file
 *
 * @param data Data frame to send
 * @param dlen Size of the frame
 */
void radio_tx_write(void* data, uint32_t dlen) {
	/* Get time to subtract when setting the timer */
	uint64_t startTime = get_time_ms();
	/* Used by log parser */
	LOG(LOG_PARSER, "Writing data into the file (preamble handler)");
	/* Compute time on air of the frame */
	transmission_duration = floor(simu_radio_transmissionTimePayload(dlen)*1000);

	FILE *fp;
	update_radio_file();
	fp = fopen(radioFile, "w");
	if(fp == NULL) {
		return;
	}
	/* Write data in binary in the file */
	fwrite(data, 1, dlen, fp);
	fclose(fp);

	/* Actual transmission time #TODO use actual data using SF */
	/* Sets timer to simulate actual transmission */
	LOG(LOG_RADIO, "Set transmission timer for %lu ms", transmission_duration
			-(get_time_ms()-startTime));
	radio_processing_sleep(transmission_duration-(get_time_ms()-startTime));
}

/**
 *  End of transmission, deleting the radio file
 *
 *  Transmission process:
 *
 * 	1. We create an empty file and set a timer for preamble
 * 	2. We write the data and set a timer for transmission time
 * 	3. <b>We remove the file</b>
 */
void radio_tx_eof() {
	LOG(LOG_RADIO, "Transmission finished");
	update_radio_file();
	remove(radioFile);	/* Delete the radio file */
	if(RadioEvents->TxDone != NULL)
		RadioEvents->TxDone();
}
/** @} */

/**
 * @addtogroup lowapp_simu_radio_cad LoWAPP Simulation Radio CAD
 * @brief Filesystem checks simulating radio CAD
 * @{
 */

/**
 * Listen Before Talk
 *
 * Check if the radio channel is free for a transmission
 *
 * @param chan Radio channel to check
 * @retval True If the channel is free
 * @retval False If the channel is not free
 */
bool simu_radio_lbt(uint8_t chan) {
	int ret;

	Settings.Channel = channelFrequencies[chan];

	update_radio_file();

	if(file_exists(radioFile) == 1) {
		return false;
	}

	ret = inotify_create(chan, Settings.LoRa.Datarate, CHAN_FREE_TIMEOUT);
	/*
	 * If the file is created during timeout, return 1,
	 * if no file was created return 0,
	 * if error while setting notify, return -1
	 */
	if(ret == 0 || (ret & IN_DELETE))	// File was deleted or event occurred
		return true;
	else if(ret > 0)
		return false;
	else
		return false;
}

/**
 * Signal radio thread to start CAD process
 */
void simu_radio_cad() {
	LOG(LOG_RADIO, "Start thread for CAD");
	if(pthread_mutex_lock(&mutex_radio) == 0) {
		setRadioActivity(RADIO_CAD);
		Settings.State = RF_CAD;
		cadAckFlag = false;
		pthread_cond_signal(&cond_radio);
		pthread_mutex_unlock(&mutex_radio);
	}
}

/**
 * CAD for standard message
 *
 * This function checks for the preamble on the radio file and calls
 * the CadDone radio callback when finished.
 *
 * @return The events received by inotify
 * @retval 0 If the radio file was not empty (preamble passed)
 */
int cad_for_standard_rx() {
	int evt;
	int ret = 0;
	bool fileExists = false;

	fileExists = check_radio_file_exists();
	/* Look for something */
	if(!fileExists) {
		evt = inotify_create(Settings.Channel, Settings.LoRa.Datarate, ceil(get_symbol_time()/1000.0));
	}

	if(fileExists || (evt & (IN_CREATE | IN_CLOSE_WRITE))) {	// File was written and saved
		int16_t size;
		size = get_file_size(radioFile);
		if(size == 0) {	// Preamble
			LOG(LOG_INFO, "Preamble detected\nWaiting for message");
			ret = 1;
		}
		else if(size > 0) {	// Data transmission
			LOG(LOG_ERR, "Message received too early");
			ret = 2;
		}
		else {	// Error while reading file size
			LOG(LOG_ERR, "Error checking the size of the file %s", radioFile);
			ret = 0;
		}
	}

	/* Only send CAD done if the file was empty */
	if(RadioEvents->CadDone != NULL)
		(RadioEvents->CadDone)(ret == 1);
	return evt;
}




/**
 * Initialise inotify
 *
 * @param[in] chan Radio channel to check
 * @param[in] sf Spreading factor to check
 * @param[out] radioFileChannelToCheck Radio file to check, corresponding to the channel
 * @param[out] fd File descriptor used by inotify
 * @param[out] wd Watch descriptor used by inotify
 * @param[out] nfds Number of elements in pollfd
 * @param[out] fds File descriptor for polling function
 * @retval -1 If an error occurred
 * @retval 0 On success
 */
int initialise_inotify(uint32_t chan, uint8_t sf, char *radioFileChannelToCheck,
		int* fd, int* wd, nfds_t* nfds, struct pollfd* fds) {

	/* Set the radio file to check using the channel */
	sprintf(radioFileChannelToCheck, "channel-%lu", chan);

	/* Create the file descriptor for accessing the inotify API */
	*fd = inotify_init1(IN_NONBLOCK);
	if (*fd == -1) {
		perror("inotify_init1");
		return -1;
	}

	/*
	 * Mark directories for events
	 * 	   - file was created
	 * 	   - file was written and closed
	 */
	*wd = inotify_add_watch(*fd, radioDir, IN_CREATE | IN_CLOSE_WRITE | IN_DELETE);
	if (*wd == -1) {
		fprintf(stderr, "Cannot watch '%s'\n", radioDir);
		perror("inotify_add_watch");
		return -1;
	}

	/* Prepare for polling */
	*nfds = 1;

	/* Inotify input */
	fds->fd = *fd;
	fds->events = POLLIN;
	return 0;
}



/**
 * Check for activity on a specific folder/file on the filesystem using inotify
 *
 * @param chan Radio channel
 * @param sf Spreading factor
 * @param timeoutms Timeout in ms
 * @return The events received by inotify
 * @retval -1 If an error related to inotify occurred
 */
int inotify_create(uint32_t chan, uint8_t sf, uint16_t timeoutms) {
	/* File descriptors used by inotify */
	int fd, poll_num;
	int wd;
	nfds_t nfds;
	struct pollfd fds;
	int events = 0;	/* Number of valid events received */
	char radioFileChannelToCheck[50];	/* Channel specific file */

	/* Initialise inotify */
	if(initialise_inotify(chan, sf, radioFileChannelToCheck, &fd, &wd, &nfds, &fds) == -1) {
		return -1;
	}
	/* Wait for events */
	while (1) {
		LOG(LOG_RADIO, "Start polling");
		/* Poll the directory for timeoutms */
		poll_num = poll(&fds, nfds, timeoutms);
		LOG(LOG_RADIO, "Poll function returned (%d)", poll_num);
		if (poll_num == -1) {
			if (errno == EINTR)
				continue;
			events = -1;
			break;
		}
		else if(poll_num == 0) {
			events = 0;
			break;
		}

		if (poll_num > 0) {
			if (fds.revents & POLLIN) {
				/* Inotify events are available */
				events = handle_events(fd, wd, radioDir, radioFileChannelToCheck);
				/* Break as soon as a valid event is detected */
				if((events & IN_CLOSE_WRITE) | (events & IN_DELETE))
					break;
			}
		}
	}

	/* Close inotify file descriptor */
	close(fd);
	return events;
}

/**
 * Check for 2 distinct activities on a specific folder/file on the filesystem using inotify
 *
 * We are using the same file descriptor by polling two times to get at
 * the same time the start of the preamble and the write of the data.
 *
 * @param chan Radio channel
 * @param sf Spreading factor
 * @param timeout1ms Timeout in ms for the first poll (preamble)
 * @param timeout2ms Timeout in ms for the second poll (write)
 * @return The events received by the second polling through inotify
 * @retval -1 If an error related to inotify occurred
 */
int inotify_create2(uint32_t chan, uint8_t sf, uint16_t timeout1ms, uint16_t timeout2ms) {
	/* File descriptors used by inotify */
	int fd, poll_num;
	int wd;
	nfds_t nfds;
	struct pollfd fds;
	int events = 0;		/* Events received by inotify */
	char radioFileChannelToCheck[50];	/* Channel specific file */
	uint16_t timeoutms[2] = {timeout1ms, timeout2ms};

	/* Initialise inotify */
	if(initialise_inotify(chan, sf, radioFileChannelToCheck, &fd, &wd, &nfds, &fds) == -1) {
		return -1;
	}

	int nPoll = 0;
	/* Wait for events */
	while (1) {
		LOG(LOG_RADIO, "Start polling");
		/* Poll the directory for timeoutms */
		poll_num = poll(&fds, nfds, timeoutms[nPoll]);
		LOG(LOG_RADIO, "Poll function returned (%d)", poll_num);
		if (poll_num == -1) {
			if (errno == EINTR)
				continue;
			events = -1;
			break;
		}
		else if(poll_num == 0) {
			events = 0;
			break;
		}

		if (poll_num > 0) {
			if (fds.revents & POLLIN) {
				/* Inotify events are available */
				events = handle_events(fd, wd, radioDir, radioFileChannelToCheck);
				/* Break as soon as a valid event is detected */
				if((events & IN_CLOSE_WRITE) | (events & IN_DELETE)) {
					/* Count the number of successfull poll returns */
					nPoll++;
					/* Only break after the second poll */
					if(nPoll == 2) {
						break;
					}
				}
			}
		}
	}

	LOG(LOG_RADIO, "Closing file descriptor for inotify");
	/* Close inotify file descriptor */
	close(fd);
	return events;
}


/**
 * Read all available inotify events from the file descriptor 'fd'
 *
 * @param fd File description
 * @param wd Watch description for the directory
 * @param toWatch Directory watched by inotify
 * @param fileToCheck Radio file we are actually interested in (throw events on
 * other files)
 * @return The events received by inotify
 * @retval -1 If an error related to inotify occurred
 */
int handle_events(int fd, int wd, char* toWatch, char* fileToCheck)
{
	/*
	 * Some systems cannot read integer variables if they are not
	 * properly aligned. On other systems, incorrect alignment may
	 * decrease performance. Hence, the buffer used for reading from
	 * the inotify file descriptor should have the same alignment as
	 * struct inotify_event.
	 */
	char buf[4096]
		__attribute__ ((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;
	ssize_t len;
	char *ptr;
	int events = 0;

	/* Loop while events can be read from inotify file descriptor. */
	while (events == 0) {
		/* Read some events. */
		len = read(fd, buf, sizeof buf);
		if (len == -1 && errno != EAGAIN) {
			printf("read error");
			exit(-1);
		}

		/*
		 * If the nonblocking read() found no events to read, then
		 * it returns -1 with errno set to EAGAIN. In that case,
		 * we exit the loop.
		 */
		if (len <= 0)
			break;

		/* Loop over all events in the buffer */
		for (ptr = buf; ptr < buf + len;
				ptr += sizeof(struct inotify_event) + event->len) {

			event = (const struct inotify_event *) ptr;

			/* Check the event and the file created */
			if (!(event->mask & IN_ISDIR) && event->len) {
				if(strcmp(event->name, fileToCheck) == 0) {
					/* Wait for the file to be created */
					if(event->mask & IN_CREATE) {
						events |= IN_CREATE;
					}
					/* Wait for the file to be written and closed */
					else if(event->mask & IN_CLOSE_WRITE) {
						events |= IN_CLOSE_WRITE;
					}
					else if(event->mask & IN_DELETE) {
						events |= IN_DELETE;
					}
				}
			}
		}
	}
	return events;
}

/** @} */

/**
 * @addtogroup lowapp_simu_radio_rx_ack LoWAPP Simulation Radio ACK reception
 * @brief Both CAD and RX processes are run at the same time when receiving ACK
 * @{
 */


/**
 * Start reception for ACK (CAD + RX)
 *
 * @param timeoutms Reception timeout in ms
 */
int8_t simu_radio_rxing_ack(uint32_t timeoutms) {
	int8_t ret = 0;
	LOG(LOG_RADIO, "Start thread RX ACK");
	if(pthread_mutex_lock(&mutex_radio) == 0) {
		radio_timeout = timeoutms;
		setRadioActivity(RADIO_RX);
		cadAckFlag = true;
		Settings.State = RF_CAD;
		pthread_cond_signal(&cond_radio);
		pthread_mutex_unlock(&mutex_radio);
	}
	else {
		ret = -1;
	}
	return ret;
}

/**
 * Simulation specific function for (small) CAD and reception of ACK in one step
 *
 * @param timeoutms Reception timeout in ms
 */
void rx_ack(uint32_t timeoutms) {
	/* Simulation specific CAD call. Needed for inotify to work properly with short preamble */
	uint64_t startRxTime = get_time_ms();
	int16_t size = simu_blocking_cad_for_rx_ack(Settings.Channel, Settings.LoRa.Datarate, TIMER_ACK_SLOT_LENGTH, TIMER_BLOCK_PREAMBLE_TIME_ACK);
	if(size > 0) {
		radio_read(size, timeoutms-(uint32_t)(get_time_ms()-startRxTime));
		setRadioActivity(RADIO_OFF);
	}
	else if(size == 0) {
		LOG(LOG_ERR, "Empty file found");
		if(RadioEvents->RxError != NULL)
			RadioEvents->RxError();
		setRadioActivity(RADIO_OFF);
	}
	else {
		// Error while reading file size
		LOG(LOG_ERR, "Error checking the size of the file %s", radioFile);
		if(RadioEvents->RxError != NULL)
			RadioEvents->RxError();
		setRadioActivity(RADIO_OFF);
	}
}

/**
 * Simulation specific wrapper for ACK CAD
 *
 * In practice, no CAD will be made for the reception of ACK. In Simulation this
 * step is required because we need to be sure the empty file was created before
 * reading the content of the radio file.
 *
 * @param chan Radio channel to check
 * @param sf Spreading factor to check
 * @param timeoutStartMs Timeout for the start of the preamble in ms
 * @param timeoutPreMs Timeout for end of preamble activivity in ms
 * @retval -1 If an error occurred
 * @return The size of the file detected after two inotify activities
 */
int simu_blocking_cad_for_rx_ack(uint32_t chan, uint8_t sf, uint16_t timeoutStartMs, uint16_t timeoutPreMs) {
	int ret = -1;
	int evt;
	bool fileExists = false;
	fileExists = check_radio_file_exists();
	/* Look for something */
	if(fileExists) {
		/* If the file already exists, we wait for only one inotify event */
		evt = inotify_create(chan, sf, timeoutStartMs);
	}
	else {
		/* We wait for two series of inotify events (both preamble and write) */
		evt = inotify_create2(chan, sf, timeoutStartMs, timeoutPreMs);
	}
	/* Check the events that occured at the second polling */
	if(fileExists || (evt & (IN_CREATE | IN_CLOSE_WRITE))) {	// File was written and saved
		LOG(LOG_DBG, "Before check size");
		ret = get_file_size(radioFile);	/* Return size if ok */
	}

	return ret;
}

/** @} */

/**
 * @addtogroup lowapp_simu_radio_rx LoWAPP Simulation Radio Reception
 * @brief Filesystem reads simulating radio reception
 * @{
 */

/**
 * Signal radio thread to start reception process
 * @param timeout Timeout (in ms) for reception process
 */
void simu_radio_rx(uint32_t timeout) {
	LOG(LOG_STATES, "Kick Radio RX");
	pthread_mutex_lock(&mutex_radio);
	radio_timeout = timeout;
	setRadioActivity(RADIO_RX);
	Settings.State = RF_RX_RUNNING;
	pthread_cond_signal(&cond_radio);
	pthread_mutex_unlock(&mutex_radio);
	LOG(LOG_STATES, "End kick Radio RX");
}

/**
 * Start reception process
 *
 * 	1. <b>Wait for the radio file to be written into</b>
 * 	2. <b>Read data from the file</b>
 * 	3. Wait for the file to be removed before taking into account the data
 *
 * @param timeoutms Reception timeout in ms
 * @retval 0 On success
 * @retval -1 Otherwise
 */
int radio_rx(uint32_t timeoutms) {
	LOG(LOG_PARSER, "Start reception process (radio_rx), timeout = %d", timeoutms);	/* Used by log parser */
	int evt;
	uint64_t startRxTime = get_time_ms();
	/* Wait for the end of the preamble */
	evt = inotify_create(Settings.Channel, Settings.LoRa.Datarate,
			(uint16_t)floor(simu_radio_transmissionTimePreamble()*1000*1.2));
	/* If the file was written into */
	if(evt & IN_CLOSE_WRITE) {
		/* Get file size */
		int16_t size = get_file_size(radioFile);
		if(size > 0) {
			return radio_read(size, timeoutms-(uint32_t)(get_time_ms()-startRxTime));
		}
		else if(size == 0) {
			LOG(LOG_ERR, "Empty file found");
			if(RadioEvents->RxError != NULL)
				RadioEvents->RxError();
			return -1;
		}
		else {
			/* Error while reading file size */
	    	LOG(LOG_ERR, "Error checking the size of the file %s", radioFile);
			if(RadioEvents->RxError != NULL)
				RadioEvents->RxError();
			return -1;
		}
	}
	else {
		LOG(LOG_ERR, "No inotify event detected");
		if(RadioEvents->RxTimeout != NULL)
			RadioEvents->RxTimeout();
		return -1;
	}
}

/**
 * Read the data from the file
 *
 * Reception process
 *
 *  1. Wait for the radio file to be written into
 *  2. Read data from the file
 *  3. <b>Wait for the file to be removed before taking into account the data</b>
 *
 * @param size Size of the data to read
 * @param timeoutms Reception timeout in ms
 * @retval 0 If success
 * @retval -1 If the file could not be read
 */
int radio_read(int size, uint32_t timeoutms) {
	uint8_t* buf;
	int ret;
	FILE *fp;
	/* Used by log parser */
	LOG(LOG_PARSER, "Reading data from the file (radio_read)");

	/* Compute time on air of the frame */
	transmission_duration = timeoutms;

	if(Settings.LoRa.FixLen && size != ACK_FRAME_LENGTH) {
		LOG(LOG_ERR, "Size did not matched the expected fix length");
		if(RadioEvents->RxError != NULL)
			RadioEvents->RxError();
		return -1;
	}
	/* Allocate memory for the data using the size of the file */
	buf = calloc(size, sizeof(uint8_t));
	if(buf == NULL) {
		LOG(LOG_ERR, "Buffer could not be allocated (%d)", errno);
		if(RadioEvents->RxError != NULL)
			RadioEvents->RxError();
		return -1;
	}

	update_radio_file();
	fp = fopen(radioFile, "r");
	if(fp == NULL) {
		LOG(LOG_ERR, "File descriptor could not be opened");
		if(RadioEvents->RxError != NULL)
			RadioEvents->RxError();
		return -1;
	}
	/* Write data in binary in the file */
	ret = fread(buf, 1, size, fp);
	if(ferror(fp)) {
		ret = -1;
	}

	fclose(fp);
	/* Wait for the transmission to be finished */

	int evt;
	if(ret != -1) {
		/* Wait for the radio file to be deleted (transmission duration + 50%) */
		evt = inotify_create(Settings.Channel, Settings.LoRa.Datarate, transmission_duration*1.5);
		/* If the file was written into */
		if(evt & IN_DELETE) {
			/* Call RxDone function */
			if(RadioEvents->RxDone != NULL)
				(RadioEvents->RxDone)(buf, ret, 0, 0);
		}
		else if(evt == 0) {	/* No event detected */
			free(buf);	/* Free buffer */
			if(RadioEvents->RxTimeout != NULL)
				(RadioEvents->RxTimeout)();
		}
		else {	/* Unexpected event occurred */
			free(buf);	/* Free buffer */
			LOG(LOG_ERR, "Unexpected inotify event");
			if(RadioEvents->RxError != NULL)
				(RadioEvents->RxError)();
		}
	}
	else {	/* An error occurred during reading */
		free(buf);	/* Free buffer */
		LOG(LOG_ERR, "Error while reading data radio file");
		if(RadioEvents->RxError != NULL)
			(RadioEvents->RxError)();
	}
	return ret;
}

/** @} */

/**
 * Initialise a timer with its handler function
 *
 * @param signal Signal used to trigger handler call
 * @param handler Handler to be executed after pTimer
 * @param pTimer Timout in ms
 * @return
 */
int init_radio_timer(int signal, void(*handler)(sigval_t), timer_t* pTimer) {
	struct sigevent sev;

	/* Create the timer */
	sev.sigev_notify = SIGEV_THREAD;	/* Events are managed in new threads */

	sev.sigev_notify_attributes = NULL;
	sev.sigev_notify_function = handler;

	sev.sigev_signo = signal;
	sev.sigev_value.sival_ptr = pTimer;
	return timer_create(CLOCK_MONOTONIC , &sev, pTimer);
}

/**
 * Sleep radio thread for some milliseconds
 *
 * @param timems Time in ms
 */
void simu_delayMs(uint32_t timems) {
	struct itimerspec its;

	/* Convert into s and ns */
	its.it_value.tv_sec = timems / 1000;
	its.it_value.tv_nsec = (timems % 1000)*1000000;
	its.it_interval.tv_sec = 0;	/* Launch only once */
	its.it_interval.tv_nsec = 0;

	/* Sleep */
	nanosleep(&(its.it_value), NULL);
}

/**
 * Sleep radio thread for some milliseconds
 *
 * @param timems Time in ms
 */
int radio_processing_sleep(uint32_t timems) {
	struct itimerspec its;

	/* Convert into s and ns */
	its.it_value.tv_sec = timems / 1000;
	its.it_value.tv_nsec = (timems % 1000)*1000000;
	its.it_interval.tv_sec = 0;	/* Launch only once */
	its.it_interval.tv_nsec = 0;

	/* Sleep */
	return nanosleep(&(its.it_value), NULL);
}

/**
 * Start a generic thread
 *
 * @param th Thread id
 * @param handler Handler function to be called
 * @param arg Arguments send to the handler
 * @retval 0 On success
 * @retval -1 Otherwise
 */
int start_generic_thread(pthread_t *th, void*(*handler)(void *arg), void *arg) {
	pthread_attr_t attr;
	int res;

	res = pthread_attr_init(&attr);
	if (res != 0) {
		perror("Attribute init failed");
		return -EXIT_FAILURE;
	}

    if (pthread_create(th, &attr, handler, arg)) {
    	perror("pthread_create");
    	return -EXIT_FAILURE;
	}
    return 0;
}

/**
 * Stop radio thread
 */
void stop_radio_thread() {
	printf("Stop radio thread\n");
	/* Wake up the radio thread */
	pthread_mutex_lock(&mutex_radio);
	th_radio_running = false;
	Settings.State = RF_IDLE;
	pthread_cond_signal(&cond_radio);
	pthread_mutex_unlock(&mutex_radio);
}

/** @} */
/** @} */


/**
 * Get the size of a file in bytes
 *
 * @param filename File to check
 * @return The size of the file in bytes
 * @retval -1 If the file could not be checked
 */
int16_t get_file_size(const char* filename) {
	LOG(LOG_STATES, "Check file size of %s", filename);
    struct stat st;
    if(stat(filename, &st) != 0) {
    	LOG(LOG_ERR, "Error code %d", errno);
        return -1;
    }
    return (int16_t)st.st_size;
}

/**
 * Check that the radio file exists
 *
 * Fix the name of the radioFile and check that it exists
 * @retval true If the radio file exists
 * @retval false If the radio file does not exists
 */
bool check_radio_file_exists() {
	update_radio_file();
	/* Is the file already there ? */
	if(file_exists(radioFile) == 1) {
		LOG(LOG_RADIO, "File exists");
		return true;
	}
	else {
		return false;
	}
}

/**
 * Check that a file exists
 *
 * @param path Path of the file to check
 * @retval true If the file exists
 * @retval false If it does not exist
 */
bool file_exists(char* path) {
	if(access( path, F_OK ) != -1) {
		return 1;
	}
	else {
		return 0;
	}
}

/**
 * Generate 32-bit random value
 */
uint32_t simu_radio_random(void) {
	return rand();
}


