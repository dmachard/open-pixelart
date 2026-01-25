#ifndef TEXT_H
#define TEXT_H

#include "display.h"
#include <Arduino.h>

void initText();
void updateText();
void setText(const char *msg, Pixel color, uint8_t speed);
void drawText();

#endif // TEXT_H
