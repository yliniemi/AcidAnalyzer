#include <drawNcurses.h>

void drawSpectrum(double *soundArray, int64_t x_size, int64_t y_size, int64_t y_startingLine, bool uprigth)
{
    static wchar_t *character[] = {L" ", L"\u2581" , L"\u2582" , L"\u2583" , L"\u2584" , L"\u2585" , L"\u2586" , L"\u2587" , L"\u2588"};
    if (uprigth)
    {
        color_set(1, NULL);
        // attroff(A_REVERSE);
        for (int64_t x = 0; x < x_size; x++)
        {
            int64_t soundInt = 0;
            if (soundArray[x] < -1000000000) soundInt = -1000000000;
            else soundInt = soundArray[x] * ((y_size) * 8) + 8;
            for (int64_t y = y_startingLine + y_size; y >= y_startingLine; y--)
            {
                int64_t charIndex = soundInt;
                if (soundInt <= 0)
                {
                    charIndex = 0;
                }
                else if (soundInt >= 8)
                {
                    charIndex = 8;
                    // mvaddch(y, x, ' ' | A_REVERSE);
                }
                mvaddwstr(y, x, character[charIndex]);
                soundInt -= 8;
            }
        }
    }
    else
    {
        // attron(A_REVERSE);
        color_set(2, NULL);
        for (int64_t x = 0; x < x_size; x++)
        {
            int64_t soundInt = 0;
            if (soundArray[x] < -1000000000) soundInt = -1000000000;
            else soundInt = soundArray[x] * ((y_size) * 8);
            for (int64_t y = y_startingLine; y < y_startingLine + y_size; y++)
            {
                int64_t charIndex = soundInt;
                if (soundInt <= 0)
                {
                    charIndex = 8;
                }
                else if (soundInt >= 8)
                {
                    charIndex = 0;
                }
                else
                {
                    charIndex = 8 - soundInt;
                }
                mvaddwstr(y, x, character[charIndex]);
                soundInt -= 8;
            }
        }
    }    
}


