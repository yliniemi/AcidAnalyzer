#pragma once

#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846264338328
#endif

double besselI0(double x);
void generateKaiserWindow(int N, double beta, double *window);
