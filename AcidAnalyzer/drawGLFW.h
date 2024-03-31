#pragma once

#include <stdbool.h>
#include <stdint.h>

struct FFTData
{
    double *audio;
    double *real_in;
    double *windowingArray;
    // fftw_complex *complex_out;
    // fftw_complex *complexPower;
    double *realPower;
    double *bands_out;
    double *log10_bands;
    
    double ratio;
    int64_t firstBin;
    int64_t lastBin;
    int64_t secondToLastBin;
};

extern struct FFTData FFTdata;

void glfwSpectrum(double *soundArray, int64_t numBars, double barWidth, int64_t numChannels, int64_t channel, bool uprigth, bool isCircle);

void killAll();
void initializeGlfw();
void seedNoise(uint64_t seed);

void startFrame();
void finalizeFrame();

double smoothStep(double x);

double lerp(double a, double b, double f) ;

double smootherStep(double x);

double getRefreshRate();

double nextBin(double currentBin, double ratio);


