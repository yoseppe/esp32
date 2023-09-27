#ifndef SOUND_SENSE_H_
#define SOUND_SENSE_H_

#include "driver/gpio.h"

void sound_sensor_init(void);
int sound_sensor_digital_readOnce(void);

#endif