#pragma preproc_asm -
#include <string.h>

unsigned char fbcopy_idx;
unsigned char sin_idx;

#include "tab.h"
#include "s.h"
const unsigned char *bufp;
unsigned char *data_ptr;
unsigned char *screen_ptr;

// xxxsmbbb
// where b = border color, m is mic, s is speaker
void port254(const unsigned char color) __z88dk_fastcall
{
    color; // color is in l
    // Direct border color setting
    __asm
        ld a,l
        out (254),a
    __endasm;    
}

// practically waits for retrace
void do_halt()
{
    __asm
        halt
    __endasm;
}

// draw a single 8x8 character (x in 8 pixel steps, y in 1 pixel steps)
void drawchar(unsigned char c, unsigned char x, unsigned char y)
{
    data_ptr = (unsigned char*)(15616-32*8+c*8);
    screen_ptr = (unsigned char*)yofs[y]+x;
    __asm
        ld bc,(_screen_ptr)
        ld hl,(_data_ptr)
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    __endasm;
}

// draw string, x in 8 pixel steps, y in 1 pixel steps
void drawstring(unsigned char *t, unsigned char x, unsigned char y)
{
    screen_ptr = (unsigned char*)yofs[y]+x;
    while (*t)
    {
        data_ptr = (unsigned char*)(15616-32*8+*t*8);
        __asm
            ld bc,(_screen_ptr)
	        ld hl,(_data_ptr)
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        __endasm;
        x++;        
        screen_ptr++;
        t++;
    }
}

// copy N scanlines from linear memory to video memory
void fbcopy(const unsigned char * src, unsigned char start, unsigned char end)
{
    for (fbcopy_idx = start; fbcopy_idx < end; fbcopy_idx++, src+=32)
    {   
        memcpy((void*)yofs[fbcopy_idx], src, 32);
    }
}

// experimental: make some noise
void playtone(unsigned char delay)
{
    unsigned char d = delay;
    port254(0xff);
    while (d--);
    d = delay;
    port254(0);
    while (d--);
}

void main()
{       
    sin_idx = 0;
    while(1)
    {
        sin_idx++;
        bufp = s_png;
        bufp += sinofs[sin_idx];
        do_halt(); // halt waits for interrupt - or vertical retrace
    // delay loop to move the border into the frame (for profiling)
    for (fbcopy_idx = 0; fbcopy_idx < 145; fbcopy_idx++) port254(0);
    port254(2);
    drawstring("Hello Worldie!", 0, 8);
    port254(1);
    // can do about 64 scanlines in a frame (with nothing else)
    fbcopy(bufp, 64, 110);
    port254(0);
    }    
}
