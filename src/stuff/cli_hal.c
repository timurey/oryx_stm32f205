#include "cli_hal.h"


/* Dimensions the buffer into which input characters are placed. */
#define cmdMAX_INPUT_SIZE	60

/* Dimensions the buffer into which string outputs can be placed. */
#define cmdMAX_OUTPUT_SIZE	512

/* Dimensions the buffer passed to the recvfrom() call. */
#define cmdSOCKET_INPUT_BUFFER_SIZE 60

/* DEL acts as a backspace. */
#define cmdASCII_DEL		( 0x7F )

xQueueHandle xRxQueue;

void vCommandInterpreterTask( void *pvParameters )
{
	long lBytes, lByte;
	static signed char cInChar, cInputIndex = 0;
	static signed char cInputString[ cmdMAX_INPUT_SIZE ], cOutputString[ cmdMAX_OUTPUT_SIZE ], cLocalBuffer[ cmdSOCKET_INPUT_BUFFER_SIZE ];
	BaseType_t xMoreDataToFollow;
	volatile int iErrorCode = 0;
	portBASE_TYPE xStatus;
	/* Just to prevent compiler warnings. */
	( void ) pvParameters;

	xRxQueue = xQueueCreate(cmdSOCKET_INPUT_BUFFER_SIZE, sizeof(char));

	if (xRxQueue!=NULL)
	{
		for( ;; )
		{
			/* Wait for incoming data on the queue. */
			lBytes=0;
			xStatus=xQueueReceive(xRxQueue, &cLocalBuffer[lBytes++], 10/portTICK_RATE_MS);

			if (xStatus==pdPASS)
			{
				while (xQueueReceive(xRxQueue, &cLocalBuffer[lBytes++], 10/portTICK_RATE_MS)==pdPASS)
				{

				}
				lBytes--;
				/* Process each received byte in turn. */
				lByte = 0;
				while( lByte < lBytes )
				{
					/* The next character in the input buffer. */
					cInChar = cLocalBuffer[ lByte ];
					lByte++;
					/* Echo the character back. */
					xputc(cInChar);
					/* Newline characters are taken as the end of the command
					string. */
					if( cInChar == '\n' || cInChar == '\r')
					{
						xputs("\r\n");
						/* Process the input string received prior to the
						newline. */
						do
						{
							/* Pass the string to FreeRTOS+CLI. */
							xMoreDataToFollow = FreeRTOS_CLIProcessCommand( cInputString, cOutputString, cmdMAX_OUTPUT_SIZE );

							/* Send the output generated by the command's
							implementation. */
							xprintf("%s",cOutputString);

						} while( xMoreDataToFollow != pdFALSE ); /* Until the command does not generate any more output. */

						/* All the strings generated by the command processing
						have been sent.  Clear the input string ready to receive
						the next command. */
						cInputIndex = 0;
						memset( cInputString, 0x00, cmdMAX_INPUT_SIZE );

						/* Transmit a spacer, just to make the command console
						easier to read. */
//						xprintf("\r\n");
					}
					else
					{
						if( cInChar == '\r' )
						{
							/* Ignore the character.  Newlines are used to
							detect the end of the input string. */
						}
						else if( (cInChar == '\b') || (cInChar == cmdASCII_DEL) )
						{
							/* Backspace was pressed.  Erase the last character
							in the string - if any. */
							if( cInputIndex > 0 )
							{
								cInputIndex--;
								cInputString[ cInputIndex ] = '\0';
							}
						}
						else
						{
							/* A character was entered.  Add it to the string
							entered so far.  When a \n is entered the complete
							string will be passed to the command interpreter. */
							if( cInputIndex < cmdMAX_INPUT_SIZE )
							{
								cInputString[ cInputIndex ] = cInChar;
								cInputIndex++;
							}
						}
					}
				}
			}
		}
	}
	else
	{
		/* The socket could not be opened. */
		vTaskDelete( NULL );
	}
}
