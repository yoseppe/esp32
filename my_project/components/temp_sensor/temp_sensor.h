#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <stddef.h>

void temp_sensor_init(void);
float temp_sensor_getTemperature();

#endif