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
0,0
};

// experimental: make some noise
void playtone(unsigned short delay) __z88dk_fastcall
{  
    delay;  // delay in hl
    port254tonebit |= 5;
        
    // A BC DE HL
    __asm
    
    ld a, (_port254tonebit) // port254
	ld de, #0
	ex de, hl
	ld b, #0xff 
	ld c, #16
	
	// de - increment value
	// b  - audio cycles
	// c  - value 16
	// a - port 254 data
	// hl - accumulator
loop:

	add hl, de
	jr	C, delayends
    jp out254 
delayends:	
	xor	a, c // 4 clocks
out254:   
	out (254), a
	dec	b
	jr	NZ, loop

    ld (_port254tonebit), a    
    
    __endasm;    
    
    port254tonebit &= ~5;
    port254(0);    
}



unsigned short seed = 0xACE1u;

unsigned short rand()
{
    seed = (seed * 7621) + 1;
    return seed;
}

unsigned short tone1 = 0;
unsigned short tone2 = 0;
unsigned char nexttone = 0;
unsigned char keeptone = 0;
unsigned short songidx;  
void main()
{           
    unsigned short i;
    port254tonebit = 0;
    for (i = 0; i < 256; i++)
    {
        unsigned char v = i * 13;
        if (v >= 192) v -= 192;
        if (v >= 160 && v <= 168) v -= 100;
        fbcopy_i_idxtab[i] = v;
    }
    sin_idx = 0;
    songidx = 1;
    
    while(1)
    {
        sin_idx++;
        bufp = s_png;
        bufp += sinofs[sin_idx];        
        do_halt(); // halt waits for interrupt - or vertical retrace

        // delay loop to move the border into the frame (for profiling)     
//        for (fbcopy_idx = 0; fbcopy_idx < 110; fbcopy_idx++) port254(0);

        //if (sin_idx & 1)
            playtone(tone1);
        //else
        //    playtone(tone2);

        if (!keeptone)
        {
            //if (nexttone)
            //    tone2 = tonetab[musicdata[songidx++]];
            //else
                tone1 = tonetab[musicdata[songidx++]];
            nexttone = !nexttone;
            keeptone = musicdata[songidx++];
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
