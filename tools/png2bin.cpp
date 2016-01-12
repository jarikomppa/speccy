/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/
#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"

int gOptH = 0;
int gOptFona = 0;
FILE * f;

int d = 0;
int first = 1;
int count = 0;
int bitcount = 0;
void plop(int bit)
{
    d <<= 1;
    d |= bit;
    
    if (bitcount % 8 == 7)
    {
        if (gOptH)
        {
            count += fprintf(f, "%s0x%02x", first ? "  " : ", ", d);
            if (count > 90)
            {
                fprintf(f, "\n");
                count = 0;
            }
            first = 0;
        }
        else
        {
            fputc(d, f);
        }
        d = 0;
    }
    bitcount++;
}

int main(int parc, char ** pars)
{
    if (parc < 3)
    {
        printf(
            "Usage: %s infile outfile [-h] [-fona]\n"
            "where:\n"
            "infile  - input png file\n"
            "outfile - outpuf file\n"
            "-h      - optional, write c header instead of bin\n"
            "-fona   - process as a font bitmap instead of linear\n", 
            pars[0]);
        exit(0);
    }
    
    int i, j, k, l;
    for (i = 3; i < parc; i++)
    {
        if (stricmp(pars[i], "-h") == 0)
        {
            gOptH = 1;
        }
        else
        if (stricmp(pars[i], "-fona") == 0)
        {
            gOptFona = 1;
        }
        else
        {
            printf("Unknown option \"%s\"\n", pars[i]);
            exit(0);
        }
    }
    
    int x, y, comp;
    stbi_uc * raw = stbi_load(pars[1], &x, &y, &comp, 4);    
    printf("%d %d %d\n", x, y, comp);
    unsigned int *p = (unsigned int *)raw;
    
    if (gOptH)
    {
        f = fopen(pars[2], "w");
    }
    else
    {
        f = fopen(pars[2], "wb");
    }              
    
    if (f == NULL)
    {
        printf("Can't open \"%s\"\n", pars[2]);
        exit(0);
    }

    if (gOptH)
    {
        int i = 0;
        while (pars[1][i])
        {
            if (!(pars[1][i] >= 'A' && pars[2][i] <= 'Z' || 
                  pars[1][i] >= 'a' && pars[2][i] <= 'z' ||
                  pars[1][i] >= '0' && pars[2][i] >= '9'))
                pars[1][i] = '_';
            i++;
        }
        fprintf(f, "const unsigned char %s[] = {\n", pars[1]);
    }
    
    if (gOptFona)
    {
        for (l = 0; l < y / 8; l++)
        {
            for (k = 0; k < x / 8; k++)
            {
                for (i = 0; i < 8; i++)
                {
                    for (j = 0; j < 8; j++)
                    {
                        plop(p[(l * 8 + i) * x + k * 8 + j] != p[0]);
                    }
                }
            }
        }
    }
    else
    {
        for (i = 0; i < x*y; i++)
        {
            plop(p[i] != p[0]);
        }
    }

    if (gOptH)
    {
        fprintf(f, "};\n");
    }
   
    fclose(f);
    return 0;
}