/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

#include <string.h>

unsigned char fbcopy_idx;
unsigned char sin_idx;

const unsigned char *bufp;
unsigned char *data_ptr;
unsigned char *screen_ptr;
unsigned char port254tonebit;

#include "data.c"
#include "hwif.c"
#include "textout.c"
#include "fbcopy.c"
#include "scroller.c"

const unsigned char musicdata[] = {
#include "tune.h"
20,0,0,0,0,0
};

extern void playtone(unsigned short delay) __z88dk_fastcall;

unsigned short tone1adder;
unsigned short tone2adder;

unsigned short seed = 0xACE1u;

unsigned short rand()
{
    seed = (seed * 7621) + 1;
    return seed;
}

//unsigned char *s_png = (unsigned char *)(0xffff - 32 * 192 * 2); //[32*192*2];

unsigned char nexttone = 0;
unsigned char keeptone = 0;
unsigned short songidx;

void cp(unsigned char *dst, unsigned short len, unsigned char *src)
{
       dst; len; src;
    // de   bc   hl
    __asm
	ld	hl, #2
	add	hl, sp
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	inc hl
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	inc hl	
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	ldir
    __endasm;
    
}

extern void lzf_unpack(unsigned char *src, unsigned short len, unsigned char *dst);

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

/*void decrunch()
{
    memset((void*)0x4000, 0, 32*192);
    memset((void*)0x5800, COLOR(0,1,0,2), 32*24);
    drawstring("Unpacking..", 0, 0);
    lzf_unpack(png_lzf, png_lzf_len, s_png);
    memset((void*)0x4000, 0, 32*192);
    memset((void*)0x5800, COLOR(0,0,7,0), 32*24);
}
*/

void main()
{           
    unsigned short i;
    //s_png = (unsigned char *)(0xffff - 32 * 192 * 2);
    //decrunch();
    port254tonebit = 0;
    for (i = 0; i < 256; i++)
    {
        unsigned char v = i * 13;
        if (v >= 192) v -= 192;
        if (v >= 160 && v <= 168) v -= 100;
        fbcopy_i_idxtab[i] = v;
    }
    sin_idx = 0;
    songidx = 0;
    
    while(1)
    {
        sin_idx++;
        bufp = s_png;
        bufp += sinofs[sin_idx];        
        do_halt(); // halt waits for interrupt - or vertical retrace

        // delay loop to move the border into the frame (for profiling)     
//        for (fbcopy_idx = 0; fbcopy_idx < 110; fbcopy_idx++) port254(0);

        port254tonebit |= 5;
        if (sin_idx & 1)
        {
            if (tone1adder)
                playtone(tone1adder);            
            else
                playtone(tone2adder);            
        }
        else
        {
            if (tone2adder)
                playtone(tone2adder);            
            else
                playtone(tone1adder);                            
        }
        
        port254tonebit &= ~5;
        port254(0);    

        if (!keeptone)
        {
            unsigned char note;
            unsigned char channel;
            keeptone = musicdata[songidx++];
            note = musicdata[songidx++];
            channel = musicdata[songidx++];
            if (channel == 0)
                tone1adder = tonetab[note];
            else
                tone2adder = tonetab[note];
            if (keeptone == 0)
            {
                songidx = 0;
                keeptone = 1;
            }
        }
        keeptone--;
        port254(2);
        scroller(160);
        port254(1);
        // can do about 64 scanlines in a frame (with nothing else)
        //fbcopy(bufp, 64, 110);
        // Let's do interlaced copy instead =)
        fbcopy_i(bufp, 13);
        port254(0);                                     
    }    
}
