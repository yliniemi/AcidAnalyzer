#include <drawNcurses.h>

void drawSpectrum(double *soundArray, int x_size, int y_size, int y_startingLine)
{
    static wchar_t *character[] = {L" ", L"\u2581" , L"\u2582" , L"\u2583" , L"\u2584" , L"\u2585" , L"\u2586" , L"\u2587" , L"\u2588"};
    for (int x = 0; x < x_size; x++)
    {
        int soundInt = 0;
        if (soundArray[x] < -1000000000) soundInt = -1000000000;
        else soundInt = soundArray[x] * ((y_size + 1) * 8);
        // mvaddch(10, x, 'X');
        // int soundInt = 123;
        for (int y = y_startingLine + y_size; y >= y_startingLine; y--)
        {
            // mvaddch(y, 20, 'X');
            int charIndex = soundInt;
            if (soundInt <= 0)
            {
                charIndex = 0;
                // mvaddch(y, x, ' ');
            }
            else if (soundInt >= 8)
            {
                charIndex = 8;
                // mvaddch(y, x, ' ' | A_REVERSE);
            }
            // else
            {
                mvaddwstr(y, x, character[charIndex]);
                // mvprintw(x,y, character[charIndex]);
            }
            soundInt -=8;
        }
    }
}


