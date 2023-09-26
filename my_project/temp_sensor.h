#ifndef DHT11_H_  
#define DHT11_H_

#define DHT_TIMEOUT_ERROR -2
#define DHT_CHECKSUM_ERROR -1
#define DHT_OKAY  0

// function prototypes

//Start by using this function
//Do not need to touch these three
void temp_sensor_init();
void errorHandle(int response);

//To get all 3 measurements in an array use
int temp_sensor_getData(int type);
//call each function for live temperature updates
//if you only need one measurements use these functions
int temp_sensor_getFtemp();
int temp_sensor_getTemp();
int temp_sensor_getHumidity();
#endif