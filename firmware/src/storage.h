#ifndef STORAGE_H
#define STORAGE_H

#include "decoder.h"
#include "nightlight.h"
#include <Arduino.h>

void initStorage();

void saveBrightness(uint8_t brightness);
uint8_t loadBrightness(uint8_t defaultVal);

void saveClockColor(uint8_t colorIndex);
uint8_t loadClockColor(uint8_t defaultVal);

void saveClockGradient(uint8_t gradientIndex);
uint8_t loadClockGradient(uint8_t defaultVal);

void saveDefaultMode(uint8_t mode);
uint8_t loadDefaultMode(uint8_t defaultVal);

void saveBLETimeout(uint8_t minutes);
uint8_t loadBLETimeout(uint8_t defaultVal);

void saveNightLightConfig(const NightLightConfig &cfg);
void loadNightLightConfig(NightLightConfig &cfg);

void saveFrame(const Frame &f);
bool loadFrame(Frame &f);

#endif // STORAGE_H
