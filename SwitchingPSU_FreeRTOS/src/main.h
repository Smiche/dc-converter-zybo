#include "utils.h"
#ifndef TEST_H__
#define TEST_H__

/**
 * Semaphore guarding the modulationConfig struct.
 */
extern SemaphoreHandle_t modulationConfSemaphore;

/*
 * Semaphore guarding the pidConfig struct
 */
extern SemaphoreHandle_t pidConfSemaphore;

extern PID_CONFIG_T pidConfig;
extern MODULATION_CONFIG_T modulationConfig;
#endif /* TEST_H__*/
