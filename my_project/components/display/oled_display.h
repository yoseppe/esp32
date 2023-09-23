#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stddef.h>
void display_sendImage(uint8_t * image);
void oled_init(void);
void spyrosoftLogo_scrollDown(void);

#endif