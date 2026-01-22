#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include "decoder.h"

void initStorage();

void saveBrightness(uint8_t brightness);
uint8_t loadBrightness(uint8_t defaultVal);

void saveClockColor(uint8_t colorIndex);
uint8_t loadClockColor(uint8_t defaultVal);

void saveDefaultMode(uint8_t mode);
uint8_t loadDefaultMode(uint8_t defaultVal);

void saveFrame(const Frame &f);
bool loadFrame(Frame &f);

#endif // STORAGE_H
