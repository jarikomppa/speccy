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
#include <string.h>
#include "../common/lzfpack.h"


int main(int parc, char ** pars)
{
    LZFPack p;
    if (parc < 3)
    {
        printf("Usage: %s infile outfile\n", pars[0]);
        exit(0);
    }
    FILE * f = fopen(pars[1], "rb");
    if (!f)
    {
        printf("\"%s\" not found\n", pars[1]);
        exit(0);
    }
    fseek(f,0,SEEK_END);
    int len = ftell(f);
    fseek(f,0,SEEK_SET);
    printf("%s - %d bytes\n", pars[1], len);
    unsigned char * data = new unsigned char[len];
    fread(data, len, 1, f);
    fclose(f);

    p.pack(data, len);

    f = fopen(pars[2], "wb");
    if (!f)
    {
        printf("Can't open \"%s\"\n", pars[2]);
        exit(0);
    }
    
    fwrite(p.mPackedData, 1, p.mMax, f);
    int dataout = ftell(f);
       
    printf("%s - %d bytes\n", pars[2], dataout);   
    fclose(f);
    return 0;
}
