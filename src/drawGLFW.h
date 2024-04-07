#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include <pthread.h>

/*
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
};


extern struct FFTData FFTdata;
*/

struct ChannelData
{
    int64_t channelNumber;
    // double *windowingArray;
    double *audioData;
    double *windowedAudio;
    fftw_plan plan;
    fftw_complex *complexFFT;
    fftw_complex *complexPower;
    double *realPower;
    double *bands;
    double *log10Bands;
    pthread_t thread;
};

struct AllChannelData
{
    int64_t numberOfChannels;
    int64_t maxNumberOfChannelsEver;
    int64_t FFTsize;
    double ratio;
    // int64_t startingPoint;
    int64_t firstBin;
    int64_t lastBin;
    int64_t secondToLastBin;
    double *windowingArray;
    // double *sharedWindowingArray;
    struct ChannelData **channelDataArray;
};

void glfwSpectrum(struct AllChannelData *allChannelData);

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



