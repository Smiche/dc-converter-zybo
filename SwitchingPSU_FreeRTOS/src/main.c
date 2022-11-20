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

/*** Global Variables ***/
#define TIMER_ID	1
#define DELAY_10_SECONDS	10000UL
#define DELAY_1_SECOND		1000UL
#define TIMER_CHECK_THRESHOLD	9
#define mainUART_COMMAND_CONSOLE_STACK_SIZE	( configMINIMAL_STACK_SIZE * 3UL )
#define mainUART_COMMAND_CONSOLE_TASK_PRIORITY	( tskIDLE_PRIORITY )

/* Task functions. */
static void task1(void *pvParameters);
static void task2(void *pvParameters);
static void task3(void *pvParameters);
/*
 * The task that manages the FreeRTOS+CLI input and output.
 */
extern void vUARTCommandConsoleStart(uint16_t usStackSize,
		UBaseType_t uxPriority);

/**
 * Tasks.
 */
static TaskHandle_t task1handle;
static TaskHandle_t task2handle;
static TaskHandle_t task3handle;

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
	 * Switch to INPUT mode
	 */
	Xil_Out32((AXI_SW_TRI_ADDRESS), 0x5);

	/**
	 * LED to OUTPUT mode
	 */
	Xil_Out32((AXI_LED_TRI_ADDRESS), 0x0);
	return init_rgb_led();
}

int main(void) {
	init();

	xTaskCreate(task1, (const char *) "Task1",
	configMINIMAL_STACK_SIZE,
	NULL,
	tskIDLE_PRIORITY, &task1handle);

	xTaskCreate(task2, (const char *) "Task2",
	configMINIMAL_STACK_SIZE,
	NULL,
	tskIDLE_PRIORITY + 1, &task2handle);

	xTaskCreate(task3, (const char *) "Task3",
	configMINIMAL_STACK_SIZE,
	NULL,
	tskIDLE_PRIORITY + 1, &task3handle);

	vTaskStartScheduler();
	xil_printf("main reached end unexpectedly.");
}

/*-----------------------------------------------------------*/
static void task1(void *pvParameters) {
	const TickType_t x1second = pdMS_TO_TICKS(DELAY_1_SECOND);
	int counter = 0;
	for (;;) {
		vTaskDelay(x1second);
		counter++;
		if (counter % 2) {
			set_led(LED_0, up);
			set_led(LED_2, up);
			set_led(LED_1, down);
			set_led(LED_3, down);
		} else {
			set_led(LED_1, up);
			set_led(LED_3, up);
			set_led(LED_0, down);
			set_led(LED_2, down);
			counter = 0;
		}
	}
}

/*-----------------------------------------------------------*/
static void task2(void *pvParameters) {
	const TickType_t ms100 = pdMS_TO_TICKS(100UL);

	int counter = 0;
	int dir = 1;

	for (;;) {
		if (dir) {
			counter+=5;
		} else {
			counter-=5;
		}

		led_set_duty(RED, counter);

		if (counter >= 99) {
			dir = 0;
		} else if (counter <= 1) {
			dir = 1;
		}
		vTaskDelay(ms100);
	}
}

/*-----------------------------------------------------------*/
static void task3(void *pvParameters) {
	const TickType_t x1second = pdMS_TO_TICKS(DELAY_1_SECOND);

	while (1) {
		vTaskDelay(x1second);
		// read switch
		u8 read = Xil_In8((AXI_SW_DATA_ADDRESS));
	}
}
