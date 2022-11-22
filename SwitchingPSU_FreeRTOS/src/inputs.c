#include "FreeRTOS.h"
#include <stdio.h>
#include "queue.h"
#include "xgpio.h"
#include "task.h"
#include "xil_printf.h"
#include "utils.h"
#include "inputs.h"

static const char sw_status_queue_length = 5;
QueueHandle_t sw_status_queue;
XGpio gpio;

const TickType_t pollDelay = pdMS_TO_TICKS(POLL_DELAY);

void init_sw_inputs() {
	sw_status_queue = xQueueCreate(sw_status_queue_length, sizeof(SW_STATUS_T)); // init queue with 5 items, item size is 4 bits (1 for each switch)
	XGpio_Initialize(&gpio, XPAR_AXI_GPIO_SW_BTN_DEVICE_ID); // init gpio connected to the switches
}

void task_sw_watch(void *pvParameters) {
	char last_sw_values = 0, sw_values = 0;
	SW_STATUS_T sw_statuses = { 0, 0, 0, 0 };

	while (1) {
		// read current values;
		sw_values = XGpio_DiscreteRead(&gpio, 1);
		// if new values are different, update sw_statuses struct and push the change to the queue
		if (last_sw_values != sw_values) {
			// Debug logging sw status
			if (DEBUG_ENABLED) {
				char str[10];
				sprintf(str, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(sw_values));
				xil_printf("%s", str);
			}

			sw_statuses.sw0 = sw_values & 1;
			sw_statuses.sw1 = sw_values >> 1 & 1;
			sw_statuses.sw2 = sw_values >> 2 & 1;
			sw_statuses.sw3 = sw_values >> 3 & 1;

			xQueueSend(sw_status_queue, &sw_statuses, 10);
			last_sw_values = sw_values;
		}

		vTaskDelay(pollDelay);
	}

}
