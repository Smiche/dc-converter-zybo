#include "FreeRTOS.h"
#include "queue.h"

#define POLL_DELAY 50UL

typedef struct INPUT_STATUS {
	char sw0;
	char sw1;
	char sw2;
	char sw3;
	char bt0;
	char bt1;
	char bt2;
	char bt3;
} INPUT_STATUS_T;

extern QueueHandle_t inputs_status_queue;
void init_inputs();
void task_input_watch(void *pvParameters);
