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

unsigned char *s_png = (unsigned char *)(0xffff - 32 * 192 * 2); //[32*192*2];

unsigned short tone1 = 0;
unsigned short tone2 = 0;
unsigned char nexttone = 0;
unsigned char keeptone = 0;
unsigned short songidx;

void cp(unsigned char *dst, unsigned short len, unsigned char *src)
{
    /*
    while (len)
    {
        *dst = *src;
        dst++;
        src++;
        len--;
    }
    */
    
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

void lzf_unpack(unsigned char *src, unsigned short len, unsigned char *dst)
{
    unsigned short idx = 0;
    while (idx < len)
    {
        unsigned char op = src[idx];
        unsigned short runlen = op >> 5;
        port254(runlen);
        idx++;
        if (runlen == 0)
        {
            // literals
            runlen = (op & 31) + 1;
            
            cp(dst,runlen, src+idx);
            dst += runlen;
            idx += runlen;                      
        }
        else
        {
            // run
            unsigned short ofs = ((op & 31) << 8) | src[idx];
            unsigned char * runsrc;
            idx++;
            if (runlen == 7)
            {
                // long run
                runlen = src[idx] + 7;
                idx++;
            }
            runlen += 2;
            runsrc = dst - ofs - 1;
            cp(dst, runlen, runsrc);
            dst += runlen;
        }            
    }
}

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

void decrunch()
{
    memset(0x4000, 0, 32*192);
    memset(0x5800, COLOR(0,1,0,2), 32*24);
    drawstring("Unpacking..", 0, 0);
    lzf_unpack(png_lzf, png_lzf_len, s_png);
    memset(0x4000, 0, 32*192);
    memset(0x5800, COLOR(0,0,7,0), 32*24);
}

void main()
{           
    unsigned short i;
    s_png = (unsigned char *)(0xffff - 32 * 192 * 2);
    decrunch();
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
