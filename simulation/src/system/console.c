/**
 * @file console.c
 * @brief Console related functions (simulate UART)
 *
 * @author Nathan Olff
 * @date August 10, 2016
 */
#include "console.h"
#include "lowapp_if.h"
#include "lowapp_log.h"

/* Thread and signals related includes */
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

/**
 * @addtogroup lowapp_simu
 * @{
 */
/**
 * @addtogroup lowapp_simu_console LoWAPP Simulation Console I/O
 * @brief Console standard streams simulating UART
 * @{
 */

/** Console thread id */
pthread_t th_console;

/** Flags used to close properly the thread */
bool th_console_running;

/** Line buffer */
uint8_t *buf = NULL;

/**
 * Dummy signal handler for SIGUSR signals
 *
 * @param sig Signal received
 * @param info Informations about the received signal
 * @param ucontext Context about the received signal
 */
void gotsig(int sig, siginfo_t *info, void *ucontext)
{

}



/**
 * Receive AT commands from console standard input
 */
static void cmd_input() {
	ssize_t nRead;	/* Number of bytes read (not including the end of string) */
	size_t bufferSize = 0;	/* Size of the buffer allocated by getline */
	clearerr(stdin);
	while(th_console_running) {
		nRead = getline((char**)&buf, &bufferSize, stdin);	/* Read line from stdin */
		if (nRead == -1) {
			printf("Failed read\n");
			printf("No line read...\n");
			th_console_running = false;
		}
		else {
			/* Remove trailing newline character */
			if (buf[nRead - 1] == '\n')
			{
				buf[nRead - 1] = '\0';
				--nRead;
			}
			printf("|%s| (size=%ld)\n", buf, nRead);
			lowapp_atcmd(buf, nRead);
		}
	}
	free(buf);
}

/**
 * Start new thread for reading incoming AT commands from console stream
 *
 * @retval 0 On success
 * @retval -1 Otherwise
 */
int8_t start_thread_cmd() {
	pthread_attr_t attr;
	int res;
	th_console = 0;

	LOG(LOG_INFO, "Thread id : %ld (start_thread_radio_rx)", syscall(SYS_gettid));

	res = pthread_attr_init(&attr);
	if (res != 0) {
		perror("Attribute init failed");
		return -EXIT_FAILURE;
	}

	th_console_running = true;
	if (pthread_create(&th_console, &attr, console_handler, NULL)) {
		perror("pthread_create");
		return -EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/**
 * Console thread execution function
 * @param arg
 */
void* console_handler(void* arg) {
	/* Setup handler for stopping console thread */
	struct sigaction sa;
	sa.sa_handler = NULL;
	sa.sa_sigaction = gotsig;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	 if (sigaction(SIGNAL_CONSOLE_END, &sa, NULL) < 0) {
			perror("sigaction");
			return (void *)-1;
	}

	cmd_input();
	pthread_exit(NULL);
}

/** @} */
/** @} */
