#include "FreeRTOS.h"
#include <stdio.h>
#include "queue.h"
#include "xgpio.h"
#include "task.h"
#include "xil_printf.h"
#include "utils.h"
#include "inputs.h"

static const char input_status_queue_length = 5;
QueueHandle_t inputs_status_queue;
// GPIO instance for the switches
XGpio gpio;

const TickType_t pollDelay = pdMS_TO_TICKS(POLL_DELAY);

void init_inputs() {
	inputs_status_queue = xQueueCreate(input_status_queue_length,
			sizeof(INPUT_STATUS_T)); // init queue which can hold 5 items
	XGpio_Initialize(&gpio, XPAR_AXI_GPIO_SW_BTN_DEVICE_ID); // init gpio connected to the switches and buttons
	/**
	 * Switch to INPUT mode for chan 1 and chan 2
	 */
	XGpio_SetDataDirection(&gpio, 1, 0x5);
	XGpio_SetDataDirection(&gpio, 2, 0x5);
}

void task_input_watch(void *pvParameters) {
	char last_sw_values = 0, sw_values = 0, last_bt_values = 0, bt_values = 0;
	INPUT_STATUS_T input_statuses = { 0, 0, 0, 0, 0, 0, 0, 0 };

	while (1) {
		// read current values;
		sw_values = XGpio_DiscreteRead(&gpio, 1);
		bt_values = XGpio_DiscreteRead(&gpio, 2);

		// if new values are different, update sw_statuses struct and push the change to the queue
		if (last_sw_values != sw_values || last_bt_values != bt_values) {
			// Debug logging sw status
			if (DEBUG_ENABLED) {
				char str[10];
				sprintf(str, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(sw_values));
				xil_printf("%s", str);
			}

			// Update values in the struct and send it to the queue.
			input_statuses.sw0 = sw_values & 1;
			input_statuses.sw1 = sw_values >> 1 & 1;
			input_statuses.sw2 = sw_values >> 2 & 1;
			input_statuses.sw3 = sw_values >> 3 & 1;

			input_statuses.bt0 = bt_values & 1;				// Mode select (idle, configuration, modulate)
			input_statuses.bt1 = bt_values >> 1 & 1;		// Parameter select (Kp, Ki) (only in configuration mode)
			input_statuses.bt2 = bt_values >> 2 & 1;		// Decrease selected parameters value (only in configuration mode)
			input_statuses.bt3 = bt_values >> 3 & 1;		// Increase selected parameters value (only in configuration mode)

			xQueueSend(inputs_status_queue, &input_statuses, 10);
			// save the changes locally
			last_sw_values = sw_values;
			last_bt_values = bt_values;
		}

		vTaskDelay(pollDelay);
	}

}
