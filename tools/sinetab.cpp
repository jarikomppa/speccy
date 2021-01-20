/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

/* Make Visual Studio not consider fopen() deprecated */
#define _CRT_SECURE_NO_DEPRECATE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

int main(int parc, char ** pars)
{
    int i;
    
    if (parc < 5)
    {
        printf("%s <filename> <tabname> <amplitude> <width>\nExample: %s sinetab.h sinofs 192 256\n", pars[0], pars[0]);
        return 0;
    }
    
    int amp = atoi(pars[3]);
    int width = atoi(pars[4]);
    
    FILE * f = fopen(pars[1], "w");

    fprintf(f, "const unsigned short %s[%d] = {\n", pars[2], width);
    for (i = 0; i < width; i++)
    {
        fprintf(f, "%4d%s", (int)floor(((sin(i * 2 * M_PI / (float)width)+1)/2) * amp), (i==width-1)?"":",");
        if (i % 16 == 15)
            fprintf(f, "\n");
    }
    fprintf(f, "};\n");
    fclose(f);
    
    return 0;
}
