/**
 * @file activity_stat.c
 *
 * @brief Save activity statistics for each node into text files
 *
 * Allows for estimation of the energy efficiency of the protocol
 *
 * @author Nathan Olff
 * @date October 5, 2016
 */

#include "activity_stat.h"
#include "lowapp_utils_list.h"
#include "lowapp_sys_timer.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

/**
 * Current state of the simulated cpu
 *
 * Used to write logs in statistics files
 */
CPU_ACTIVITY_T cpuActivity;

/**
 * Current state of the simulated radio
 *
 * Used to write logs in statistics files
 */
RADIO_ACTIVITY_T radioActivity;

/**
 * Linked list for storing cpu activity
 *
 * The list is filled while the state machine is running. The content of the list
 * is written into statistics files when the state machine returns to the application
 */
static LL_T cpuActivities;

/**
 * Linked list for storing radio activity
 *
 * The list is filled while the state machine is running. The content of the list
 * is written into statistics files when the state machine returns to the application
 */
static LL_T radioActivities;

/**
 * String literals used to write CPU activity
 */
char *cpuActivityString[] = {
		"CPU_SLEEP",	/**< CPU sleep string literal */
		"CPU_ACTIVE",	/**< CPU active string literal */
};

/** String literals used to write radio activity */
char *radioActivityString[] = {
		"RADIO_OFF",	/**< Radio off string literal */
		"RADIO_CAD",	/**< Radio CAD string literal */
		"RADIO_RX",		/**< Radio RX string literal */
		"RADIO_TX"		/**< Radio TX string literal */
};

/** Name of the cpu statistics file */
char activityCPUFile[128] = {0};
/** Name of the radio statistics file */
char activityRadioFile[128] = {0};

/**
 * Initialise both cpu and radio statistics files
 *
 * @param path Path to the directory storing the statistics files
 * @param uuidChar UUID of the node, stored as a string for concatenation
 */
void initActivities(char* path, char* uuidChar) {
	/* Get path of the activity files */
	strcpy(activityCPUFile, path);
	strcat(activityCPUFile, "Stats");
	/* Create folder in case it doesn't exists */
	struct stat st = {0};
	if (stat(activityCPUFile, &st) == -1) {
	    mkdir(activityCPUFile, 0700);
	}

	/* Build path for the cpu stat file */
	strcat(activityCPUFile, "/cpu-");
	strcat(activityCPUFile, uuidChar);
	strcat(activityCPUFile, ".txt");

	/* Build path for the radio stat file */
	strcpy(activityRadioFile, path);
	strcat(activityRadioFile, "Stats/");
	strcat(activityRadioFile, "radio-");
	strcat(activityRadioFile, uuidChar);
	strcat(activityRadioFile, ".txt");

	/* Erase file content */
	fopen(activityCPUFile, "w");
	fopen(activityRadioFile, "w");

	/* Set both activities to off / sleep mode */
	setCPUActivity(CPU_SLEEP);
	setRadioActivity(RADIO_OFF);
}

/**
 * Set the CPU activity
 *
 * Adds an element to the linked list of cpu activities
 * @param newAct New activity to add in the list
 */
void setCPUActivity(CPU_ACTIVITY_T newAct) {
	add_to_list(&cpuActivities, newAct, get_time_us());
	cpuActivity = newAct;
}

/**
 * Write CPU activities from the list to the statistics file
 */
void writeCPUActivity() {
	FILE *pFile = fopen(activityCPUFile, "a");
	while(get_size_list(&cpuActivities)) {
		uint16_t data;
		uint64_t time;
		get_head(&cpuActivities, &data, &time);
		fprintf(pFile, "%lu:%s\n", time, cpuActivityString[data]);
		pop_head(&cpuActivities);
	}
	fclose(pFile);
}

/**
 * Set the radio activity
 *
 * Adds an element to the linked list of radio activities
 * @param newAct New activity to add in the list
 */
void setRadioActivity(RADIO_ACTIVITY_T newAct) {
	add_to_list(&radioActivities, newAct, get_time_us());
	radioActivity = newAct;
}

/**
 * Write radio activities from the list to the statistics file
 */
void writeRadioActivity() {
	FILE *pFile = fopen(activityRadioFile, "a");
	while(get_size_list(&radioActivities)) {
		uint16_t data;
		uint64_t time;
		get_head(&radioActivities, &data, &time);
		fprintf(pFile, "%lu:%s\n", time, radioActivityString[data]);
		pop_head(&radioActivities);
	}
	fclose(pFile);
}
