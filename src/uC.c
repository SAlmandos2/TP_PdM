 /*
===============================================================================
 Name        : uC.c
 Author      : Almandos Santiago
 Version     : 1.0
 Copyright   :
 Description : main definition
===============================================================================
*/

/*=====[Includes]===========================================================*/

#include "sapi.h"			/**< sapi hal*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "ring_buffer.h"	/**< lpcOpen ring buffer implementation*/
#include "CLI.h"			/**< CLI implementation*/
#include "app_commands.h"	/**< commands created to process with CLI */


/*=====[Definitions and macros]=============================================*/

#define uartBUFFER_SIZE	16					/**< Size of ring buffer uart.*/
											/**< Must be power of 2 and at least 2 (see ring_buffer.h for more detail) */

#define appIN_BUFFER_SIZE	64				/**< Size of input buffer */
#define appOUT_BUFFER_SIZE	256				/**< Size of output buffer */

#define ETX    0x03     					/**< ASCII end of text */

/* keep alive leds */
#define appTICK_SPEED		50				/**< Tick 50ms */
#define app_msToTick(ms)	(tick_t)( ms/appTICK_SPEED )     /**< macro to change ms to Ticks */

/*=====[Enumerations]=======================================================*/

typedef enum{
	IDLE,
	RECEIVING,
	PROCESSING,
}stateUART_t;


/*=====[Variables]=========================================================*/

RINGBUFF_T rbRxBuffer;
char cRxBuffer[uartBUFFER_SIZE] = {0};


/*=====[Callback functions]================================================*/

/** Data UART_USB reception */
void UART_USBOnRx( void *noUsado )
{
   char c = uartRxRead( UART_USB );
   if(c == ETX)
   {
	  /* Implement a forced exit */
   }
   RingBuffer_Insert( &rbRxBuffer, &c );
}


/*=====[Functions]========================================================*/

/** Led to know if program is running */
void app_ToggleLED()
{
	gpioToggle( LEDR );
}
/*-----------------------------------------------------------*/

void UART_USBConfig()
{
	/* Initialize UART_USB and interrupts */
    uartConfig(UART_USB, 115200);
    /* Define callback and event interrupt */
    uartCallbackSet(UART_USB, UART_RECEIVE, UART_USBOnRx, NULL);
    /* enable UART_USB interrupts */
    uartInterrupt(UART_USB, true);
}
/*-----------------------------------------------------------*/

void app_FMS_Init()
{
	RingBuffer_Init( &rbRxBuffer, &cRxBuffer, sizeof( char ), sizeof( cRxBuffer ) );
}
/*-----------------------------------------------------------*/

void app_FSM()
{
	static stateUART_t xState_UART = IDLE;				/**< State to handle state machine */
	static char cCommand[appIN_BUFFER_SIZE] = {0};		/**< Buffer to store input data */
	static int xItem = 0;

	char cRx;
	char cOutputBuffer[appOUT_BUFFER_SIZE] = {0};		/**< Buffer to store data to print */

	switch(xState_UART)
	{
		case IDLE:
			/* if there is something in uart, then go to receiving */
			if( RingBuffer_GetCount( &rbRxBuffer) > 0 )
				xState_UART = RECEIVING;
			break;
		case RECEIVING:
			if( RingBuffer_Pop( &rbRxBuffer, &cRx ) == 1)
			{
				/* store input data on buffer until new line arrive, then jump to process data */
            if( cRx == ETX)
            {
               memset( cCommand, 0, appIN_BUFFER_SIZE );
               memset( cOutputBuffer, 0, appOUT_BUFFER_SIZE );
               xItem = 0;
               xState_UART = IDLE;
            }
            else if ( ( cRx == '\b' ) && ( xItem > 0 ) )
            {
               cCommand[--xItem] = '\0';   
            }
				else if( cRx == '\n' )
					xState_UART = PROCESSING;
				else if ( isprint(cRx) != 0 && ( xItem < appIN_BUFFER_SIZE - 1 ) )
					cCommand[xItem++] = cRx;
			}
			break;
		case PROCESSING:
			/* Process command until it finish */
			if( CLI_ProcessCommand( cCommand, cOutputBuffer, appOUT_BUFFER_SIZE ) )
			{
				uartWriteString( UART_USB, cOutputBuffer );
			}
			else
			{
				uartWriteString( UART_USB, cOutputBuffer );
				/* Clean buffers and return to idle */
				memset( cCommand, 0, appIN_BUFFER_SIZE );
				memset( cOutputBuffer, 0, appOUT_BUFFER_SIZE );
				xItem = 0;
				xState_UART = IDLE;
			}
			break;
		default:
			/* Should never enter here but if it does, then print error and reset to idle */
			memset( cCommand, 0, appIN_BUFFER_SIZE );
			memset( cOutputBuffer, 0, appOUT_BUFFER_SIZE );
			xItem = 0;
			uartWriteString( UART_USB, "ERROR: estado desconocido\r\n\r\n" );
			xState_UART = IDLE;
			break;
	}
}
/*-----------------------------------------------------------*/

/*=====[Main function, entry point]========================================*/

int main(void) {
   const tick_t ulxBlink = app_msToTick( 500 );TP
   tick_t ulxCurrTick = 0;
   // ---------- Board configuration --------------------
   boardInit();

   /* To toggle led keep alive */
   /** Initialize timer 50ms (max value)*/
   tickInit( appTICK_SPEED );
   ulxCurrTick = tickRead();

   // ---------- Others configurations ------------------
   /* Register commands */
   app_commandRegisterCLICommands();
   /* Configure UART_USB */
   UART_USBConfig();
   /* Initialize state machine */
   app_FMS_Init();

   // ---------- For ever loop --------------------------
   while(1) {
      // main state machine
      app_FSM();

      /* To toggle LED every ulxBlink Ticks */
      if(tickRead() - ulxCurrTick > ulxBlink)
      {
         app_ToggleLED();
         ulxCurrTick = tickRead();
      }
   }
   return 0 ;
}
