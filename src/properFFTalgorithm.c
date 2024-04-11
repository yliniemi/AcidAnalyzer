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

#include <sys/ioctl.h>
#include <stdlib.h>

// #include <pthread.h>
// #include <semaphore.h>
#include <string.h>

extern struct Global global;

// extern struct FFTData FFTdata;
// struct FFTData FFTdata;

pthread_t readAudioBuffer;

double substract_universal[6]  =
{ 4.432, 4.668, 4.754, 4.783, 4.789, 4.790 };   // this is here until I make changing KAISER_BETA dynamic. These are the values for KAISER_BETA = 5.0

void multiplyArrays(double *array1, double *array2, double *result, int64_t size)
{
    for (int64_t i = 0; i < size; i++)
    {
        result[i] = array1[i] * array2[i];
    }
}

double checkRatio(double startingPoint, double ratio, int64_t rounds)
{
    for (int64_t i = 0; i < rounds; i++)
    {
        // int64_t add = startingPoint * ratio + 1.0;
        // if (add <= 0) add = 1;
        // startingPoint += floor((startingPoint * ratio) + 1.0);
        startingPoint = nextBin(startingPoint, ratio);
    }
    return startingPoint;
}

double findRatio(int64_t maxDepth, double low, double high, double lowestValid, double startingPoint, double endGoal, int64_t rounds)
{
    double middle = (low + high) * 0.5;
    double result = checkRatio(startingPoint, middle, rounds);
    #ifdef THIS_IS_A_TEST
    fprintf(stdout, "%.17f : %f\n", middle, result);
    #endif
    if (maxDepth <= 0) return low;
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

/*
void normalizeLogBands(double *bands, int64_t size, double dynamicRange)
{
    static double normalizerBar = -INFINITY;
    bandCeiling -= 0.0001;          // now it takes 60 seconds to come 10 dB down
    for (int64_t i = 0; i < size; i++)
    {
        if (bands[i] > bandCeiling) bandCeiling = bands[i];
    }
    for (int64_t i = 0; i < size; i++)
    {
        bands[i] = (bands[i] - bandCeiling + dynamicRange) / dynamicRange;
    }
}
*/

void powTwoBands(double *realPower, double *bands, int64_t n_bands, int64_t startingPoint, double ratio)
{
  
  for (int64_t i = 0; i < n_bands; i++)
  {
    bands[i] = 0;
    int64_t endPoint = nextBin(startingPoint, ratio);
    for (int64_t j = startingPoint; j < endPoint; j++)
    {
      bands[i] += realPower[j];
    }
    startingPoint = endPoint;
  }
}

double logBands(double *bands, double *logBands, int64_t n_bands, int64_t startingPoint, double ratio, double highestBar)
{
  double newHighestBar = -INFINITY;
  for (int64_t i = 0; i < n_bands; i++)
  {
    logBands[i] = log10(bands[i]);
    int64_t delta = nextBin(startingPoint, ratio) - startingPoint;
    if (delta < 1) delta = 1;
    if (delta > 6) delta = 6;
    double band = logBands[i] - substract_universal[delta - 1];
    if (band > newHighestBar) newHighestBar = band;
    logBands[i] = band - highestBar;
    startingPoint += delta;
  }
  return newHighestBar;
}

void channelAnalysis(struct ChannelData *channelData, struct AllChannelData *allChannelData)
{
    readChunkFromBuffer(&global.allBuffer, (uint8_t*)channelData->audioData, channelData->channelNumber);
    multiplyArrays(channelData->audioData, allChannelData->windowingArray, channelData->windowedAudio, allChannelData->FFTsize);  // windowing
    fftw_execute(channelData->plan);
    multiplyArrays((double*)channelData->complexFFT, (double*)channelData->complexFFT, (double*)channelData->complexPower, allChannelData->FFTsize);  // these two steps turn co complex numbers
    complexPowerToRealPower(channelData->complexPower, channelData->realPower, allChannelData->FFTsize / 2);  // into the power of the lenght of ampltude
    zeroSmallBins(channelData->realPower, allChannelData->FFTsize / 2, 1.0 / (pow(10, global.dynamicRange + 0.3)));
    powTwoBands(channelData->realPower, channelData->bands, global.numBars, allChannelData->firstBin, allChannelData->ratio);
    channelData->highestBar = logBands(channelData->bands, channelData->log10Bands, global.numBars, allChannelData->firstBin, allChannelData->ratio, allChannelData->normalizerBar);
    // normalizeLogBands(channelData->log10Bands, global.numBars, global.dynamicRange);
    // if (channelData->highestBar >= allChannelData->normalizerBar) printf("highestBar = %f\n", channelData->highestBar - allChannelData->normalizerBar);
}


void allocateChannelData(struct ChannelData *channelData, int64_t FFTsize)
{
    // channelData->FFTsize = FFTsize;
    // channelData->windowingArray = &global.windowingArray;
    channelData->audioData = (double*) malloc(sizeof(double) * FFTsize);
    channelData->windowedAudio = (double*) fftw_malloc(sizeof(double) * FFTsize);
    channelData->complexFFT = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FFTsize);
    channelData->complexPower = (fftw_complex*) malloc(sizeof(fftw_complex) * FFTsize);
    channelData->realPower = (double*) malloc(sizeof(double) * FFTsize);
    channelData->bands = (double*) malloc(sizeof(double) * FFTsize);
    channelData->log10Bands = (double*) malloc(sizeof(double) * FFTsize);
    channelData->plan = fftw_plan_dft_r2c_1d(FFTsize, channelData->windowedAudio, channelData->complexFFT, FFTW_ESTIMATE);
    channelData->edge = (struct Edge*) malloc(sizeof(struct Edge) * (FFTsize + 4));
    channelData->vertex = (struct Vertex*) malloc(sizeof(struct Vertex) * (FFTsize + 4));
    channelData->highestBar = -INFINITY;
}

void increaseNumberOfChannels(struct AllChannelData *allChannelData, int64_t newNumberOfChannels)
{
    if (allChannelData->maxNumberOfChannelsEver >= newNumberOfChannels) return;
    struct ChannelData **oldChannelDataArray = allChannelData->channelDataArray;
    struct ChannelData **newChannelDataArray = malloc(newNumberOfChannels * sizeof(struct ChannelData*));
    memcpy(allChannelData->channelDataArray, newChannelDataArray, sizeof(struct ChannelData*) * allChannelData->maxNumberOfChannelsEver);
    allChannelData->channelDataArray = newChannelDataArray;
    
    for (int64_t i = allChannelData->maxNumberOfChannelsEver; i < newNumberOfChannels; i++)
    {
        allChannelData->channelDataArray[i] = (struct ChannelData*)malloc(sizeof(struct ChannelData));
        if (allChannelData->channelDataArray[i] == NULL)
        {
            // printf("failed to initialize one of the buffers\n");
            // refresh();
        }
        allocateChannelData(allChannelData->channelDataArray[i], global.FFTsize);
        allChannelData->channelDataArray[i]->channelNumber = i;
        // allChannelData->channelDataArray[i]->FFTsize = global.FFTsize;
        // allChannelData->channelDataArray[i]->windowingArray = allChannelData->sharedWindowingArray;
    }
    
    allChannelData->maxNumberOfChannelsEver = newNumberOfChannels;
    // allChannelData->numberOfChannels = newNumberOfChannels;
    free(oldChannelDataArray);
}

double findNormalizer(struct AllChannelData *allChannelData)
{
    // allChannelData.normalizerBar -= 0.0001;
    for (int64_t channel = 0; channel < allChannelData->numberOfChannels; channel++)
    {
        printf("asdfghjkl\n");
        double highestBar = allChannelData->channelDataArray[channel]->highestBar;
        if (highestBar > allChannelData->normalizerBar) allChannelData->normalizerBar = highestBar;
    }
}


void* threadFunction(void* arg)
{
    /*
    double *windowingArray = (double*) malloc(sizeof(double) * global.FFTsize);
    double *real_in = (double*) fftw_malloc(sizeof(double) * global.FFTsize);
    fftw_complex *complex_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * global.FFTsize);
    fftw_complex *complexPower = (fftw_complex*) malloc(sizeof(fftw_complex) * global.FFTsize);
    double *realPower = (double*) malloc(sizeof(double) * global.FFTsize);
    double *bands_out = (double*) malloc(sizeof(double) * global.FFTsize);
    double *log10_bands = (double*) malloc(sizeof(double) * global.FFTsize);
    fftw_plan thePlan;
    // fftw_init_threads();
    // fftw_plan_with_nthreads(global.threadsPerChannel);
    thePlan = fftw_plan_dft_r2c_1d(global.FFTsize, real_in, complex_out, FFTW_ESTIMATE);
    double *audio = (double*) malloc(sizeof(double) * global.FFTsize);
    */
    
    // printf("sizeof(pthread_t) = %lld\n", sizeof(pthread_t));
    
    struct AllChannelData allChannelData;
    allChannelData.numberOfChannels = 0;
    allChannelData.maxNumberOfChannelsEver = 0;
    
    allChannelData.windowingArray = (double*) malloc(sizeof(double) * global.FFTsize);
    generateKaiserWindow(global.FFTsize, KAISER_BETA, allChannelData.windowingArray);
    allChannelData.ratio = 0;
    allChannelData.firstBin = 3;
    allChannelData.FFTsize = global.FFTsize;
    allChannelData.normalizerBar = -INFINITY;
    
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
       static int64_t old_ns = 0;
       static int64_t new_ns;
       new_ns = nanoTime();
       if (global.smoothenAnimation && new_ns - old_ns < 995000000 / global.fps) new_ns = old_ns + 995000000 / global.fps;
       int64_t delta_ns = new_ns - old_ns;
        
        allChannelData.normalizerBar -= (double)delta_ns / 1000000000 * global.decreaseCeilingBelsPerSecond;
        for (int64_t channel = 0; channel < allChannelData.numberOfChannels; channel++)
        {
            // printf("asdfghjkl\n");
            double highestBar = allChannelData.channelDataArray[channel]->highestBar;
            if (highestBar > allChannelData.normalizerBar) allChannelData.normalizerBar = highestBar;
        }
        // findNormalizer(&allChannelData);
        
        
        allChannelData.numberOfChannels = global.channels;
        if (allChannelData.numberOfChannels > allChannelData.maxNumberOfChannelsEver)
        {
            increaseNumberOfChannels(&allChannelData, allChannelData.numberOfChannels);
        }
        
        if (global.allBuffer.buffers != NULL)
        {
            struct winsize w;
            ioctl(0, TIOCGWINSZ, &w);
            static int64_t windowRows = 0;
            static int64_t oldSampleRate = 1;
            static int64_t oldFFTsize = 0;
            static int64_t oldChannelNumber = 0;
            static int64_t frameNumber = 0;
            bool updatedSomething = false;
            
            static int64_t oldNumBars = -1;
            
            // if (global.usingGlfw) global.fps = getRefreshRate();
            
            if (global.usingNcurses) global.numBars = w.ws_col;
            
            if (global.numBars != oldNumBars || global.sampleRate != oldSampleRate || global.FFTsize != oldFFTsize || oldChannelNumber != allChannelData.numberOfChannels)
            {
                if (global.sampleRate != oldSampleRate) increaseBufferReadIndex(&global.allBuffer, 1000000000000);
                oldSampleRate = global.sampleRate;
                oldNumBars = global.numBars;
                oldFFTsize = global.FFTsize;
                oldChannelNumber = allChannelData.numberOfChannels;
                allChannelData.firstBin = max(global.minFrequency * global.FFTsize / global.sampleRate, 3);
                // printf("firstBin = %lld\n", allChannelData.firstBin);
                int64_t lastBin = min(global.FFTsize / 2, global.maxFrequency * global.FFTsize / global.sampleRate);
                allChannelData.lastBin = lastBin;
                /*
                printw("\nstartingPoint = %lld, bins = %lld, columns = %lld", startingPoint, lastBin, global.numBars);
                refresh();
                struct timespec rem;
                struct timespec req = {30, 0};
                nanosleep(&req, &rem);
                */
                allChannelData.ratio = findRatio(60, 1, 2, 1, allChannelData.firstBin, lastBin, global.numBars);
                if (lastBin - allChannelData.firstBin < global.numBars) global.numBars = lastBin - allChannelData.firstBin;
                double intermediaryBin = allChannelData.firstBin;
                for (int64_t i = 1; i < global.numBars; i++)
                {
                    // intermediaryBin += floor((intermediaryBin * ratio) + 1.0);
                    intermediaryBin = nextBin(intermediaryBin, allChannelData.ratio);
                }
                allChannelData.secondToLastBin = intermediaryBin;
                lastBin = nextBin(intermediaryBin, allChannelData.ratio);
                // printf("lastBin = %lld or %lld or %lld\n", FFTdata.lastBin,  lastBin, (int64_t)checkRatio(FFTdata.firstBin, FFTdata.ratio, global.numBars));
                allChannelData.lastBin = lastBin;
                calculateLocationData(&allChannelData);
                updatedSomething = true;
                
                printf("sample rate = %lld, FFT = %lld, ratio = %f, bars = %lld, bins %lld-%lld\n", global.sampleRate, global.FFTsize, allChannelData.ratio, global.numBars, allChannelData.firstBin, allChannelData.lastBin);
            }
            if (w.ws_row != windowRows)
            {
                windowRows = w.ws_row;
                
                updatedSomething = true;
            }
            
            // static double newSamplesRemainder = 0;
            double bufferDepth = getBufferReadSpace(&global.allBuffer);
            double slope = lerp(1.005, 1.05, (bufferDepth - 2 * (double)global.sampleRate / (global.fps - 1)) / (double)global.sampleRate);
            if (bufferDepth < 2 * global.sampleRate / (global.fps - 1)) slope = 1.005;
            static double remainder = 0;
            double newSamples = ((double)global.sampleRate * delta_ns) / 1000000000.0 * slope + 1 + remainder;
            remainder = newSamples - (int64_t)newSamples;
            // if (newSamples < global.leastReadSamples) global.leastReadSamples = newSamples;
            double samplesAfterReading = bufferDepth - newSamples;
            if (samplesAfterReading < global.sampleRate / global.fps)
            {
                newSamples = newSamples * lerp(0.9, 1.0, samplesAfterReading / (double)global.sampleRate * global.fps);
            }
            // if (global.smoothenAnimation == false) newSamples = ((double)global.sampleRate * delta_ns) / 1000000000.0 * slope + 1 + remainder;    // not actually needed because of the if block before
            // newSamplesRemainder = newSamples - floor(newSamples);
            int64_t readSamples = increaseBufferReadIndex(&global.allBuffer, newSamples);
            if (readSamples <= 0)
            {
                old_ns = new_ns;
                nanoSleepUniversal(1000000);
                goto skip;
            }
            if (readSamples < global.leastReadSamples) global.leastReadSamples = readSamples;
            if (readSamples > global.mostReadSamples) global.mostReadSamples = readSamples;
            bufferDepth = getBufferReadSpace(&global.allBuffer);
            if (bufferDepth < global.minBufferDepth) global.minBufferDepth = bufferDepth;
            if (bufferDepth > global.maxBufferDept) global.maxBufferDept = bufferDepth;
            // printw("s");
            frameNumber++;
            
            
            for (int64_t channel = 0; channel < allChannelData.numberOfChannels; channel++)
            {
                
                channelAnalysis(allChannelData.channelDataArray[channel], &allChannelData);
                
                // color_set(1, NULL);
            }
            
            if (global.usingGlfw)
            {
                glfwSpectrum(&allChannelData);
            }
            
            if (global.usingNcurses)
            {
                for (int64_t channel = 0; channel < allChannelData.numberOfChannels; channel++)
                {
                    struct ChannelData *channelData = allChannelData.channelDataArray[channel];
                    
                    if (global.channels == 2 && channel == 1)
                    {
                        drawSpectrum(channelData->log10Bands, global.numBars, max(w.ws_row, 3) / allChannelData.numberOfChannels, max(w.ws_row, 3) / global.channels * channel, false);
                    }
                    else
                    {
                        drawSpectrum(channelData->log10Bands, global.numBars, max(w.ws_row, 3) / allChannelData.numberOfChannels, max(w.ws_row, 3) / global.channels * channel, true);
                    }
                }
            }
            
            
            static int64_t printDebug_ns = 0;
            int64_t new_debug_ns = nanoTime();
            if ((printDebug_ns + 10000000000 < new_debug_ns) || updatedSomething == true)
            {
                int64_t printDebug_delta_ns = new_debug_ns - printDebug_ns;
                double actualFPS = (double)frameNumber / (double)printDebug_delta_ns * 1000000000;
                if (global.usingNcurses)
                {
                    move(w.ws_row - 1, 0);
                    color_set(3, NULL);
                    printw("fps = %.2f, capturedSamples = %lld, readSamples = %lld-%lld, bufferDepth = %lld-%lld, normalizer = %f\n", actualFPS, global.mostCapturedSamples, global.leastReadSamples, global.mostReadSamples, global.minBufferDepth, global.maxBufferDept, allChannelData.normalizerBar);
                }
                else
                {
                    printf("fps = %.2f, capturedSamples = %lld, readSamples = %lld-%lld, bufferDepth = %lld-%lld, normalizer = %f\n", actualFPS, global.mostCapturedSamples, global.leastReadSamples, global.mostReadSamples, global.minBufferDepth, global.maxBufferDept, allChannelData.normalizerBar);
                }
                printDebug_ns = new_debug_ns;
                frameNumber = 0;
                global.leastReadSamples = 1000000000;
                global.mostReadSamples = 0;
                global.minBufferDepth = 1000000000;
                global.maxBufferDept = 0;
                global.mostCapturedSamples = 0;
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
            skip:;
            old_ns = nanoTime();
        }
    }
}

void startProperFFTalgorithm()
{
    pthread_create(&readAudioBuffer, NULL, threadFunction, NULL);
}

