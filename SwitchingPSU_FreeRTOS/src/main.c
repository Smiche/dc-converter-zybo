/*
 Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 Copyright (C) 2012 - 2018 Xilinx, Inc. All Rights Reserved.

 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 the Software, and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software. If you wish to use our Amazon
 FreeRTOS name, please do so in a fair use way that does not cause confusion.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 http://www.FreeRTOS.org
 http://aws.amazon.com/freertos


 1 tab == 4 spaces!
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
/* Xilinx includes. */
#include "xil_types.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xttcps.h"
#include "xparameters.h"
#include "xscugic.h"
#include "xuartps_hw.h"
#include "xgpio.h"
/* Sample commands from FreeRTOS-CLI*/
#include "Sample-CLI-commands.h"

#include <zynq_registers.h> // Defines hardware registers
#include "led.h"
#include "inputs.h"
#include "utils.h"
#include "PID.h"
#include "converter.h"

/*** Global Variables ***/
#define TIMER_ID	1
#define DELAY_10_SECONDS	10000UL
#define DELAY_1_SECOND		1000UL
#define DELAY_100_MS		100UL
#define TIMER_CHECK_THRESHOLD	9
#define mainUART_COMMAND_CONSOLE_STACK_SIZE	( configMINIMAL_STACK_SIZE * 3UL )
#define mainUART_COMMAND_CONSOLE_TASK_PRIORITY	( tskIDLE_PRIORITY )
#define MAX_VOLTAGE 100 // TODO value for max voltage should be calculated somehow.
#define IDLE_TASK_NAME "Idle"
#define MODULATING_TASK_NAME "Modulating"

/* Task functions. */
static void tIdle(void *pvParameters);
static void tStateControl(void *pvParameters);
static void tModulate(void *pvParameters);
// Method handling Configuration mode.
static void ConfigurePID(INPUT_STATUS_T *inputs,
		unsigned char *parameter_select);
static void ConfigureMOD(INPUT_STATUS_T *inputs);

static void createIdleTask();
static void createModulatingTask();

/*
 * The task that manages the FreeRTOS+CLI input and output.
 */
extern void vUARTCommandConsoleStart(uint16_t usStackSize,
		UBaseType_t uxPriority);

/**
 * Task handles.
 */
static TaskHandle_t tIdleHandle;
static TaskHandle_t tModulateHandle;
static TaskHandle_t tSWInputHandle;
static TaskHandle_t tStateHandle;

/**
 * Semaphore guarding the modulationConfig struct.
 */
SemaphoreHandle_t modulationConfSemaphore;
static char MODE = IDLE;

PID_CONFIG_T pidConfig = { 0, 0, 0 };
MODULATION_CONFIG_T modulationConfig = { 0, 50, 0 };

int init() {
	int Status;
	/**
	 * Register commands from FreeRTOS-CLI.
	 */
	vRegisterSampleCLICommands();

	/**
	 * Begin listening for UART commands.
	 */
	vUARTCommandConsoleStart( mainUART_COMMAND_CONSOLE_STACK_SIZE,
	mainUART_COMMAND_CONSOLE_TASK_PRIORITY);

	/**
	 * LED to OUTPUT mode
	 */
	Xil_Out32((AXI_LED_TRI_ADDRESS), 0x0);
	/**
	 * Init Switch/Button inputs
	 */
	Status = init_inputs();
	if (Status == XST_FAILURE) {
		xil_printf("Inputs initialization failed.");
	}

	/**
	 * Init RGB LED
	 */
	Status = init_rgb_led();
	if (Status == XST_FAILURE) {
		xil_printf("RGB led initialization failed.");
	}

	/*
	 * Create semaphore
	 */
	modulationConfSemaphore = xSemaphoreCreateBinary();
	if (modulationConfSemaphore == NULL) {
		// TODO Handle failure in semaphore creation
	} else {
		xSemaphoreGive(modulationConfSemaphore);
	}

	return Status;
}

int main(void) {
	init();

	xTaskCreate(tStateControl, (const char *) "StateController",
	configMINIMAL_STACK_SIZE * 2, // Double stack size for printf
	NULL,
	tskIDLE_PRIORITY + 1, &tStateHandle);

	xTaskCreate(task_input_watch, (const char *) "SwIO",
	configMINIMAL_STACK_SIZE,
	NULL,
	tskIDLE_PRIORITY + 1, &tSWInputHandle);

	vTaskStartScheduler();
	xil_printf("main reached end unexpectedly.");
}

/*-----------------------------------------------------------*/
static void tStateControl(void *pvParameters) {
	const TickType_t ms100 = pdMS_TO_TICKS(100UL);
	const TickType_t x1second = pdMS_TO_TICKS(DELAY_1_SECOND);

	INPUT_STATUS_T input_statuses = { 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned char parameter_select = 1;
	char xStatus = 0;
	unsigned char modeCounter = 0; // 0-255 with overflow
	MODE = IDLE;
	char modeChanged = 1;

	while (1) {
		// Receive switch value changes through queue.
		xStatus = xQueueReceive(inputs_status_queue, &input_statuses, 50);
		vTaskDelay(ms100);

		// mode switching is always possible
		if (input_statuses.bt0) {
			modeCounter++;
			MODE = modeCounter % MAX_STATES; // limit mode to 2
			modeChanged = 1;
			// Giving some time for task loops to exit before starting new tasks.
			// Tasks must exit within this time.
			vTaskDelay(x1second);
			xil_printf("Mode changed to: %d \n", MODE);
		}

		// display current mode with the green LEDs too
		if (modeChanged) {
			set_led(1 << MODE);
		}

		switch (MODE) {
		case IDLE:
			if (!modeChanged) {
				break;
			}

			createIdleTask();

			break;
		case CONFIGURING:
			// Check if any input is present, then pass to Configure
			if (xStatus == pdPASS) {
				ConfigurePID(&input_statuses, &parameter_select);
				vTaskDelay(ms100);
			}
			break;
		case MODULATING:
			// Check if any input is present, then update modulation config
			if (xStatus == pdPASS) {
				ConfigureMOD(&input_statuses);
			}

			if (!modeChanged) {
				break;
			}

			createModulatingTask();

			break;
		default:
			xil_printf("Uknown state reached. \n");
		}

		modeChanged = 0;
	}

}

/*-----------------------------------------------------------*/
/*
 * Idle
 * converter off
 */
static void tIdle(void *pvParameters) {
	const TickType_t x1second = pdMS_TO_TICKS(DELAY_1_SECOND);
	while (MODE == IDLE) {
		// Do nothing
		vTaskDelay(x1second);
		xil_printf("Idling. \n");
	}
	vTaskDelete( NULL);
}

/*
 * Configuration
 * Select configurable parameter Kp or Ki (or Kd)
 * Increase or decrease value of selected parameter
 *
 * Reference voltage can be adjusted only from console
 */
static void ConfigurePID(INPUT_STATUS_T *inputs,
		unsigned char *parameter_select) {
	float * configPtr = &pidConfig.Kp;

	if (inputs->bt1 == 1) {
		*parameter_select += 1;
		*parameter_select = *parameter_select % 3;
	}
	// If third or fourth button is pressed changes selected PID value up or down
	// Changes Kp +-1, Ki +- 0.01, Kd +- 0,1
	if (inputs->bt2 == 1) {
		*(configPtr + *parameter_select) += 0.1;
	}

	if (inputs->bt3 == 1) {
		if (*(configPtr + *parameter_select) - 0.1 >= 0) {
			*(configPtr + *parameter_select) -= 0.1;
		}
	}

	// Prints PID values to console
	if (inputs->bt2 || inputs->bt3) {
		xil_printf("PID values changed:\n");
		printf("Kp: %f Ki: %f Kd: %f \n", pidConfig.Kp, pidConfig.Ki,
				pidConfig.Kd);
	}
}

/*-----------------------------------------------------------*/
/*
 * Modulate
 * converter on, uses PID, reference voltage can be adjusted with console?
 */
static void tModulate(void *pvParameters) {
	const TickType_t xHalfSecond = pdMS_TO_TICKS(500UL);
	const TickType_t x100ms = pdMS_TO_TICKS(DELAY_100_MS);

	float PID_out = 0;
	float voltage = 0;

	while (MODE == MODULATING) {
		// check that modulation conf semaphore exists
		if (modulationConfSemaphore == NULL) {
			vTaskDelay(x100ms);
			continue;
		}

		if ( xSemaphoreTake(modulationConfSemaphore,
				(TickType_t ) 100) == pdTRUE) {

			// Calculates PID output
			PID_out = PID(pidConfig.Kp, pidConfig.Ki, pidConfig.Kd,
					modulationConfig.voltageRef, voltage,
					modulationConfig.saturationLimit);

			// Calculate converter output
			// TODO check converter model usage
			voltage = model(PID_out);
			// how we print it to console?
			// like this?: xil_printf("u3: %f\n", voltage);

			// debug print, can be used to plot a graph from serial output
			if (modulationConfig.debugModulation) {
				printf("$%f %f;", voltage, PID_out);
			}

			// Show voltage output as a percentage of max voltage
			led_set_duty(RED, (voltage / (float) MAX_VOLTAGE) * 100);
			xSemaphoreGive(modulationConfSemaphore);
		} else {
			xil_printf(
					"Unable to change modulation config. Resource is busy. Will retry in half a second.");
			vTaskDelay(xHalfSecond);
		}

	}

	vTaskDelete( NULL);
}

void ConfigureMOD(INPUT_STATUS_T *inputs) {
	// check if semaphore exists
	if (modulationConfSemaphore == NULL) {
		return;
	}

	if ( xSemaphoreTake( modulationConfSemaphore, ( TickType_t ) 50 ) == pdTRUE) {
		/* We were able to obtain the semaphore and can now access the
		 shared resource. */

		if (inputs->bt2 == 1) {
			modulationConfig.voltageRef += 0.5;
		} else if (inputs->bt3 == 1) {
			modulationConfig.voltageRef -= 0.5;
		}

		if (inputs->sw0 == 1) {
			xil_printf("Enabling modulation debug prints.");
			modulationConfig.debugModulation = 1;
		} else {
			modulationConfig.debugModulation = 0;
		}

		if (inputs->bt2 || inputs->bt3) {
			printf("Reference voltage changed to: %f \n",
					modulationConfig.voltageRef);
		}
		/* We have finished accessing the shared resource.  Release the
		 semaphore. */
		xSemaphoreGive(modulationConfSemaphore);
	} else {
		/* We could not obtain the semaphore and can therefore not access
		 the shared resource safely. */
		xil_printf("Unable to change modulation config. Resource is busy.");
	}
}

static void createIdleTask() {
	xTaskCreate(tIdle, (const char *) IDLE_TASK_NAME,
	configMINIMAL_STACK_SIZE,
	NULL,
	tskIDLE_PRIORITY, &tIdleHandle);
}

static void createModulatingTask() {
	xTaskCreate(tModulate, (const char *) MODULATING_TASK_NAME,
	configMINIMAL_STACK_SIZE,
	NULL,
	tskIDLE_PRIORITY, &tModulateHandle);
}

