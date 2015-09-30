#include <string.h>

unsigned char fbcopy_idx;
unsigned char sin_idx;

const unsigned char *bufp;
unsigned char *data_ptr;
unsigned char *screen_ptr;

#include "data.c"
#include "hwif.c"
#include "textout.c"
#include "fbcopy.c"

// experimental: make some noise
void playtone(unsigned char delay)
{
    unsigned char d = delay;
    port254(0xff);
    while (d--);
    d = delay;
    port254(0);
    while (d--);
    d = delay;
    port254(0xff);
    while (d--);
    port254(0);
}

unsigned short seed = 0xACE1u;

unsigned char rand()
{
    seed = (seed * 7621) + 1;
    return seed;
}

unsigned char tone = 0;
unsigned char keeptone = 0;
  
void main()
{       
    unsigned short i;
    for (i = 0; i < 256; i++)
    {
        unsigned char v = i * 13;
        if (v >= 192) v -= 192;        
        fbcopy_i_idxtab[i] = v;
    }
    sin_idx = 0;
    while(1)
    {
        sin_idx++;
        bufp = s_png;
        bufp += sinofs[sin_idx];
        do_halt(); // halt waits for interrupt - or vertical retrace

        // delay loop to move the border into the frame (for profiling)     
        //for (fbcopy_idx = 0; fbcopy_idx < 145; fbcopy_idx++) port254(0);
    
        port254(1);
        // can do about 64 scanlines in a frame (with nothing else)
        //fbcopy(bufp, 64, 110);
        // Let's do interlaced copy instead =)
        fbcopy_i(bufp, 23);
        port254(2);
        drawstring("http://iki.fi/sol", 8, 160);
        port254(0);
        
        // random jazz generator:
        if (!keeptone)
        {
            tone = rand();
            keeptone = 5;
        }
        keeptone--;
        playtone(tone);
    }    
}
