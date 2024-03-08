#pragma once

#include <stdbool.h>
#include <stdint.h>

void glfwSpectrum(double *soundArray, int64_t numBars, double barWidth, int64_t numChannels, int64_t channel, bool uprigth, bool isCircle, double emptyCircleRatio);

void killAll();
void initializeGlfw();
void seedNoise(uint64_t seed);

void startFrame();
void finalizeFrame();

