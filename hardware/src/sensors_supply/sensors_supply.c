/*
Description: AT mode general functions implementation

Author: Boulet Benjamin
*/
#include "sensors_supply.h"
#include "board.h"

static Gpio_t UartSensorSupply;
static Gpio_t I2cSensorSupply;

void UartSensorOn( void )
{
	// Init the IO for the power supply
	GpioInit( &UartSensorSupply, UART_SUPPLY_PIN, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
	GpioWrite( &UartSensorSupply, 1 );
}

void UartSensorOff( void )
{
	// Init the IO for the power supply
	GpioWrite( &UartSensorSupply, 0 );
}

void I2cSensorOn( void )
{
	// Init the IO for the power supply
	GpioInit( &I2cSensorSupply, I2C_SUPPLY_PIN, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
	GpioWrite( &I2cSensorSupply, 1 );
}

void I2cSensorOff( void )
{
	// Init the IO for the power supply
	GpioWrite( &I2cSensorSupply, 0 );
}
	
// End of file
