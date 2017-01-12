/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Bleeper board UART driver implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include "board.h"
#include "uart-board.h"
#include <lowapp_if.h>	/* Include lowapp header */

UART_HandleTypeDef UartHandle;
uint8_t RxData = 0;
uint8_t TxData = 0;
bool TxBusy = false;
bool TransmitionCompleted = false;
uint8_t TransmitedChar=0;

/************************/
/*** WYRES PARAMETERS ***/
/************************/
extern uint8_t ATBufferR[256];
extern uint8_t ATBufferT[256];
extern const uint8_t EnterAtModeMsg[];
extern uint8_t Event;
extern uint8_t	InAtMode;
extern uint8_t UartSensorBuffer[128];
uint8_t StringIsComplete=0;
/** Flag used to throw commands longer than the AT Buffer */
bool flagTooLongCommand;
/** Flag used to detect \r in commands too long for the AT Buffer */
bool crDetectedInLongCommand = false;
/* Flag indicating that a reception is under way */
volatile bool RxBusy = false;

void UartMcuInit( Uart_t *obj, uint8_t uartId, PinNames tx, PinNames rx )
{
    obj->UartId = uartId;

    __HAL_RCC_USART1_FORCE_RESET( );
    __HAL_RCC_USART1_RELEASE_RESET( );

    __HAL_RCC_USART1_CLK_ENABLE( );

    GpioInit( &obj->Tx, tx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART1 );
    GpioInit( &obj->Rx, rx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF7_USART1 );

    /* Set flag for ignoring long commands to false */
    flagTooLongCommand = false;
}

void UartMcuConfig( Uart_t *obj, UartMode_t mode, uint32_t baudrate, WordLength_t wordLength, StopBits_t stopBits, Parity_t parity, FlowCtrl_t flowCtrl )
{
    UartHandle.Instance = USART1;
    UartHandle.Init.BaudRate = baudrate;

    if( mode == TX_ONLY )
    {
        if( obj->FifoTx.Data == NULL )
        {
            assert_param( FAIL );
        }
        UartHandle.Init.Mode = UART_MODE_TX;
    }
    else if( mode == RX_ONLY )
    {
        if( obj->FifoRx.Data == NULL )
        {
            assert_param( FAIL );
        }
        UartHandle.Init.Mode = UART_MODE_RX;
    }
    else if( mode == RX_TX )
    {
        if( ( obj->FifoTx.Data == NULL ) || ( obj->FifoRx.Data == NULL ) )
        {
            assert_param( FAIL );
        }
        UartHandle.Init.Mode = UART_MODE_TX_RX;
    }
    else
    {
       assert_param( FAIL );
    }

    if( wordLength == UART_8_BIT )
    {
        UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    }
    else if( wordLength == UART_9_BIT )
    {
        UartHandle.Init.WordLength = UART_WORDLENGTH_9B;
    }

    switch( stopBits )
    {
    case UART_2_STOP_BIT:
        UartHandle.Init.StopBits = UART_STOPBITS_2;
        break;
    case UART_1_STOP_BIT:
    default:
        UartHandle.Init.StopBits = UART_STOPBITS_1;
        break;
    }

    if( parity == NO_PARITY )
    {
        UartHandle.Init.Parity = UART_PARITY_NONE;
    }
    else if( parity == EVEN_PARITY )
    {
        UartHandle.Init.Parity = UART_PARITY_EVEN;
    }
    else
    {
        UartHandle.Init.Parity = UART_PARITY_ODD;
    }

    if( flowCtrl == NO_FLOW_CTRL )
    {
        UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    }
    else if( flowCtrl == RTS_FLOW_CTRL )
    {
        UartHandle.Init.HwFlowCtl = UART_HWCONTROL_RTS;
    }
    else if( flowCtrl == CTS_FLOW_CTRL )
    {
        UartHandle.Init.HwFlowCtl = UART_HWCONTROL_CTS;
    }
    else if( flowCtrl == RTS_CTS_FLOW_CTRL )
    {
        UartHandle.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
    }

    UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;

    if( HAL_UART_Init( &UartHandle ) != HAL_OK )
    {
    	assert_param( FAIL );
    }

    HAL_NVIC_SetPriority( USART1_IRQn, 8, 0 );
    HAL_NVIC_EnableIRQ( USART1_IRQn );

    /* Enable the UART Data Register not empty Interrupt */
    HAL_UART_Receive_IT( &UartHandle, &RxData, 1 );
}

void UartMcuDeInit( Uart_t *obj )
{
    __HAL_RCC_USART1_FORCE_RESET( );
    __HAL_RCC_USART1_RELEASE_RESET( );
    __HAL_RCC_USART1_CLK_DISABLE( );

    GpioInit( &obj->Tx, obj->Tx.pin, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    GpioInit( &obj->Rx, obj->Rx.pin, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
}

uint8_t UartMcuPutChar( Uart_t *obj, uint8_t data )
{
	__disable_irq( );
	TxData = data;
    if( IsFifoFull( &obj->FifoTx ) == false )
    {
		if ( TxBusy == true ) {
    		FifoPush( &obj->FifoTx, TxData );
    		__enable_irq( );
		}
		else
		{
			TxBusy = true;
			// Enable the USART Transmit interrupt
			HAL_UART_Transmit_IT(&UartHandle, &TxData, 1 );
			__enable_irq( );
		}
    	return 0; // OK
    }
	__enable_irq( );
    return 1; // Busy
}

uint8_t UartMcuGetChar( Uart_t *obj, uint8_t *data )
{
    __disable_irq( );
    if( IsFifoEmpty( &obj->FifoRx ) == false )
    {
        *data = FifoPop( &obj->FifoRx );
        __enable_irq( );
        return 0;
    }
    __enable_irq( );
    return 1;
}

void HAL_UART_TxCpltCallback( UART_HandleTypeDef *handle )
{
    if( IsFifoEmpty( &Uart1.FifoTx ) == false )
    {
        TxData = FifoPop( &Uart1.FifoTx );
        //  Write one byte to the transmit data register
        HAL_UART_Transmit_IT( &UartHandle, &TxData, 1 );
    }
    else
    {
        // Disable the USART Transmit interrupt
    	TxBusy = false;
    }
    if( Uart1.IrqNotify != NULL )
    {
        Uart1.IrqNotify( UART_NOTIFY_TX );
    }

    if(TransmitedChar == (Uart1.FifoTx.End))
    {
    	TransmitedChar=0;
    	TransmitionCompleted=true;
    }
    else
    {
        TransmitedChar++;
    }
}

bool GetTransmissionCompletedFlag(void)
{
	if(TransmitionCompleted)
	{
		TransmitionCompleted=0;
		return true;
	}
	else
	{
		return false;
	}
}

void HAL_UART_RxCpltCallback( UART_HandleTypeDef *handle )
{
	/* Variables used to skip + symbols at the beginning of the buffer */
	uint8_t i=1, Offset=0;
	bool fifoFull = IsFifoFull( &Uart1.FifoRx );
	/*
	 * If the RX Buffer was filled previously, we look for the two
	 * EOL characters to send the error and go back to normal processing
	 * mode
	 */
	if(flagTooLongCommand) {
		/*
		 * If the first EOL character was detected previously, we
		 * check the current RxData character for the second EOL
		 * character.
		 * If it matches, we send the error message and clear the
		 * flagToolLongCommand variable.
		 * If not, we ignore the first EOL found by clearing the
		 * crDetectedInLongCommand flag.
		 */
		if(crDetectedInLongCommand) {
			if(RxData == GetEndChar2()) {
				flagTooLongCommand = false;
				lowapp_atcmderror();
				RxBusy = false;
			}
			else {
				crDetectedInLongCommand = false;
			}
		}
		/* If we detect the first EOL character, we set the corresponding flag */
		else if(RxData == GetEndChar()) {
			crDetectedInLongCommand = true;
		}
		FifoFlush(&Uart1.FifoRx); 					// Flush the FiFo
		HAL_UART_Receive_IT( &UartHandle, &RxData, 1 );
	}
	else if( fifoFull == false)
    {
		/* Set flag for reception in progress */
		RxBusy = true;
        // Read one byte from the receive data register
        FifoPush( &Uart1.FifoRx, RxData );
        /*
         * Process non full buffer but also FIFO full that ends
         * with both end of string characters
         */
        if(CompleteStringInFiFo(&Uart1.FifoRx))
        {
        	StringIsComplete=1;
        	/*
        	 * Ignore symbols + at the beginning of the strings
        	 *
        	 * This is done for allowing proper wake up from stop mode
        	 */
        	while(ATBufferR[i] == '+')
			{
				Offset++;
				i++;
			}
			lowapp_atcmd(ATBufferR, Uart1.FifoRx.End-2-Offset);		// Send AT command to the LoWAPP core
			FifoFlush(&Uart1.FifoRx); 					// Flush the FiFo
			memset(ATBufferR, 0, 256); // Reset buffer
			RxBusy = false;
        }
        if( Uart1.IrqNotify != NULL )
     	{
        	Uart1.IrqNotify( UART_NOTIFY_RX );
     	}

    	HAL_UART_Receive_IT( &UartHandle, &RxData, 1 );
    }
    else	/* FIFO full */
    {
    	if(Uart1.FifoRx.Data[Uart1.FifoRx.End-1] == GetEndChar()) {
    		/*
    		 * If the first end character is in the buffer and the second
    		 * is in RxData, we send the error message
    		 */
    		if(RxData == GetEndChar2()) {
    			RxBusy = false;
				lowapp_atcmderror();
			}
    		else {
    			crDetectedInLongCommand = true;
    			flagTooLongCommand = true;
    		}
    	}
    	else {
			/*
			 * If the command continues after the ATBuffer, we need to ignore
			 * those commands.
			 */
			flagTooLongCommand = true;
    	}
    	FifoFlush(&Uart1.FifoRx); 					// Flush the FiFo
		HAL_UART_Receive_IT( &UartHandle, &RxData, 1 );
    }
}

void HAL_UART_ErrorCallback( UART_HandleTypeDef *handle )
{

}

void USART1_IRQHandler( void )
{
    HAL_UART_IRQHandler( &UartHandle );
}

void UartMcuWaitFlagTC( void )
{
	while(__HAL_UART_GET_FLAG( &UartHandle, USART_FLAG_TC ) != SET){;}
	__HAL_UART_CLEAR_FLAG(&UartHandle, USART_FLAG_TC);
}

uint8_t GetStringCompleteFlag( void )
{
	if(StringIsComplete)
	{
		StringIsComplete=0; // Clear the flag
		return 1;
	}
	else
	{
		return 0;
	}
}

void FlushFiFoRx ( void )
{
	FifoFlush(&Uart1.FifoRx); 					// Flush the FiFo
}

void FlushFiFoTx ( void )
{
	FifoFlush(&Uart1.FifoTx); 					// Flush the FiFo
}
