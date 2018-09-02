// Small functions that don't seem to fit anywhere else..

#define HWIF_IMPLEMENTATION
#include "main.h"

void strcat(char *tgt, char *src)
{
    while (*tgt)
        tgt++;
    while (*src)
    {
        *tgt = *src;
        tgt++;
        src++;
    }
    *tgt = 0;
}

unsigned char xorshift8(void) 
{
    y8 ^= (y8 << 7);
    y8 ^= (y8 >> 5);
    return y8 ^= (y8 << 3);
}

void int2str(unsigned char val, char s[4])
{
    if (val > 99)
    {
        s[0] = '0';
        while (val > 99)
        {
            s[0]++;
            val -= 100; 
        }
        s[1] = '0';
        while (val > 9)
        {
            s[1]++;
            val -= 10; 
        }
        s[2] = '0' + val;
        s[3] = 0;
    }
    else
    if (val > 9)
    {
        s[0] = '0';
        while (val > 9)
        {
            s[0]++;
            val -= 10; 
        }
        s[1] = '0' + val;
        s[2] = 0;
    }
    else
    {
        s[0] = '0' + val;
        s[1] = 0;
    }
}
