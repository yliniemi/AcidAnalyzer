/* PipeWire */
/* SPDX-FileCopyrightText: Copyright © 2022 Wim Taymans */
/* SPDX-License-Identifier: MIT */
 
/*
 [title]
 Audio capture using \ref pw_stream "pw_stream".
 [title]
 */
#define _XOPEN_SOURCE_EXTENDED 1
#include <wchar.h>

#include <locale.h>

// #include <argparse/argparse.hpp>

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <complex.h> 
#include <fftw3.h>
#include <time.h>
#include <ncursesw/ncurses.h>
#include <pthread.h>
#include <cblas.h>

#include <spa/param/audio/format-utils.h>

#include <pipewire/pipewire.h>

#include <stdio.h>
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846264338328
#endif
// 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608
int NUMBER_OF_FFT_SAMPLES = 8192;
int kaiserBeta = 5.0;
int dynamicRange = 5.0;

const int bins_8192_128[129]  =
{ 3,4,5,6,7,8,9,10,11,12,13,15,17,19,21,23,25,27,29,31,33,35,37,39,42,45,48,51,54,57,60,63,66,70,74,78,82,86,90,94,99,104,109,114,119,125,131,137,143,149,156,163,170,177,185,193,201,210,219,228,238,248,259,270,281,293,305,318,331,345,359,374,389,405,421,438,456,474,493,513,534,555,577,600,624,649,675,702,730,759,789,820,852,885,920,956,993,1032,1072,1114,1158,1203,1250,1299,1350,1402,1456,1513,1572,1633,1696,1762,1830,1901,1975,2051,2130,2212,2297,2386,2478,2574,2673,2776,2883,2994,3109,3229,3353 };

const float substract_universal[6]  =
{ 4.432, 4.668, 4.754, 4.783, 4.789, 4.790 };

double *real_in;
fftw_complex *complex_out;
fftw_plan p;

int tooMuch = 0;
int tooLittle = 0;

pthread_t readStuff;

int capturedSamples = 0;

double besselI0(double x) {
    double denominator;
    double numerator;
    double z;
    
    if (x == 0.0) {
        return 1.0;
    } else {
        z = x * x;
        numerator = (z* (z* (z* (z* (z* (z* (z* (z* (z* (z* (z* (z* (z* 
            (z* 0.210580722890567e-22  + 0.380715242345326e-19 ) +
            0.479440257548300e-16) + 0.435125971262668e-13 ) +
            0.300931127112960e-10) + 0.160224679395361e-7  ) +
            0.654858370096785e-5)  + 0.202591084143397e-2  ) +
            0.463076284721000e0)   + 0.754337328948189e2   ) +
            0.830792541809429e4)   + 0.571661130563785e6   ) +
            0.216415572361227e8)   + 0.356644482244025e9   ) +
            0.144048298227235e10);
        
        denominator = (z*(z*(z-0.307646912682801e4)+
            0.347626332405882e7)-0.144048298227235e10);
    }
    
    return -numerator/denominator;
}

void generateKaiserWindow(int N, double beta, double *window) {
    int i;
    double alpha, denom;

    alpha = (N - 1) / 2.0;
    denom = besselI0(beta);

    for (i = 0; i < N; i++) {
        double x = 2.0 * i / (N - 1) - 1.0;
        double val = besselI0(beta * sqrt(1.0 - x * x)) / denom;
        window[i] = val;
    }
}

double checkRatio(double ratio, double startingPoint, int rounds)
{
    for (int i = 0; i < rounds; i++)
    {
        // int add = startingPoint * ratio + 1.0;
        // if (add <= 0) add = 1;
        startingPoint += floor((startingPoint * ratio) + 1.0);
    }
    return startingPoint;
}

double findRatio(int maxDepth, double low, double high, double lowestValid, double startingPoint, double endGoal, int rounds)
{
    double middle = (low + high) * 0.5;
    double result = checkRatio(middle, startingPoint, rounds);
    #ifdef THIS_IS_A_TEST
    fprintf(stdout, "%.17f : %f\n", middle, result);
    #endif
    if (result == endGoal || maxDepth <= 0) return lowestValid;    
    if (result > endGoal)
    {
        return findRatio(maxDepth - 1, low, middle, lowestValid, startingPoint, endGoal, rounds);
    }
    else
    {
        return findRatio(maxDepth - 1, middle, high, middle, startingPoint, endGoal, rounds);
    }
}

/*
void multiplyArrays(const double* array1, const double* array2, double* result, int size) {
    const double alpha = 1.0;
    const int incX = 1;
    const int incY = 1;
    const int k = 1;  // The number of subdiagonals in the banded matrix (not used here)

    cblas_dsbmv(CblasRowMajor, CblasLower, size, k, alpha, array1, incX, array2, incY, 0.0, result, incY);
}
*/

void multiplyArrays(const double* array1, const double* array2, double* result, int size)
{
    for (int i = 0; i < size; i++)
    {
        result[i] = array1[i] * array2[i];
    }
}

// #include <ipp.h>
void deinterleaveArrays(const float* interleavedData, float** outputArrays, int numArrays, int length) {
    /* if (numArrays == 2) {
        ippsDeinterleave_32f(interleavedData, outputArrays[0], outputArrays[1], length);
    } else { */
        for (int i = 0; i < numArrays; i++) {
            for (int j = 0; j < length; j++) {
                outputArrays[i][j] = interleavedData[j * numArrays + i];
            }
        // }
    }
}

double hueToRGB(double p, double q, double t) {
    if (t < 0) {
        t += 1.0;
    }
    if (t > 1) {
        t -= 1.0;
    }
    if (t < 1.0 / 6.0) {
        return p + ((q - p) * 6.0 * t);
    }
    if (t < 1.0 / 2.0) {
        return q;
    }
    if (t < 2.0 / 3.0) {
        return p + ((q - p) * 6.0 * (2.0 / 3.0 - t));
    }
    return p;
}

void HSLtoRGB(double h, double s, double l, double* r, double* g, double* b) {
    if (s == 0) {
        // In the case of a grayscale color (saturation = 0), the RGB values are all equal to the lightness.
        *r = *g = *b = l;
        return;
    }

    double q = (l < 0.5) ? l * (1 + s) : l + s - (l * s);
    double p = 2 * l - q;

    // Convert hue to the range [0, 1]
    h = fmod(h, 1.0);
    if (h < 0) {
        h += 1.0;
    }

    // Calculate the RGB components based on the hue
    *r = hueToRGB(p, q, h + 1.0 / 3.0);
    *g = hueToRGB(p, q, h);
    *b = hueToRGB(p, q, h - 1.0 / 3.0);
}

void drawWave(double *soundArray, int x_size, int y_size)
{
    for (int y = 0; y < y_size; y++)
    {
        for (int x = 0; x < x_size; x++)
        {
            if (((double)y / (double)y_size * 2.0) - 1 > soundArray[x])
            {
                // mvaddch(y, x, '█');  // █ ▄ ▀
                // mvaddch(y, x, ' ' | A_REVERSE);  // █ ▄ ▀
                // mvaddch(y, x, ACS_CKBOARD);
                mvaddch(y, x, ' ' | A_REVERSE);
            }
            else
            {
                mvaddch(y, x, ' ');
            }
        }
    }
}

void drawSpectrum(double *soundArray, int x_size, int y_size)
{
    static wchar_t *character[] = {L" ", L"\u2581" , L"\u2582" , L"\u2583" , L"\u2584" , L"\u2585" , L"\u2586" , L"\u2587" , L"\u2588"};
    mvaddch(10, 10, 'X');
    for (int x = 0; x < x_size; x++)
    {
        int soundInt = 0;
        if (soundArray[x] < -1000000000) soundInt = -1000000000;
        else soundInt = soundArray[x] * ((y_size + 1) * 8);
        // mvaddch(10, x, 'X');
        // int soundInt = 123;
        for (int y = y_size; y >= 0; y--)
        {
            // mvaddch(y, 20, 'X');
            int charIndex = soundInt;
            if (soundInt <= 0)
            {
                charIndex = 0;
                // mvaddch(y, x, ' ');
            }
            else if (soundInt >= 8)
            {
                charIndex = 8;
                // mvaddch(y, x, ' ' | A_REVERSE);
            }
            // else
            {
                mvaddwstr(y, x, character[charIndex]);
                // mvprintw(x,y, character[charIndex]);
            }
            soundInt -=8;
        }
    }
}

struct RingBuffer
{
    int size;
    int elementSize;
    int chunkSize;
    int numberOfBuffers;
    int readIndex;
    int writeIndex;
    uint8_t **buffers;
    bool partialWrite;
    bool partialRead;
};

struct RingBuffer allBuffer;

void initializeBufferWithChunksSize(struct RingBuffer *rb, int numberOfBuffers, int size, int elementSize, int chunkSize)
{
    if (rb->buffers != NULL)
    {
        for (int i = 0; i < rb->numberOfBuffers; i++)
        {
            if (rb->buffers[i] != NULL) free(rb->buffers[i]);
        }
        free(rb->buffers);
    }
    rb->numberOfBuffers = numberOfBuffers;
    rb->elementSize = elementSize;
    rb->chunkSize = chunkSize;
    rb->size = size + rb->chunkSize + 1;
    rb->readIndex = 0;
    rb->writeIndex = 0;
    rb->buffers = calloc(rb->numberOfBuffers, sizeof(uint8_t*));
    rb->partialWrite = false;
    rb->partialRead = false;
    for (int i = 0; i < rb->numberOfBuffers; i++)
    {
        rb->buffers[i] = (uint8_t*)calloc(rb->size * rb->elementSize, sizeof(uint8_t));
    }
}

void initializeBuffer(struct RingBuffer *rb, int numberOfBuffers, int size, int elementSize)
{
    initializeBufferWithChunksSize(rb, numberOfBuffers, size, elementSize, 0);
}

bool isBufferEmpty(struct RingBuffer* rb)
{
    return (rb->writeIndex == rb->readIndex);
}

bool isBufferFull(struct RingBuffer* rb)
{
    return !((rb->size - rb->writeIndex + rb->readIndex - 1) % rb->size);
}

int getBufferWriteSpace(struct RingBuffer* rb)
{
    return (rb->size - rb->writeIndex + rb->readIndex - 1) % rb->size;
}

int increaseBufferWriteIndex(struct RingBuffer* rb, int count)
{
    int space = (rb->size - rb->writeIndex + rb->readIndex - 1) % rb->size;
    int toCopy = count > space ? space : count;

    if (count > space && !rb->partialWrite)
    {
        tooMuch++;
        return -495232356;
    }
    
    if (toCopy <= 0)
    {
        tooMuch++;
        return toCopy;
    }

    if (rb->writeIndex + toCopy <= rb->size)
    {
        rb->writeIndex += toCopy;
    }
    else
    {
        int firstHalfSize = rb->size - rb->writeIndex;
        int secondHalfSize = toCopy - firstHalfSize;
        rb->writeIndex = secondHalfSize;
    }
    return toCopy;
}

int writeBuffer(struct RingBuffer* rb, uint8_t *data, int bufferNumber, int count)
{
    int space = (rb->size - rb->writeIndex + rb->readIndex - 1) % rb->size;
    int toCopy = count > space ? space : count;
    if (bufferNumber > rb->numberOfBuffers - 1)
    {
        return -123456789;
    }

    if (count > space && !rb->partialWrite)  // rb->partialWrite will cause problems. perhaps i whould deprecate it
    {
        tooMuch++;
        return -495232356;
    }

    if (toCopy <= 0)
    {
        tooMuch++;
        return toCopy;
    }
    static int writeTimes = 0;
    if (rb->writeIndex + toCopy <= rb->size)
    {
        memcpy(&rb->buffers[bufferNumber][rb->writeIndex * rb->elementSize], &data[0], toCopy * rb->elementSize);
    }
    else
    {
        int firstHalfSize = rb->size - rb->writeIndex;
        memcpy(&rb->buffers[bufferNumber][rb->writeIndex * rb->elementSize], &data[0], firstHalfSize * rb->elementSize);

        int secondHalfSize = toCopy - firstHalfSize;
        memcpy(&rb->buffers[bufferNumber][0], &data[firstHalfSize * rb->elementSize], secondHalfSize * rb->elementSize);
    }
    return toCopy;
}

int getBufferReadSpace(struct RingBuffer* rb)
{
    return (rb->size + rb->writeIndex - rb->readIndex) % rb->size - rb->chunkSize;
}

int increaseBufferReadIndex(struct RingBuffer *rb, int count)
{
    int available = (rb->size + rb->writeIndex - rb->readIndex) % rb->size - rb->chunkSize;
    int toRead = count > available ? available : count;

    if (count > available && !rb->partialRead)  // rb->partialRead will cause problems. perhaps i whould deprecate it
    {
        tooLittle++;
        return -895232356;
    }
    
    if (toRead <= 0)
    {
        tooLittle++;
        return toRead;
    }

    static int readTimes = 0;
    if (rb->readIndex + toRead <= rb->size)
    {
        rb->readIndex += toRead;
    }
    else
    {
        int firstHalfSize = rb->size - rb->readIndex;
        int secondHalfSize = toRead - firstHalfSize;
        rb->readIndex = secondHalfSize;
    }
    return toRead;
}

int readBuffer(struct RingBuffer *rb, uint8_t *destination, int bufferNumber, int count)
{
    int available = (rb->size + rb->writeIndex - rb->readIndex) % rb->size - rb->chunkSize;
    int toRead = count > available ? available : count;

    if (bufferNumber > rb->numberOfBuffers - 1)
    {
        return -987654321;
    }

    if (count > available && !rb->partialRead)  // rb->partialRead will cause problems. perhaps i whould deprecate it
    {
        tooLittle++;
        return -895232356;
    }
    
    if (toRead <= 0)
    {
        tooLittle++;
        return toRead;
    }

    static int readTimes = 0;
    if (rb->readIndex + toRead <= rb->size)
    {
        memcpy(&destination[0], &rb->buffers[bufferNumber][rb->readIndex * rb->elementSize], toRead * rb->elementSize);
    }
    else
    {
        int firstHalfSize = rb->size - rb->readIndex;
        memcpy(&destination[0], &rb->buffers[bufferNumber][rb->readIndex * rb->elementSize], firstHalfSize * rb->elementSize);

        int secondHalfSize = toRead - firstHalfSize;
        memcpy(&destination[firstHalfSize * rb->elementSize], &rb->buffers[bufferNumber][0], secondHalfSize * rb->elementSize);

    }
    return toRead;
}

int readChunkFromBuffer(struct RingBuffer *rb, uint8_t *destination, int bufferNumber)
{
    int available = (rb->size + rb->writeIndex - rb->readIndex) % rb->size;
    int toRead = rb->chunkSize > available ? available : rb->chunkSize;

    if (bufferNumber > rb->numberOfBuffers - 1)
    {
        return -987654321;
    }

    if (rb->chunkSize > available && !rb->partialRead)
    {
        tooLittle++;
        return -895232356;
    }
    
    if (toRead <= 0)
    {
        tooLittle++;
        return toRead;
    }

    static int readTimes = 0;
    if (rb->readIndex + toRead <= rb->size)
    {
        memcpy(&destination[0], &rb->buffers[bufferNumber][rb->readIndex * rb->elementSize], toRead * rb->elementSize);
    }
    else
    {
        int firstHalfSize = rb->size - rb->readIndex;
        memcpy(&destination[0], &rb->buffers[bufferNumber][rb->readIndex * rb->elementSize], firstHalfSize * rb->elementSize);

        int secondHalfSize = toRead - firstHalfSize;
        memcpy(&destination[firstHalfSize * rb->elementSize], &rb->buffers[bufferNumber][0], secondHalfSize * rb->elementSize);

    }
    return toRead;
}

void freeBuffer(struct RingBuffer* rb)
{
    if (rb->buffers != NULL)
    {    
        for (int i = 0; i < rb->numberOfBuffers; i++)
        {
            if (rb->buffers[i] != NULL) free(rb->buffers[i]);
        }
        free(rb->buffers);
    }
    rb->numberOfBuffers = 0;
    rb->readIndex = 0;
    rb->writeIndex = 0;
    rb->chunkSize = 0;
}

struct data {
        struct pw_main_loop *loop;
        struct pw_stream *stream;
 
        struct spa_audio_info format;
        unsigned move:1;
};
 
/* our data processing function is in general:
 *
 *  struct pw_buffer *b;
 *  b = pw_stream_dequeue_buffer(stream);
 *
 *  .. consume stuff in the buffer ...
 *
 *  pw_stream_queue_buffer(stream, b);
 */
static void on_process(void *userdata)
{
        struct data *data = userdata;
        struct pw_buffer *b;
        struct spa_buffer *buf;
        float *samples, min, max;
        uint32_t c, n, n_samples, peak;
        static int n_channels = 0;
 
        if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
                pw_log_warn("out of buffers: %m");
                return;
        }
 
        buf = b->buffer;
        if ((samples = buf->datas[0].data) == NULL)
                return;
        
        n_channels = data->format.info.raw.channels;

        n_samples = buf->datas[0].chunk->size / sizeof(float) / n_channels;
        if (getBufferWriteSpace(&allBuffer) >= n_samples)
        {
            struct winsize w;
            ioctl(0, TIOCGWINSZ, &w);

            double *audio = calloc(n_samples, sizeof(double));
            for (int c = 0; c < n_channels; c++)
            {
                for (int i = 0; i < n_samples; i++)
                {
                    audio[i] = ((float*)buf->datas[0].data)[(i * n_channels + c)];
                }
                // mvprintw(w.ws_row - 2, 0, "started writing");
                // refresh();
                writeBuffer(&allBuffer, (uint8_t*)audio, c, n_samples);
                // mvprintw(w.ws_row - 2, 0, "writing done   ");
                // refresh();
            }
            increaseBufferWriteIndex(&allBuffer, n_samples);
            free(audio);
        }
        // writeBuffer(&allBuffer, (uint8_t*)buf->datas[0].data, 0, n_samples);


 
        /* move cursor up */
        static int frame = 0;
        capturedSamples = n_samples;
        
        data->move = true;
        // fflush(stdout);
 
        pw_stream_queue_buffer(data->stream, b);
}
 
/* Be notified when the stream param changes. We're only looking at the
 * format changes.
 */

static int channels = 0;
static int sampleRate = 1;
void on_stream_param_changed(void *_data, uint32_t id, const struct spa_pod *param)
{
        struct data *data = _data;
        
        /* NULL means to clear the format */
        if (param == NULL || id != SPA_PARAM_Format)
                return;
 
        if (spa_format_parse(param, &data->format.media_type, &data->format.media_subtype) < 0)
                return;
 
        /* only accept raw audio */
        if (data->format.media_type != SPA_MEDIA_TYPE_audio ||
            data->format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
                return;
 
         /* call a helper function to parse the format for us. */
        spa_format_audio_raw_parse(param, &data->format.info.raw);

        
        if (sampleRate != data->format.info.raw.rate)
        {
            sampleRate = data->format.info.raw.rate;
        }
        if (channels != data->format.info.raw.channels)
        {
            channels = data->format.info.raw.channels;
            // fprintf(stdout, "channels changed to %d\n", channels);
            initializeBufferWithChunksSize(&allBuffer, channels, 65536, sizeof(double), NUMBER_OF_FFT_SAMPLES);
            allBuffer.partialRead = true;
            printw("initialized the buffer with %d size", allBuffer.size);
            refresh;
        }
        
        

 
}
 
static const struct pw_stream_events stream_events = {
        PW_VERSION_STREAM_EVENTS,
        .param_changed = on_stream_param_changed,
        .process = on_process,
};
 
static void do_quit(void *userdata, int signal_number)
{
        struct data *data = userdata;
        pw_main_loop_quit(data->loop);
}

void complexPowerToRealPower(fftw_complex *complexData, double *realPower, int numberOfElements)
{
    for (int i = 0; i < numberOfElements; i++)
    {
        realPower[i] = creal(complexData[i]) + cimag(complexData[i]);
    }
}

int min(int a, int b)
{
    return a < b ? a : b;
}

int max(int a, int b)
{
    return a > b ? a : b;
}



void zeroSmallBins(double* output, int samples, double substract)
{
    double biggest = 0;
    for (int i = 0; i < samples; i++)
    {
        if (biggest < output[i]) biggest = output[i];
    }
    for (int i = 0; i < samples; i++)
    {
        // we get rid of the unwanted frequency side lobes this way. kaiser 2 is quite eficient and we shouldn't see more than -60 dB on the side lobes
        // at the same time we make sure that we don't take logarithm out of zero. that would be -infinite
        output[i] = fmax(output[i] - biggest * 0.0000001, 0.0);
        // output[i] = 1;
    }
}

void normalizeLogBands(double *bands, int size, double dynamicRange)
{
    static double bandCeiling = -INFINITY;
    bandCeiling -= 0.00027777777777777;          // now it takes 60 seconds to come 10 dB down
    for (int i = 0; i < size; i++)
    {
        if (bands[i] > bandCeiling) bandCeiling = bands[i];
    }
    for (int i = 0; i < size; i++)
    {
        bands[i] = (bands[i] - bandCeiling + dynamicRange) / dynamicRange;
    }
}

void powTwoBands(double *realPower, double *bands, int n_bands, int startingPoint, double ratio)
{
  
  for (int i = 0; i < n_bands; i++)
  {
    bands[i] = 0;
    int endPoint = startingPoint + floor((startingPoint * ratio) + 1.0);
    for (int j = startingPoint; j < endPoint; j++)
    {
      bands[i] += realPower[j];
    }
    startingPoint = endPoint;
  }
}

void logBands(double *bands, double *logBands, int n_bands, int startingPoint, double ratio)
{
  for (int i = 0; i < n_bands; i++)
  {
    logBands[i] = log10(bands[i]);
    int delta = floor((startingPoint * ratio) + 1.0);
    if (delta < 1) delta = 1;
    if (delta > 6) delta = 6;
    logBands[i] = logBands[i] - substract_universal[delta - 1];
    startingPoint += delta;
  }
}

void* threadFunction(void* arg)
{
    struct timespec start, end;
    double *windowingArray = (double*) malloc(sizeof(double) * NUMBER_OF_FFT_SAMPLES);
    double *real_in = (double*) fftw_malloc(sizeof(double) * NUMBER_OF_FFT_SAMPLES);
    fftw_complex *complex_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NUMBER_OF_FFT_SAMPLES);
    fftw_complex *complexPower = (fftw_complex*) malloc(sizeof(fftw_complex) * NUMBER_OF_FFT_SAMPLES);
    double *realPower = (double*) malloc(sizeof(double) * NUMBER_OF_FFT_SAMPLES);
    double *bands_out = (double*) malloc(sizeof(double) * NUMBER_OF_FFT_SAMPLES);
    double *log10_bands = (double*) malloc(sizeof(double) * NUMBER_OF_FFT_SAMPLES);
    fftw_plan thePlan;
    fftw_init_threads();
    fftw_plan_with_nthreads(4);
    thePlan = fftw_plan_dft_r2c_1d(NUMBER_OF_FFT_SAMPLES, real_in, complex_out, FFTW_ESTIMATE);;
    generateKaiserWindow(NUMBER_OF_FFT_SAMPLES, kaiserBeta, windowingArray);

    double *data = (double*) malloc(sizeof(double) * NUMBER_OF_FFT_SAMPLES);
    double *audio = (double*) malloc(sizeof(double) * NUMBER_OF_FFT_SAMPLES);
    double *sumAudio = (double*) malloc(sizeof(double) * NUMBER_OF_FFT_SAMPLES);
    double *sumScreen = (double*) malloc(sizeof(double) * NUMBER_OF_FFT_SAMPLES);

    while(true)
    {
        if (allBuffer.buffers != NULL)
        {
            struct winsize w;
            ioctl(0, TIOCGWINSZ, &w);
            static int windowColums = 0;
            static double ratio = 0;
            static int startingPoint = 3;
            if (w.ws_col != windowColums)
            {
                startingPoint = max(20 * NUMBER_OF_FFT_SAMPLES / 48000 + 1, 3);
                // startingPoint = 7;
                windowColums = w.ws_col;
                int n_bins = min(NUMBER_OF_FFT_SAMPLES / 2, NUMBER_OF_FFT_SAMPLES / 2 * 20000 / (sampleRate / 2));
                ratio = findRatio(60, 0, 1, 0, startingPoint, n_bins, windowColums);
            }
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
            static uint64_t old_ns = 0;
            uint64_t new_ns = ts.tv_sec * 1000000000 + ts.tv_nsec;
            uint64_t delta_ns = new_ns - old_ns;

            uint64_t newSamples = min(NUMBER_OF_FFT_SAMPLES, sampleRate * delta_ns / 996000000 + 1);
            int readSamples = increaseBufferReadIndex(&allBuffer, newSamples);
            if (readSamples <= 0) goto skip;
            memset(audio, 0, sizeof(double) * NUMBER_OF_FFT_SAMPLES);
            memset(data, 0, sizeof(double) * NUMBER_OF_FFT_SAMPLES);   // this isn't really needed
            static int frameNumber = 0;
            for (int channel = 0; channel < channels; channel++)
            {
                refresh();
                readChunkFromBuffer(&allBuffer, (uint8_t*)data, channel);
                refresh();
                for (int i = 0; i < NUMBER_OF_FFT_SAMPLES; i++)
                {
                    audio[i] += data[i];
                }
            }

            /*
            doWindowing(inputReal);
            fft_execute(fftReal);
            powerOfTwo(output);   // if we end up doing log() of the output anyways there is no need to do costly sqrt() because it's the same as dividing log() by 2
            // sqrtBins();     // we don't actually need this since we are dealing with the power of the signal and not the amplitude
            zeroSmallBins(output);  // we do this because there are plenty of of reflections in the surrounding bins
            powTwoBands(output, bands);
            logBands(bands, peakBands);
            normalizeBands(bands);
            */
            memset(sumAudio, 0, sizeof(double) * NUMBER_OF_FFT_SAMPLES);
            for (int i = 0; i < NUMBER_OF_FFT_SAMPLES; i++)
            {
                sumAudio[i] += audio[i];
            }
            multiplyArrays(sumAudio, windowingArray, real_in, NUMBER_OF_FFT_SAMPLES);  // windowing
            fftw_execute(thePlan);
            multiplyArrays((double*)complex_out, (double*)complex_out, (double*)complexPower, NUMBER_OF_FFT_SAMPLES);  // these two steps turn co complex numbers
            complexPowerToRealPower(complexPower, realPower, NUMBER_OF_FFT_SAMPLES / 2);  // into the power of the lenght of ampltude
            // zeroSmallBins(output);
            zeroSmallBins(realPower, NUMBER_OF_FFT_SAMPLES / 2, 0.0000005);
            powTwoBands(realPower, bands_out, windowColums, startingPoint, ratio);
            logBands(bands_out, log10_bands, windowColums, startingPoint, ratio);
            // normalizeLogBands(log10_bands, w.ws_col);

            normalizeLogBands(log10_bands, windowColums, 5.0);
            int sumGroup = NUMBER_OF_FFT_SAMPLES / w.ws_col;
            memset(sumScreen, 0, sizeof(double) * NUMBER_OF_FFT_SAMPLES);
            for (int i = 0; i < NUMBER_OF_FFT_SAMPLES; i++)
            {
                sumScreen[i / sumGroup] += audio[i] / (sumGroup * channels);
            }
            color_set(1, NULL);
            if (readSamples > 0)
            {
                // drawWave(sumScreen, w.ws_col, w.ws_row);
                drawSpectrum(log10_bands, windowColums, max(w.ws_row, 3));
            }
            // mvprintw(0, 0, "%d, %d", allBuffer.writeIndex, readSamples);
            // mvprintw(1, 0, "%d, %d", allBuffer.readIndex, startingPoint);
            refresh();
            old_ns = new_ns;
        }
        skip:
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        static uint64_t old_ns = 0;
        uint64_t new_ns = ts.tv_sec * 1000000000 + ts.tv_nsec;
        uint64_t delta_ns = new_ns - old_ns;
        struct timespec rem;
        uint64_t sleepThisLong = min(max((int)17000000 - (int)delta_ns, 0), 100000000);
        struct timespec req = {sleepThisLong / 1000000000, sleepThisLong % 1000000000};
        nanosleep(&req, &rem);
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        old_ns = ts.tv_sec * 1000000000 + ts.tv_nsec;
    }
}

#ifndef THIS_IS_A_TEST
int main(int argc, char *argv[])
{
        struct data data = { 0, };
        const struct spa_pod *params[1];
        uint8_t buffer[1024];
        struct pw_properties *props;
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

        double colorR, colorG, colorB;
        setlocale(LC_ALL, "");
        initscr();
        curs_set(0);
        start_color();
        srand(time(NULL));
        // HSLtoRGB((double)rand() / RAND_MAX, 1.0, 0.5, &colorR, &colorG, &colorB);
        // mvprintw(1, 0, "%lf, %lf, %lf", colorR, colorG, colorB);
        printw("program started \u2581\u2582\u2583\u2584\u2585\u2586\u2587\u2588");
        refresh;
        refresh();
        // nanosleep(&(struct timespec){.tv_sec = 10}, NULL);
        // init_color(COLOR_WHITE, 900, 0, 0);
        // init_color(COLOR_BLACK, 0, 0, 0);
        // init_pair(1, COLOR_WHITE, COLOR_BLACK);
        init_pair(1, 1 + rand() % 15, COLOR_BLACK);
        init_pair(1, 15, COLOR_BLACK);
        color_set(1, NULL);
        // attron(COLOR_PAIR(1));

        // fprintf(stdout, "sizeof(fftw_complex) = %d\n", sizeof(fftw_complex));
 
        /* and wait while we let things run */
        pthread_create(&readStuff, NULL, threadFunction, NULL);
        sleep(1);
        
        pw_init(&argc, &argv);
 
        /* make a main loop. If you already have another main loop, you can add
         * the fd of this pipewire mainloop to it. */
        data.loop = pw_main_loop_new(NULL);
 
        pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit, &data);
        pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit, &data);
 
        /* Create a simple stream, the simple stream manages the core and remote
         * objects for you if you don't need to deal with them.
         *
         * If you plan to autoconnect your stream, you need to provide at least
         * media, category and role properties.
         *
         * Pass your events and a user_data pointer as the last arguments. This
         * will inform you about the stream state. The most important event
         * you need to listen to is the process event where you need to produce
         * the data.
         */
        
        props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                        PW_KEY_MEDIA_CATEGORY, "Capture",
                        PW_KEY_MEDIA_ROLE, "Music",
                        NULL);
        if (argc > 1)
                /* Set stream target if given on command line */
                pw_properties_set(props, PW_KEY_TARGET_OBJECT, argv[1]);
 
        /* uncomment if you want to capture from the sink monitor ports */
        pw_properties_set(props, PW_KEY_STREAM_CAPTURE_SINK, "true");
 
        data.stream = pw_stream_new_simple(
                        pw_main_loop_get_loop(data.loop),
                        "audio-capture",
                        props,
                        &stream_events,
                        &data);
 
        /* Make one parameter with the supported formats. The SPA_PARAM_EnumFormat
         * id means that this is a format enumeration (of 1 value).
         * We leave the channels and rate empty to accept the native graph
         * rate and channels. */
        params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
                        &SPA_AUDIO_INFO_RAW_INIT(
                                .format = SPA_AUDIO_FORMAT_F32));
 
        /* Now connect this stream. We ask that our process function is
         * called in a realtime thread. */
        /*
        pw_stream_connect(data.stream,
                          PW_DIRECTION_INPUT,
                          PW_ID_ANY,
                          PW_STREAM_FLAG_AUTOCONNECT |
                          PW_STREAM_FLAG_MAP_BUFFERS |
                          PW_STREAM_FLAG_RT_PROCESS,
                          params, 1);
        */
        pw_stream_connect(data.stream,
                          PW_DIRECTION_INPUT,
                          PW_ID_ANY,
                          PW_STREAM_FLAG_AUTOCONNECT |
                          PW_STREAM_FLAG_MAP_BUFFERS |
                          PW_STREAM_FLAG_RT_PROCESS,
                          params, 1);
        
        pw_main_loop_run(data.loop);
 
        pw_stream_destroy(data.stream);
        pw_main_loop_destroy(data.loop);
        pw_deinit();
 
        return 0;
}
#endif