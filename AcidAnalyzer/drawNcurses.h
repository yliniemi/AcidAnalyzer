#pragma once

#define _XOPEN_SOURCE_EXTENDED 1

#include <locale.h>
#include <wchar.h>
#include <ncursesw/ncurses.h>

void drawSpectrum(double *soundArray, int x_size, int y_size, int y_startingLine, bool uprigth);
