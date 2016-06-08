/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

#define ANYKEYX() ANYKEY()

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

#include <string.h>

unsigned char *data_ptr;
unsigned char *screen_ptr;

unsigned short framecounter;

#define HWIF_IMPLEMENTATION

#include "c0.h"
#include "c1.h"
#include "c2.h"
#include "c3.h"
#include "c4.h"
#include "c5.h"
#include "c6.h"
#include "c7.h"
#include "c8.h"
#include "yofstab.h"
#include "hwif.c"
#include "textout.c"
#include "sprites.c"

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

// returns values from 1 to 255 inclusive, period is 255
unsigned char y8;
unsigned char xorshift8(void) {
    y8 ^= (y8 << 7);
    y8 ^= (y8 >> 5);
    return y8 ^= (y8 << 3);
}

void setcolor(char x, char y, unsigned char c)
{
   *((char*)0x4000+192*32+(unsigned short)y*32+x) = c;
}

char * const minefield = (char*)0x7600;
char * const minefound = (char*)0x7628;
char infoloop = 0;
char first = 0;
char total = 0;
char found = 0;

static const unsigned char keyy[40] = 
{ 68, 68,  68,  68,  68,  68,  68,  68,  68,  68,  
  91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 
  114, 114,114,114,114,114,114,114,114,114,
  138,138,138,138,138,138,138,138,138,138
   };

static const unsigned char keyx[40] = 
{ 15, 37, 62, 84, 107, 131, 153, 177, 201, 226,
  24, 48, 73, 95, 119, 142, 164, 188, 212, 237,
  30, 52, 77, 99, 122, 146, 168, 192, 216, 241,
  13, 40, 64, 89, 112, 139, 161, 185, 209, 238};

void drawval(unsigned char c, unsigned char v)
{
    const unsigned char *data = 0;
    switch (v)
    {
        case 0:
            data = c0;
            break;
        case 1:
            data = c1;
            break;
        case 2:
            data = c2;
            break;
        case 3:
            data = c3;
            break;
        case 4:
            data = c4;
            break;
        case 5:
            data = c5;
            break;
        case 6:
            data = c6;
            break;
        case 7:
            data = c7;
            break;
        case 8:
            data = c8;
            break;    
    }
    if (data)
    drawsprite_16((char *)data, keyx[c]-8, keyy[c]-8);
}


void setup_mines()
{
    unsigned char i;
    total = 0;
    for (i = 0; i < 40; i++)
    {
        if (xorshift8() > 200)
        {
            minefield[i] = 1;            
        }
        else
        {
            minefield[i] = 0;
            total++;
        }
        minefound[i] = 0;
    }
}

char count_mines(char x)
{
    char total = 0;
    
    // <-
    if (x != 0 && x != 10 && x != 20 && x != 30)
        total += minefield[x - 1];
    // ->
    if (x != 9 && x != 19 && x != 29 && x != 39)
        total += minefield[x + 1];
    // ^
    if (x > 9)
        total += minefield[x - 10];
    // v
    if (x < 30)
        total += minefield[x + 10];
    
    // <^
    if (x > 9 && x != 10 && x != 20 && x != 30)  
        total += minefield[x - 11];
    // >^
    if (x > 9 && x != 19 && x != 29 && x != 39)
        total += minefield[x - 9];
    // <v
    if (x < 30 && x != 0 && x != 10 && x != 20)
        total += minefield[x + 9];
    // >v
    if (x < 30 && x != 9 && x != 19 && x != 29)
        total += minefield[x + 11];          
    
    return total;
}

void reset_game()
{
    unsigned char i;
    do_halt();
    do_port254(0);
    readkeyboard();
        
    cp((unsigned char*)0x4000, 6912, (char*)0x5b00);
    for (i = 0; i < 64; i++)
        setcolor(i,176/8,0x5);
    for (i = 0; i < 32; i++)
        setcolor(i,168/8,0x1);

    setup_mines();
    framecounter = 500;
    first = 1;
    found = 0;    
}

extern void playfx(unsigned short fx) __z88dk_fastcall; 

void do_key(char key)
{
    short i, j;
    while (ANYKEY())
    {
        readkeyboard();
        do_halt();
    }

    if (minefield[key])
    {
        // boom
        drawstring("Oops, you appear to have stepped", 0, 176);
        drawstring("on a rotten egg and died.       ", 0, 184);
        for (j = 0; j < 20; j++)
        {
            playfx(1);
            for (i = 0; i < 32*21; i++)
            {
                if (xorshift8() > 220)
                    *((char*)0x4000+192*32+i) = xorshift8() & 7;
            }
            for (i = 0; i < 51; i++) xorshift8();
            do_halt();
        }        
        reset_game();    
    }
    else
    {
        drawval(key, count_mines(key));
        if (minefound[key] == 0)
        {
            minefound[key] = 1;
            found++;
            if (found == total)
            {
                framecounter = 0;
                drawstring("Congratulations!! You did it!   ", 0, 176);
                drawstring("Step on a mine for a new game.  ", 0, 184);
            }
        }
        playfx(0);
    }    
}

void drawscore()
{
    char temp[6];
    char c;
    temp[2] = '/';
    temp[5] = 0;

    if (found < 10) temp[0] = ' '; else
    if (found < 20) temp[0] = '1'; else
    if (found < 30) temp[0] = '2'; else
        temp[0] = '3';

    c = found;
    while (c >= 10) c -= 10;
    temp[1] = '0' + c;


    if (total < 10) temp[3] = ' '; else
    if (total < 20) temp[3] = '1'; else
    if (total < 30) temp[3] = '2'; else
        temp[3] = '3';

    c = total;
    while (c >= 10) c -= 10;
    temp[4] = '0' + c;
    
    drawstring(temp, 32-5, 168);
}

void main()
{   
//    unsigned char i;         
    
    y8 = 1;
    
    reset_game();
    /*
    for (i = 0; i < 40; i++)
        if (minefield[i] == 0)
            drawval(i, count_mines(i));
   */
    
    while(1)
    {
        drawscore();
        readkeyboard(); 
        if (KEYDOWN(1)) do_key(0);
        if (KEYDOWN(2)) do_key(1);
        if (KEYDOWN(3)) do_key(2);
        if (KEYDOWN(4)) do_key(3);
        if (KEYDOWN(5)) do_key(4);
        if (KEYDOWN(6)) do_key(5);
        if (KEYDOWN(7)) do_key(6);
        if (KEYDOWN(8)) do_key(7);
        if (KEYDOWN(9)) do_key(8);
        if (KEYDOWN(0)) do_key(9);
        if (KEYDOWN(Q)) do_key(10);
        if (KEYDOWN(W)) do_key(11);
        if (KEYDOWN(E)) do_key(12);
        if (KEYDOWN(R)) do_key(13);
        if (KEYDOWN(T)) do_key(14);
        if (KEYDOWN(Y)) do_key(15);
        if (KEYDOWN(U)) do_key(16);
        if (KEYDOWN(I)) do_key(17);
        if (KEYDOWN(O)) do_key(18);
        if (KEYDOWN(P)) do_key(19);
        if (KEYDOWN(A)) do_key(20);
        if (KEYDOWN(S)) do_key(21);
        if (KEYDOWN(D)) do_key(22);
        if (KEYDOWN(F)) do_key(23);
        if (KEYDOWN(G)) do_key(24);
        if (KEYDOWN(H)) do_key(25);
        if (KEYDOWN(J)) do_key(26);
        if (KEYDOWN(K)) do_key(27);
        if (KEYDOWN(L)) do_key(28);
        if (KEYDOWN(ENTER)) do_key(29);
        if (KEYDOWN(SHIFT)) do_key(30);
        if (KEYDOWN(Z)) do_key(31);
        if (KEYDOWN(X)) do_key(32);
        if (KEYDOWN(C)) do_key(33);
        if (KEYDOWN(V)) do_key(34);
        if (KEYDOWN(B)) do_key(35);
        if (KEYDOWN(N)) do_key(36);
        if (KEYDOWN(M)) do_key(37);
        if (KEYDOWN(SYM)) do_key(38);
        if (KEYDOWN(SPACE)) do_key(39);

        framecounter++;
        do_halt();
        if (framecounter > 500)
        {
            framecounter = 0;
                     // 12345678901234567890123456789012
            switch (infoloop)
            {
            case 0:
                drawstring("Welcome to \"Rotten Egg Mines\"   ", 0, 176);
                drawstring("By Jari Komppa http://iki.fi/sol", 0, 184);
                break;
            case 1:
                drawstring("How to play:                    ", 0, 176);
                drawstring("Press keys to find mines.       ", 0, 184);
                break;
            case 2:
            case 4:
            case 6:
            case 8:
                drawstring(". . . . . . . . . . . . . . . . ", 0, 176);
                drawstring(" . . . . . . . . . . . . . . . .", 0, 184);
                break;   
            case 3:
                drawstring("Sound effects played via        ", 0, 176);
                drawstring("BeepFX by Shiru                 ", 0, 184);
                break;   
            case 5:
                drawstring("Art made by using the           ", 0, 176);
                drawstring("Image Spectrumizer              ", 0, 184);
                break;   
            case 7:
                drawstring("Sources can be found at         ", 0, 176);
                drawstring("github.com/jarikomppa/speccy    ", 0, 184);
                break;   
            case 12:
                drawstring("    [ . . . . . . . . . . .]    ", 0, 176);
                drawstring("    [. . . . . . . . . . . ]    ", 0, 184);
                break;         
            case 13:
                drawstring("        [ . . . . . . .]        ", 0, 176);
                drawstring("        [. . . . . . . ]        ", 0, 184);
                break;         
            case 14:
                drawstring("            [ . . .]            ", 0, 176);
                drawstring("            [. . . ]            ", 0, 184);
                break;         
            case 15:
                drawstring("                                ", 0, 176);
                drawstring("                                ", 0, 184);
                break;         
            }
            infoloop++;
            if (infoloop == 16)
                infoloop = 0;
        }
    }       
}
