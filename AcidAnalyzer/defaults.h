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
#define MIN_FREQUENCY 20
#define MAX_FREQUENCY 20000
#define USING_NCURSES false
#define USING_GLFW true
#define NUM_BARS 320
// #define DUMMY_NOISE

struct Global
{
    struct RingBuffer allBuffer;
    int64_t sampleRate;
    int64_t channels;
    double fps;
    int64_t FFTsize;
    int64_t threads;
    double kaiserBeta;
    double dynamicRange;
    int64_t bufferExtra;
    double minFrequency;
    double maxFrequency;
    bool usingNcurses;
    bool usingGlfw;
    int64_t numBars;
    int64_t colors[2][3];
    int64_t openglVersion;
    bool dummyNoise;
};

extern struct Global global;

#ifdef MAIN

struct Global global;
    
void setGobalDefaults()
{
    global.sampleRate = 1;
    global.channels = 1;
    global.fps = FPS;
    global.FFTsize = NUMBER_OF_FFT_SAMPLES;
    global.threads = sysconf(_SC_NPROCESSORS_ONLN);
    global.dynamicRange = DYNAMIC_RANGE;
    global.kaiserBeta = KAISER_BETA;
    global.bufferExtra = BUFFER_EXTRA;
    global.minFrequency = MIN_FREQUENCY;
    global.maxFrequency = MAX_FREQUENCY;
    global.usingNcurses = USING_NCURSES;
    global.usingGlfw = USING_GLFW;
    global.numBars = NUM_BARS;
    global.openglVersion = 3;
    global.dummyNoise = true;
}

#endif


