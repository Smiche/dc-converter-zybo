#include "FreeRTOS.h"
#include "queue.h"

#define POLL_DELAY 50UL

extern QueueHandle_t sw_status_queue;
void init_sw_inputs();
void task_sw_watch(void *pvParameters);
