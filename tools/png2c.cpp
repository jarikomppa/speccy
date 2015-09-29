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
    FILE * f = fopen("s.h","w");
    fprintf(f, "unsigned char s_png[%d] = {\n", (x * y) / 8);
    
    int i;
    int d = 0;
    int first = 1;
    for (i = 0; i < x*y; i++)
    {
        d <<= 1;
        d |= (p[i] & 0xffffff) == 0;
        if (i % 8 == 7)
        {
            fprintf(f,"%s %d", (!first) ? ",":" ", d);
            first = 0;
            d = 0;
        }
        if (i != 0 && i % (32*8) == 0) fprintf(f,"\n");
    }
    
    fprintf(f," };\n");
    fclose(f);
    return 0;
}