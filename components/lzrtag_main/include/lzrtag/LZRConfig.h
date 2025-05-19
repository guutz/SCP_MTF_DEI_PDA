#include "driver/gpio.h"

#define WS2812_NUMBER 8
#define PIN_IR_OUT GPIO_NUM_16
#define PIN_IR_IN GPIO_NUM_17
#define PIN_WS2812_OUT GPIO_NUM_25
#define MQTT_SERVER_ADDR "mqtt://mqtt.eclipse.org" // Example MQTT server