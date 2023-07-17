// These are the defaults for my current setup. The user can change these with command line arguments when I learn how to parse them and via keyboard when I get to it.

#pragma once
#define NUMBER_OF_FFT_SAMPLES 8192  // 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608. Must be a power of two. Otherwise it will be substantially slower. 4096 and 8192 are great for fast paced music. 16384 might be great for ambient or some other music that changes slowly. anything more than that is just for testing my algorithm. The ability to analyze 10 seconds worth of data 120 times a second isn't very usefull but can be a nice way to test the efficiency of my algorithm.
#define KAISER_BETA 5.0             // bigger number means wider mainlobes but the side lobes will be lower. 5.0 is a nice compromise between those two for music.
#define DYNAMIC_RANGE 5.0           // Bels, 5.0 Bels = 50 deciBels. This is a good range for modern music. For actually analyzing the noise floor of recordings and such a wider range would be preferable. But when increasing this you should also increase KAISER_BETA. Otherwise you can't tell what is noise and what is just a sidelobe.
#define FPS 120                     // I set this to 120 because that's the refresh rate of my screen. 60 would be a better default but because that's a more common frame rate but having a higher frame rate is no issue for modern CPUs.
// when changing the fps I could multiply or divide the current fps with fourth root of two. 2^0.25 = 1.189207115002721. This way I can double or half the fps by pressing a button four times
#define COLOR0 0, 0, 1000
#define COLOR1 300, 300, 1000


struct Global
{
    struct RingBuffer allBuffer;
    int sampleRate;
    int channels;
};

