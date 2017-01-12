/**
 * @file main.c
 * @brief Main file of the Linux simulation
 *
 * @author Nathan Olff
 * @date August 1, 2016
 */

#include <activity_stat.h>
#include <argp.h>
#include <configuration.h>
#include <console.h>
#include <lowapp_core.h>
#include <lowapp_if.h>
#include <lowapp_log.h>
#include <lowapp_shared_res.h>
#include <lowapp_sys.h>
#include <lowapp_sys_timer.h>
#include <pthread.h>
#include <radio-simu.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/** Group system level functions for the LoWAPP core */
LOWAPP_SYS_IF_T _lowappSysIf;

/** Thread managing the console inputs (for AT commands) */
extern pthread_t th_console;
extern bool th_console_running;

/** Thread managing the radio */
extern pthread_t th_radio;

extern pthread_mutex_t mutex_wakeup;
extern pthread_cond_t cond_wakeup;

void releaseResources();
void quitIRQ(int arg);
void register_sigint_handler(void (*handler)(int));
void register_sys_functions(LOWAPP_SYS_IF_T *lowappSys);

/**
 * @addtogroup lowapp_simu
 * @{
 */

/**
 * @addtogroup lowapp_simu_argp Argument processing using Linux argp
 * @{
 */
/**
 * Program documentation string for --help option
 */
static char doc[] =
		"lowapp_simu -- Simulation running LoRa-based LoWAPP protocol as Linux processes";

/**
 * Parser function for argp
 * @param key Key of the parsed argument
 * @param arg Value of the parsed argument
 * @param state Parsing state used by argp
 * @return Error code if an unkown argument was found
 */
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

/**
 * List of options to be recognized by argp
 */
static struct argp_option options[] = {
		{ "uuid", 'u', "UUID", 0, "UUID of the node file, stored in DIRECTORY/Nodes/" },
		{ "config", 'c', "CONFIG_FILE", 0, "Relative path to the node configuration file from DIRECTORY or working directory" },
		{ "directory", 'd', "DIRECTORY", 0, "Root directory of the simulation (with Radio/ and Nodes/ subdirectories)" },
		{ 0 } };

/**
 * The ARGP structure itself
 */
static struct argp argp = { options, parse_opt, NULL, doc };

extern struct arguments arguments;

/**
 * Reboot flag for reseting device
 */
bool reboot;

/** @} */

/**
 * Main function, entry point of the program
 */
int main(int argc, char* argv[]) {
	do {
		reboot = false;
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

		/* Init activity logger	*/
		initActivities(arguments.directory, arguments.uuid);

		/* Init mutexes */
		init_mutexes();

		/* Start reandom number generator */
		srand(time(NULL)+arguments.uuid[0]+arguments.uuid[1]);

		/* Set system level functions for the core */
		register_sys_functions(&_lowappSysIf);

		/* Initialise LoWAPP core */
		lowapp_init(&_lowappSysIf);
		start_thread_cmd();

		register_sigint_handler(quitIRQ);
		while (!reboot) {
			setCPUActivity(CPU_ACTIVE);
			/* Run state machine indefinitely */
			lowapp_process();
			/* Sleep until next event occurs */
			lock_wakeUp();
			setCPUActivity(CPU_SLEEP);
			writeCPUActivity();
			pthread_cond_wait(&cond_wakeup, &mutex_wakeup);
			unlock_wakeUp();
		}
		printf("End of program\r\n");
		releaseResources();
	/* Reset device with reboot variable */
	} while(reboot);
	return 0;
}



/**
 * Free resources when the program is terminated
 */
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
	clean_timer1();
	clean_timer2();
	clean_repet_timer();
	clean_queues();
}

/**
 * Register a handler to intercept Ctrl+C/SIGINT signal
 *
 * @param handler Function to be called when SIGINT is catched
 */
void register_sigint_handler(void (*handler)(int)) {
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
}

/**
 * Handler for SIGINT signal
 */
void quitIRQ(int arg) {
	releaseResources();
	printf("main exit\n");
	exit(0);
}

/** @} */
