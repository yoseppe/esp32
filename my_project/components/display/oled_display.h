#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stddef.h>
void display_invertEverythingNow(void);
void display_sendImage(uint8_t * image, bool invertImages);
void display_sendChar(uint8_t * image, bool invertImages, int xpos, int ypos, int width, int height);
void oled_init(void);
void spyrosoftLogo_scrollDown(void);
void display_sendImageTest(uint8_t * image, bool invertImages, int xdimension);

#endif