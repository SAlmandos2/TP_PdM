/*
 * app_commands.c
 *
 *  Created on: 5 dic. 2020
 *      Author: Santiago-N
 */

/*=====[Includes]===========================================================*/

#include "CLI.h"
#include "sapi.h"
#include "printf.h"
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>
#include <ctype.h>
#include <string.h>


/*=====[Enumerations]=======================================================*/

/**
 * Enumerate flags for validation numbers.
 * This flags are used on private functions prvValidateNumber and prvValidateExtractParammeters.
 */
enum Validation_e {
	NON_NUMERIC = 0x0,		/**< Flag for non numeric */
	OVERFLOW	= 0x1,		/**< Flag for overflow  */
	NUMERIC		= 0x2		/**< Flag for numeric */
};


/*=====[Private functions declarations]=====================================*/

/*
 * Take a string and return if it is a valid number or not, and if it fit the requirements.
 * @param	pcNumber	pointer to string that contains the number to validate.
 * @param	uxNumber	size of number to validate.
 * @return	return a flag type Validation_e. NON_NUMERIC, OVERFLOW or NUMERIC.
 */
static int prvValidateNumber(const char* pcNumber, size_t uxNumber);

/*
 * Validate and extract two numbers after the command.
 * Validate and extract two numbers after the command, and save them as doubles on pdParam1 and pdParam2.
 * If fail, then a string is saved on pcWriteBuffer specifying the motive.
 * @param	pcWriteBuffer	Pointer output buffer string.
 * @param	xWriteBufferLen	Size of output buffer.
 * @param	pcCommandString	String with command and parameters.
 * @param	pdParam1		Pointer to storage the first parameter if valid.
 * @param	pdParam2		Pointer to storage the second parameter if valid.
 * @return	return pdPASS if both parameters are valid, and pdFAIL if at least one parameter is invalid or overflow.
 */
static int prvValidateExtractParammeters ( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString , double* pdParam1, double* pdParam2);

/*
 * This function handle "suma" command.
 * @param	pcWriteBuffer	Buffer to store output string.
 * @param	xWriteBufferLen	Size of output buffer.
 * @param	pcCommandString	
		String to analyze.
 * @return	pdTRUE if has to be called again. pdFALSE if function ended.
 */
static int prvCommand_Suma( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * This function handle "resta" command.
 * @param	pcWriteBuffer	Buffer to store output string.
 * @param	xWriteBufferLen	Size of output buffer.
 * @param	pcCommandString	String to analyze.
 * @return	pdTRUE if has to be called again. pdFALSE if function ended.
 */
static int prvCommand_Resta( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * This function handle "multiplica" command.
 * @param	pcWriteBuffer	Buffer to store output string.
 * @param	xWriteBufferLen	Size of output buffer.
 * @param	pcCommandString	String to analyze.
 * @return	pdTRUE if has to be called again. pdFALSE if function ended.
 */
static int prvCommand_Multiplica( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * This function handle "divide" command.
 * @param	pcWriteBuffer	Buffer to store output string.
 * @param	xWriteBufferLen	Size of output buffer.
 * @param	pcCommandString	String to analyze.
 * @return	pdTRUE if has to be called again. pdFALSE if function ended.
 */
static int prvCommand_Divide( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );


/*=====[Private global variables definition]=====================================*/

/**
 *  The definition of the "suma" command.
 *  This command will add two decimal numbers. Only accept 6 digit numbers and a negative sign and decimal point.
 */
static const CLI_Command_Definition_t sSumaCommand =
{
	"suma",
	"\r\nsuma:\r\n realiza la sumatoria de dos números decimales. Acepta signo y/o punto decimal, y números de hasta 6 dígitos\r\n",
	prvCommand_Suma,
	2
};

/**
 *  The definition of the "resta" command.
 *  This command will subtract two decimal numbers. Only accept 6 digit numbers and a negative sign and decimal point.
 */
static const CLI_Command_Definition_t sRestaCommand =
{
	"resta",
	"\r\nresta:\r\n realiza la resta de dos números decimales. Acepta signo y/o punto decimal, y números de hasta 6 dígitos\r\n",
	prvCommand_Resta,
	2
};

/**
 *  The definition of the "multiplica" command.
 *  This command will multiply two decimal numbers. Only accept 6 digit numbers and a negative sign and decimal point.
 */
static const CLI_Command_Definition_t sMultiplicaCommand =
{
	"multiplica",
	"\r\nmultiplica:\r\n realiza la multiplicación de dos números decimales. Acepta signo y/o punto decimal, y números de hasta 6 dígitos\r\n",
	prvCommand_Multiplica,
	2
};

/**
 *  The definition of the "divide" command.
 *  This command will divide two decimal numbers. Only accept 6 digit numbers and a negative sign and decimal point.
 */
static const CLI_Command_Definition_t sDivideCommand =
{
	"divide",
	"\r\ndivide:\r\n realiza la divición de dos números decimales. El primer número es el numerador, y el segundo es el denominador.\r\nAcepta signo y/o punto decimal, y números de hasta 6 dígitos\r\n",
	prvCommand_Divide,
	2
};


/*=====[Private functions implementation]===================================*/

static int prvValidateNumber(const char* pcNumber, size_t uxNumber)
{
	bool bDecimalPoint = false, bNegative = false;
	const char* pcNumberCheck = pcNumber;
	size_t uxNumberSize = uxNumber;

	/* Check if no overflow size number entered */
	if( uxNumberSize > (FLT_DIG + 2) )
		return OVERFLOW;

	/* Check if negative */
	if( *pcNumberCheck == '-')
	{
		bNegative = true;
		pcNumberCheck++;
		uxNumberSize--;
	}

	for( ; uxNumberSize > 0 ; uxNumberSize--, pcNumberCheck++ )
	{
		/* Check only one decimal point entered */
		if( *pcNumberCheck == '.' )
		{
			if ( bDecimalPoint )
				break;
			else
				bDecimalPoint = true;
		}
		else
		{
			/* Check character is numeric */
			if( isdigit( *pcNumberCheck ) == 0 )
				break;
		}
	}

	/* If break before because not numeric or finish with point  */
	if( ( uxNumberSize != 0 ) || ( bDecimalPoint && *pcNumberCheck == '.' ) )	// if break after finish or finish with point
		return NON_NUMERIC;

	/* If number larger than expected */
	if ( (uxNumber > 6 	&& !bNegative 	&& !bDecimalPoint)		||				// if not negative and not decimal but larger than 6 digits
		 (uxNumber > 7 	&&  bNegative 	&& !bDecimalPoint)		||				// if negative but not decimal but larger than 7 digits
		 (uxNumber > 7 	&& !bNegative 	&&  bDecimalPoint)						// if not negative but decimal but larger than 7 digits
		)
		return OVERFLOW;


	return NUMERIC;
}
/*--------------------------------------------------------------------*/

static int prvValidateExtractParammeters ( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString , double* pdParam1, double* pdParam2)
{
	const char *pcParam1, *pcParam2;
	int xLenParam1, xLenParam2;

	char cParameter1[15], cParameter2[15];

	int bParamValid = 0;

	/* Obtain pointers to parameters */
	pcParam1 = CLI_GetParameter( pcCommandString, 1, &xLenParam1 );
	pcParam2 = CLI_GetParameter( pcCommandString, 2, &xLenParam2 );

	/* Validate parameters */
		
	bParamValid |= prvValidateNumber( pcParam1, xLenParam1 );
	bParamValid |= prvValidateNumber( pcParam2, xLenParam2 );

	/* if some of them is not numeric, then report error and why */
	if( bParamValid != NUMERIC )
	{
		if( ( bParamValid & NON_NUMERIC ) != 0 )
			snprintf( pcWriteBuffer, xWriteBufferLen, "Ingrese un número correcto\r\n" );
		else if( ( bParamValid & OVERFLOW ) != 0 )
			snprintf( pcWriteBuffer, xWriteBufferLen, "El número excede el permitido\r\n" );

		return pdFAIL;
	}

	/* copy parameters and store them in their double pointers */
	strncpy(cParameter1, pcParam1, xLenParam1);
	strncpy(cParameter2, pcParam2, xLenParam2);

	*pdParam1 = strtod(cParameter1, NULL);
	*pdParam2 = strtod(cParameter2, NULL);

	return pdPASS;
}
/*--------------------------------------------------------------------*/

static int prvCommand_Suma( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	double xNum1, xNum2;

	/* validate and extract numbers entered with command */
	if ( prvValidateExtractParammeters( pcWriteBuffer, xWriteBufferLen, pcCommandString, &xNum1, &xNum2 ) == pdFAIL )
		return pdFALSE;

	/* format and print */
	snprintf( pcWriteBuffer, xWriteBufferLen, "%g\r\n", xNum1 + xNum2);

	return pdFALSE;
}
/*--------------------------------------------------------------------*/

static int prvCommand_Resta( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	double xNum1, xNum2;

	/* validate and extract numbers entered with command */
	if ( prvValidateExtractParammeters( pcWriteBuffer, xWriteBufferLen, pcCommandString, &xNum1, &xNum2 ) == pdFAIL )
			return pdTRUE;

	/* format and print */
	snprintf( pcWriteBuffer, xWriteBufferLen, "%g\r\n", xNum1 - xNum2 );

	return pdFALSE;
}
/*--------------------------------------------------------------------*/

static int prvCommand_Multiplica( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	double xNum1, xNum2;

	/* validate and extract numbers entered with command */
	if ( prvValidateExtractParammeters( pcWriteBuffer, xWriteBufferLen, pcCommandString, &xNum1, &xNum2 ) == pdFAIL )
			return pdTRUE;

	/* format and print */
	snprintf( pcWriteBuffer, xWriteBufferLen, "%g\r\n", xNum1 * xNum2 );

	return pdFALSE;
}
/*--------------------------------------------------------------------*/

static int prvCommand_Divide( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	double xNum1, xNum2;
	double xRes;

	/* validate and extract numbers entered with command */
	if ( prvValidateExtractParammeters( pcWriteBuffer, xWriteBufferLen, pcCommandString, &xNum1, &xNum2 ) == pdFAIL )
			return pdTRUE;

	/* If denominator is zero, then error */
	if(xNum2 == 0)
		snprintf( pcWriteBuffer, xWriteBufferLen, "ERROR\r\n");
	else
	{
		/* format and print */
		snprintf( pcWriteBuffer, xWriteBufferLen, "%g\r\n", xNum1 / xNum2 );
	}

	return pdFALSE;
}


/*=====[Public functions implementation]===================================*/

void app_commandRegisterCLICommands( void )
{
	/* Register all the command line commands defined immediately above. */
	CLI_RegisterCommand( &sSumaCommand );
	CLI_RegisterCommand( &sRestaCommand );
	CLI_RegisterCommand( &sMultiplicaCommand );
	CLI_RegisterCommand( &sDivideCommand );
}
