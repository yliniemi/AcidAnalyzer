#include <properFFTalgorithm.h>

double substract_universal[6]  =
{ 4.432, 4.668, 4.754, 4.783, 4.789, 4.790 };

struct RingBuffer allBuffer;
int sampleRate = 1;
int channels = 1;

/*
void setChannels(int newChannels)
{
	channels = newChannels;
}

void setSampleRate(int newSampleRate)
{
	sampleRate = newSampleRate;
}
*/

void multiplyArrays(const double* array1, const double* array2, double* result, int size)
{
    for (int i = 0; i < size; i++)
    {
        result[i] = array1[i] * array2[i];
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
    generateKaiserWindow(NUMBER_OF_FFT_SAMPLES, KAISER_BETA, windowingArray);

    double *audio = (double*) malloc(sizeof(double) * NUMBER_OF_FFT_SAMPLES);

    while(true)
    {
        if (allBuffer.buffers != NULL)
        {
            struct winsize w;
            ioctl(0, TIOCGWINSZ, &w);
            static int windowColums = 0;
            static double ratio = 0;
            static int startingPoint = 3;
            static int oldSampleRate = 1;
            if (w.ws_col != windowColums || sampleRate != oldSampleRate)
            {
                windowColums = w.ws_col;
                oldSampleRate = sampleRate;
                startingPoint = max(20 * NUMBER_OF_FFT_SAMPLES / sampleRate + 1, 3);
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
            
            
            static int frameNumber = 0;
            for (int channel = 0; channel < channels; channel++)
            {
                readChunkFromBuffer(&allBuffer, (uint8_t*)audio, channel);
	            multiplyArrays(audio, windowingArray, real_in, NUMBER_OF_FFT_SAMPLES);  // windowing
	            fftw_execute(thePlan);
	            multiplyArrays((double*)complex_out, (double*)complex_out, (double*)complexPower, NUMBER_OF_FFT_SAMPLES);  // these two steps turn co complex numbers
	            complexPowerToRealPower(complexPower, realPower, NUMBER_OF_FFT_SAMPLES / 2);  // into the power of the lenght of ampltude
	            zeroSmallBins(realPower, NUMBER_OF_FFT_SAMPLES / 2, 0.0000005);
	            powTwoBands(realPower, bands_out, windowColums, startingPoint, ratio);
	            logBands(bands_out, log10_bands, windowColums, startingPoint, ratio);
	            normalizeLogBands(log10_bands, windowColums, 5.0);
	            color_set(1, NULL);
	            if (channels == 2 && channel == 1)
	            {
	                drawSpectrum(log10_bands, windowColums, max(w.ws_row, 3) / channels, max(w.ws_row, 3) / channels * channel, false);
	            }
	            else
	            {
	                drawSpectrum(log10_bands, windowColums, max(w.ws_row, 3) / channels, max(w.ws_row, 3) / channels * channel, true);
	            }
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
        uint64_t sleepThisLong = min(max((int)8333333 - (int)delta_ns, 0), 100000000);
        struct timespec req = {sleepThisLong / 1000000000, sleepThisLong % 1000000000};
        nanosleep(&req, &rem);
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        old_ns = ts.tv_sec * 1000000000 + ts.tv_nsec;
    }
}

