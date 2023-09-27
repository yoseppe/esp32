#include "esp_timer.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "sound_sensor.h"

static const char *TAG = "SOUND_SENSOR";

#define SOUND_SENSOR_GPIO GPIO_NUM_4

void sound_sensor_init(void) {
    // SOUND SENSOR DIGITAL INPUT CONFIG
    gpio_reset_pin(SOUND_SENSOR_GPIO);
    gpio_set_direction(SOUND_SENSOR_GPIO, GPIO_MODE_INPUT);
    // END SOUND SENSOR DIGITAL INPUT CONFIG
}

int sound_sensor_digital_readOnce(void) {
    int value = gpio_get_level(SOUND_SENSOR_GPIO);
    return value;
}