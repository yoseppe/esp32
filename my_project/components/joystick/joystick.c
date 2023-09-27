#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <sys/time.h>
#include "oled_display.h"
#include "spyrosoftImages.h"

#include "soc/soc_caps.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

#include "fonts.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

//#include "temp_sensor.h"
#include "v2.h"
#include "sound_sensor.h"

static const char *TAG = "JOYSTICK";

#define TEMP_SENSOR_GPIO GPIO_NUM_19
#define PUSH_BUTTON_GPIO GPIO_NUM_5
#define PHYSICAL_SWITCH_GPIO GPIO_NUM_18

#if CONFIG_IDF_TARGET_ESP32
#define EXAMPLE_ADC1_CHAN0 ADC_CHANNEL_6
#define EXAMPLE_ADC1_CHAN1 ADC_CHANNEL_7
#define EXAMPLE_ADC_ATTEN ADC_ATTEN_DB_11
#endif

//=============================================================================================
//=====================TIME INJECTED FROM MAIN=================================================
//=============================================================================================
#include "esp_netif_sntp.h"
void time_sync_notification_cb(struct timeval *tv);
static void obtain_time(void);
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}
static void obtain_time(void)
{

#if LWIP_DHCP_GET_NTP_SRV
    ESP_LOGI(TAG, "Starting SNTP");
    esp_netif_sntp_start();
#if LWIP_IPV6 && SNTP_MAX_SERVERS > 2
    ip_addr_t ip6;
    if (ipaddr_aton("2a01:3f7::1", &ip6)) {    // ipv6 ntp source "ntp.netnod.se"
        esp_sntp_setserver(2, &ip6);
    }
#endif

#else
    ESP_LOGI(TAG, "Initializing and starting SNTP");
#if CONFIG_LWIP_SNTP_MAX_SERVERS > 1
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2,
                               ESP_SNTP_SERVER_LIST(CONFIG_SNTP_TIME_SERVER, "pool.ntp.org" ) );
#else
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
#endif
    config.sync_cb = time_sync_notification_cb;     // Note: This is only needed if we want
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    config.smooth_sync = true;
#endif

    esp_netif_sntp_init(&config);
#endif

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}
char strftime_buf[64];
struct tm timeinfo;
time_t now;

void updateTime(void) {
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        time(&now);
    }
    // Set timezone to Zagreb time and print it
    setenv("TZ", "GMT-2", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
}
//=============================================================================================
//=====================TIME INJECTED FROM MAIN=================================================
//=============================================================================================

static int adc_raw[2];
static int voltage[2];
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);

adc_oneshot_unit_handle_t adc1_handle;

enum inputs {
    INPUT_PUSH_BUTTON,
    INPUT_UP_ARROW,
    INPUT_DOWN_ARROW,
    INPUT_RIGHT_ARROW,
    INPUT_LEFT_ARROW,
};

enum menuSelectedItem {
    MENU_ITEM_1,
    MENU_ITEM_2,
    MENU_ITEM_3,
    MENU_ITEM_4,
    STATE_TIME,
    STATE_TEMPERATURE,
    STATE_3,
    STATE_4
};

enum menuSelectedItem currentlySelectedItem = MENU_ITEM_1;

bool invertImages = false;

void display_sendNumber(int num, bool invertImages, int xpos, int ypos, int width, int height) {
    display_sendChar(&console_font_8x8[(0x30+num)*8], invertImages, xpos, ypos, width, height);
}

void display_sendLetter(char letter, bool invertImages, int xpos, int ypos, int width, int height) {
    display_sendChar(&console_font_8x8[(0x41+letter)*8], invertImages, xpos, ypos, width, height);
}

void enterState_TIME(void) {
    updateTime();
    ESP_LOGI(TAG, "The current date/time in Zagreb is: %s", strftime_buf);

    int hour = timeinfo.tm_hour;
    int min = timeinfo.tm_min;
    int sec = timeinfo.tm_sec;
    int day = timeinfo.tm_mday;
    int weekDay = timeinfo.tm_wday;
    int month = timeinfo.tm_mon+1;
    int year = timeinfo.tm_year+1900;

    ESP_LOGI(TAG, "Hours: %d", hour);
    ESP_LOGI(TAG, "Minutes: %d", min);
    ESP_LOGI(TAG, "Seconds: %d", sec);
    ESP_LOGI(TAG, "Day: %d", day);
    ESP_LOGI(TAG, "Month: %d", month);
    ESP_LOGI(TAG, "Year: %d", year);


    display_sendImage(image_blank, invertImages);



    display_sendNumber(day / 10, invertImages, 20, 10, 8, 8);
    display_sendNumber(day % 10, invertImages, 28, 10, 8, 8);
    display_sendChar(&console_font_8x8[('.')*8], invertImages, 36, 10, 8, 8);
    display_sendNumber(month / 10, invertImages, 44, 10, 8, 8);
    display_sendNumber(month % 10, invertImages, 52, 10, 8, 8);
    display_sendChar(&console_font_8x8[('.')*8], invertImages, 60, 10, 8, 8);
    display_sendNumber(year / 1000, invertImages, 68, 10, 8, 8);
    display_sendNumber(year / 100 % 10, invertImages, 76, 10, 8, 8);
    display_sendNumber(year / 10 % 100, invertImages, 84, 10, 8, 8);
    display_sendNumber(year % 10, invertImages, 92, 10, 8, 8);
    display_sendChar(&console_font_8x8[('.')*8], invertImages, 100, 10, 8, 8);

    display_sendNumber(hour / 10, invertImages, 30, 30, 8, 8);
    display_sendNumber(hour % 10, invertImages, 38, 30, 8, 8);
    display_sendChar(&console_font_8x8[(':')*8], invertImages, 46, 30, 8, 8);
    display_sendNumber(min / 10, invertImages, 54, 30, 8, 8);
    display_sendNumber(min % 10, invertImages, 62, 30, 8, 8);
    display_sendChar(&console_font_8x8[(':')*8], invertImages, 70, 30, 8, 8);
    display_sendNumber(sec / 10, invertImages, 78, 30, 8, 8);
    display_sendNumber(sec % 10, invertImages, 86, 30, 8, 8);
}

void enterState_TEMPERATURE(void) {
    display_sendImage(image_blank, invertImages);

    DHT11_init(TEMP_SENSOR_GPIO);
    int temp = DHT11_read().temperature;
    int hum = DHT11_read().humidity;

    if(temp) 
    printf("Temperature is %d \n", DHT11_read().temperature);
    printf("Humidity is %d\n", DHT11_read().humidity);
    printf("Status code is %d\n", DHT11_read().status);

    display_sendChar(&console_font_8x8[('T')*8], invertImages, 0+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[('e')*8], invertImages, 8+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[('m')*8], invertImages, 16+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[('p')*8], invertImages, 24+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[('e')*8], invertImages, 32+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[('r')*8], invertImages, 40+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[('a')*8], invertImages, 48+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[('t')*8], invertImages, 56+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[('u')*8], invertImages, 64+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[('r')*8], invertImages, 72+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[('e')*8], invertImages, 80+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[(':')*8], invertImages, 88+2, 10, 8, 8);

    display_sendNumber( temp / 10, invertImages, 96+2, 10, 8, 8);
    display_sendNumber( temp % 10, invertImages, 104+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[(0x100)*8], invertImages, 112+2, 10, 8, 8);
    display_sendChar(&console_font_8x8[(0x43)*8], invertImages, 120+2, 10, 8, 8);

    display_sendChar(&console_font_8x8[('H')*8], invertImages, 0+2, 30, 8, 8);
    display_sendChar(&console_font_8x8[('u')*8], invertImages, 8+2, 30, 8, 8);
    display_sendChar(&console_font_8x8[('m')*8], invertImages, 16+2, 30, 8, 8);
    display_sendChar(&console_font_8x8[('i')*8], invertImages, 24+2, 30, 8, 8);
    display_sendChar(&console_font_8x8[('d')*8], invertImages, 32+2, 30, 8, 8);
    display_sendChar(&console_font_8x8[('i')*8], invertImages, 40+2, 30, 8, 8);
    display_sendChar(&console_font_8x8[('t')*8], invertImages, 48+2, 30, 8, 8);
    display_sendChar(&console_font_8x8[('y')*8], invertImages, 56+2, 30, 8, 8);
    display_sendChar(&console_font_8x8[(':')*8], invertImages, 64+2, 30, 8, 8);

    display_sendNumber( hum / 10, invertImages, 72+2, 30, 8, 8);
    display_sendNumber( hum % 10, invertImages, 80+2, 30, 8, 8);
    display_sendChar(&console_font_8x8[(0x25)*8], invertImages, 88+2, 30, 8, 8);
}

void enterState_3(void) { // SOUND
    display_sendImage(image_blank, invertImages);
    sound_sensor_init();
    enum before {
        DETECTED,
        LISTENING
    };
    enum before lastState = LISTENING;
    while(1) {
        if(sound_sensor_digital_readOnce() == 1) {
            if(lastState == LISTENING) {
                ESP_LOGI(TAG, "CHANGED MODE FROM LISTENING TO DETECTEDDDDDDDDDDDDDDDDDD+++++++++");
                display_sendImage(image_soundDetected, invertImages);
                lastState = DETECTED;
                //vTaskDelay(pdMS_TO_TICKS(5));
            }
        } else {
            if(lastState == DETECTED) {
                ESP_LOGI(TAG, "CHANGED MODE FROM DETECTED TO LISTENINGGGGGGGGGGGGGGGGGG--------");
                display_sendImage(image_blank, invertImages);
                lastState = LISTENING;
                //vTaskDelay(pdMS_TO_TICKS(5));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void enterState_4(void) { // QR CODE
    display_sendImage(image_blank, invertImages);
    display_sendChar(image_spyroQRcode, false, 32, 0, 64, 64);
}

void inputHandler(int input) {
    if (input == INPUT_PUSH_BUTTON) {
        //display_sendImage(image_buttonPushed);
        //vTaskDelay(pdMS_TO_TICKS(100));
        switch(currentlySelectedItem) {
            case MENU_ITEM_1:
                enterState_TIME();
                currentlySelectedItem = STATE_TIME;
            break;
            case MENU_ITEM_2:
                enterState_TEMPERATURE();
                currentlySelectedItem = STATE_TEMPERATURE;
            break;
            case MENU_ITEM_3:
                enterState_3();
                currentlySelectedItem = STATE_3;
            break;
            case MENU_ITEM_4:
                enterState_4();
                currentlySelectedItem = STATE_4;
            break;
            default:
            break;
        }
    }
    switch(currentlySelectedItem) {
        case STATE_TIME:
            if (input == INPUT_LEFT_ARROW) {
                display_sendImage(image_menuItem1Selected, invertImages);
                currentlySelectedItem = MENU_ITEM_1;
            }
        break;
        case STATE_TEMPERATURE:
            if (input == INPUT_LEFT_ARROW) {
                display_sendImage(image_menuItem2Selected, invertImages);
                currentlySelectedItem = MENU_ITEM_2;
            }
        break;
        case STATE_3:
            if (input == INPUT_LEFT_ARROW) {
                display_sendImage(image_menuItem3Selected, invertImages);
                currentlySelectedItem = MENU_ITEM_3;
            }
        break;
        case STATE_4:
            if (input == INPUT_LEFT_ARROW) {
                display_sendImage(image_menuItem4Selected, invertImages);
                currentlySelectedItem = MENU_ITEM_4;
            }
        break;
        case MENU_ITEM_1:
            switch(input) {
                case INPUT_DOWN_ARROW:
                    display_sendImage(image_menuItem3Selected, invertImages);
                    currentlySelectedItem = MENU_ITEM_3;
                break;
                case INPUT_RIGHT_ARROW:
                    display_sendImage(image_menuItem2Selected, invertImages);
                    currentlySelectedItem = MENU_ITEM_2;
                break;
                default:
                break;
            }
        break;
        case MENU_ITEM_2:
            switch(input) {
                case INPUT_DOWN_ARROW:
                    display_sendImage(image_menuItem4Selected, invertImages);
                    currentlySelectedItem = MENU_ITEM_4;
                break;
                case INPUT_LEFT_ARROW:
                    display_sendImage(image_menuItem1Selected, invertImages);
                    currentlySelectedItem = MENU_ITEM_1;
                break;
                default:
                break;
            }
        break;
        case MENU_ITEM_3:
            switch(input) {
                case INPUT_UP_ARROW:
                    display_sendImage(image_menuItem1Selected, invertImages);
                    currentlySelectedItem = MENU_ITEM_1;
                break;
                case INPUT_RIGHT_ARROW:
                    display_sendImage(image_menuItem4Selected, invertImages);
                    currentlySelectedItem = MENU_ITEM_4;
                break;
                default:
                break;
            }
        break;
        case MENU_ITEM_4:
            switch(input) {
                case INPUT_UP_ARROW:
                    display_sendImage(image_menuItem2Selected, invertImages);
                    currentlySelectedItem = MENU_ITEM_2;
                break;
                case INPUT_LEFT_ARROW:
                    display_sendImage(image_menuItem3Selected, invertImages);
                    currentlySelectedItem = MENU_ITEM_3;
                break;
                default:
                break;
            }
        break;
        default:
        break;
    }
}

void joystick_init(void)
{
    // PUSH BUTTON CONFIG
    gpio_reset_pin(PUSH_BUTTON_GPIO);
    gpio_set_direction(PUSH_BUTTON_GPIO, GPIO_MODE_INPUT);
    // END PUSH BUTTON CONFIG

    // PHYSICAL SWITCH CONFIG
    gpio_reset_pin(PHYSICAL_SWITCH_GPIO);
    gpio_set_direction(PHYSICAL_SWITCH_GPIO, GPIO_MODE_INPUT);
    // END PHYSICAL SWITCH CONFIG

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = EXAMPLE_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN1, &config));

    //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    adc_cali_handle_t adc1_cali_chan1_handle = NULL;
    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN0, EXAMPLE_ADC_ATTEN, &adc1_cali_chan0_handle);
    bool do_calibration1_chan1 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN1, EXAMPLE_ADC_ATTEN, &adc1_cali_chan1_handle);
}

void joystick_startReadingStates(void) {
    int cnt = 0;
    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw[0]));
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN1, &adc_raw[1]));
        
        //ESP_LOGI(TAG, "raw X: %d", adc_raw[0]);
        //ESP_LOGI(TAG, "raw Y: %d", adc_raw[1]);
        //ESP_LOGI(TAG, "cnt = %d", cnt);

        if(gpio_get_level(PHYSICAL_SWITCH_GPIO) == 0) { // if turned ON
            //ESP_LOGI(TAG, "SSSSSSSSSSSSSSSSSSSSSSSSSSWITCH TURNED ON");
            invertImages = true;
            display_invertEverythingNow();
            //ESP_LOGI(TAG, "bool invertImages = %d", invertImages);
        } else {
            //ESP_LOGI(TAG, "SSSSSSSSSSSSSSSSSSSSSSSSSSWITCH TURNED OFF");
            invertImages = false;
            //ESP_LOGI(TAG, "bool invertImages = %d", invertImages);
        }

        if(cnt > 40) {
            //display_sendImage(image_spyroLogo9);
            cnt = 0;
        } else {
            if(gpio_get_level(PUSH_BUTTON_GPIO) == 0) {
                cnt = 0;
                ESP_LOGI(TAG, "oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooobuttonPUSH");
                //display_sendImage(image_buttonPushed);
                inputHandler(INPUT_PUSH_BUTTON);
                while(gpio_get_level(PUSH_BUTTON_GPIO) != 1) {
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                ESP_LOGI(TAG, "================================================================================centre");
            }
            else if(adc_raw[0] <= 1000) {
                cnt = 0;
                ESP_LOGI(TAG, "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<LEFT");
                //display_sendImage(image_leftArrow);
                inputHandler(INPUT_LEFT_ARROW);
                while(adc_raw[0] < 1500) {
                    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw[0]));
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                ESP_LOGI(TAG, "================================================================================centre");
            }
            else if(adc_raw[0] >= 3500) {
                cnt = 0;
                ESP_LOGI(TAG, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>RIGHT");
                //display_sendImage(image_rightArrow);
                inputHandler(INPUT_RIGHT_ARROW);
                while(adc_raw[0] < 1500 || adc_raw[0] > 2500) {
                    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw[0]));
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                ESP_LOGI(TAG, "================================================================================centre");
            }
            else if(adc_raw[1] <= 500) {
                cnt = 0;
                ESP_LOGI(TAG, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^UP");
                //display_sendImage(image_upArrow);
                inputHandler(INPUT_UP_ARROW);
                while(adc_raw[1] < 1500 || adc_raw[1] > 2500) {
                    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN1, &adc_raw[1]));
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                ESP_LOGI(TAG, "================================================================================centre");
            }
            else if(adc_raw[1] >= 4000) {
                cnt = 0;
                ESP_LOGI(TAG, "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||DOWN");
                //display_sendImage(image_downArrow);
                inputHandler(INPUT_DOWN_ARROW);
                while(adc_raw[1] < 1500 || adc_raw[1] > 2500) {
                    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN1, &adc_raw[1]));
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                ESP_LOGI(TAG, "================================================================================centre");
            }
            else {
                vTaskDelay(pdMS_TO_TICKS(50));
                cnt++;
            }
        }
    }
}

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}