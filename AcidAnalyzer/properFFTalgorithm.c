/*
    Proper FFT Algorithm
    Copyright (C) 2023 Antti Yliniemi
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <properFFTalgorithm.h>
#include <nanoTime.h>
#include <ringBuffer.h>
#include <defaults.h>
#include <kaiser.h>
#include <drawNcurses.h>
#include <drawGLFW.h>
#include <ncurses.h>

#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#include <pthread.h>

extern struct Global global;

pthread_t readAudioBuffer;

double substract_universal[6]  =
{ 4.432, 4.668, 4.754, 4.783, 4.789, 4.790 };   // this is here until I make changing KAISER_BETA dynamic. These are the values for KAISER_BETA = 5.0

void multiplyArrays(const double* array1, const double* array2, double* result, int64_t size)
{
    for (int64_t i = 0; i < size; i++)
    {
        result[i] = array1[i] * array2[i];
    }
}

double checkRatio(double ratio, double startingPoint, int64_t rounds)
{
    for (int64_t i = 0; i < rounds; i++)
    {
        // int64_t add = startingPoint * ratio + 1.0;
        // if (add <= 0) add = 1;
        startingPoint += floor((startingPoint * ratio) + 1.0);
    }
    return startingPoint;
}

double findRatio(int64_t maxDepth, double low, double high, double lowestValid, double startingPoint, double endGoal, int64_t rounds)
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

void complexPowerToRealPower(fftw_complex *complexData, double *realPower, int64_t numberOfElements)
{
    for (int64_t i = 0; i < numberOfElements; i++)
    {
        realPower[i] = creal(complexData[i]) + cimag(complexData[i]);
    }
}

int64_t min(int64_t a, int64_t b)
{
    return a < b ? a : b;
}

int64_t max(int64_t a, int64_t b)
{
    return a > b ? a : b;
}

void zeroSmallBins(double* output, int64_t samples, double substract)
{
    double biggest = 0;
    for (int64_t i = 0; i < samples; i++)
    {
        if (biggest < output[i]) biggest = output[i];
    }
    for (int64_t i = 0; i < samples; i++)
    {
        // we get rid of the unwanted frequency side lobes this way. kaiser 2 is quite eficient and we shouldn't see more than -60 dB on the side lobes
        // at the same time we make sure that we don't take logarithm out of zero. that would be -infinite
        output[i] = fmax(output[i] - biggest * substract, 0.0);
        // output[i] = 1;
    }
}

void normalizeLogBands(double *bands, int64_t size, double dynamicRange)
{
    static double bandCeiling = -INFINITY;
    bandCeiling -= 0.00027777777777777;          // now it takes 60 seconds to come 10 dB down
    for (int64_t i = 0; i < size; i++)
    {
        if (bands[i] > bandCeiling) bandCeiling = bands[i];
    }
    for (int64_t i = 0; i < size; i++)
    {
        bands[i] = (bands[i] - bandCeiling + dynamicRange) / dynamicRange;
    }
}

void powTwoBands(double *realPower, double *bands, int64_t n_bands, int64_t startingPoint, double ratio)
{
  
  for (int64_t i = 0; i < n_bands; i++)
  {
    bands[i] = 0;
    int64_t endPoint = startingPoint + floor((startingPoint * ratio) + 1.0);
    for (int64_t j = startingPoint; j < endPoint; j++)
    {
      bands[i] += realPower[j];
    }
    startingPoint = endPoint;
  }
}

void logBands(double *bands, double *logBands, int64_t n_bands, int64_t startingPoint, double ratio)
{
  for (int64_t i = 0; i < n_bands; i++)
  {
    logBands[i] = log10(bands[i]);
    int64_t delta = floor((startingPoint * ratio) + 1.0);
    if (delta < 1) delta = 1;
    if (delta > 6) delta = 6;
    logBands[i] = logBands[i] - substract_universal[delta - 1];
    startingPoint += delta;
  }
}

void* threadFunction(void* arg)
{
    double *windowingArray = (double*) malloc(sizeof(double) * global.FFTsize);
    double *real_in = (double*) fftw_malloc(sizeof(double) * global.FFTsize);
    fftw_complex *complex_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * global.FFTsize);
    fftw_complex *complexPower = (fftw_complex*) malloc(sizeof(fftw_complex) * global.FFTsize);
    double *realPower = (double*) malloc(sizeof(double) * global.FFTsize);
    double *bands_out = (double*) malloc(sizeof(double) * global.FFTsize);
    double *log10_bands = (double*) malloc(sizeof(double) * global.FFTsize);
    fftw_plan thePlan;
    fftw_init_threads();
    fftw_plan_with_nthreads(global.threads);
    thePlan = fftw_plan_dft_r2c_1d(global.FFTsize, real_in, complex_out, FFTW_ESTIMATE);
    generateKaiserWindow(global.FFTsize, KAISER_BETA, windowingArray);

    double *audio = (double*) malloc(sizeof(double) * global.FFTsize);
    
    if (global.usingNcurses)
    {
        printw("initialized FFT thread\n");
        refresh();
    }
    else
    {
        printf("initialized FFT thread\n");
    }
    
    if (global.usingGlfw)
    {
        initializeGlfw();
    }
    // if (global.usingNcurses) initializeNcurses();
    
    while(true)
    {
        if (global.allBuffer.buffers != NULL)
        {
            struct winsize w;
            ioctl(0, TIOCGWINSZ, &w);
            static int64_t windowRows = 0;
            static double ratio = 0;
            static int64_t startingPoint = 3;
            static int64_t oldSampleRate = 1;
            static int64_t frameNumber = 0;
            bool updatedSomething = false;
            
            static int64_t oldNumBars = -1;
            if (global.usingNcurses) global.numBars = w.ws_col;
            
            if (global.numBars != oldNumBars || global.sampleRate != oldSampleRate)
            {
                if (global.sampleRate != oldSampleRate) increaseBufferReadIndex(&global.allBuffer, 1000000000000);
                oldSampleRate = global.sampleRate;
                oldNumBars = global.numBars;
                startingPoint = max(global.minFrequency * global.FFTsize / global.sampleRate + 1, 3);
                int64_t n_bins = min(global.FFTsize / 2, global.maxFrequency * global.FFTsize / global.sampleRate);
                /*
                printw("\nstartingPoint = %lld, bins = %lld, columns = %d", startingPoint, n_bins, global.numBars);
                refresh();
                struct timespec rem;
                struct timespec req = {30, 0};
                nanosleep(&req, &rem);
                */
                ratio = findRatio(60, 0, 1, 0, startingPoint, n_bins, global.numBars);
                updatedSomething = true;
            }
            if (w.ws_row != windowRows)
            {
                windowRows = w.ws_row;
                updatedSomething = true;
            }
            static int64_t old_ns = 0;
            int64_t new_ns = nanoTime();
            int64_t delta_ns = new_ns - old_ns;

            int64_t newSamples = global.sampleRate * delta_ns / 999000000 + 1;
            int64_t readSamples = increaseBufferReadIndex(&global.allBuffer, newSamples);
            if (readSamples <= 0)
            {
                nanoSleepUniversal(1000000);
                goto skip;
            }
            // printw("s");
            frameNumber++;
            if (global.usingGlfw) startFrame();
            
            for (int64_t channel = 0; channel < global.channels; channel++)
            {
                readChunkFromBuffer(&global.allBuffer, (uint8_t*)audio, channel);
                multiplyArrays(audio, windowingArray, real_in, global.FFTsize);  // windowing
                fftw_execute(thePlan);
                multiplyArrays((double*)complex_out, (double*)complex_out, (double*)complexPower, global.FFTsize);  // these two steps turn co complex numbers
                complexPowerToRealPower(complexPower, realPower, global.FFTsize / 2);  // into the power of the lenght of ampltude
                zeroSmallBins(realPower, global.FFTsize / 2, 1.0 / (pow(10, global.dynamicRange + 0.3)));
                powTwoBands(realPower, bands_out, global.numBars, startingPoint, ratio);
                logBands(bands_out, log10_bands, global.numBars, startingPoint, ratio);
                normalizeLogBands(log10_bands, global.numBars, global.dynamicRange);
                color_set(1, NULL);
                if (global.channels == 2 && channel == 1)
                {
                    if (global.usingGlfw)
                    {
                        glfwSpectrum(log10_bands, global.numBars, 1.0, global.channels, channel, false, true);
                    }
                    if (global.usingNcurses)
                    {
                        drawSpectrum(log10_bands, global.numBars, max(w.ws_row, 3) / global.channels, max(w.ws_row, 3) / global.channels * channel, false);
                    }
                }
                else
                {
                    if (global.usingGlfw)
                    {
                        glfwSpectrum(log10_bands, global.numBars, 1.0, global.channels, channel, true, true);
                    }
                    if (global.usingNcurses)
                    {
                        drawSpectrum(log10_bands, global.numBars, max(w.ws_row, 3) / global.channels, max(w.ws_row, 3) / global.channels * channel, true);
                    }
                }
                
                // printw("%d", global.channels);
                // refresh();
            }
            
            
            static uint64_t printDebug_ns = 0;
            if ((w.ws_row % global.channels != 0 && (printDebug_ns + 10000000000 < new_ns)) || updatedSomething == true)
            {
                uint64_t printDebug_delta_ns = new_ns - printDebug_ns;
                double actualFPS = (double)frameNumber / (double)printDebug_delta_ns * 1000000000;
                if (global.usingNcurses)
                {
                    move(w.ws_row - 1, 0);
                    color_set(3, NULL);
                    printw("fps = %6f, sample rate = %d, FFTsize = %d, threads = %d, ratio = %f, bars = %d ", actualFPS, global.sampleRate, global.FFTsize, global.threads, ratio, global.numBars);
                }
                else
                {
                    printf("fps = %6f, sample rate = %d, FFTsize = %d, threads = %d, ratio = %f, bars = %d \n", actualFPS, global.sampleRate, global.FFTsize, global.threads, ratio, global.numBars);
                }
                printDebug_ns = new_ns;
                frameNumber = 0;
            }
            
            if (global.usingNcurses) refresh();
            if (global.usingGlfw) finalizeFrame();
            
            int64_t c = getch();
            if ((c == 27) || (c == 'q'))
            {
                killAll();
            }
            old_ns = new_ns;
        }
        
        if (global.usingGlfw == false)
        {
            int64_t interval = 1000000000 / global.fps;       // in nanoseconds
            static int64_t old_ns = 0;
            int64_t new_ns = nanoTime();
            int64_t delta_ns = new_ns - old_ns;
            int64_t sleepThisLong = max(interval - (int)delta_ns, 0);
            int64_t remainingSleep = nanoSleepUniversal(sleepThisLong);
            old_ns = nanoTime();
        }
        
        skip:;
        
    }
}

void startProperFFTalgorithm()
{
    pthread_create(&readAudioBuffer, NULL, threadFunction, NULL);
}

