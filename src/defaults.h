// These are the defaults for my current setup. The user can change these with command line arguments when I learn how to parse them and via keyboard when I get to it.

#pragma once

#ifndef PI
#define PI 3.14159265358979323846264338328
#endif

#include <ringBuffer.h>

#define NUMBER_OF_FFT_SAMPLES 6144  // 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608. Must be a power of two. Otherwise it will be substantially slower. 4096 and 8192 are great for fast paced music. 16384 might be great for ambient or some other music that changes slowly. anything more than that is just for testing my algorithm. The ability to analyze 10 seconds worth of data 120 times a second isn't very usefull but can be a nice way to test the efficiency of my algorithm.
#define KAISER_BETA 6.0 * PI             // bigger number means wider mainlobes but the side lobes will be lower. 5.0 is a nice compromise between those two for music.
#define DYNAMIC_RANGE 5.5           // Bels, 5.0 Bels = 50 deciBels. This is a good range for modern music. For actually analyzing the noise floor of recordings and such a wider range would be preferable. But when increasing this you should also increase KAISER_BETA. Otherwise you can't tell what is noise and what is just a sidelobe.
#define FPS 120                     // I set this to 120 because that's the refresh rate of my screen. 60 would be a better default but because that's a more common frame rate but having a higher frame rate is no issue for modern CPUs.
// when changing the fps I could multiply or divide the current fps with fourth root of two. 2^0.25 = 1.189207115002721. This way I can double or half the fps by pressing a button four times
#define COLOR0 0, 0, 1000
#define COLOR1 300, 300, 1000
#define BUFFER_EXTRA 2 * 48000
#define MIN_FREQUENCY 0
#define MAX_FREQUENCY 20000
#define USING_NCURSES false
#define USING_GLFW true
#define NUM_BARS 320
// #define DUMMY_NOISE

#define ABSOLUTE_MAXIMUM_CHANNELS 1024

// #define NUM_TEST_CHANNELS 200
// #define TEST_A_MASSIVE_NUMBER_OF_CHANNELS
// #define FORCE_CORE_AFFINITY


enum BarMode
{
    BAR,
    CIRCLE,
    WAVE,
    NEWWAVE
};

struct Global
{
    struct RingBuffer allBuffer;
    int64_t sampleRate;
    int64_t channels;
    double fps;
    int64_t FFTsize;
    // int64_t threadsPerChannel;
    // double windowingArray;
    double kaiserBeta;
    double dynamicRange;
    int64_t bufferExtra;
    double minFrequency;
    double maxFrequency;
    bool usingNcurses;
    bool usingGlfw;
    int64_t numBars;
    int64_t colors[2][3];
    // int64_t openglVersion;
    bool dummyNoise;
    int64_t mostCapturedSamples;
    int64_t leastReadSamples;
    int64_t mostReadSamples;
    int64_t minBufferDepth;
    int64_t maxBufferDept;
    double barWidth;
    double colorRange;
    double colorSpeed;
    double colorSaturation;
    double colorStart;
    double colorBrightness;
    double colorOpacity;
    bool hanging;
    bool wave;
    double minBarHeight;
    bool smoothenAnimation;
    double decreaseCeilingBelsPerSecond;
    enum BarMode barMode;
    int64_t simulateNumberOfChannels;
    bool forceCoreAffinity;
    int64_t readSamples;
};

extern struct Global global;

#ifdef MAIN

struct Global global;
    
void setGobalDefaults()
{
    global.sampleRate = 1;
    global.channels = 0;
    global.fps = 120;
    global.FFTsize = 8192;
    // global.threadsPerChannel = 1;
    global.dynamicRange = 4.5;      // Bels, not desiBels
    global.kaiserBeta = 6.0 * PI;
    global.bufferExtra = 2 * 48000;
    global.minFrequency = 40;
    global.maxFrequency = 20000;
    global.usingNcurses = false;
    global.usingGlfw = true;
    global.numBars = 256;
    global.dummyNoise = false;
    global.mostCapturedSamples = 0;
    global.readSamples = 0;
    global.leastReadSamples = 1000000000;
    global.mostReadSamples = 0;
    global.minBufferDepth = 1000000000;
    global.maxBufferDept = 0;
    global.barWidth = 0.8;
    global.barMode = WAVE;
    global.colorRange = 0.15;
    global.colorSpeed = 0.03;
    global.colorSaturation = 1.0;
    global.colorStart = 0.0;
    global.colorBrightness = 1.0;
    global.colorOpacity = 0.85;
    global.hanging = false;
    global.wave = true;
    global.minBarHeight = 0.0;
    global.smoothenAnimation = true;
    global.decreaseCeilingBelsPerSecond = 0.01;   // 10 seconds for spectrum to rise by 1 desiBels
    global.simulateNumberOfChannels = 0;
    global.forceCoreAffinity = false;
    
    int64_t defaultColors[2][3] = {{0, 0, 1000}, {300, 300, 1000}};
    for (int64_t i = 0; i < 2; i++)
    {
        for (int64_t j = 0; j < 3; j++)
        {
            global.colors[i][j] = defaultColors[i][j];
        }
    }
}

#endif


