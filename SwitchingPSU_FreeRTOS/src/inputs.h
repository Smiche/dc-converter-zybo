#include "FreeRTOS.h"
#include "queue.h"

#define POLL_DELAY 50UL

typedef struct INPUT_STATUS {
	char sw0;
	char sw1;
	char sw2;
	char sw3;
	char bt0;		// Mode select (idle, configuration, modulate)
	char bt1;		// Parameter select (Kp, Ki) (only in configuration mode)
	char bt2;		// Decrease selected parameters value (only in configuration mode)
	char bt3;		// Increase selected parameters value (only in configuration mode)
} INPUT_STATUS_T;

extern QueueHandle_t inputs_status_queue;
void init_inputs();
void task_input_watch(void *pvParameters);
