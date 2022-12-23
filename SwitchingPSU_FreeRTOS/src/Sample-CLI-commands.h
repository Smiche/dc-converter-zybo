/*
 * FreeRTOS V202112.00
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
 *
 * http://www.FreeRTOS.org/cli
 *
 ******************************************************************************/

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"
#include "main.h"

#ifndef  configINCLUDE_TRACE_RELATED_CLI_COMMANDS
#define configINCLUDE_TRACE_RELATED_CLI_COMMANDS 0
#endif

#ifndef configINCLUDE_QUERY_HEAP_COMMAND
#define configINCLUDE_QUERY_HEAP_COMMAND 0
#endif

/* --- Function declarations --- */

/*
 * The function that registers the commands that are defined within this file.
 */
void vRegisterSampleCLICommands(void);

/*
 * Implements the task-stats command.
 */
static BaseType_t prvTaskStatsCommand(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString);

/*
 * Implements the echo-three-parameters command.
 */
static BaseType_t prvThreeParameterEchoCommand(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString);

/*
 * Implements the echo-parameters command.
 */
static BaseType_t prvParameterEchoCommand(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString);
/*
 *
 */
static BaseType_t prvStateCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
		const char *pcCommandString);
/**
 *
 */
static BaseType_t prvVoltageCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
		const char *pcCommandString);
/**
 *
 */
static BaseType_t prvPIDCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
		const char *pcCommandString);

/*
 *
 */
static BaseType_t prvSaturationCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
		const char *pcCommandString);

/* --- Command initializations ---*/

/* Structure that defines the "task-stats" command line command.  This generates
 a table that gives information on each task in the system. */
static const CLI_Command_Definition_t xTaskStats =
		{ "task-stats", /* The command string to type. */
				"\r\ntask-stats:\r\n Displays a table showing the state of each FreeRTOS task\r\n",
				prvTaskStatsCommand, /* The function to run. */
				0 /* No parameters are expected. */
		};

/* Structure that defines the "echo_3_parameters" command line command.  This
 takes exactly three parameters that the command simply echos back one at a
 time. */
static const CLI_Command_Definition_t xThreeParameterEcho =
		{ "echo-3-parameters",
				"\r\necho-3-parameters <param1> <param2> <param3>:\r\n Expects three parameters, echos each in turn\r\n",
				prvThreeParameterEchoCommand, /* The function to run. */
				3 /* Three parameters are expected, which can take any value. */
		};

/* Structure that defines the "echo_parameters" command line command.  This
 takes a variable number of parameters that the command simply echos back one at
 a time. */
static const CLI_Command_Definition_t xParameterEcho =
		{ "echo-parameters",
				"\r\necho-parameters <...>:\r\n Take variable number of parameters, echos each in turn\r\n",
				prvParameterEchoCommand, /* The function to run. */
				-1 /* The user can enter any number of commands. */
		};

/*
 * Console command definition for voltage change
 */
static const CLI_Command_Definition_t xVoltageCommand =
		{ "voltage",
				"\r\nvoltage <reference value>: Changes current reference value to <reference value>.\r\n",
				prvVoltageCommand, 1 };

/*
 * Console command definition for saturation change
 */
static const CLI_Command_Definition_t xSaturationCommand =
		{ "saturation",
				"\r\nsaturation <saturation limit>: Changes current saturation limit to <saturation limit>.\r\n",
				prvSaturationCommand, 1 };

/*
 * Console command definition for state change
 */
static const CLI_Command_Definition_t xStateCommand =
		{ "mode",
				"\r\nmode <state>: Changes current state to <state>. <state> can be one of the following: 0 (idle), 1 (configurate), 2 (modulate).\r\n",
				prvStateCommand, 1 };
/*
 * Console command definition for pid
 */
static const CLI_Command_Definition_t xPIDCommand =
		{ "pid",
				"\r\npid: <parameter> <value> Changes <parameter> to <value>. Parameters are Ki, Kd, Kp.\r\n",
				prvPIDCommand, 2 };

/*-----------------------------------------------------------*/

void vRegisterSampleCLICommands(void) {
	/* Register all the command line commands defined immediately above. */
	FreeRTOS_CLIRegisterCommand(&xTaskStats);
	FreeRTOS_CLIRegisterCommand(&xThreeParameterEcho);
	FreeRTOS_CLIRegisterCommand(&xParameterEcho);
	FreeRTOS_CLIRegisterCommand(&xStateCommand);
	FreeRTOS_CLIRegisterCommand(&xPIDCommand);
	FreeRTOS_CLIRegisterCommand(&xVoltageCommand);
	FreeRTOS_CLIRegisterCommand(&xSaturationCommand);

}
/*-----------------------------------------------------------*/

static BaseType_t prvTaskStatsCommand(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString) {
	const char * const pcHeader =
			"     State   Priority  Stack    #\r\n************************************************\r\n";
	BaseType_t xSpacePadding;

	/* Remove compile time warnings about unused parameters, and check the
	 write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	 write buffer length is adequate, so does not check for buffer overflows. */
	(void) pcCommandString;
	(void) xWriteBufferLen;
	configASSERT(pcWriteBuffer);

	/* Generate a table of task stats. */
	strcpy(pcWriteBuffer, "Task");
	pcWriteBuffer += strlen(pcWriteBuffer);

	/* Minus three for the null terminator and half the number of characters in
	 "Task" so the column lines up with the centre of the heading. */
	configASSERT(configMAX_TASK_NAME_LEN > 3);
	for (xSpacePadding = strlen("Task");
			xSpacePadding < ( configMAX_TASK_NAME_LEN - 3); xSpacePadding++) {
		/* Add a space to align columns after the task's name. */
		*pcWriteBuffer = ' ';
		pcWriteBuffer++;

		/* Ensure always terminated. */
		*pcWriteBuffer = 0x00;
	}
	strcpy(pcWriteBuffer, pcHeader);
	vTaskList(pcWriteBuffer + strlen(pcHeader));

	/* There is no more data to return after this single string, so return
	 pdFALSE. */
	return pdFALSE;
}
/*-----------------------------------------------------------*/

static BaseType_t prvThreeParameterEchoCommand(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString) {
	const char *pcParameter;
	BaseType_t xParameterStringLength, xReturn;
	static UBaseType_t uxParameterNumber = 0;

	/* Remove compile time warnings about unused parameters, and check the
	 write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	 write buffer length is adequate, so does not check for buffer overflows. */
	(void) pcCommandString;
	(void) xWriteBufferLen;
	configASSERT(pcWriteBuffer);

	if (uxParameterNumber == 0) {
		/* The first time the function is called after the command has been
		 entered just a header string is returned. */
		sprintf(pcWriteBuffer, "The three parameters were:\r\n");

		/* Next time the function is called the first parameter will be echoed
		 back. */
		uxParameterNumber = 1U;

		/* There is more data to be returned as no parameters have been echoed
		 back yet. */
		xReturn = pdPASS;
	} else {
		/* Obtain the parameter string. */
		pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
		uxParameterNumber, /* Return the next parameter. */
		&xParameterStringLength /* Store the parameter string length. */
		);

		/* Sanity check something was returned. */
		configASSERT(pcParameter);

		/* Return the parameter string. */
		memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		sprintf(pcWriteBuffer, "%d: ", (int) uxParameterNumber);
		strncat(pcWriteBuffer, pcParameter, (size_t) xParameterStringLength);
		strncat(pcWriteBuffer, "\r\n", strlen("\r\n"));

		/* If this is the last of the three parameters then there are no more
		 strings to return after this one. */
		if (uxParameterNumber == 3U) {
			/* If this is the last of the three parameters then there are no more
			 strings to return after this one. */
			xReturn = pdFALSE;
			uxParameterNumber = 0;
		} else {
			/* There are more parameters to return after this one. */
			xReturn = pdTRUE;
			uxParameterNumber++;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvParameterEchoCommand(char *pcWriteBuffer,
		size_t xWriteBufferLen, const char *pcCommandString) {
	const char *pcParameter;
	BaseType_t xParameterStringLength, xReturn;
	static UBaseType_t uxParameterNumber = 0;

	/* Remove compile time warnings about unused parameters, and check the
	 write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	 write buffer length is adequate, so does not check for buffer overflows. */
	(void) pcCommandString;
	(void) xWriteBufferLen;
	configASSERT(pcWriteBuffer);

	if (uxParameterNumber == 0) {
		/* The first time the function is called after the command has been
		 entered just a header string is returned. */
		sprintf(pcWriteBuffer, "The parameters were:\r\n");

		/* Next time the function is called the first parameter will be echoed
		 back. */
		uxParameterNumber = 1U;

		/* There is more data to be returned as no parameters have been echoed
		 back yet. */
		xReturn = pdPASS;
	} else {
		/* Obtain the parameter string. */
		pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, /* The command string itself. */
		uxParameterNumber, /* Return the next parameter. */
		&xParameterStringLength /* Store the parameter string length. */
		);

		if (pcParameter != NULL) {
			/* Return the parameter string. */
			memset(pcWriteBuffer, 0x00, xWriteBufferLen);
			sprintf(pcWriteBuffer, "%d: ", (int) uxParameterNumber);
			strncat(pcWriteBuffer, (char *) pcParameter,
					(size_t) xParameterStringLength);
			strncat(pcWriteBuffer, "\r\n", strlen("\r\n"));

			/* There might be more parameters to return after this one. */
			xReturn = pdTRUE;
			uxParameterNumber++;
		} else {
			/* No more parameters were found.  Make sure the write buffer does
			 not contain a valid string. */
			pcWriteBuffer[0] = 0x00;

			/* No more data to return. */
			xReturn = pdFALSE;

			/* Start over the next time this command is executed. */
			uxParameterNumber = 0;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

/*
 * Console command for state change
 */
static BaseType_t prvStateCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
		const char *pcCommandString) {
	const char *pcStateParameter;
	BaseType_t xParameterStringLength;

	(void) pcCommandString;

	pcStateParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1,
			&xParameterStringLength);

	// Check if mode semaphore exists
	if (modeSemaphore == NULL) {
		vTaskDelay(x100ms);
		return;
	}

	/*
	 * Change state according to the given parameter
	 * State parameters are idle, conf, modulate
	 */
	if ((int) pcStateParameter == 0 || (int) pcStateParameter == 1
			|| (int) pcStateParameter == 2) {
		/*
		 * TODO ready for test: handle these separately with a semaphore used in the tStateControl task.
		 * Should make a separate struct for configuration from console.
		 */
		if ( xSemaphoreTake( modeSemaphore, ( TickType_t ) 50 ) == pdTRUE) {
			/* We were able to obtain the semaphore and can now access the
			 shared resource. */
			MODE = (int) pcStateParameter;
			modeChanged = 1;
			xSemaphoreGive(modeSemaphore);
		} else {
			/* We could not obtain the semaphore and can therefore not access
			 * the shared resource safely.
			 */
			xil_printf("Unable to change mode. Resource is busy.");
		}

	} else {
		/* Print unknown mode error  */
		char err_msg[] = "Mode should be one of these: 0, 1, 2.\nUnknown mode: ";
		memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		strncat(pcWriteBuffer, (char *) err_msg, (size_t) err_msg);
		strncat(pcWriteBuffer, (char *) pcStateParameter,
				(size_t) xParameterStringLength);
		strncat(pcWriteBuffer, "\r\n", strlen("\r\n"));
	}

	return pdFALSE;
}

/*
 *  Console command for changing PID parameters
 */
static BaseType_t prvPIDCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
		const char *pcCommandString) {
	char *pcPIDkeyParameter;
	char *pcPIDvalParameter;
	BaseType_t xParameter1StringLength, xParameter2StringLength;

	pcPIDkeyParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1,
			&xParameter1StringLength);

	pcPIDvalParameter = FreeRTOS_CLIGetParameter(pcCommandString, 2,
			&xParameter2StringLength);

	/*
	 * TODO ready for test: handle these separately with a semaphore used in the tStateControl task.
	 * Should make a separate struct for configuration from console.
	 */

	// check if semaphore exists
	if (pidConfSemaphore == NULL) {
		return;
	}

	if ( xSemaphoreTake( pidConfSemaphore, ( TickType_t ) 50 ) == pdTRUE) {
			/* We were able to obtain the semaphore and can now access the
			 shared resource. */

		if ((strcmp(pcPIDkeyParameter, "Ki") == 0) && ((int) pcPIDvalParameter)) {
			pidConfig.Ki = (int) pcPIDvalParameter;
		} else if ((strcmp(pcPIDkeyParameter, "Kd") == 0) && ((int) pcPIDvalParameter)) {
			pidConfig.Kd = (int) pcPIDvalParameter;
		} else if ((strcmp(pcPIDkeyParameter, "Kp") == 0) && ((int) pcPIDvalParameter)) {
			pidConfig.Kp = (int) pcPIDvalParameter;
		}else {
			// Value not recognized, fail.
			memset(pcWriteBuffer, 0x00, xWriteBufferLen);
			strncat(pcWriteBuffer, "Unable to change pid config. Unknown parameters.\r\n",
			strlen("Unable to change pid config. Unknown parameters.\r\n"));
		}
		xSemaphoreGive(pidConfSemaphore);
	} else {
		/* We could not obtain the semaphore and can therefore not access
		 * the shared resource safely.
		 */
		memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		strncat(pcWriteBuffer, "Unable to change pid config. Resource is busy.\r\n",
		strlen("Unable to change pid config. Resource is busy.\r\n"));
	}
	return pdFALSE;
}

/*
 * Console command for voltage change
 */
static BaseType_t prvVoltageCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
		const char *pcCommandString) {
	char *pcVoltageParameter;
	BaseType_t xParameterStringLength;

	pcVoltageParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1,
			&xParameterStringLength);

	/*
	 * TODO ready for test: handle these separately with a semaphore used in the tStateControl task.
	 *  Should make a separate struct for configuration from console.
	 */

	// check if semaphore exists
	if (modulationConfSemaphore == NULL) {
		return;
	}

	if ( xSemaphoreTake( modulationConfSemaphore, ( TickType_t ) 50 ) == pdTRUE) {
		/* We were able to obtain the semaphore and can now access the
		 shared resource. */

		if ((int) pcVoltageParameter) {
			modulationConfig.voltageRef = (int) pcVoltageParameter;

			/* Print the new mode. */
			memset(pcWriteBuffer, 0x00, xWriteBufferLen);
			strncat(pcWriteBuffer, (char *) pcVoltageParameter,
					(size_t) xParameterStringLength);
			strncat(pcWriteBuffer, "\r\n", strlen("\r\n"));
		} else {
			// Value not recognized, fail.
			memset(pcWriteBuffer, 0x00, xWriteBufferLen);
			strncat(pcWriteBuffer, "Unable to change modulation config. Unknown parameters.\r\n",
			strlen("Unable to change modulation config. Unknown parameters.\r\n"));
		}

		/* Release semaphore */
		xSemaphoreGive(modulationConfSemaphore);
	} else {
		/* We could not obtain the semaphore and can therefore not access
		 * the shared resource safely.
		 */
		memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		strncat(pcWriteBuffer, "Unable to change modulation config. Resource is busy.\r\n",
				strlen("Unable to change modulation config. Resource is busy.\r\n"));

	}
	return pdFALSE;
}

/*
 * Console command for saturation change
 */
static BaseType_t prvSaturationCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
		const char *pcCommandString) {
	char *pcSaturationParameter;
	BaseType_t xParameterStringLength;

	pcSaturationParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1,
			&xParameterStringLength);

	/*
	 * TODO ready for test: handle these separately with a semaphore used in the tStateControl task.
	 *  Should make a separate struct for configuration from console.
	 */

	// check if semaphore exists
	if (modulationConfSemaphore == NULL) {
		return;
	}

	if ( xSemaphoreTake( modulationConfSemaphore, ( TickType_t ) 50 ) == pdTRUE) {
		/* We were able to obtain the semaphore and can now access the
		 shared resource. */

		if ((int) pcSaturationParameter) {
			modulationConfig.saturationLimit = (int) pcSaturationParameter;

			/* Print the new mode. */
			memset(pcWriteBuffer, 0x00, xWriteBufferLen);
			strncat(pcWriteBuffer, (char *) pcSaturationParameter,
					(size_t) xParameterStringLength);
			strncat(pcWriteBuffer, "\r\n", strlen("\r\n"));
		} else {
			// Value not recognized, fail.
			memset(pcWriteBuffer, 0x00, xWriteBufferLen);
			strncat(pcWriteBuffer, "Unable to change modulation config. Unknown parameters.\r\n",
			strlen("Unable to change modulation config. Unknown parameters.\r\n"));
		}

		/* Release semaphore */
		xSemaphoreGive(modulationConfSemaphore);
	} else {
		/* We could not obtain the semaphore and can therefore not access
		 * the shared resource safely.
		 */
		memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		strncat(pcWriteBuffer, "Unable to change modulation config. Resource is busy.\r\n",
		strlen("Unable to change modulation config. Resource is busy.\r\n"));

	}
	return pdFALSE;
}
