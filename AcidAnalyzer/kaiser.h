#pragma once

#include <stdint.h>
// double besselI0(double x);
void generateKaiserWindow(int64_t N, double beta, double *window);
