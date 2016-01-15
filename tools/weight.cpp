/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int weight(unsigned char c, int *countbit)
{
    int i;
    int w = 0;
    for (i = 0; i < 8; i++)    
        if (c & (1 << i)) 
        {
            w++;
            countbit[i]++;
        }
    return w;
}

int main(int parc, char ** pars)
{
    if (parc < 2)
    {
        printf("Usage: %s infile\n", pars[0]);
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
    
    unsigned char * data = new unsigned char[len];
    fread(data, len, 1, f);
    fclose(f);
    
    int i;
    int count = 0;
    int countbit[8] = {0,0,0,0,0,0,0,0};
    for (i = 0; i < len; i++)
        count += weight(data[i], countbit);
    printf("Weight: %d/%d (%3.3f%%) - %3.3f seconds\n"
           "[%7d, %7d, %7d, %7d, %7d, %7d, %7d, %7d] / %5d"
           "[%6.3f%%,%7.3f%%,%7.3f%%,%7.3f%%,%7.3f%%,%7.3f%%,%7.3f%%,%7.3f%%]\n",
        count, 
        len*8, 
        ((float)count*100) / (len*8), 
        (count * 977 + ((len * 8)-count) * 489) / (1000 * 1000.0f),
        countbit[0], countbit[1], countbit[2], countbit[3], countbit[4], countbit[5], countbit[6], countbit[7], len,
        ((float)countbit[0]*100)/len,
        ((float)countbit[1]*100)/len,
        ((float)countbit[2]*100)/len,
        ((float)countbit[3]*100)/len,
        ((float)countbit[4]*100)/len,
        ((float)countbit[5]*100)/len,
        ((float)countbit[6]*100)/len,
        ((float)countbit[7]*100)/len);

    return 0;
}    