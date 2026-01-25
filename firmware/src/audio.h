#ifndef AUDIO_H
#define AUDIO_H

#include "display.h"
#include <Arduino.h>

void initAudio();
void updateAudioSpectrum(const uint8_t *spectrum, uint8_t size);
void setAudioStyle(uint8_t style);
void drawAudioVisualizer();

#endif
