/**
 * @file main_example_app_gps_xadow.c
 * @date January 2, 2017
 * @brief Example application using a GPS module
 *
 * The main loop retrieve the GPS location from the module every 30 seconds
 * and send the data as a broadcast message to all nodes within the same group.
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

#include "lgps.h"

/**
 * @addtogroup lowapp_ex_app_gps
 * @{
 */

/** Interval in ms between two GPS coordinates requests */
#define GPS_MODULE_INTERVAL	30000

/** Group system level functions for the LoWAPP core */
LOWAPP_SYS_IF_T _lowappSysIf;

/**
 * Timer used to retrieve GPS coordinates and send broadcast frame
 */
TimerEvent_t timer_gps_coord;

/**
 * Flag used to retrieve GPS coordinates
 */
bool flagRequestGpsCoord = false;

void timer_gps_coord_cb(void);
void send_gps_coords_from_module(void);

int main(void)
{
	volatile uint8_t sleepType;
	uint8_t *utc_data;
	uint8_t buffer[256] = "";
	uint8_t floatString[20] = "";
	BoardInitMcu();
	UartSensorOn();

	BoardInitPeriph();

	AtModeInit(19200);

	/* Set system level functions for the core */
	register_sys_functions(&_lowappSysIf);

	/* Initialise LoWAPP core */
	lowapp_init(&_lowappSysIf);

	/* Initialise timer for GPS coordinates */
	TimerInit(&timer_gps_coord, timer_gps_coord_cb);
	TimerSetValue(&timer_gps_coord, GPS_MODULE_INTERVAL);
	TimerStart(&timer_gps_coord);

	/* Initialise the GPS module */
	gps_init();

    while(1)
    {
    	sleepType = lowapp_process();

    	/* Deep sleep */
    	switch(sleepType) {
    	case LOWAPP_SM_DEEP_SLEEP:
    		/*
    		 * If the flag was raised by the timer, we retrieve the data
    		 * from the module and call lowapp_atcmd to send the request
    		 * containing the coordinates.
    		 *
    		 * Only do this whil in deep sleep mode to ensure there is enough
    		 * time to communicate with the I2C device.
    		 */
    		if(flagRequestGpsCoord) {
    			flagRequestGpsCoord = false;
    			send_gps_coords_from_module();
    		}
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

/**
 * Every X seconds, raise a flag to send GPS coordinates
 */
void timer_gps_coord_cb(void) {
	flagRequestGpsCoord = true;
	TimerStart(&timer_gps_coord);
}

/**
 * Create a broadcast request using the GPS coordinates from the module
 */
void send_gps_coords_from_module() {
	double lat, lon;
	uint8_t offset = 0;
	/* Retrieve coordinates from the module */
	lat = (double)gps_get_latitude(0);
	lon = (double)gps_get_longitude(0);

	/* Build send request using GPSAPP special formatting */
	uint8_t buffer[50] = "AT+SEND=";
	offset = 8;
	buffer[offset++] = 0x45;
	buffer[offset++] = 0x01;
	/* Take the 4 most significant bytes (endianness) */
	buffer[offset++] = *(((uint8_t*)(&lat))+7);
	buffer[offset++] = *(((uint8_t*)(&lat))+6);
	buffer[offset++] = *(((uint8_t*)(&lat))+5);
	buffer[offset++] = *(((uint8_t*)(&lat))+4);
	/* Take the 4 most significant bytes (endianness) */
	buffer[offset++] = *(((uint8_t*)(&lon))+7);
	buffer[offset++] = *(((uint8_t*)(&lon))+6);
	buffer[offset++] = *(((uint8_t*)(&lon))+5);
	buffer[offset++] = *(((uint8_t*)(&lon))+4);
	buffer[offset++] = 0xFF;
	buffer[offset++] = 0xFF;

	/* Send request */
	lowapp_atcmd(buffer, offset);
}

/** @} */
