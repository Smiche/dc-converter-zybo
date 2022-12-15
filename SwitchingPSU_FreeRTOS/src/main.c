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
#define TIMER_CHECK_THRESHOLD	9
#define mainUART_COMMAND_CONSOLE_STACK_SIZE	( configMINIMAL_STACK_SIZE * 3UL )
#define mainUART_COMMAND_CONSOLE_TASK_PRIORITY	( tskIDLE_PRIORITY )
/* Values for state variable */
#define modeIdle 0
#define modeConf 1
#define modeMod 2
#define MAX_STATES 3


/* Task functions. */
static void tIdle(void *pvParameters);
static void tConf(void *pvParameters);
static void tModulate(void *pvParameters);
static void tState(void *pvParameters);
/*
 * The task that manages the FreeRTOS+CLI input and output.
 */
extern void vUARTCommandConsoleStart(uint16_t usStackSize,
		UBaseType_t uxPriority);

/**
 * Tasks.
 */
static TaskHandle_t tIdleHandle;
static TaskHandle_t tConfHandle;
static TaskHandle_t tModulateHandle;
static TaskHandle_t tSWInputHandle;
static TaskHandle_t tStateHandle;

/*
 * Global program variables
 *
 * modeSemaphore is for the tasks idle, conf, modulate. Needs some way to make sure
 * that correct task gets the semaphore and at mode change passes the semaphore for
 * the next mode to be ran.
 *
 * confSemaphore is a semaphore for the configuration with buttons or serial.
 */
SemaphoreHandle_t modeSemaphore;
SemaphoreHandle_t confSemaphore;
float Kp, Ki, Kd, voltageRef, saturation_limit;
int stateVar = 0;



int init() {
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
	init_inputs();
	return init_rgb_led();

	/*
	 * Create semaphores
	 */
	modeSemaphore = xSemaphoreCreateBinary();
	if (modeSemaphore == NULL) {
		// TODO Handle failure in semaphore creation
	}
	confSemaphore = xSemaphoreCreateBinary();
	if (confSemaphore == NULL) {
		// TODO Handle failure in semaphore creation
	}

	/*
	 * Initialize PID parameters and reference voltage
	 * TODO Initialize values
	 */
	Kp = Ki = Kd = voltageRef = 0;
	saturation_limit = 50; //?
}

int main(void) {
	init();

	xTaskCreate(tIdle, (const char *) "Idle",
	configMINIMAL_STACK_SIZE,
	NULL,
	tskIDLE_PRIORITY, &tIdleHandle);

	xTaskCreate(tConf, (const char *) "Configure",
	configMINIMAL_STACK_SIZE,
	NULL,
	tskIDLE_PRIORITY + 1, &tConfHandle);

	xTaskCreate(tModulate, (const char *) "Modulate",
	configMINIMAL_STACK_SIZE,
	NULL,
	tskIDLE_PRIORITY + 1, &tModulateHandle);

	xTaskCreate(task_input_watch, (const char *) "SwIO",
	configMINIMAL_STACK_SIZE,
	NULL,
	tskIDLE_PRIORITY + 1, &tSWInputHandle);

	xTaskCreate(tState, (const char *) "HandleState",
	configMINIMAL_STACK_SIZE,
	NULL,
	tskIDLE_PRIORITY + 2, &tStateHandle);

	vTaskStartScheduler();
	xil_printf("main reached end unexpectedly.");
}

/*-----------------------------------------------------------*/
/*
 * Idle
 * converter off
 */
static void tIdle(void *pvParameters) {
	const TickType_t x1second = pdMS_TO_TICKS(DELAY_1_SECOND);
	for (;;) {
		// Do nothing
		vTaskDelay(x1second);
	}
}

/*-----------------------------------------------------------*/
/*
 * Configuration
 * Select configurable parameter Kp or Ki (or Kd)
 * Increase or decrease value of selected parameter
 *
 * Reference voltage can be adjusted only from console
 */
static void tConf(void *pvParameters) {
	const TickType_t ms100 = pdMS_TO_TICKS(100UL);

	INPUT_STATUS_T input_statuses = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int parameter_select = 1;

	for (;;) {
		/* Wait for semaphore to run */
		if (modeSemaphore != NULL) {
			if (xSemaphoreTake(modeSemaphore, (TickType_t) 10) == pdTRUE) {

				for (;;) {
					/* Check from state variable if permission to run */
					if (stateVar != modeConf) {
						// Give semaphore away
						if (xSemaphoreGive(modeSemaphore) != pdTRUE) {
							// TODO Handle giving semaphore away failed
						}
						// Break to outer loop and wait for next turn
						break;
					}

					// Receive switch value changes through queue.
					if (xQueueReceive(inputs_status_queue, &input_statuses, 10) == pdTRUE) {
						// If second button is pressed changes what PID parameter is altered
						if(input_statuses.bt1 == 1){
							if (parameter_select < 3) {
								parameter_select += 1;
								} else if (parameter_select == 3) {
									parameter_select = 1;
								}
						}
						// If third or fourth button is pressed changes selected PID value up or down
						// Changes Kp +-1, Ki +- 0.01, Kd +- 0,1
						if(input_statuses.bt2 == 1){
							if(parameter_select == 1){
								Kp += 1;
							}else if(parameter_select == 2){
								Ki += 0.01;
							}else if(parameter_select == 3){
								Kd += 0.1;
							}

						}

						if(input_statuses.bt3 == 1){
							if(parameter_select == 1){
								if(Kp > 0){
									Kp -= 1;
								}
							}else if(parameter_select == 2){
								if(Ki > 0){
									Ki -= 0.01;
								}
							}else if(parameter_select == 3){
								if(Ki > 0){
									Kd -= 0.1;
								}
							}
						}
						xil_printf("SW0: %d SW1: %d SW2: %d SW3: %d\n", input_statuses.sw0,
								input_statuses.sw1, input_statuses.sw2, input_statuses.sw3);
						xil_printf("BT0: %d BT1: %d BT2: %d BT3: %d\n", input_statuses.bt0,
								input_statuses.bt1, input_statuses.bt2, input_statuses.bt3);
						// Prints PID values to console
						xil_printf("PID values are:\n");
						xil_printf("Kp: %f Ki: %f Kd: %f\n", Kp, Ki, Kd);
					}

					vTaskDelay(ms100);
				}
			}
			else {
				// Semaphore couldn't be taken.
				// TODO Handle semaphore not taken
			}
		}
	}
}

/*-----------------------------------------------------------*/
/*
 * Modulate
 * converter on, uses PID, reference voltage can be adjusted with console?
 */
static void tModulate(void *pvParameters) {
	const TickType_t x1second = pdMS_TO_TICKS(DELAY_1_SECOND);
	
	INPUT_STATUS_T input_statuses = { 0, 0, 0, 0, 0, 0, 0, 0 };
	float PID_out = 0;
	float voltage = 0;
	
	while (1) {
		vTaskDelay(x1second);
		// read switch
		u8 read = Xil_In8((AXI_SW_DATA_ADDRESS));
		// Receive switch value changes through queue.
		if (xQueueReceive(inputs_status_queue, &input_statuses, 10) == pdTRUE) {
			// Changes reference voltage according which button is pressed
			if(input_statuses.bt2 == 1){
				voltageRef += 1;
			}else if(input_statuses.bt3 == 1){
				voltageRef -= 1;
			}
		}
		// Calculates PID output
		PID_out = PID(Kp, Ki, Kd, voltageRef, voltage, saturation_limit);

		// Calculate converter output
		// TODO check converter model usage
		voltage = model(PID_out);
		// how we print it to console?
		// like this?: xil_printf("u3: %f\n", voltage);

		// TODO show voltage output with rgb led
		// Should the led duty be calculated from the voltage?
		led_set_duty(RED, (int) voltage);
	}
}


// State change handler
static void tState(void *pvParameters) {
	INPUT_STATUS_T input_statuses = { 0, 0, 0, 0, 0, 0, 0, 0 };
	// Receive switch value changes through queue.
	if (xQueueReceive(inputs_status_queue, &input_statuses, 10) == pdTRUE) {
		// If first button pressed
		if (input_statuses.bt0 == 1) {
			// Check for console usage
			// TODO Check for console usage with semaphore
			// Change state
			stateVar++;
			if (stateVar >= MAX_STATES) {
				stateVar = 0;
			}
		}
		// TODO state changes from console
	}
}
