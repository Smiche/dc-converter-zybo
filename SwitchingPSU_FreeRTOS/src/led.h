#define LED_0 0
#define LED_1 1
#define LED_2 2
#define LED_3 3

#define RED 0
#define GREEN 1
#define BLUE 2

enum led_status {
	up = 1, down = 0,
};

typedef union {
	struct {
		unsigned char led0 :1;
		unsigned char led1 :1;
		unsigned char led2 :1;
		unsigned char led3 :1;
	} fields;
	int bits;
} LED_STATUS_T;

int init_rgb_led();
void led_set_duty(int led_color, int duty);
void led_set_duty_individual(XTtcPs * colorTTC, int duty);
void set_led(int value);
void led_pwm(int led_pos, double duty_cycle);
