/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

int main(int parc, char ** pars)
{
    int i;
    FILE * f = fopen("sinetab.h", "w");

    fprintf(f, "const unsigned short sinofs[256] = {\n");
    for (i = 0; i < 256; i++)
    {
        fprintf(f, "%4d%s", (int)floor(((sin(i * 2 * M_PI / 256.0f)+1)/2) * 192), (i==255)?"":",");
        if (i % 16 == 15)
            fprintf(f, "\n");
    }
    fprintf(f, "};\n");
    fclose(f);
    
    return 0;
}