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

#include <stdint.h>
#include <stdbool.h>


// void multiplyArrays(const double* array1, const double* array2, double* result, int64_t size);

// double checkRatio(double ratio, double startingPoint, int64_t rounds);

double findRatio(int64_t maxDepth, double low, double high, double lowestValid, double startingPoint, double endGoal, int64_t rounds);

// void complexPowerToRealPower(fftw_complex *complexData, double *realPower, int64_t numberOfElements);

// int64_t min(int64_t a, int64_t b);

// int64_t max(int64_t a, int64_t b);

// void zeroSmallBins(double* output, int64_t samples, double substract);

// void normalizeLogBands(double *bands, int64_t size, double dynamicRange);

// void powTwoBands(double *realPower, double *bands, int64_t n_bands, int64_t startingPoint, double ratio);

// void logBands(double *bands, double *logBands, int64_t n_bands, int64_t startingPoint, double ratio);

void* threadFunction(void* arg);

