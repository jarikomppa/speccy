/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/
#include <stdio.h>
#include <stdlib.h>

int main(int parc, char ** pars)
{
    int i, j, k;
    FILE * f = fopen("smallscantab.h", "w");
    fprintf(f, "const unsigned char smallscan[64] = {\n");
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            int a = i;
            int b = j;
            if (a > b)
            {
                int c = a;
                a = b;
                b = c;
            }
            int res = 0;
            for (k = a; k <= b; k++)
            {
                res |= 1 << (7-k);
            }
            fprintf(f, "0x%02x%s", res, ((i*8+j)==63)?" ":",");
            fprintf(f, " // %d,%d - ", i, j);
            for (k = 0; k < 8; k++)
                fprintf(f, "%d", (res & (1 << (7-k))) != 0);
            fprintf(f, "\n");                    
        }                 
    }

    fprintf(f, "};\n");
    
    fprintf(f, "\n");
    
    fclose(f);
    
    return 0;
}
