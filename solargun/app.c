/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

#include <string.h>

unsigned char *data_ptr;
unsigned char *screen_ptr;

unsigned short framecounter;

unsigned char gamestate;
char inputscheme;

#define HWIF_IMPLEMENTATION

#include "yofstab.h"
#include "hwif.c"
#include "textout.c"

extern void playfx(unsigned short fx) __z88dk_fastcall;  

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

extern void initfbcopy();
extern void ingame();
extern void init_ingame();
extern void gameover();
extern void mainmenu();

void main()
{         
    do_halt();
    do_port254(0);
        
    cp((unsigned char*)0x4000, 32*64, (char*)0x5b00);
    cp((unsigned char*)0x4000+(32*192), 32*8, (char*)0x5b00+32*64);
    
    drawstring("Score:0000", 20, 56);
    drawstring("Gun charged", 2, 56);
             // 12345678901
    drawstring("***", 15, 56);
    
    //initmusic();
    initfbcopy();

    framecounter = 0;
    
    gamestate = 3;
    
    inputscheme = 0;
    
    while(1)
    {
        switch (gamestate)
        {
        case 0:
            init_ingame();
            gamestate = 1;
            break;
        case 1:             
            ingame();
            break;
        case 2:
            gameover();
            gamestate = 3;
            break;
        case 3:
            mainmenu();
            gamestate = 0;
            break;
        }    
        framecounter++;

    }       
}
