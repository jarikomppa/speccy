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
#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

#include "yofstab.h"
#include "bg.h"
#include "hwif.c"
#include "textout.c"
#include "fbcopy.c"
#include "scroller.c"
#include "ship.h"
#include "rock_16.h"
#include "sprites.c"

extern void playfx(unsigned short fx) __z88dk_fastcall;  

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
    char life;
};

struct Enemy enemy[18];
unsigned char player_y;
unsigned char player_x;
unsigned char playsound;

char needclean = 16;
void attribclean()
{
    unsigned short i;    
    char o;
    if (needclean == 0)
        return;
    needclean--;
    o = (framecounter * 3) & 15;
    for (i = 0; i < 32*16; i += 16)
        *((unsigned char*)0x4000+192*32+ 8*32+i + o) = 7;    
}

void attribline(char x, char y, unsigned char c)
{
    unsigned short i;
    char d = 32 - x;
    i = 0x4000+192*32+ 7*32 + y * 32 + x;
    while (d)
    {
        *((unsigned char*)i) = c;
        d--;
        i++;
    }
    needclean = 16;
}

void attribmess(char x, char y, char c)
{
    unsigned short i;
    i = 0x4000+192*32+ 8*32 + y * 32 + x;

    *((unsigned char*)i) = c;    
    *((unsigned char*)i + 1) = c;    
    *((unsigned char*)i + 2) = c;    

    *((unsigned char*)i + 31) = c;    
    *((unsigned char*)i + 32) = c;    
    *((unsigned char*)i + 33) = c;    
    *((unsigned char*)i + 34) = c;    
    *((unsigned char*)i + 35) = c;    

    *((unsigned char*)i + 64) = c;    
    *((unsigned char*)i + 65) = c;    
    *((unsigned char*)i + 66) = c;    

    needclean = 16;
}

void kill(char i)
{
    enemy[i].life = 0;
    attribmess(enemy[i].x/8,enemy[i].y/8,COLOR(0,1,0,2));
    playsound = 2;
}

void kill_enemies(unsigned char y)
{
    char i;
    for (i = 0; i < 18; i++)
    {
        if (enemy[i].life)
        {
            if (enemy[i].x > player_x/2 &&
                enemy[i].y / 16 == y)
            {
                kill(i);
            }
        }
    }
}
void enemy_physics(char spriteofs)
{
    char i;
    char spriteidx = spriteofs * 6;
    for (i = 0; i < 6; i++, spriteidx++)
    {
        if (enemy[spriteidx].life)
        {
            unsigned char xe = enemy[spriteidx].x;
            unsigned char ye = enemy[spriteidx].y;
            
            xe += enemy[i].xi;
            if (xe > (256 - 16)) xe = 8;        
            if (xe < 4) xe = 240; //(256 - 16);
    
            ye += enemy[spriteidx].yi;
            if (ye < 8) enemy[spriteidx].yi = -enemy[spriteidx].yi;//176;
            if (ye > 104) enemy[spriteidx].yi = -enemy[spriteidx].yi;//64;
                
            enemy[spriteidx].x = xe;
            enemy[spriteidx].y = ye;
                
            if (enemy[spriteidx].x < 128)
            {
                
/*

 x----+
 |    |
 |  a----+
 |  | |  |
 +--|-y  |
    |    |
    +----+

*/                
                char x = player_x / 2 + 4;
                char y = player_y / 2 + 4;

                if (x + 8 > xe &&
                    y + 8 > ye &&
                    x < xe + 16 &&
                    y < ye + 16)
                {
                    kill(spriteidx);
                }
            }
        }               
    }
}


void main()
{         
    char recharge = 0;  
    char player_xm = 0;
    char player_ym = 0;
    unsigned short i;
    unsigned char ci;
    unsigned char *dst;//, *src;
    char bgi = 1;
    unsigned short bgo = 0;
    char spritemux = 0;
    player_y = 10;
    player_x = 10;
    playsound = 0;

    do_halt();
    
    dst = (unsigned char*)(0x4000 + (32*192));// + 20*32);
    for (i = 0; i < 32*24; i++)
        *dst++ = 7;
    
    cp((unsigned char*)0x4000, 32*64, (char*)0x5b00);
    cp((unsigned char*)0x4000+(32*192), 32*8, (char*)0x5b00+32*64);
    
    for (i = 0; i < 18; i++)
    {
        enemy[i].x = (i * 117) & 255;        
        enemy[i].y = ((i * 17) & 127);        
        if (enemy[i].y > 104) enemy[i].y -= 60;
        if (enemy[i].y < 8) enemy[i].y += 60;
        enemy[i].xi = -(((i & 7) + 1));
        enemy[i].yi = ((i * 3) & 7) - 4;
        enemy[i].life = 1;
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
        i = ((player_y) + (bgo << 1)) / 16;
        fbcopy_i(i, (framecounter >> 3) & 31, 13);
            
        bgo += bgi;
        if (bgo == 0) bgi = 1;
        if (bgo == (126 << 1)) bgi = -1;
        port254(2);

        spriteofs = 6 * spritemux;
        spritemux++;
        if (spritemux == 3) spritemux = 0;

        if (playsound)
        {
            playfx(playsound - 1);
        }
        else
        {
            spriteofs = 6 * spritemux;
            spritemux++;
            if (spritemux == 3) spritemux = 0;
            
            for (ci = 0; ci < 6; ci++, spriteofs++)
            {
                if (enemy[spriteofs].life != 0)
                {
                    drawsprite_16(rock_16, enemy[spriteofs].x, enemy[spriteofs].y + 64);
                }
            }        
        }
        
        playsound = 0;

        drawsprite_16(ship, player_x / 2, player_y / 2 + 64);
    
        port254(1);
        
        enemy_physics(spritemux);
        
        readkeyboard();
        
        if (!KEYUP(Q)) player_ym -= 5;
        if (!KEYUP(A)) player_ym += 5;
        if (!KEYUP(O)) player_xm -= 5;
        if (!KEYUP(P)) player_xm += 5;
       
        player_xm = (player_xm * 3) / 4;
        player_ym = (player_ym * 3) / 4;
                        
        player_x += player_xm / 2;
        player_y += player_ym / 2;
        
        if (player_x < 8) 
            {
                player_x = 8;
//                player_xm = -player_xm;
            }
        if (player_x > 240) 
            {
                player_x = 240;
//                player_xm = -player_xm;
                
            }
        if (player_y < 8) 
            {
                player_y = 8;
//                player_ym = -player_ym;
            }
        if (player_y > 208) 
            {
                player_y = 208;
//                player_ym = -player_ym;
            }

        if (recharge == 25 && !KEYUP(SPACE))
        {
            kill_enemies(player_y/32);
            attribline(player_x/16+2,player_y/16+2,COLOR(1,1,6,2));
            recharge = 0;
            playsound = 1;
        }
        
        if (recharge < 25 && KEYUP(SPACE))
        {
            recharge++;
        }

        port254(5);
        attribclean();
        port254(0);                                             
    }    
}
