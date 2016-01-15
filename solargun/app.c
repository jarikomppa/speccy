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
#include "bg.h"
#include "hwif.c"
#include "textout.c"
#include "fbcopy.c"
#include "scroller.c"
#include "ship.h"
#include "rock_16.h"
#include "sprites.c"
const unsigned char logo[] = {
#include "logo.h"
};

unsigned short seed = 0xACE1u;

unsigned short rand()
{
    seed = (seed * 7621) + 1;
    return seed;
}

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

struct Enemy
{
    unsigned char x;
    unsigned char y;
    char xi;
    char yi;
};

struct Enemy enemy[18];

void enemy_physics(char spriteofs)
{
    char i;
    char spriteidx = spriteofs * 6;
    for (i = 0; i < 6; i++, spriteidx++)
    {
        enemy[spriteidx].x += enemy[i].xi;
        if (enemy[spriteidx].x > (256 - 16)) enemy[spriteidx].x = 8;        
        if (enemy[spriteidx].x < 4) enemy[spriteidx].x = 240; //(256 - 16);

        enemy[spriteidx].y += enemy[spriteidx].yi;
        if (enemy[spriteidx].y < 64) enemy[spriteidx].y = 176;
        if (enemy[spriteidx].y > 176) enemy[spriteidx].y = 64;
    }
}


#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

void main()
{           
    unsigned char player_y = 100;
    unsigned char player_x = 10;
    unsigned short i;
    unsigned char ci;
    unsigned char *dst;//, *src;
    char bgi = 1;
    unsigned short bgo = 0;
    char spritemux = 0;
    
    do_halt();
    
    dst = (unsigned char*)(0x4000 + (32*192));// + 20*32);
    for (i = 0; i < 32*24; i++)
        *dst++ = 7;
    
    cp((unsigned char*)0x4000, 32*64, logo);
    cp((unsigned char*)0x4000+(32*192), 32*8, logo+32*64);
    
    for (i = 0; i < 18; i++)
    {
        enemy[i].x = (i * 117) & 255;        
        enemy[i].y = ((i * 17) & 127) + 64;        
        enemy[i].xi = -(((i & 7) + 1));
        enemy[i].yi = ((i * 3) & 7) - 4;
    }
    
    enemy_physics(0);
    enemy_physics(1);
    enemy_physics(2);
    
    //initmusic();
    initfbcopy();

    framecounter = 0;
    
//        lzf_unpack(test_scr_lzf, 0x4000);
    while(1)
    {
        char spriteofs = 0;
        framecounter++;
        do_halt(); // halt waits for interrupt - or vertical retrace

        // delay loop to move the border into the frame (for profiling)     
//        for (fbcopy_idx = 0; fbcopy_idx < 110; fbcopy_idx++) port254(0);

//        music();        

        
        port254(1);
        // can do about 64 scanlines in a frame (with nothing else)
        //fbcopy(s_png + sinofs[framecounter & 0xff] * 32, 64, 110);
        // Let's do interlaced copy instead =)
        fbcopy_i(bgo >> 2, (framecounter >> 3) & 31, 13);
        bgo += bgi;
        if (bgo == 0) bgi = 1;
        if (bgo == (126 << 1)) bgi = -1;
        port254(2);
//        scroller(160);
//        spritetest();

        spriteofs = 6 * spritemux;
        spritemux++;
        if (spritemux == 3) spritemux = 0;
        
        for (ci = 0; ci < 6; ci++, spriteofs++)
        {
            drawsprite_16(rock_16, enemy[spriteofs].x, enemy[spriteofs].y);
        }        

        drawsprite_16(ship, player_x, player_y);

        port254(1);
        
        enemy_physics(spritemux);
        
        readkeyboard();
        
        if (!KEYUP(Q)) player_y--;
        if (!KEYUP(A)) player_y++;
        if (!KEYUP(O)) player_x--;
        if (!KEYUP(P)) player_x++;
        /*
        if (!(keydata[KEYBYTE_Q] & KEYBIT_Q)) player_y--;
        if (!(keydata[KEYBYTE_A] & KEYBIT_A)) player_y++;
        */
        /*
        if ((keydata[KEYBYTE_Q] & KEYBIT_Q) != KEYBIT_Q) player_y--;
        if ((keydata[KEYBYTE_A] & 0x1f) != 0x1f) player_y++;
        */
        port254(0);                                     
    }    
}
