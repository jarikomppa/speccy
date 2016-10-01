/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org
 * (practically public domain)
*/

#include <string.h>

unsigned char fbcopy_idx;

unsigned char *data_ptr;
unsigned char *screen_ptr;

unsigned char port254tonebit;
unsigned short framecounter = 0;

#include "yofstab.h"
#include "hwif.c"
#include "test.h"

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

void drawstring(unsigned char *aS, unsigned char aX, unsigned char aY)
{
    unsigned char i, *s, *d, sx, x;
    unsigned char *datap = (unsigned char*)(int*)test_data - 32*8;
    unsigned char *widthp = (unsigned char*)(int*)test_width - 32;
    for (i = 0; i < 8; i++)
    {
        s = aS;
        sx = 0;
        x = aX;
        d = (unsigned char*)yofs[aY];
        while (*s)
        {
            unsigned char data = datap[*s * 8];
            unsigned char width = widthp[*s];
            *d |= data >> sx;
            sx += width;
            if (sx > 8)
            {
                d++;
                sx -= 8;
                *d |= data << (width - sx);
            }
            s++;
        }
        aY++;
        datap++;
    }
}

void main()
{
    unsigned short i;
    do_halt();

    for (i = 0; i < 192*32; i++)
        *(unsigned char*)(0x4000+i) = 0;
    for (i = 0; i < 24*32; i++)
        *(unsigned char*)(0x5800+i) = (i&1)?7:7+8;


    framecounter = 0;

    while(1)
    {
        framecounter++;
        do_halt(); // halt waits for interrupt - or vertical retrace
        for (i = 0; i < 600; i++);

        port254(1);
        drawstring("Test", 0, 0);
        //drawstring("Quick Brown Fox Jumped Over The Lazy Dog.", 0, 0);
        //drawstring_dumb("What a fixed world it is!", 0, 9);
        port254(0);
    }
}