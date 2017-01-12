/*
Description: Board iwwdg general functions implementation

Author: Boulet Benjamin
*/

#include "lowpower_board.h"
#include "board.h"
#include "sx1272-board.h"
#include "uart-board.h"
#include "stm32l1xx_hal_gpio.h"
#include "stm32l1xx_hal_rcc.h"
#include "stm32l1xx_hal_rcc_ex.h"
#include "stm32l1xx_hal_pwr.h"


extern uint8_t Event;
extern bool RxBusy;

void SystemLowPower_Config( bool UartOn )
{
	GPIO_InitTypeDef GPIO_InitStructure = {0};
//
	/* DeInit the busses */
	if(UartOn == false)
	{
		UartDeInit( &Uart1 );
	}

	/* Enable GPIOs clock */
	__HAL_RCC_GPIOA_CLK_ENABLE( );
    __HAL_RCC_GPIOB_CLK_ENABLE( );
    __HAL_RCC_GPIOC_CLK_ENABLE( );
    __HAL_RCC_GPIOD_CLK_ENABLE( );
    __HAL_RCC_GPIOE_CLK_ENABLE( );
    __HAL_RCC_GPIOH_CLK_ENABLE( );


	/* Configure all GPIO port pins in Analog Input mode (floating input trigger OFF) exception PA8 (Accelerometer) / SWDIO / SWCLK */
	if(UartOn == false)
	{
		GPIO_InitStructure.Pin = GPIO_PIN_0 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_9 |
									GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15;
	}
	else
	{
		GPIO_InitStructure.Pin = GPIO_PIN_0 | GPIO_PIN_3 | GPIO_PIN_4 |
									GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15;
	}
	GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure all GPIO port pins in Analog Input mode (floating input trigger OFF) */
	GPIO_InitStructure.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_12 |
							GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 ;

	GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Configure all GPIO port pins in Analog Input mode (floating input trigger OFF) */
	GPIO_InitStructure.Pin = GPIO_PIN_All;
	GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
	HAL_GPIO_Init(GPIOH, &GPIO_InitStructure);

	/* Disable GPIOs clock */
	__HAL_RCC_GPIOA_CLK_DISABLE( );
	__HAL_RCC_GPIOB_CLK_DISABLE( );
	__HAL_RCC_GPIOC_CLK_DISABLE( );
	__HAL_RCC_GPIOD_CLK_DISABLE( );
	__HAL_RCC_GPIOE_CLK_DISABLE( );
    /* Apparently these are not defined for the L151CC */
	__HAL_RCC_GPIOH_CLK_DISABLE( );
}

void SystemWakeUp_Config( bool UartOn )
{
	/*Reinit the IOs*/
	SX1272AntSwInit();
	BoardInitPeriph( );
	if(UartOn == true)
	{
		UartInit( &Uart1, 1, UART_TX, UART_RX );
		UartConfig(&Uart1, RX_TX, 19200, UART_8_BIT, UART_1_STOP_BIT, NO_PARITY, NO_FLOW_CTRL);
	}
}
	
void EnterSleepMode( bool fullSleep )
{
	/* Only go to sleep mode if no UART reception is in progress */
	if(RxBusy == false) {
		if(fullSleep) {
			/* Disable SPI and radio DIO pins */
			BoardDeInitMcu( );
			/* Disable I/O for energy consumption optimization */
			SystemLowPower_Config(UART_ON);
		}

		// Disable the Power Voltage Detector
		HAL_PWR_DisablePVD( );
		SET_BIT( PWR->CR, PWR_CR_CWUF );

		// Enable Ultra low power mode
		HAL_PWREx_EnableUltraLowPower( );

		// Enable the fast wake up from Ultra low power mode
		HAL_PWREx_EnableFastWakeUp( );

		/* Disable Systick to avoid 1kHz systick interrupt */
		HAL_SuspendTick();

		/* Enter sleep mode */
		HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);

		/* Enable Systick after wake up */
		HAL_ResumeTick();
		if(fullSleep) {
			/* Enable SPI and radio DIO pins */
			BoardInitMcu();
			/* Enable antenna, board periph and uart pins */
			SystemWakeUp_Config(true);
		}
	}
}

// End of file
