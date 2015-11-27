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

#include "data.c"
#include "hwif.c"
#include "textout.c"
#include "fbcopy.c"
#include "scroller.c"
//#include "sprites.c"
//#include "music.c"


unsigned short seed = 0xACE1u;

unsigned short rand()
{
    seed = (seed * 7621) + 1;
    return seed;
}


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

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))


void main()
{           
    unsigned short i;
    
    //initmusic();
    initfbcopy();

    framecounter = 0;
    
    while(1)
    {
        framecounter++;
        do_halt(); // halt waits for interrupt - or vertical retrace

        // delay loop to move the border into the frame (for profiling)     
//        for (fbcopy_idx = 0; fbcopy_idx < 110; fbcopy_idx++) port254(0);

//        music();        

        port254(2);
        scroller(160);
        //spritetest();
        port254(1);
        
        // can do about 64 scanlines in a frame (with nothing else)
        //fbcopy(bufp, 64, 110);
        // Let's do interlaced copy instead =)
        fbcopy_i(sinofs[framecounter & 0xff], 13);
        port254(0);                                     
    }    
}
