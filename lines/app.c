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
#include "sintab.h"
#include "sintabx.h"
#include "hwif.c"

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

void cls(unsigned char color) __z88dk_fastcall
{
    color; // color (should be) in l
    __asm
        ld a, l
        ld (0x6000), a
        ld hl, #0x6000
        ld de, #0x6001
        ld bc, #0x17ff
        ldir
    __endasm;
}

void flip()
{
    __asm
        ld hl, #0x6000
        ld de, #0x4000
        ld bc, #0x1800
        ldir        
    __endasm;
}

unsigned short div16_8(unsigned short a, unsigned short b) __z88dk_callee __naked
{
    a; b;
    __asm
	   pop bc
	   pop hl
	   pop de
	   push bc
	   
 	   xor a
 	   ld d, a
 	   ld b, #16
 	
 	loop_8_0:
 	
 	   add hl,hl
 	   rla
 	   jr c, loop_8_2
 	   
 	   cp e
 	   jr c, loop_8_1
 	
 	loop_8_2:
 	
 	   sub e
 	   inc l
 	
 	loop_8_1:
 	
 	   djnz loop_8_0
 	
 	   ret
 	
    __endasm;
} 	

/*
unsigned short div16_8(unsigned short a, unsigned short b) __z88dk_callee __naked
{
//        div_hl_c:
    a; b;
    __asm
        pop iy
        pop hl
        pop bc
        push iy
        ld de, #0
_loop:
        inc de
        sbc hl, bc
        jp nc,_loop
        ld h, d
        ld l, e
        ret        
    __endasm;
}
*/
unsigned short div16_8_c(unsigned short a, unsigned short b) 
{
    unsigned short r = 0;
    while (a >= b)
    {
        a -= b;
        r++;
    }
    return r;
}


void drawline(unsigned short x1,
              unsigned short y1, 
              unsigned short x2, 
              unsigned short y2) 
{       
    unsigned short x, y;
    unsigned short xinc;
    unsigned short yinc;    
    unsigned short len, i;
    static const unsigned char bits[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    
    len = x2 - x1;
    i = y2 - y1;
    if (i > len) len = i;
    if (len == 0) return;

    xinc = div16_8(((x2 - x1) << 8), len);
    yinc = div16_8(((y2 - y1) << 8), len);

    x = (x1 << 8) + ((1 << 8) / 2); 
    y = (y1 << 8) + ((1 << 8) / 2);

    for (i = 1; i <= len; i++) 
    {
        unsigned char xp = x >> 8;
        *(unsigned char*)(0x2000 + yofs[(y >> 8)] + (xp)/8) |= bits[xp & 7];
        //*(unsigned char*)(0x1b00 + yofs[y >> 8] + i/8) = 0xff;
            
        x = x + xinc;
        y = y + yinc;
    }
}

void fx()
{
    drawline(0,0,255,191);
    drawline(10,10,20,10);
    drawline(40,40,40,50);
    drawline(60,60,70,70);
    drawline(90,90,100,110);
    drawline(120,120,140,130);
    
    drawline(0,0,framecounter & 255, 100);
    drawline(0,0,100, framecounter & 127);
    
    flip();
    cls(0);
}

void main()
{
    unsigned short i;
    unsigned char *dst;

    do_halt();

    dst = (unsigned char*)(0x4000 + (32*192));
    for (i = 0; i < 32*24; i++)
        *dst++ = 7<<3;

    framecounter = 0;

    while(1)
    {
        framecounter++;
        do_halt(); // halt waits for interrupt - or vertical retrace

        port254(1);
        fx();
        port254(0);
    }
}
