#include "xil_io.h"
#include <zynq_registers.h>
#include "xttcps.h"

#include "led.h"

/**
 * PWM TTCPS
 */
typedef struct {
	u32 OutputHz; /* Output frequency */
	XInterval Interval; /* Interval value */
	u8 Prescaler; /* Prescaler value */
	u16 Options; /* Option settings */
} TmrCntrSetup;

#define	TICK_TIMER_FREQ_HZ	100  /* Tick timer counter's output frequency */
static TmrCntrSetup SettingsTable = { TICK_TIMER_FREQ_HZ, 1000, 0,
XTTCPS_OPTION_MATCH_MODE | XTTCPS_OPTION_WAVE_POLARITY };
TmrCntrSetup *TimerSetup = &SettingsTable; // Settings instance pointer.
static XTtcPs rTTC, gTTC, bTTC;

/**
 * Initialize TTC for RGB led
 */
int init_rgb_led() {
	int Status;

	XTtcPs_Config *rTTCConfig, *gTTCConfig, *bTTCConfig;

	// Find each device.
	rTTCConfig = XTtcPs_LookupConfig(XPAR_PS7_TTC_0_DEVICE_ID);
	gTTCConfig = XTtcPs_LookupConfig(XPAR_PS7_TTC_1_DEVICE_ID);
	bTTCConfig = XTtcPs_LookupConfig(XPAR_PS7_TTC_2_DEVICE_ID);

	if (NULL == rTTCConfig || NULL == gTTCConfig || NULL == bTTCConfig) {
		return XST_FAILURE;
	}

	// Initialize with current config.
	Status = XTtcPs_CfgInitialize(&rTTC, rTTCConfig, rTTCConfig->BaseAddress);
	Status = XTtcPs_CfgInitialize(&gTTC, gTTCConfig, gTTCConfig->BaseAddress);
	Status = XTtcPs_CfgInitialize(&bTTC, bTTCConfig, bTTCConfig->BaseAddress);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Set options defined in TimerSetup
	XTtcPs_SetOptions(&rTTC, TimerSetup->Options);
	XTtcPs_SetOptions(&gTTC, TimerSetup->Options);
	XTtcPs_SetOptions(&bTTC, TimerSetup->Options);

	// Recalculate interval
	XTtcPs_CalcIntervalFromFreq(&rTTC, TimerSetup->OutputHz,
			&(TimerSetup->Interval), &(TimerSetup->Prescaler));
	XTtcPs_CalcIntervalFromFreq(&gTTC, TimerSetup->OutputHz,
			&(TimerSetup->Interval), &(TimerSetup->Prescaler));
	XTtcPs_CalcIntervalFromFreq(&bTTC, TimerSetup->OutputHz,
			&(TimerSetup->Interval), &(TimerSetup->Prescaler));

	// Set the interval and prescale
	XTtcPs_SetInterval(&rTTC, TimerSetup->Interval);
	XTtcPs_SetInterval(&gTTC, TimerSetup->Interval);
	XTtcPs_SetInterval(&bTTC, TimerSetup->Interval);

	XTtcPs_SetPrescaler(&rTTC, TimerSetup->Prescaler);
	XTtcPs_SetPrescaler(&gTTC, TimerSetup->Prescaler);
	XTtcPs_SetPrescaler(&bTTC, TimerSetup->Prescaler);

	// Set match to 0, this keeps rgb led OFF
	XTtcPs_SetMatchValue(&rTTC, 0, 0);
	XTtcPs_SetMatchValue(&gTTC, 1, 0);
	XTtcPs_SetMatchValue(&bTTC, 2, 0);

	// Start counters.
	XTtcPs_Start(&rTTC);
	XTtcPs_Start(&gTTC);
	XTtcPs_Start(&bTTC);

	return XST_SUCCESS;
}

/**
 * Set duty cycle 0-100
 */
void led_set_duty(int led_color, int duty) {
	switch (led_color) {
	case RED:
		led_set_duty_individual(&rTTC, duty);
		break;
	case GREEN:
		led_set_duty_individual(&gTTC, duty);
		break;
	case BLUE:
		led_set_duty_individual(&bTTC, duty);
		break;
	}
}

void led_set_duty_individual(XTtcPs * colorTTC, int duty) {
	int step = TimerSetup->Interval / 100;
	XTtcPs_SetMatchValue(colorTTC, 0, duty * step);
}

/**
 * Set LED to ON/OFF
 */
void set_led(int value) {
	LED_STATUS_T led_status = { .bits = value };
	// Read current LED values
//	u8 read = Xil_In8((AXI_LED_DATA_ADDRESS));
//
//	// Change bit at LED address to 0/1
//	if (status == up) {
//		Xil_Out8((AXI_LED_DATA_ADDRESS), (read | 1 << led_pos));
//	} else {
//		Xil_Out8((AXI_LED_DATA_ADDRESS), (read & ~(1 << led_pos)));
//	}
	Xil_Out8((AXI_LED_DATA_ADDRESS), led_status.bits);
}
