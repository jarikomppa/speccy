/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/
#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(int parc, char ** pars)
{
    int x, y, comp;
    stbi_uc * raw = stbi_load("s.png", &x, &y, &comp, 4);
    printf("%d %d %d\n", x, y, comp);
    unsigned int *p = (unsigned int *)raw;
    FILE * f = fopen("png.bin","wb");
    
    int i;
    int d = 0;
    for (i = 0; i < x*y; i++)
    {
        d <<= 1;
        d |= (p[i] & 0xffffff) == 0;
        if (i % 8 == 7)
        {
            fputc(d, f);
            d = 0;
        }
    }
    
    fclose(f);
    return 0;
}