#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

int main(int parc, char ** pars)
{
    int i;
    FILE * f = fopen("tab.h", "w");
    fprintf(f, "unsigned short yofs[192] = {\n");
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
    
    fprintf(f, "unsigned short sinofs[256] = {\n");
    for (i = 0; i < 256; i++)
    {
        fprintf(f, "%4d%s", (int)floor(((sin(i * 2 * M_PI / 256.0f)+1)/2) * 192) * 32, (i==255)?"":",");
        if (i % 16 == 15)
            fprintf(f, "\n");
    }
    fprintf(f, "};\n");
    fclose(f);
    
    return 0;
}