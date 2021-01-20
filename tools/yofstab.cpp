/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

/* Make Visual Studio not consider fopen() deprecated */
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>

int main(int parc, char ** pars)
{
    int i;
    FILE * f = fopen("yofstab.h", "w");
    fprintf(f, "const unsigned short yofs[192] = {\n");
    for (i = 0; i < 192; i++)
    {
        fprintf(f, "0x%4x%s",
            ((((i>>0)&7)<< 3) | (((i >> 3)&7) <<0) | ((i >> 6) & 3) << 6) * 32 + 0x4000,
            (i == 191)?"":",");
        if (i % 10 == 9)
            fprintf(f, "\n");
    }

    fprintf(f, "};\n");
    
    fprintf(f, "\n");
    
    fclose(f);
    
    return 0;
}
