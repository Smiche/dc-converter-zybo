#define DEBUG_ENABLED 0
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c\n"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

typedef struct CONVERTER_CONFIG {
	float Kp;
	float Ki;
	float Kd;
	float voltageRef;
	float saturationLimit;
} CONVERTER_CONFIG_T;

/* Values for state variable */
#define IDLE 0
#define CONFIGURING 1
#define MODULATING 2
#define MAX_STATES 3
