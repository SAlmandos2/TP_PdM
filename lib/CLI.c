/*
 * myCLI.c
 *
 *  Created on: 3 dic. 2020
 *      Author: Santiago-N
 */

/*=====[Includes]===========================================================*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "CLI.h"

/*=====[Definitions and macros]=============================================*/

#define pdFAIL	( (int)0 )
#define pdPASS	( (int)1 )

#define pdFALSE	( (int)0 )
#define pdTRUE	( (int)1 )

/*=====[Callback functions]================================================*/

/*
 * The callback function that is executed when "help" is entered.
 * This is the only default command that is always present.
 * Type pdCOMMAND_LINE_CALLBACK
 *
 * @param	pcWriteBuffer 	pointer to where output string will be writted.
 * @param	xWriteBufferLen	size of buffer pointed by pcWriteBuffer.
 * @param	pcCommandString	pointer to string containing command ingressed.
 * @return	True if prvHelpCommand has to be called again, false if finished.
 */
static int prvHelpCommand( 	char *pcWriteBuffer,
							size_t xWriteBufferLen,
							const char *pcCommandString
							);


/*=====[Private functions declarations]=====================================*/

/*
 * Return the number of parameters that follow the command name.
 * @param	pcCommandString	a pointer to the command string to analyze.
 * @return	The number of parameters entered
 */
static int8_t prvGetNumberOfParameters( const char *pcCommandString	);


/*=====[Private global variables definition]=====================================*/

/**
 *  The definition of the "help" command.
 *  This command is always at the front of the list of registered commands.
 */
static const CLI_Command_Definition_t xHelpCommand =
{
	"help",
	"\r\nhelp:\r\n Lista todos los comandos registrados\r\n\r\n",
	prvHelpCommand,
	0
};

/*
 * Definition of array of pointers to commands.
 * "help" command will always be the first.
 */
static const CLI_Command_Definition_t* xRegisteredCommands[cliMAX_COMMANDS] = {&xHelpCommand, 0};


/*=====[Private callback implementation]===================================*/

static int prvHelpCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int xReturn = pdTRUE;
	static int loop = 0;

	( void ) pcCommandString;

	/* If no command found, an empty string must be returned */
	*pcWriteBuffer = '\0';

	/* Find next registered command */
	while( ( xRegisteredCommands[loop] == NULL ) && ( loop < cliMAX_COMMANDS ) )
		loop++;

	/* If next command found */
	if (loop < cliMAX_COMMANDS)
	{
		if ( strlen( xRegisteredCommands[loop]->pcHelpString) < xWriteBufferLen )
			snprintf( pcWriteBuffer, xWriteBufferLen, "%s", xRegisteredCommands[loop]->pcHelpString );
		else
			snprintf( pcWriteBuffer, xWriteBufferLen, "Tamaño de buffer pequeño\r\n");
		loop++;
	}

	/* Find next registered command */
	while( ( xRegisteredCommands[loop] == NULL ) && ( loop < cliMAX_COMMANDS ) )
		loop++;

	/* If not other command found, mark last call and reset loop */
	if (loop == cliMAX_COMMANDS)
	{
		loop = 0;
		xReturn = pdFALSE;
	}

	return xReturn;
}


/*=====[Private functions implementation]===================================*/

static int8_t prvGetNumberOfParameters( const char *pcCommandString )
{
	int8_t cParameters = 0;
	int xLastCharacterWasSpace = pdFALSE;

	/* Count the number of space delimited words in pcCommandString. */
	while( *pcCommandString != 0x00 )
	{
		if( ( *pcCommandString ) == ' ' )
		{
			if( xLastCharacterWasSpace != pdTRUE )
			{
				cParameters++;
				xLastCharacterWasSpace = pdTRUE;
			}
		}
		else
		{
			xLastCharacterWasSpace = pdFALSE;
		}

		pcCommandString++;
	}

	/* If the command string ended with spaces, then there will have been too
	many parameters counted. */
	if( xLastCharacterWasSpace == pdTRUE )
	{
		cParameters--;
	}

	/* The value returned is one less than the number of space delimited words,
	as the first word should be the command itself. */
	return cParameters;
}


/*=====[Public functions implementation]===================================*/

int CLI_RegisterCommand( const CLI_Command_Definition_t * const pxCommandToRegister )
{
	int xReturn = pdFAIL;
	int loop;

	/* Check the parameter is not NULL. */
	assert( pxCommandToRegister );

	/* Find an empty place to register the command */
	for(loop = 0; loop < cliMAX_COMMANDS ; loop++)
	{
		if(xRegisteredCommands[loop] == NULL)
			break;
	}

	/* If end of array reached, then return fail */
	if(loop == cliMAX_COMMANDS)
	{
		xReturn = pdFAIL;
	}
	else
	{
		/* Register the command */
		xRegisteredCommands[loop] = pxCommandToRegister;
		xReturn = pdPASS;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

int CLI_ProcessCommand( const char * const pcCommandInput, char * pcWriteBuffer, size_t xWriteBufferLen  )
{
//	static const CLI_Definition_List_Item_t *pxCommand = NULL;
	int xReturn = pdTRUE;
	int loop;
	const char *pcRegisteredCommandString;
	size_t xCommandStringLength;

	/* Note:  This function is not re-entrant.  It must not be called from more
	thank one task. */

	for(loop = 0; loop < cliMAX_COMMANDS ; loop++)
	{
		if( xRegisteredCommands[loop] != NULL)
		{
			pcRegisteredCommandString = xRegisteredCommands[loop]->pcCommand;
			xCommandStringLength = strlen( pcRegisteredCommandString );

			/* To ensure the string lengths match exactly, so as not to pick up
			a sub-string of a longer command, check the byte after the expected
			end of the string is either the end of the string or a space before
			a parameter. */
			if( ( pcCommandInput[ xCommandStringLength ] == ' ' ) || ( pcCommandInput[ xCommandStringLength ] == 0x00 ) )
			{
				if( strncmp( pcCommandInput, pcRegisteredCommandString, xCommandStringLength ) == 0 )
				{
					/* The command has been found.  Check it has the expected
					number of parameters.  If cExpectedNumberOfParameters is -1,
					then there could be a variable number of parameters and no
					check is made. */

					if( xRegisteredCommands[loop]->cExpectedNumberOfParameters >= 0 )
					{
						if( prvGetNumberOfParameters( pcCommandInput ) != xRegisteredCommands[loop]->cExpectedNumberOfParameters )
						{
							xReturn = pdFALSE;
						}
					}

					break;
				}
			}
		}
	}
	if( ( loop != cliMAX_COMMANDS ) && ( xReturn == pdFALSE ) )
	{
		/* The command was found, but the number of parameters with the command
		was incorrect. */
		snprintf( pcWriteBuffer, xWriteBufferLen, "Incorrect command parameter(s).  Enter \"help\" to view a list of available commands.\r\n\r\n" );
	}
	else if( loop != cliMAX_COMMANDS )
	{
		/* Call the callback function that is registered to this command. */
		xReturn = xRegisteredCommands[loop]->pxCommandInterpreter( pcWriteBuffer, xWriteBufferLen, pcCommandInput );
	}
	else
	{
		/* the command was not found. */
		snprintf( pcWriteBuffer, xWriteBufferLen, "Command not recognised.  Enter 'help' to view a list of available commands.\r\n\r\n" );
		xReturn = pdFALSE;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

const char* CLI_GetParameter( const char *pcCommandString, unsigned int uxWantedParameter, int *pxParameterStringLength )
{
	unsigned int uxParametersFound = 0;
	const char *pcReturn = NULL;

	*pxParameterStringLength = 0;

	while( uxParametersFound < uxWantedParameter )
	{
		/* Index the character pointer past the current word.  If this is the start
		of the command string then the first word is the command itself. */
		while( ( ( *pcCommandString ) != 0x00 ) && ( ( *pcCommandString ) != ' ' ) )
		{
			pcCommandString++;
		}

		/* Find the start of the next string. */
		while( ( ( *pcCommandString ) != 0x00 ) && ( ( *pcCommandString ) == ' ' ) )
		{
			pcCommandString++;
		}

		/* Was a string found? */
		if( *pcCommandString != 0x00 )
		{
			/* Is this the start of the required parameter? */
			uxParametersFound++;

			if( uxParametersFound == uxWantedParameter )
			{
				/* How long is the parameter? */
				pcReturn = pcCommandString;
				while( ( ( *pcCommandString ) != 0x00 ) && ( ( *pcCommandString ) != ' ' ) )
				{
					( *pxParameterStringLength )++;
					pcCommandString++;
				}

				if( *pxParameterStringLength == 0 )
				{
					pcReturn = NULL;
				}

				break;
			}
		}
		else
		{
			break;
		}
	}

	return pcReturn;
}
/*-----------------------------------------------------------*/

