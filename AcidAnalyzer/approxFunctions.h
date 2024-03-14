#pragma once

#ifndef PI
#define PI 3.14159265358979323846264338328
#endif

#include <stdint.h>

float sqrtApprox(float number);

void fillSinArray();

int32_t sinApprox(uint32_t angle);  // 16777216 is a full circle

int32_t cosApprox(uint32_t angle);  // 16777216 is a full circle

