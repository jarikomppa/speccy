/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// 16-bit incrementer values for all notes in 8 octaves at 8kHz (Hz / 8000 * 64k),
// tuned at 440HZ on middle-A, and note number 60 is C5. The highest couple
// octaves can probably be ditched because they're simply too high.
// Hz for a note can be calculated by multiplying (or dividing) the value of the
// previous note with the 12th root of 2.

int hz_to_incrementer(float hz)
{
    return (hz / (255 * 50)) * 65536;
}

float note(int n)
{
    float o = 440.0f;
    if (n > 0)
    {
        while (n)
        {
            o *= pow(2, 1/12.0f);
            n--;
        }
    }
    if (n < 0)
    {
        while (n)
        {
            o /= pow(2, 1/12.0f);
            n++;
        }
    }
    return o;
}

int main(int parc, char ** pars)
{
    int i;
    FILE * f = fopen("tonetab.h", "w");  

    fprintf(f, "const unsigned short tonetab[100] = {\n");
    for (i = -52; i < 48; i++)
    {    
        fprintf(f, "0x%04x%s", hz_to_incrementer(note(i)), (i==47)?"":",");
        if ((i+52) % 8 == 7)
            fprintf(f, "\n");
    }
    fprintf(f, "};\n");
    fclose(f);
    
    return 0;
}