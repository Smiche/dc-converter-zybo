#define LED_0 0
#define LED_1 1
#define LED_2 2
#define LED_3 3

enum led_status {
	up = 1, down = 0,
};

void set_led(int led_mask, enum led_status status);
void led_pwm(int led_pos, double duty_cycle);
