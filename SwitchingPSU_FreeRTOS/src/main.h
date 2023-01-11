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
extern SemaphoreHandle_t modeSemaphore;

/*
 * Semaphore preventing acces from console when entering config with buttons
 */
extern SemaphoreHandle_t ConfButtonSemaphore;

extern PID_CONFIG_T pidConfig;
extern MODULATION_CONFIG_T modulationConfig;
extern char MODE;
extern char modeChanged;
#endif /* TEST_H__*/
