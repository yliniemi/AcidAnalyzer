#include <approxFunctions.h>

#include <math.h>

int16_t sinArray[320];
int16_t *cosArray = &sinArray[64];

void fillSinArray()
{
    for (int i = 0; i < 320; i++)
    {
        sinArray[i] = round(sin((double)i * 2.0 * PI / 256.0) * 32767.0);
    }
}

int32_t sinApprox(uint32_t angle)  // 16777216 is a full circle
{
    uint8_t angle256 = angle / 65536;
    uint32_t subAngle256 = angle % 65536;
    return sinArray[angle256] * (65536 - subAngle256) + sinArray[(uint8_t)(angle256 + 1)] * subAngle256;
}

int32_t cosApprox(uint32_t angle)  // 16777216 is a full circle
{
    uint8_t angle256 = angle / 65536;
    uint32_t subAngle256 = angle % 65536;
    return cosArray[angle256] * (65536 - subAngle256) + cosArray[(uint8_t)(angle256 + 1)] * subAngle256;
}

float sqrtApprox(float number)
{
  union { float f; uint32_t u; } y = {number};
  y.u = 0x5F1FFFF9ul - (y.u >> 1);
  return number * 0.703952253f * y.f * (2.38924456f - number * y.f * y.f);
}


