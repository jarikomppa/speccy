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
#include "sprites.c"
#include "sprite.h"

void cp(unsigned char *dst, unsigned short len, unsigned char *src)  __z88dk_callee
{
       dst; len; src;
    // de   bc   hl
    __asm
    pop iy
    pop de
    pop bc
    pop hl
    push iy
	ldir
    __endasm;
    
}

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

void drawsprite_varbuf(unsigned char *sprite, unsigned short base, unsigned char x, unsigned char y)
{
    unsigned char i;
    unsigned short * yofs_c = (unsigned short*)(char*)yofs;
    for (i = 0; i < 16; i++)
        yofs_c[i+y] += base - 0x4000;

    drawsprite(sprite, x, y);

    for (i = 0; i < 16; i++)
        yofs_c[i+y] += 0x4000 - base;
}

void cp32(unsigned short dst, const unsigned short src)  __z88dk_callee
{
       dst; src;
    // de  hl
    __asm
    pop bc ; return address
    pop de ; dst
    pop hl ; src
    push bc ; put return address back
    
	ldi ; 32x [de++]=[hl++], bc--
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
    __endasm;    
}

unsigned short i1, i2, i3, i4, j1, j2, j3, j4;


void fx()
{
    unsigned short t;
    unsigned short o = 0;
    unsigned short xs, ys;
    switch (framecounter)
    {
    case 0: o = 0x5b00; break;
    case 1: o = 0x6b00; break;
    case 2: o = 0x7b00; break;
    case 3: o = 0x8b00; break;
    case 4: o = 0x9b00; framecounter = 0; break;
    }    
    
    xs = sintabx[(i1 >> 1) & 255] + 
         sintabx[(i2 >> 2) & 255] +
         sintabx[(i3 >> 2) & 255] +
         sintabx[(i4 >> 3) & 255];
    ys = sintab[(j1 >> 1) & 255] + 
         sintab[(j2 >> 2) & 255] +
         sintab[(j3 >> 2) & 255] +
         sintab[(j4 >> 3) & 255];
    drawsprite_varbuf(bubble,o, xs >> 2, ys >> 2);
    //sintabx[(i >> 1) & 255], sintab[(j >> 1) & 255]);
    i1 += 2;
    j1 += 3;
    i2 += 5;
    j2 += 7;
    i3 += 11;
    j3 += 13;
    i4 += 17;
    j4 += 19;
    
    if (i1 > 512) i1 -= 510;
    if (j1 > 512) j1 -= 509;
    if (i2 > 1024) i2 -= 1020;
    if (j2 > 1024) j2 -= 1021;
    if (i3 > 1024) i3 -= 1019;
    if (j3 > 1024) j3 -= 1023;    
    
    /*
    i += 3;
    j += 2;
    if (i > 512)
        i -= 509;
    if (j > 522)
        j -= 511;
        */
    //cp((unsigned char*)0x4000, (16*8)*32, (unsigned char*)(o));
    
    for (t = 0; t < (16*8)/* *32*/; t++)
    {
        cp32(0x4000+32*t, o+32*t);
     //   *(unsigned char*)(0x4000+t) = *(unsigned char*)(t+o);
    }
}

void main()
{           
    unsigned short  t;
    //unsigned char *dst;
    
    do_halt();

    for (t = 8*8; t < (16*8)/* *32*/; t++)
    {
        cp32(0x5b00+32*(t-8*8), 0x4000);
        cp32(0x6b00+32*(t-8*8), 0x4000);
        cp32(0x7b00+32*(t-8*8), 0x4000);
        cp32(0x8b00+32*(t-8*8), 0x4000);
        cp32(0x9b00+32*(t-8*8), 0x4000);

        cp32(0x5b00+32*t, 0x4000+32*t);
        cp32(0x6b00+32*t, 0x4000+32*t);
        cp32(0x7b00+32*t, 0x4000+32*t);
        cp32(0x8b00+32*t, 0x4000+32*t);
        cp32(0x9b00+32*t, 0x4000+32*t);
     //   *(unsigned char*)(0x4000+t) = *(unsigned char*)(t+o);
    }
    /*
    dst = (unsigned char*)(0x4000 + (32*192));
    for (i = 0; i < 32*24; i++)
        *dst++ = 7<<3;
      */  
    framecounter = 0;
    
    while(1)
    {
        framecounter++;
        //do_halt(); // halt waits for interrupt - or vertical retrace
       
        //port254(1);
        fx();
        //port254(0);                                     
    }    
}
