#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <stddef.h>

void temp_sensor_init(void);
float temp_sensor_getTemperature();

#endif

#ifndef DHT_H
#define DHT_H

#define DHT_OK 0
#define DHT_CHECKSUM_ERROR -1
#define DHT_TIMEOUT_ERROR -2

class DHT {

	public:

		DHT();

		void 	setDHTgpio( gpio_num_t gpio);
		void 	errorHandler(int response);
		int 	readDHT();
		float 	getHumidity();
		float 	getTemperature();

	private:

		gpio_num_t DHTgpio;
		float 	humidity = 0.;
		float 	temperature = 0.;

		int 	getSignalLevel( int usTimeOut, bool state );

};

#endif