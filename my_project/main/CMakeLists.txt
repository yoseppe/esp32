idf_component_register(SRCS "my_project_main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi nvs_flash display esp_adc)
