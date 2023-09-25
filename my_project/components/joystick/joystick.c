#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#include "driver/gpio.h"
#include "sdkconfig.h"

#define PUSH_BUTTON_GPIO GPIO_NUM_5

#if CONFIG_IDF_TARGET_ESP32
#define EXAMPLE_ADC1_CHAN0 ADC_CHANNEL_6
#define EXAMPLE_ADC1_CHAN1 ADC_CHANNEL_7
#define EXAMPLE_ADC_ATTEN ADC_ATTEN_DB_11
#endif

static const char *TAG = "JOYSTICK";

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

void enterState_TIME(void) {
    display_sendImage(image_FERLogo);
}

void enterState_TEMPERATURE(void) {
    display_sendImage(image_spyroLogo9);
}

void enterState_3(void) {
    display_sendImage(image_spyroLogo7);
}

void enterState_4(void) {
    display_sendImage(image_spyroLogo11);
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
                display_sendImage(image_menuItem1Selected);
                currentlySelectedItem = MENU_ITEM_1;
            }
        break;
        case STATE_TEMPERATURE:
            if (input == INPUT_LEFT_ARROW) {
                display_sendImage(image_menuItem2Selected);
                currentlySelectedItem = MENU_ITEM_2;
            }
        break;
        case STATE_3:
            if (input == INPUT_LEFT_ARROW) {
                display_sendImage(image_menuItem3Selected);
                currentlySelectedItem = MENU_ITEM_3;
            }
        break;
        case STATE_4:
            if (input == INPUT_LEFT_ARROW) {
                display_sendImage(image_menuItem4Selected);
                currentlySelectedItem = MENU_ITEM_4;
            }
        break;
        case MENU_ITEM_1:
            switch(input) {
                case INPUT_DOWN_ARROW:
                    display_sendImage(image_menuItem3Selected);
                    currentlySelectedItem = MENU_ITEM_3;
                break;
                case INPUT_RIGHT_ARROW:
                    display_sendImage(image_menuItem2Selected);
                    currentlySelectedItem = MENU_ITEM_2;
                break;
                default:
                break;
            }
        break;
        case MENU_ITEM_2:
            switch(input) {
                case INPUT_DOWN_ARROW:
                    display_sendImage(image_menuItem4Selected);
                    currentlySelectedItem = MENU_ITEM_4;
                break;
                case INPUT_LEFT_ARROW:
                    display_sendImage(image_menuItem1Selected);
                    currentlySelectedItem = MENU_ITEM_1;
                break;
                default:
                break;
            }
        break;
        case MENU_ITEM_3:
            switch(input) {
                case INPUT_UP_ARROW:
                    display_sendImage(image_menuItem1Selected);
                    currentlySelectedItem = MENU_ITEM_1;
                break;
                case INPUT_RIGHT_ARROW:
                    display_sendImage(image_menuItem4Selected);
                    currentlySelectedItem = MENU_ITEM_4;
                break;
                default:
                break;
            }
        break;
        case MENU_ITEM_4:
            switch(input) {
                case INPUT_UP_ARROW:
                    display_sendImage(image_menuItem2Selected);
                    currentlySelectedItem = MENU_ITEM_2;
                break;
                case INPUT_LEFT_ARROW:
                    display_sendImage(image_menuItem3Selected);
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