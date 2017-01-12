/**
 * @file main.c
 * @date October 20, 2017
 * @brief Main program for executing the LoWAPP core hardware implementation
 *
 * @author Nathan Olff
 */
#include "board.h"
#include "lowapp_inc.h"
#include "lowapp_sys_impl.h"
#include "lowapp_core.h"
#include "lowpower_board.h"

#include "lowapp_sys_uart.h"
#include "sensors_supply.h"

/** Group system level functions for the LoWAPP core */
LOWAPP_SYS_IF_T _lowappSysIf;


/**
 * Main function of the hardware implementation for running the LoWAPP core
 */
int main(void)
{
	volatile uint8_t sleepType;
	BoardInitMcu();
	UartSensorOn();

	BoardInitPeriph();

	AtModeInit(19200);

	/* Set system level functions for the core */
	register_sys_functions(&_lowappSysIf);

	/* Initialise LoWAPP core */
	lowapp_init(&_lowappSysIf);

    while(1)
    {
    	sleepType = lowapp_process();

    	/* Deep sleep */
    	switch(sleepType) {
    	case LOWAPP_SM_DEEP_SLEEP:
			EnterSleepMode(true);
			/*
			 * Temporary fix for CAD failures
			 *
			 * Add delay to allow proper wakeup from sleep mode
			 */
			DelayMs(2);
			break;
    	case LOWAPP_SM_TX:
    	case LOWAPP_SM_RX:
    		/* Shallow sleep */
    		EnterSleepMode(false);
    		break;
    	default:
//    		EnterSleepMode(false);
    		break;
    	}
	}
}
