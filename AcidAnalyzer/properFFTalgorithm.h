#pragma once

#include <defines.h>
#include <kaiser.h>
#include <stdint.h>
#include <drawNcurses.h>
#include <ringBuffer.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include <sys/ioctl.h>
#include <time.h>

extern struct RingBuffer allBuffer;
extern int sampleRate;
extern int channels;

// void setChannels(int newChannels);

// void setSampleRate(int newSampleRate);

void multiplyArrays(const double* array1, const double* array2, double* result, int size);

double checkRatio(double ratio, double startingPoint, int rounds);

double findRatio(int maxDepth, double low, double high, double lowestValid, double startingPoint, double endGoal, int rounds);

void complexPowerToRealPower(fftw_complex *complexData, double *realPower, int numberOfElements);

int min(int a, int b);

int max(int a, int b);

void zeroSmallBins(double* output, int samples, double substract);

void normalizeLogBands(double *bands, int size, double dynamicRange);

void powTwoBands(double *realPower, double *bands, int n_bands, int startingPoint, double ratio);

void logBands(double *bands, double *logBands, int n_bands, int startingPoint, double ratio);

void* threadFunction(void* arg);

