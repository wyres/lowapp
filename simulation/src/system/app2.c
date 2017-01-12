/**
 * @file app2.c
 *
 * @brief Application code 2 : Receiving every 8 seconds with pollrx
 *
 * Application code used for testing general functionality and
 * lowapp_cmd calls without using stdin.
 *
 * @author Nathan Olff
 * @date October 4, 2016
 */
#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>
#include <sys/syscall.h>
#include <argp.h>
#include <signal.h>

#include "configuration.h"
#include "radio-simu.h"
#include "console.h"
#include "lowapp_if.h"
#include "lowapp_core.h"
#include "lowapp_shared_res.h"

//extern ConfigNode_t myConfig;
//extern int8_t cad_return;

LOWAPP_SYS_IF_T _lowappSysIf;

extern pthread_t th_console;
extern bool th_console_running;

extern pthread_t th_radio;

extern pthread_mutex_t mutex_wakeup;
extern pthread_cond_t cond_wakeup;

void releaseResources();
void quitIRQ(int arg);
void register_sigint_handler(void (*handler)(int));
void register_sys_functions(LOWAPP_SYS_IF_T *lowappSys);



static char doc[] =
		"lowapp_simu -- Simulation running LoRa-based LoWAPP protocol as Linux processes";


static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	struct arguments *arguments = state->input;

	switch (key) {
	case 'u':
		arguments->uuid = arg;
		break;
	case 'c':
		arguments->config = arg;
		break;
	case 'd':
		arguments->directory = arg;
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}


static struct argp_option options[] = {
		{ "uuid", 'u', "UUID", 0, "UUID of the node file, stored in DIRECTORY/Nodes/" },
		{ "config", 'c', "CONFIG_FILE", 0, "Relative path to the node configuration file from DIRECTORY or working directory" },
		{ "directory", 'd', "DIRECTORY", 0, "Root directory of the simulation (with Radio/ and Nodes/ subdirectories)" },
		{ 0 } };

static struct argp argp = { options, parse_opt, NULL, doc };

extern struct arguments arguments;




int main(int argc, char* argv[]) {
	arguments.config = NULL;
	arguments.uuid = NULL;
	/* Default root directory for simulation is working directory */
	arguments.directory = "./";

	/* Process program's arguments using argp */
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	/* Initialise logging system */
	init_log();
	/* Initialise node */
	if (node_init(&arguments) < 0) {
		return -1;
	}

	/* Setup timers */
	init_timer();
	init_repet_timer();

	/* Set system level functions for the core */
	register_sys_functions(&_lowappSysIf);

	/* Initialise LoWAPP core */
	lowapp_init(&_lowappSysIf);

	start_thread_cmd();

	register_sigint_handler(quitIRQ);
	set_log_level(0);
	uint64_t startTime;
	startTime = get_time_ms();
	while (1) {
		/* Run state machine indefinitely */
		lowapp_process();

		if(get_time_ms()-startTime > 8000) {
			lowapp_atcmd("at+pollrx");
			printf("TIME OUT\n");
			startTime = get_time_ms();
		}

		/* Sleep until next event occurs */
		lock_wakeUp();
		pthread_cond_wait(&cond_wakeup, &mutex_wakeup);
		unlock_wakeUp();
	}
	printf("End of program\r\n");
	releaseResources();
	return 0;
}


void register_sys_functions(LOWAPP_SYS_IF_T *lowappSys) {
	lowappSys->SYS_getTimeMs = get_time_ms;
	lowappSys->SYS_setTimer = set_timer1;
	lowappSys->SYS_cancelTimer = cancel_timer1;
	lowappSys->SYS_setTimer2 = set_timer2;
	lowappSys->SYS_cancelTimer2 = cancel_timer2;
	lowappSys->SYS_setRepetitiveTimer = set_repet_timer;
	lowappSys->SYS_cancelRepetitiveTimer = cancel_repet_timer;
	lowappSys->SYS_delayMs = simu_delayMs;
	lowappSys->SYS_getConfig = get_config;
	lowappSys->SYS_setConfig = set_config;
	lowappSys->SYS_writeConfig = save_configuration;
	lowappSys->SYS_readConfig = read_configuration;
	lowappSys->SYS_cmdResponse = cmd_response;
	lowappSys->SYS_radioTx = simu_radio_send;
	lowappSys->SYS_radioCAD = simu_radio_cad;
	lowappSys->SYS_radioLBT = simu_radio_lbt;
	lowappSys->SYS_radioRx = simu_radio_rx;
	lowappSys->SYS_radioInit = simu_radio_init;
	lowappSys->SYS_radioSetTxConfig = simu_radio_setTxConfig;
	lowappSys->SYS_radioSetRxConfig = simu_radio_setRxConfig;
	lowappSys->SYS_radioTimeOnAir = simu_radio_timeOnAir;
	lowappSys->SYS_radioSetChannel = simu_radio_setChannel;
	lowappSys->SYS_initTimer = init_timer1;
	lowappSys->SYS_initTimer2 = init_timer2;
	lowappSys->SYS_initRepetitiveTimer = init_repet_timer;
	lowappSys->SYS_random = simu_radio_random;
	lowappSys->SYS_radioSleep = dummy;
	lowappSys->SYS_radioSetPreamble = setPreambleLength;
	lowappSys->SYS_radioSetRxFixLen = setRxFixLen;
	lowappSys->SYS_radioSetTxFixLen = setTxFixLen;
	lowappSys->SYS_radioSetTxTimeout = setTxTimeout;
	lowappSys->SYS_radioSetRxContinuous = setRxContinuous;
	lowappSys->SYS_radioSetCallbacks = simu_radio_setCallbacks;
}


void releaseResources() {
	printf("Ctrl+C received\r\n");
	th_console_running = false;
	pthread_kill(th_console, SIGNAL_CONSOLE_END);
	pthread_join(th_console, NULL);	// Wait for the console thread to stop
	printf("console thread joined\n");
	stop_radio_thread();
	pthread_join(th_radio, NULL);
	printf("radio thread joined\n");
	clean_mutex();
	clean_timer();
	clean_repet_timer();
	clean_queues();
	printf("main exit\n");
	exit(0);
}

void register_sigint_handler(void (*handler)(int)) {
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
}

void quitIRQ(int arg) {
	releaseResources();
}

