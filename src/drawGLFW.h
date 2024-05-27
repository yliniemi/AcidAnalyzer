#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <math.h>
#include <complex.h>
#include <fftw3.h>

#include <pthread.h>
#include <semaphore.h>

#include <defaults.h>


struct Vector2D
{
    double x;
    double y;
};

struct Line2D
{
    struct Vector2D start;
    struct Vector2D end;
};

struct Vector3D
{
    double x;
    double y;
    double z;
};

struct XYZ
{
    float x, y, z;
};

struct RGBA
{
    float r, g, b, a;
};

struct Vertex
{
    struct XYZ xyz;
    struct RGBA rgba;
};

struct RGB
{
	double r;
	double g;
	double b;
};

struct HSV
{
	double h;
	double s;
	double v;
};

struct Edge
{
    double location;
    double height;
    struct RGB color;
};

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
    enum BarMode barMode;
    struct Edge *edge;
    struct Vertex *vertex;
    double highestBar;
    pthread_t thread;
    sem_t analyzerGo;
    sem_t glfwGo;
    double powerSum;
    int64_t powerSumNumberOfSamples;
    double minSample;
    double maxSample;
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
    double normalizerBar;
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

void calculateLocationData(struct ChannelData *channelData, struct AllChannelData *allChannelData);

// void calculateLocationData(struct AllChannelData *allChannelData);
void calculateWaveData(struct ChannelData *channelData, struct AllChannelData *allChannelData);

void calculateWaveVertexData(struct ChannelData *channelData, struct AllChannelData *allChannelData);

void glfwSpectrumInit();

void writeVertexData(struct AllChannelData *allChannelData);




