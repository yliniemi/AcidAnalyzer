/*
    Acid Analyzer
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <complex.h> 
#include <fftw3.h>
// #include <time.h>
// #include <kaiser.h>
#include <ringBuffer.h>
#include <properFFTalgorithm.h>

#define MAIN
#include <defaults.h>

#ifndef DUMMY_NOISE
#include <readPipewire.h>
#endif

#include <locale.h>
#include <wchar.h>
#include <ncurses.h>

#include <string.h>

// #include <time.h>
#include <nanoTime.h>
#include <dummyAudio.h>

// #include <approxFunctions.h>


int64_t replaceChar(char *str, char orig, char rep) {
    char *ix = str;
    int64_t n = 0;
    while((ix = strchr(ix, orig)) != NULL) {
        *ix++ = rep;
        n++;
    }
    return n;
}

void parseArguments(int64_t argc, char *argv[])
{
    for (int64_t i = 0; i < argc - 1; i++)
    {
        if (strcmp(argv[i], "--fps") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.fps = strtod(argv[i + 1], NULL);
            printf("fps = %f\n", global.fps);
        }
        if (strcmp(argv[i], "--FFTsize") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.FFTsize = strtol(argv[i + 1], NULL, 10);
            printf("FFTsize = %lld\n", global.FFTsize);
        }
        if (strcmp(argv[i], "--dynamicRange") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.dynamicRange = strtod(argv[i + 1], NULL) / 10.0;  // conversion from Bel to deciBel
            printf("dynamicRange = %f\n", global.dynamicRange);
        }
        if (strcmp(argv[i], "--kaiserBeta") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.kaiserBeta = strtod(argv[i + 1], NULL);
            printf("kaiserBeta = %f\n", global.kaiserBeta);
        }
        if (strcmp(argv[i], "--bufferExtra") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.bufferExtra = strtol(argv[i + 1], NULL, 10);
            printf("bufferExtra = %lld\n", global.bufferExtra);
        }
        if (strcmp(argv[i], "--barMode") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.barMode = strtol(argv[i + 1], NULL, 10);
            printf("barMode = %lld\n", global.barMode);
        }
        if (strcmp(argv[i], "--minFrequency") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.minFrequency = strtod(argv[i + 1], NULL);
            printf("minFrequency = %f\n", global.minFrequency);
        }
        if (strcmp(argv[i], "--barWidth") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.barWidth = strtod(argv[i + 1], NULL);
            printf("barWidth = %f\n", global.barWidth);
        }
        if (strcmp(argv[i], "--maxFrequency") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.maxFrequency = strtod(argv[i + 1], NULL);
            printf("maxFrequency = %f\n", global.maxFrequency);
        }
        if (strcmp(argv[i], "--bars") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.numBars = strtol(argv[i + 1], NULL, 10);
            printf("numBars = %lld\n", global.numBars);
        }
        if (strcmp(argv[i], "--colors0") == 0)
        {
            for (int j = 0; j < 3; j++)
            {
                replaceChar(argv[i + 1 + j], ',', '.');
                global.colors[0][j] = strtol(argv[i + 1 + j], NULL, 10);
            }
            printf("colors[0] = {%lld, %lld, %lld}\n", global.colors[0][0], global.colors[0][1], global.colors[0][2]);
        }
        if (strcmp(argv[i], "--colors1") == 0)
        {
            for (int j = 0; j < 3; j++)
            {
                replaceChar(argv[i + 1 + j], ',', '.');
                global.colors[1][j] = strtol(argv[i + 1 + j], NULL, 10);
            }
            printf("colors[1] = {%lld, %lld, %lld}\n", global.colors[1][0], global.colors[1][1], global.colors[1][2]);
        }
        if (strcmp(argv[i], "--ncurses") == 0)
        {
            if (strcmp(argv[i + 1], "0") == 0 || strcmp(argv[i + 1], "false") == 0)
            {
                global.usingNcurses = false;
            }
            else
            {
                global.usingNcurses = true;
            }
            printf("usingNcurses = %s\n", global.usingNcurses ? "true" : "false");
        }
        if (strcmp(argv[i], "--glfw") == 0)
        {
            if (strcmp(argv[i + 1], "0") == 0 || strcmp(argv[i + 1], "false") == 0)
            {
                global.usingGlfw = false;
            }
            else
            {
                global.usingGlfw = true;
            }
            printf("usingGlfw = %s\n", global.usingGlfw ? "true" : "false");
        }
        if (strcmp(argv[i], "--colorRange") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.colorRange = strtod(argv[i + 1], NULL);
            printf("colorRange = %f\n", global.colorRange);
        }
        if (strcmp(argv[i], "--colorSpeed") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.colorSpeed = strtod(argv[i + 1], NULL);
            printf("colorSpeed = %f\n", global.colorSpeed);
        }
        if (strcmp(argv[i], "--colorSaturation") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.colorSaturation = strtod(argv[i + 1], NULL);
            printf("colorSaturation = %f\n", global.colorSaturation);
        }
        if (strcmp(argv[i], "--colorStart") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.colorStart = strtod(argv[i + 1], NULL);
            printf("colorStart = %f\n", global.colorStart);
        }
        if (strcmp(argv[i], "--colorBrightness") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.colorBrightness = strtod(argv[i + 1], NULL);
            printf("colorBrightness = %f\n", global.colorBrightness);
        }
        if (strcmp(argv[i], "--colorOpacity") == 0)
        {
            replaceChar(argv[i + 1], ',', '.');
            global.colorOpacity = strtod(argv[i + 1], NULL);
            printf("colorOpacity = %f\n", global.colorOpacity);
        }
    }
}



#ifndef THIS_IS_A_TEST
int main(int argc, char *argv[])
{
    setGobalDefaults();
    /*
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
    */
    
    
    printf("\033]0;\u2581\u2582\u2583\u2584\u2585\u2586\u2587\u2588 Acid Analyzer \u2588\u2587\u2586\u2585\u2584\u2583\u2582\u2581\007");
    
    parseArguments(argc, argv);
    if (argc > 1 && global.usingNcurses) sleep(5);
    srand(nanoTime());
    if (global.usingNcurses)
    {
        setlocale(LC_ALL, "");
        setlocale(LC_NUMERIC, "en_GB.UTF-8");
        initscr();
        
        noecho();
        timeout(0);
        
        curs_set(0);
        start_color();
        printw("program started \u2581\u2582\u2583\u2584\u2585\u2586\u2587\u2588\n");
        
        refresh();
        
        init_color(COLOR_BLUE, global.colors[0][0], global.colors[0][1], global.colors[0][2]);
        init_color(COLOR_CYAN, global.colors[1][0], global.colors[1][1], global.colors[1][2]);
        
        init_pair(1, COLOR_BLUE, COLOR_BLACK);
        init_pair(2, COLOR_BLACK, COLOR_CYAN);
        init_pair(3, COLOR_WHITE, COLOR_BLACK);
        color_set(3, NULL);
        printw("colors loaded\n");
        refresh();
        // attron(COLOR_PAIR(1));
        
        // fprintf(stdout, "sizeof(fftw_complex) = %lld\n", sizeof(fftw_complex));
        
        /* and wait while we let things run */
    }
    
    startProperFFTalgorithm();
    // pthread_create(&readStuff, NULL, threadFunction, NULL);
    // initializeGlfw();
    
    // pthread_create(&glfwThread, NULL, testGlfw, NULL);
    
    #ifdef DUMMY_NOISE
    startDummyAudio();
    #else
    startPipewire(argc, argv);
    #endif
    
    return 0;
}
#endif
