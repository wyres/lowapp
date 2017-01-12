/**
 * @file lgps.c
 * @date January 2, 2017
 * @brief Xadow GPS v2 interface
 *
 * Communication with the Xadow GPS module v2 through I2C.
 * Inspired by the Xadow_GPS_v2_test project from Seeed Studio
 * (https://github.com/WayenWeng/Xadow_GPS_v2_test/blob/master/gps_test/lgps.c)
 *
 * @author Nathan Olff
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "board.h"
#include "i2c-board.h"
#include "lgps.h"

/**
 * @addtogroup lowapp_ex_app_gps LoWAPP Example Application with GPS
 * @brief Example application using a GPS module
 * @{
 */

/**
 * @addtogroup lowapp_ex_app_gps_driver Driver for the GPS module
 * @{
 */

/**
 * Store the UTC date / time retrieved from the module
 */
uint8_t gps_utc_date_time[GPS_UTC_DATE_TIME_SIZE] = {0};

void gps_init(void) {
	I2cInit(&I2c, I2C_SCL, I2C_SDA);
	I2cMcuFormat(&I2c, MODE_I2C, I2C_DUTY_CYCLE_2, true, I2C_ACK_ADD_7_BIT, 100000);
	I2cSetAddrSize(&I2c, I2C_ADDR_SIZE_8);
}


uint8_t gps_check_online(void)
{
	uint8_t data[GPS_SCAN_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_SCAN_ID;

    I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_SCAN_SIZE+2);i++)
	{
	    I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

	if(data[5] == GPS_DEVICE_ADDR)return 1;
	else return 0;
}

uint8_t* gps_get_utc_date_time(void)
{
	uint8_t data[GPS_UTC_DATE_TIME_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_UTC_DATE_TIME_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_UTC_DATE_TIME_SIZE+2);i++)
	{
	    I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

	for(i=0;i<GPS_UTC_DATE_TIME_SIZE;i++)
	gps_utc_date_time[i] = data[i+2];

	return gps_utc_date_time;
}

uint8_t gps_get_status(void)
{
	uint8_t data[GPS_STATUS_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_STATUS_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_STATUS_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

	return data[2];
}

float gps_get_latitude(uint8_t *buffer)
{
	uint8_t data[GPS_LATITUDE_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_LATITUDE_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_LATITUDE_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

	if(buffer != NULL) {
		/* Copy into destination string */
		memcpy(buffer, data+2, GPS_LATITUDE_SIZE);
	}
	return atof(&data[2]);
}

uint8_t gps_get_ns(void)
{
	uint8_t data[GPS_NS_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_NS_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_NS_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

    if(data[2] == 'N' || data[2] == 'S')return data[2];
    else return data[2] = '-';

}

float gps_get_longitude(uint8_t *buffer)
{
	uint8_t data[GPS_LONGITUDE_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_LONGITUDE_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_LONGITUDE_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}
	if(buffer != NULL) {
		/* Copy into destination string */
		memcpy(buffer, data+2, GPS_LONGITUDE_SIZE);
	}
	return atof(&data[2]);
}

uint8_t gps_get_ew(void)
{
	uint8_t data[GPS_EW_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_EW_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_EW_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

	if(data[2] == 'E' || data[2] == 'W')return data[2];
	else return data[2] = '-';
}

float gps_get_speed(uint8_t *buffer)
{
	uint8_t data[GPS_SPEED_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_SPEED_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_SPEED_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

	if(buffer != NULL) {
		/* Copy into destination string */
		memcpy(buffer, data+2, GPS_SPEED_SIZE);
	}
	return atof(&data[2]);
}

float gps_get_course(uint8_t* buffer)
{
	uint8_t data[GPS_COURSE_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_COURSE_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_COURSE_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

	if(buffer != NULL) {
		/* Copy into destination string */
		memcpy(buffer, data+2, GPS_COURSE_SIZE);
	}
	return atof(&data[2]);
}

uint8_t gps_get_position_fix(void)
{
	uint8_t data[GPS_POSITION_FIX_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_POSITION_FIX_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_POSITION_FIX_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

	return data[2];
}

uint8_t gps_get_sate_used(void)
{
	uint8_t data[GPS_SATE_USED_SIZE+2];
	uint8_t i;
	uint8_t value;
	uint8_t addr = GPS_SATE_USED_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_SATE_USED_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

	if(data[3] >= '0' && data[3] <= '9')value = (data[3] - '0') * 10;
	else value = 0;
	if(data[2] >= '0' && data[2] <= '9')value += (data[2] - '0');
	else value += 0;

	return value;
}

float gps_get_altitude(uint8_t *buffer)
{
	uint8_t data[GPS_ALTITUDE_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_ALTITUDE_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);


	for(i=0;i<(GPS_ALTITUDE_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}
	if(buffer != NULL) {
		/* Copy into destination string */
		memcpy(buffer, data+2, GPS_ALTITUDE_SIZE);
	}
	return atof(&data[2]);
}

uint8_t gps_get_mode(void)
{
	uint8_t data[GPS_MODE_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_MODE_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_MODE_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

	return data[2];
}

uint8_t gps_get_mode2(void)
{
	uint8_t data[GPS_MODE2_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_MODE2_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_MODE2_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

	return data[2];
}

uint8_t gps_get_sate_in_veiw(void)
{
	uint8_t data[GPS_SATE_IN_VIEW_SIZE+2];
	uint8_t i;
	uint8_t addr = GPS_SATE_IN_VIEW_ID;

	I2cSimpleWriteBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &addr, 1);

	for(i=0;i<(GPS_SATE_IN_VIEW_SIZE+2);i++)
	{
		I2cSimpleReadBuffer(&I2c, (GPS_DEVICE_ADDR << 1), &(data[i]), 1);
	}

	return data[2];
}

/** @} */
/** @} */
