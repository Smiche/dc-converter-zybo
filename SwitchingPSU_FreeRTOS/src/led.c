#include "xil_io.h"
#include <zynq_registers.h>

#include "led.h"
/**
 * Set LED to ON/OFF
 */
void set_led(int led_pos, enum led_status status) {
	// Read current LED values
	u8 read = Xil_In8((AXI_LED_DATA_ADDRESS));

	// Change bit at LED address to 0/1
	if (status == up) {
		Xil_Out8((AXI_LED_DATA_ADDRESS), (read | 1 << led_pos));
	} else {
		Xil_Out8((AXI_LED_DATA_ADDRESS), (read & ~(1 << led_pos)));
	}
}

/**
 * LED PWM
 * duty_cycle within 0-1
 */
void led_pwm(int led_pos, double duty_cycle) {

}
