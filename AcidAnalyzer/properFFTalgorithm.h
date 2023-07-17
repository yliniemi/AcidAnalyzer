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

#pragma once

#include <ringBuffer.h>
#include <defaults.h>
#include <kaiser.h>
#include <stdint.h>
#include <drawNcurses.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include <sys/ioctl.h>
#include <time.h>

extern struct Global global;

// void setChannels(int newChannels);

// void setSampleRate(int newSampleRate);

void multiplyArrays(const double* array1, const double* array2, double* result, int size);

double checkRatio(double ratio, double startingPoint, int rounds);

double findRatio(int maxDepth, double low, double high, double lowestValid, double startingPoint, double endGoal, int rounds);

void complexPowerToRealPower(fftw_complex *complexData, double *realPower, int numberOfElements);

int min(int a, int b);

int max(int a, int b);

void zeroSmallBins(double* output, int samples, double substract);

void normalizeLogBands(double *bands, int size, double dynamicRange);

void powTwoBands(double *realPower, double *bands, int n_bands, int startingPoint, double ratio);

void logBands(double *bands, double *logBands, int n_bands, int startingPoint, double ratio);

void* threadFunction(void* arg);

