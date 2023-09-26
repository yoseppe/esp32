idf_component_register(SRCS "joystick.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_adc display images main v2 ssd1306)