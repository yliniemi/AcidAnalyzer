#pragma once

#define _XOPEN_SOURCE_EXTENDED 1

#include <locale.h>
#include <wchar.h>
#include <ncurses.h>

void drawSpectrum(double *soundArray, int64_t x_size, int64_t y_size, int64_t y_startingLine, bool uprigth);
