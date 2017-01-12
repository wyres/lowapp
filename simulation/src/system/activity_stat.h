/**
 * @file activity_stat.h
 *
 * @brief Save activity statistics for each node into text files
 *
 * Allows for estimation of the energy efficiency of the protocol
 *
 * @author Nathan Olff
 * @date October 5, 2016
 */

#ifndef LOWAPP_SIMU_ACTIVITY_STAT_H_
#define LOWAPP_SIMU_ACTIVITY_STAT_H_


/**
 * Possible states of the Cpu, used for statistics
 */
enum CPU_ACTIVITY {
	CPU_SLEEP = 0,	/**< CPU_SLEEP */
	CPU_ACTIVE,   	/**< CPU_ACTIVE */
};

/**
 * Possible states of the radio, used for statistics
 */
enum RADIO_ACTIVITY {
	RADIO_OFF = 0,	/**< RADIO_OFF */
	RADIO_CAD,    	/**< RADIO_CAD */
	RADIO_RX,     	/**< RADIO_RX */
	RADIO_TX      	/**< RADIO_TX */
};

/** Typedef for cpu activity */
typedef enum CPU_ACTIVITY CPU_ACTIVITY_T;
/** Typedef for radio activity */
typedef enum RADIO_ACTIVITY RADIO_ACTIVITY_T;

void initActivities(char* path, char* uuidChar);

void setCPUActivity(CPU_ACTIVITY_T newAct);
void writeCPUActivity();

void setRadioActivity(RADIO_ACTIVITY_T newAct);
void writeRadioActivity();

#endif
