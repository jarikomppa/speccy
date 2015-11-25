/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void fprintbin(FILE *f, int val)
{
    fprintf(f, "0b%d%d%d%d%d%d%d%d",
        (val & (1 << 7)) != 0,
        (val & (1 << 6)) != 0,
        (val & (1 << 5)) != 0,
        (val & (1 << 4)) != 0,
        (val & (1 << 3)) != 0,
        (val & (1 << 2)) != 0,
        (val & (1 << 1)) != 0,
        (val & (1 << 0)) != 0);
}

unsigned int spritedata[64];
unsigned int spritemask[64];

int main(int parc, char ** pars)
{    
    int x, y, comp;
    memset(spritedata,0,sizeof(int)*64);
    memset(spritemask,0,sizeof(int)*64);
    if (parc < 3)
    {
        printf("Usage: %s pngfile outfile\n", pars[0]);
        return 0;
    }
    stbi_uc * raw = stbi_load(pars[1], &x, &y, &comp, 4);
    printf("%s - %d %d %d\n", pars[1], x, y, comp);
    unsigned int *p = (unsigned int *)raw;
    
    int i, j, c;
    for (i = 0, c = 0; i < y; i++)
    {
        int d = 0;
        int m = 0;
        int xout = 0;
        for (j = 0; j < x; j++, c++)
        {
            d <<= 1;
            d |= (p[c] == 0xffffffff) != 0;
            m <<= 1;
            m |= (p[c] == 0xffffffff || p[c] == 0xff000000) == 1;
            if (j % 8 == 7)
            {
                spritedata[i] |= d << ((3 - xout) * 8);
                spritemask[i] |= m << ((3 - xout) * 8);
                m = 0;
                d = 0;
                xout++;
            }        
        }
    }    

    FILE * f = fopen(pars[2],"w");

    fprintf(f, "const unsigned char bubble[(%d + 1) * %d * 2 * 8] = {\n", x/8, y);
    int shift;
    for (shift = 0; shift < 8; shift++)
    {
        fprintf(f,"/* shift %d*/\n", shift);
        for (i = 0; i < y; i++)
        {
            for (j = 0; j < (x/8)+1; j++)
            {
                fprintbin(f, ~((spritemask[i]) >> shift) >> (8 * (3 - j)));
                fprintf(f, ", ");            
                fprintbin(f, (spritedata[i] >> shift) >> (8 * (3 - j)));
                fprintf(f, ", ");            
            }
            fprintf(f,"\n");
        }
    }
    fprintf(f," };\n");
    fclose(f);
    return 0;
}