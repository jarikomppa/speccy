#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int parc, char ** pars)
{
    int i;
    FILE * f = fopen("tonetab.h", "w");

    fprintf(f, "const unsigned short tonetab[80] = {\n");
    for (i = 0; i < 80; i++)
    {
        fprintf(f, "0x%04d%s", (unsigned short)floor(pow(pow(2,1/12.0f),i+80)), (i==79)?"":",");
        if (i % 8 == 7)
            fprintf(f, "\n");
    }
    fprintf(f, "};\n");
    fclose(f);
    
    return 0;
}