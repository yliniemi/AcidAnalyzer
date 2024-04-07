#include <dummyAudio.h>

#include <nanoTime.h>
#include <stdlib.h>

// #include <pthread.h>

#include <ringBuffer.h>

#include <defaults.h>

#include <locale.h>
#include <wchar.h>
#include <ncurses.h>



void dummyLoop()
{
    int64_t n_channels = global.channels;
    int64_t n_samples = getBufferWriteSpace(&global.allBuffer);
    
    {
        double *audio = calloc(n_samples, sizeof(double));
        for (int64_t channel = 0; channel < n_channels; channel++)
        {
            for (int64_t i = 0; i < n_samples; i++)
            {
                audio[i] = rand() % 65535 - 32768;
            }
            writeBuffer(&global.allBuffer, (uint8_t*)audio, channel, n_samples);
        }
        increaseBufferWriteIndex(&global.allBuffer, n_samples);
        free(audio);
    }
}

void startDummyAudio()
{
    global.sampleRate = 48000;
    global.channels = 2;
    initializeBufferWithChunksSize(&global.allBuffer, global.channels, global.bufferExtra, sizeof(double), global.FFTsize);
    global.allBuffer.partialRead = true;
    while (true)
    {
        dummyLoop();
        nanoSleepUniversal(10000000);
    }
}
