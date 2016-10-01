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

void drawstring_dumb(unsigned char *s, unsigned char x, unsigned char y)
{
    unsigned char i;
    while (*s)
    {
        for (i = 0; i < 8; i++)
        {
            *(unsigned char *)(yofs[y+i]+x) |= test_data[(*s-32)*8+i];
        }
        s++;
        x++;
    }
}


void drawstring(unsigned char *s, unsigned char x, unsigned char y)
{
    unsigned char i;
    char sx = 0;
    while (*s)
    {
        for (i = 0; i < 8; i++)
        {
            *(unsigned char *)(yofs[y+i]+x) |= test_data[(*s-32)*8+i] >> sx;                        
        }
        sx += test_width[*s-32];
        if (sx > 8)
        {
            x++;
            sx -= 8;
            for (i = 0; i < 8; i++)
            {
                *(unsigned char *)(yofs[y+i]+x) |= test_data[(*s-32)*8+i] << (test_width[*s-32]-sx);
            }
        }
        s++;
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

        port254(1);
        drawstring("What a proportional world it is!", 0, 0);
        drawstring_dumb("What a fixed world it is!", 0, 9);
        port254(0);
    }
}
