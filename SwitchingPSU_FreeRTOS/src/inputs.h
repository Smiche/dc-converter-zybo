#include "FreeRTOS.h"
#include "queue.h"

#define POLL_DELAY 50UL

typedef struct SW_STATUS {
	char sw0;
	char sw1;
	char sw2;
	char sw3;
} SW_STATUS_T;

extern QueueHandle_t sw_status_queue;
void init_sw_inputs();
void task_sw_watch(void *pvParameters);
