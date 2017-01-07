/* * Part of Jari Komppa's zx spectrum suite * 
https://github.com/jarikomppa/speccy * released under the unlicense, see 
http://unlicense.org * (practically public domain) */

#include <string.h>

unsigned char *data_ptr;
unsigned char *screen_ptr;

unsigned char port254tonebit;
unsigned short framecounter = 0;
#include "yofstab.h"
#include "crate_16_16_1.h"
#include "crate_16_16_2.h"
#include "crate_16_16_3.h"
#include "crate_16_32_1.h"
#include "crate_16_32_2.h"
#include "crate_32_16_1.h"
#include "crate_32_16_2.h"
#include "crate_32_32_1.h"
#include "crate_32_32_2.h"
#include "door.h"
#include "key.h"
#include "player.h"
#include "paper.h"
const unsigned char logo_small[] = {
#include "logo_small.h"
};
const unsigned char leveldata[] = {
//#include "leveldata.h"
#include "goodlevels.h"
};
#define HWIF_IMPLEMENTATION
#include "hwif.c"

#include "propfont.h"
#include "drawstring.c"
#include "drawnxn.c"

extern void playfx(unsigned short fx) __z88dk_fastcall;  

const unsigned char empty_png[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

enum TILETYPES
{
    TILE_EMPTY = 0,
    TILE_PLAYER = 2,
    TILE_DOOR = 3,
    TILE_KEY = 4,
    TILE_NODRAW = 50,
    TILE_1X1_1 = 60,
    TILE_1X1_2 = 61,
    TILE_1X1_3 = 62,
    TILE_2X1_1 = 70,
    TILE_2X1_2 = 71,
    TILE_1X2_1 = 80,
    TILE_1X2_2 = 81,
    TILE_2X2_1 = 90,
    TILE_2X2_2 = 91,
    TILE_PAPER = 10    
};

const unsigned char ytile[] = {0x14, 0x22, 0x44, 0x28, 0x10, 0x28, 0x44, 0x28};
const unsigned char xtile[] = {0x00, 0x44, 0xaa, 0x11, 0xa8, 0x45, 0x02, 0x00};
const unsigned char nwcor[] = {0x00, 0x04, 0x0a, 0x11, 0x10, 0x21, 0x16, 0x08};
const unsigned char necor[] = {0x00, 0x20, 0x50, 0x88, 0x08, 0x84, 0x68, 0x10};
const unsigned char swcor[] = {0x08, 0x16, 0x21, 0x10, 0x11, 0x0a, 0x04, 0x00};
const unsigned char secor[] = {0x10, 0x68, 0x84, 0x08, 0x88, 0x50, 0x20, 0x00};

unsigned char y8;
unsigned char xorshift8(void) 
{
    y8 ^= (y8 << 7);
    y8 ^= (y8 >> 5);
    return y8 ^= (y8 << 3);
}

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

unsigned char controlscheme;
unsigned char mapcol[8*8*8];
unsigned char map[8*8*8];
unsigned char vizmap[8*8];
unsigned short mapofs;
unsigned short playerofs;
unsigned char totalkeys;
unsigned char keys;
unsigned char level;
unsigned short steps;
unsigned char cheater;

unsigned short parse_short(unsigned char *p)
{
    unsigned short i = *p; p++;
    i |= *p << 8;
    return i;
}

void color16x16(char c, char x, char y)
{
    unsigned short p = 0x5800 + y * 64 + x*2+8+32;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p+=31;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; 
}

void color16x32(char c, char x, char y)
{
    unsigned short p = 0x5800 + y * 64 + x*2+8+32;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p+=31;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p+=31;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p+=31;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c;
}

void color32x16(char c, char x, char y)
{
    unsigned short p = 0x5800 + y * 64 + x*2+8+32;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p+=31;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; 
}

void color32x32(char c, char x, char y)
{
    unsigned short p = 0x5800 + y * 64 + x*2+8+32;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p+=31;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p+=31;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p+=31;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; p++;
    *(unsigned char*)p = c; 
}

void update_keys()
{
    unsigned char k;
    for (k = 0; k < 3; k++)
    {
        if (k >= totalkeys || k < keys)
            draw16x16(empty_png, 0, k*16);
        else
            draw16x16(key_png, 0, k*16);
    }
}

void prettify_map()
{
    unsigned char i;
    unsigned char x, y, z;
    unsigned short p;
    static const unsigned char colors[16] = 
    {           
        1|64, 
        //1|64, 
        //2|64, 
        2|64, 
        3,
        4, 
        4, 
        4|64,
        4|64,
        //4|64,
        5, 
        5, 
        //5, 
        5|64, 
        //5|64, 
        6, 
        7,7,7,7,7,
        //7|64 
    };
    
    for (p = 0; p < 8*8*8; p++)
        mapcol[p] = 7|64;

    for (i = 0; i < 20; i++)
    {
        x = xorshift8() & 7;
        y = xorshift8() & 7;
        z = xorshift8() & 7;
        if (x < 7 && y < 7)
        {
            p = z * 8 * 8 + y * 8 + x;
            if (map[p] == TILE_1X1_1 &&
                map[p+1] == TILE_1X1_1 &&
                map[p+8] == TILE_1X1_1 &&
                map[p+9] == TILE_1X1_1)
            {
                do { z = xorshift8() & 3; } while (z < 2);
                switch (z)
                {
                case 0: z = TILE_2X2_1; break;
                default: z = TILE_2X2_2; break;
                }
                map[p] = z; // 2x2 crate                
                map[p+1] = TILE_NODRAW; // nop
                map[p+8] = TILE_NODRAW; // nop
                map[p+9] = TILE_NODRAW; // nop
            }
        }
    }

    for (i = 0; i < 20; i++)
    {
        x = xorshift8() & 7;
        y = xorshift8() & 7;
        z = xorshift8() & 7;
        if (y < 7)
        {
            p = z * 8 * 8 + y * 8 + x;
            if (map[p] == TILE_1X1_1 &&
                map[p+8] == TILE_1X1_1)
            {
                do { z = xorshift8() & 3; } while (z < 2);
                switch (z)
                {
                case 0: z = TILE_1X2_1; break;
                default: z = TILE_1X2_2; break;
                }
                map[p] = z; // 1x2 crate
                map[p+8] = TILE_NODRAW; // nop
            }
        }
    }

    for (i = 0; i < 20; i++)
    {
        x = xorshift8() & 7;
        y = xorshift8() & 7;
        z = xorshift8() & 7;
        if (x < 7)
        {
            p = z * 8 * 8 + y * 8 + x;
            if (map[p] == TILE_1X1_1 &&
                map[p+1] == TILE_1X1_1)
            {
                do { z = xorshift8() & 3; } while (z < 2);
                switch (z)
                {
                case 0: z = TILE_2X1_1; break;
                default: z = TILE_2X1_2; break;
                }
                map[p] = z; // 2x1 crate
                map[p+1] = TILE_NODRAW; // nop
            }
        }
    }

    for (i = 0; i < 80; i++)
    {
        x = xorshift8() & 7;
        y = xorshift8() & 7;
        z = xorshift8() & 7;
        p = z * 8 * 8 + y * 8 + x;
        if (map[p] == TILE_1X1_1)
        {
                do { z = xorshift8() & 3; } while (z < 3);
                switch (z)
                {
                case 0: z = TILE_1X1_1; break;
                case 1: z = TILE_1X1_2; break;
                default: z = TILE_1X1_3; break;
                }
            map[p] = z; // 1x1 crate
        }
    }

    for (p = 0; p < 8*8*8; p++)
    {
        char c = colors[xorshift8() & 15];
        switch (map[p])
        {
            case TILE_1X1_1: 
            case TILE_1X1_2:
            case TILE_1X1_3:
                mapcol[p] = c;
                break;
            case TILE_2X1_1:
            case TILE_2X1_2:
                mapcol[p] = c;
                mapcol[p+1] = c;
                break;
            case TILE_1X2_1:
            case TILE_1X2_2:
                mapcol[p] = c;
                mapcol[p+8] = c;
                break;
            case TILE_2X2_1:
            case TILE_2X2_2:
                mapcol[p] = c;
                mapcol[p+1] = c;
                mapcol[p+8] = c;
                mapcol[p+9] = c;
                break;
        }
    }
}

void get_code(char *ptr)
{
    const unsigned char codeset[] =
   //12345678901234567890123456789012
    "EEETAAOOIINSHRDLCUUMWFGYPBVKJXQZ";    
    unsigned char *l;
    unsigned char a = 0,b = 0,c = 0,d;
    l = (unsigned char*)leveldata + level * 78;
    for (d = 0; d < 78;)
    {
        if (d < 78) a ^= *l; l++; d++;
        if (d < 78) b ^= *l; l++; d++;
        if (d < 78) c ^= *l; l++; d++;
    }
    *ptr = codeset[a & 31]; ptr++;
    a >>= 5;
    *ptr = codeset[b & 31]; ptr++;
    b >>= 5;
    *ptr = codeset[c & 31]; ptr++;
    c >>= 5;
    a += b;
    a += c;
    *ptr = codeset[a & 31];    
}

void update_code()
{
    char i;
    char temp[] = "\12 Code:    ";
    get_code(temp+7);
    for (i = 0; i < 6; i++)
        draw8x8(empty_png, 25+i,8);
    drawstring(temp, 25, 8);        
}

void ending()
{
    unsigned short i;
    unsigned short x, y, j;
    unsigned char *p;

    for (y = 0; y < 24; y++)
    {
        i = 0x5800 + y * 32;
        for (x = 0; x < 32; x++, i++)
        {
             *(unsigned char*)(i) = 7;
        }
    }
   
    for (y = 19*8; y < 24*8; y++)        
    {
        i = yofs[y];
        for (x = 0; x < 32; x++, i++)
            *(unsigned char*)(i) = 0;
    }
    
    for (y = 19; y < 24; y++)
    {
        i = 0x5800 + y * 32;
        for (x = 0; x < 32; x++, i++)
        {
             *(unsigned char*)(i) = 0;
        }
    }
      
    {
        char temp[] = "\12 Code:e3?#";
        for (i = 0; i < 6; i++)
            draw8x8(empty_png, 25+i,8);
        drawstring(temp, 25, 8);        
    }

    {
        char temp[] = "\12 Level:??@? ";
        for (i = 0; i < 6; i++)
            draw8x8(empty_png, 25+i,0);
        drawstring(temp, 25, 0);        
    }    

    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            draw16x16(empty_png, 8 + x * 2, 8 + y * 16);
        }
    }
    draw16x16(player_png, 8 + 7, 8 + 56);
       
              /*   1234567012345670123456701234567012345670 */
    drawstring("\15... what now?", 0, 19*8);

    framecounter = 0;       
    
    while(framecounter < 500)
    {
        for (i = 0; i < 16; i++)
            draw16x16(player_png, 8 + 7, 8 + 56);
        if (framecounter < 5*32*8)
        {
            *(unsigned char*)(0x5800 + 19*32+framecounter/8) = 7;
        }
        framecounter++;        
    }

    for (y = 19*8; y < 24*8; y++)        
    {
        i = yofs[y];
        for (x = 0; x < 32; x++, i++)
            *(unsigned char*)(i) = 0;
    }
    
    framecounter = 0;
        
    for (i = 0; i < 100; i++)
    {
        for (y = 0; y < 8; y++)
        {
            for (x = 0; x < 8; x++)
            {
                p = (unsigned char*)empty_png;
                if (xorshift8() < i/2)
                    p = (unsigned char*)(((framecounter + x *4 + y * 2) & 4095)+10000);
                draw16x16(p, 8 + (x^6) * 2, 8 + (y^5) * 16);
                if (x == 0)
                draw16x16(player_png, 8 + 7, 8 + 56);
                framecounter++;
            }
        }            
    }

    for (j = 0; j < 9; j++)
    {
        for (i = 0; i < 80-j*4; i++)
        {
            for (y = 0; y < 14 && y < 9 + j / 2; y++)
            {
                for (x = 0; x < 8 + j; x++)
                {
                    p = (unsigned char*)empty_png;
                    if (xorshift8() < 60)
                        p = (unsigned char*)(((framecounter + x * 4 + y * 2) & 4095)+10000);
                    
                    if (j < 3 || p != empty_png)
                        draw16x16(p, 8 - j + (x) * 2, (y) * 16);
                    if (j < 3 && x == 0)
                        draw16x16(player_png, 8 + 7, 8 + 56);
                        
                    framecounter++;
                }
            }            
        }
    }
    


    __asm
        ld hl, #0 // this resets
        jp (hl)
    __endasm;
}


void unpack_map(unsigned char level)
{
    unsigned short i;
    unsigned char x,y,z,k;
    unsigned char *l;

    if (level == 50)
        ending();

    for (y = 19*8; y < 24*8; y++)        
    {
        i = yofs[y];
        for (x = 0; x < 32; x++, i++)
            *(unsigned char*)(i) = 0;
    }
    
    for (y = 19; y < 24; y++)
    {
        i = 0x5800 + y * 32;
        for (x = 0; x < 32; x++, i++)
        {
             *(unsigned char*)(i) = 0;
        }
    }

    if (level == 0)
    {
                  /*   1234567012345670123456701234567012345670 */
        drawstring("\47Another workday at the storage is over.", 0, 19*8);
        drawstring("\22Time to head home.", 0, 20*8);
        if (controlscheme == 1)
            drawstring("\25(Use W,A,S,D to move)", 0, 22*8);
        if (controlscheme == 2)
            drawstring("\25(Use Q,A,O,P to move)", 0, 22*8);
        
    }

    if (level == 1)
    {
                  /*   1234567012345670123456701234567012345670 */
        drawstring("\44Wait, what? Did I make a wrong turn?", 0, 19*8);
    }

    if (level == 2)
    {
                  /*   123456701234567012345670123456701234567012345670 */        
        drawstring("\34Hey guys, this isn't funny..", 0, 19*8);
        drawstring("\27Is anyone there? Hello?", 0, 20*8);
    }

    if (level == 3)
    {
                  /*   123456701234567012345670123456701234567012345670 */        
        drawstring("\56Locked door? Why? At least the key is nearby..", 0, 19*8);
    }

    if (level == 4)
    {
                  /*   123456701234567012345670123456701234567012345670 */        
        drawstring("\33What's with all these keys?", 0, 19*8);
    }
    
    if (level == 5)    
    {
                  /*   123456701234567012345670123456701234567012345670 */        
        drawstring("\45Hey, where's the key? Wait a second..", 0, 19*8);
        if (controlscheme == 1)
            drawstring("\40(try Q and E keys at the glitch)", 0, 21*8);
        if (controlscheme == 2)
            drawstring("\40(try N and M keys at the glitch)", 0, 21*8);
    }

    if (level == 6)    
    {
                  /*   123456701234567012345670123456701234567012345670 */        
        drawstring("\35And now the exit is missing..", 0, 19*8);
    }

    if (level == 7)
    {
                  /*   123456701234567012345670123456701234567012345670 */        
        drawstring("\53Moving through those glitches feels weird..", 0, 19*8);
        drawstring("\26..tastes like.. color?", 0, 20*8);
    }
    
    if (level == 9)
    {
                  /*   123456701234567012345670123456701234567012345670 */
        drawstring("\33What the heck is going on..", 0, 19*8);
    }

    if (level == 11)
    {
                  /*   123456701234567012345670123456701234567012345670 */
        drawstring("\45Feels like I've been walking forever.", 0, 19*8);
        drawstring("\40Is someone moving things around?", 0, 20*8);
    }

    if (level == 14)
    {
                  /*   123456701234567012345670123456701234567012345670 */
        drawstring("\53The building isn't big enough for all this.", 0, 19*8);
    }


    if (level == 17)
    {
                  /*   123456701234567012345670123456701234567012345670 */
        drawstring("\37Wait.. have I been here before?", 0, 19*8);
    }

    if (level == 23)
    {
                  /*   123456701234567012345670123456701234567012345670 */
        drawstring("\46I think I'm getting the hang of this..", 0, 19*8);
        drawstring("\22(whatever this is)", 0, 20*8);
    }
    
    if (level == 27)
    {
                  /*   123456701234567012345670123456701234567012345670 */
        drawstring("\42It keeps getting more complicated.", 0, 19*8);
    }

    if (level == 39)
    {
                  /*   123456701234567012345670123456701234567012345670 */
        drawstring("\23Will this ever end?", 0, 19*8);
    }

    if (level == 45)
    {
                  /*   123456701234567012345670123456701234567012345670 */
        drawstring("\36The glitches taste different..", 0, 19*8);
    }

    if (level == 48)
    {
                  /*   123456701234567012345670123456701234567012345670 */
        drawstring("\37The end is near, I can feel it.", 0, 19*8);
    }

    if (level == 49)
    {
                  /*   123456701234567012345670123456701234567012345670 */
        drawstring("\13This is it.", 0, 19*8);
    }

    for (i = 0; i < 8*8*8; i++)
        map[i] = 0;
    for (i = 0; i < 8*8; i++)
        vizmap[i] = 0xff;
    l = (unsigned char*)leveldata + level * 78;
    i = 0;
    for (z = 0; z < 8; z++)
    {
        for (y = 0; y < 8; y++)
        {
            unsigned char v = *l; 
            l++;
            for (x = 0; x < 8; x++, i++)
            {
                if (v & (0x80 >> x))
                {
                    map[i] = TILE_1X1_1;
                }
            }
        }
    }
    
    playerofs = parse_short(l); l += 2;

    map[parse_short(l)] = TILE_DOOR; l += 2;

    totalkeys = 0;
    keys = 0;

    for (k = 0; k < 3; k++)
    {
        i = parse_short(l); l+= 2;
        if (i <= 8*8*8)
        {
            map[i] = TILE_KEY;
            totalkeys++;
        }
    } 

    for (k = 0; k < 2; k++)
    {
        i = parse_short(l); l+= 2;
        if (i <= 8*8*8)
        {
            map[i] = TILE_PAPER; // papers
        }
    } 

    update_keys();
    update_code();

    {
        char temp[] = "\12 Level:     ";
        char d = 8;
        for (i = 0; i < 6; i++)
            draw8x8(empty_png, 25+i,0);
        level++;
        if (level > 9)
        {
            temp[d] = '0';
            while (level > 9)
            {
                level-=10;
                temp[d]++;
            }
            d++;
        }
        temp[d] = '0';
        while (level > 0)
        {
            level--;
            temp[d]++;
        }
        drawstring(temp, 25, 0);
    }
    
    prettify_map();
    prettify_map();
    prettify_map();
    prettify_map();
    prettify_map();
    
    framecounter = 0;
        
}

void paper()
{
    unsigned short i;
    unsigned char x,y;
    for (y = 19*8; y < 24*8; y++)        
    {
        i = yofs[y];
        for (x = 0; x < 32; x++, i++)
            *(unsigned char*)(i) = 0;
    }
    
    for (y = 19; y < 24; y++)
    {
        i = 0x5800 + y * 32;
        for (x = 0; x < 32; x++, i++)
        {
             *(unsigned char*)(i) = 0;
        }
    }

    if (level == 4)
    {
                /*            1       2       3       4       5       6 */
                /*     123456701234567012345670123456701234567012345670 */
        drawstring("\36There's a piece of paper here:", 0, 19*8);
        drawstring("\31\"You hear them whispering", 0, 20*8);
        drawstring("\32In the crowns of the trees", 0, 21*8);
        drawstring("\26You're whirling 'round", 0, 22*8);
        drawstring("\32But your eyes don't agree\"", 0, 23*8);
    }

    if (level == 8)
    {
                /*            1       2       3       4       5       6 */
                /*     123456701234567012345670123456701234567012345670 */
        drawstring("\56\"Therefore, add a screw or other fixing device", 0, 19*8); 
        drawstring("\60that is suitable for use in the material of your", 0, 20*8);
        drawstring("\55wall. If you're uncertain, contact your local", 0, 21*8); 
        drawstring("\25hardware specialist.\"", 0, 22*8);
    }

    if (level == 14)
    {
                /*            1       2       3       4       5       6 */
                /*     123456701234567012345670123456701234567012345670 */
        drawstring("\52\"If we are parsing according to production", 0, 19*8);
        drawstring("\54S->bABMC, then C.i inherits the value of A.s", 0, 20*8);
        drawstring("\40indirectly through M.i and M.s.\"", 0, 21*8);
        drawstring("\32Suddenly, you feel sleepy.", 0, 23*8);
    }    
    if (level == 17 || level == 23)
    {
        switch (xorshift8() & 7)
        {
        case 1:
                    /*            1       2       3       4       5       6 */
                    /*     123456701234567012345670123456701234567012345670123 */
            drawstring("\61\"Nothing assailed the company nor withstood their", 0, 19*8);
            drawstring("\63passage, and yet steadily fear grew on the Dwarf as", 0, 20*8);
            drawstring("\60he went on: most of all because he knew now that", 0, 21*8);
            drawstring("\40there could be no turning back;\"", 0, 22*8);
            break;
        case 2:
                    /*            1       2       3       4       5       6 */
                    /*     123456701234567012345670123456701234567012345670 */
            drawstring("\54\"He felt the awful attraction of the depths.", 0, 19*8);
            drawstring("\54Convulsively he seized onto the ramparts. He", 0, 20*8);
            drawstring("\41was almost lured into the abyss.\"", 0, 21*8);
            break;
        case 3:
                    /*            1       2       3       4       5       6 */
                    /*     123456701234567012345670123456701234567012345670 */
            drawstring("\53\"He continued to feed the fire, and the boy", 0, 19*8);
            drawstring("\55stayed on until the desert turned pink in the", 0, 20*8);
            drawstring("\54setting sun. He felt the urge to go out into", 0, 21*8);
            drawstring("\52the desert, to see if its silence held the", 0, 22*8);
            drawstring("\32answers to his questions.\"", 0, 23*8);
            break;
        case 4:
                    /*            1       2       3       4       5       6 */
                    /*     123456701234567012345670123456701234567012345670 */
            drawstring("\56\"The scientist hated to fail in anything. This", 0, 19*8);
            drawstring("\53tenacious attitude had cost the man several", 0, 20*8);
            drawstring("\52friends over the years, but earned him the", 0, 21*8);
            drawstring("\51Noble Prize in Physics at the astonishing", 0, 22*8);
            drawstring("\36age of twenty-five years old.\"", 0, 23*8);
            break;
        case 5:
                    /*            1       2       3       4       5       6 */
                    /*     123456701234567012345670123456701234567012345670 */
            drawstring("\52\"In that moment, I felt drained in a way I", 0, 19*8);
            drawstring("\51can't quite explain. I feared I was about", 0, 20*8);
            drawstring("\54to enter a dark place I'd never been invited", 0, 21*8);
            drawstring("\13to before.\"", 0, 22*8);
            break;
        case 6:
                    /*            1       2       3       4       5       6 */
                    /*     123456701234567012345670123456701234567012345670 */
            drawstring("\55\"Then came some new observations. In the last", 0, 19*8);
            drawstring("\54few years, several teams of researchers have", 0, 20*8);
            drawstring("\46studied tiny ripples in the background", 0, 21*8);
            drawstring("\51microwave radiation discovered by Penzias", 0, 22*8);
            drawstring("\14and Wilson.\"", 0, 23*8);
            break;
        default:
                    /*            1       2       3       4       5       6 */
                    /*     123456701234567012345670123456701234567012345670123 */
            drawstring("\57\"So she walked up to the door and knocked on it", 0, 19*8);
            drawstring("\56firmly five times. The voices stopped, a chair", 0, 20*8);
            drawstring("\56scraped across the floor, and the door opened,", 0, 21*8);
            drawstring("\61spilling warm naptha light out on the damp step.\"", 0, 22*8);
            break;            
        }
    }    
    if (level == 35)
    {
                /*            1       2       3       4       5       6 */
                /*     123456701234567012345670123456701234567012345670123 */
        drawstring("\61\"That sobered them all. They were all farmers, or", 0, 19*8);
        drawstring("\57lived in a community where farming - or farming", 0, 20*8);
        drawstring("\51weather - was the most important topic of", 0, 21*8);
        drawstring("\51conversation. They all knew what weeks of", 0, 22*8);
        drawstring("\30pounding rain would do.\"", 0, 23*8);
    }    
    if (level == 40)
    {
                /*            1       2       3       4       5       6       7*/
                /*     12345670123456701234567012345670123456701234567012345670 */
        drawstring("\66\"So, it's absolutely simple what I have to say to you.", 0, 19*8);
        drawstring("\55It's what my teacher said to me and I'm still", 0, 20*8);
        drawstring("\55deeply discovering the reverberation of that.", 0, 21*8);
        drawstring("\61And it's simply, Stop looking for what you want.\"", 0, 23*8);
    }    
    if (level == 49)
    {
                /*            1       2       3       4       5       6       7*/
                /*     1234567012345670123456701234567012345670123456701234567012345670 */
        drawstring("\43\"Left of west and coming in a hurry", 0, 19*8);
        drawstring("\51With the furies breathing down your neck.", 0, 20*8);
        drawstring("\61Team by team reporters baffled, trumped, tethered", 0, 21*8);
        drawstring("\57cropped. Look at that low playing! Fine, then.\"", 0, 22*8);        
    }    
    framecounter = 0;
}

void update_steps(unsigned short steps)
{
    char i;
    char temp[] = "\14 Steps:     ";
    char d = 8;
    unsigned short origsteps = steps;
    for (i = 0; i < 6; i++)
        draw8x8(empty_png, 25+i,24);
    if (origsteps > 9999)
    {
        temp[d] = '0';
        while (steps > 9999)
        {
            steps-=10000;
            temp[d]++;
        }
        d++;
    }
    if (origsteps > 999)
    {
        temp[d] = '0';
        while (steps > 999)
        {
            steps-=1000;
            temp[d]++;
        }
        d++;
    }
    if (origsteps > 99)
    {
        temp[d] = '0';
        while (steps > 99)
        {
            steps-=100;
            temp[d]++;
        }
        d++;
    }
    if (origsteps > 9)
    {
        temp[d] = '0';
        while (steps > 9)
        {
            steps-=10;
            temp[d]++;
        }
        d++;
    }
    temp[d] = '0';
    while (steps > 0)
    {
        steps--;
        temp[d]++;
    }
    drawstring(temp, 25, 24);
}

void ingame()
{
    char x, y, z;
    unsigned short i, j, lin;
    unsigned char *p;
    steps = 0;
    for (i = 0; i < 32*24; i++)
        *(unsigned char*)(0x5800+i) = 0;
    for (i = 0; i < 192*32; i++)        
        *(unsigned char*)(0x4000+i) = 0;
    for (i = 0; i < 32*24; i++)
        *(unsigned char*)(0x5800+i) = 7;
    for (i = 0; i < 16; i++)
    {
        draw8x8(ytile, 7, i*8+8);
        draw8x8(ytile, 8+16, i*8+8);
        draw8x8(xtile, 8+i, 0);
        draw8x8(xtile, 8+i, 16*8+8);        
    }
    draw8x8(nwcor, 7, 0);
    draw8x8(necor, 8+16, 0);
    draw8x8(swcor, 7, 16*8+8);
    draw8x8(secor, 8+16, 16*8+8);
    unpack_map(level);
    update_steps(0);    
                    
    while(1)
    {
        if (framecounter < 5*32*2)
        {
            *(unsigned char*)(0x5800 + 19*32+framecounter/2) = 7;
        }
        framecounter++;
        mapofs = playerofs & (7 << 6);
        lin = mapofs;
        for (y = 0; y < 8; y++)
        {
            for (x = 0; x < 8; x++, lin++)
            {
                unsigned short ofs = (y^5)*8+(x^6);
                unsigned char c = map[mapofs + ofs];
                unsigned char mode = TILE_1X1_1;
                if (playerofs == mapofs + ofs)
                    c = TILE_PLAYER;
                switch (c)
                {
                    case TILE_EMPTY:                         
                        p = (unsigned char*)empty_png;                        
                        if ((mapofs >= 8*8 && map[mapofs - 8*8 + ofs] < 5) ||
                            (mapofs <= 8*8*7 && map[mapofs + 8*8 + ofs] < 5))
                        {
                            if (!((framecounter + x + y) & 3) && ((xorshift8() & 5) == 0))
                            {
                                p = (unsigned char*)(((framecounter & 4095) >> 3)+10000);
                            }
                            c = framecounter;
                        }                        
                        break;
                    case TILE_PLAYER: p = (unsigned char*)player_png; break;
                    case TILE_DOOR: p = (unsigned char*)door_png; break;
                    case TILE_KEY: p = (unsigned char*)key_png; break;
                    case TILE_NODRAW: p = 0; mode = TILE_NODRAW; break; // no draw
                    case TILE_1X1_1: p = (unsigned char*)crate_16_16_1_png; break;
                    case TILE_1X1_2: p = (unsigned char*)crate_16_16_2_png; break;
                    case TILE_1X1_3: p = (unsigned char*)crate_16_16_3_png; break;
                    case TILE_2X1_1: p = (unsigned char*)crate_32_16_1_png; mode=TILE_2X1_1; break;
                    case TILE_2X1_2: p = (unsigned char*)crate_32_16_2_png; mode=TILE_2X1_1; break;
                    case TILE_1X2_1: p = (unsigned char*)crate_16_32_1_png; mode=TILE_1X2_1; break;
                    case TILE_1X2_2: p = (unsigned char*)crate_16_32_2_png; mode=TILE_1X2_1;break;
                    case TILE_2X2_1: p = (unsigned char*)crate_32_32_1_png; mode=TILE_2X2_1; break;
                    case TILE_2X2_2: p = (unsigned char*)crate_32_32_2_png; mode=TILE_2X2_1; break;
                    case TILE_PAPER: p = (unsigned char*)paper_png; break;
                    default: p = (unsigned char*)yofs; break;
                }
                if (vizmap[ofs] != c)
                {
                    if (mode == TILE_1X1_1)
                    {
                        draw16x16(p, 8 + (x^6)*2, 8+(y^5)*16);
                    }
                    if (mode == TILE_2X1_1)
                    {
                        draw32x16(p, 8 + (x^6)*2, 8+(y^5)*16);
                    }
                    if (mode == TILE_1X2_1)
                    {
                        draw16x32(p, 8 + (x^6)*2, 8+(y^5)*16);
                    }
                    if (mode == TILE_2X2_1)
                    {
                        draw32x32(p, 8 + (x^6)*2, 8+(y^5)*16);
                    }                        
                    vizmap[ofs] = c;
                }
                color16x16(mapcol[lin], x, y);
            }
        }
        
        readkeyboard();
        i = 0;
        if (controlscheme == 1)
        {
            if (KEYDOWN(W)) i = 1;
            if (KEYDOWN(S)) i = 2;
            if (KEYDOWN(A)) i = 3;
            if (KEYDOWN(D)) i = 4;
            if (KEYDOWN(Q)) i = 5;
            if (KEYDOWN(E)) i = 6;
        }
        if (controlscheme == 2)
        {
            if (KEYDOWN(Q)) i = 1;
            if (KEYDOWN(A)) i = 2;
            if (KEYDOWN(O)) i = 3;
            if (KEYDOWN(P)) i = 4;
            if (KEYDOWN(N)) i = 5;
            if (KEYDOWN(M)) i = 6;
        }
        if (KEYDOWN(L) && cheater) i = 7;
        if (KEYDOWN(K) && cheater) i = 8;
        if (KEYDOWN(J) && cheater) i = 9;
        
        if (i)
        {
            z = (playerofs >> 6) & 7;
            y = (playerofs >> 3) & 7;
            x = (playerofs >> 0) & 7;
            while (ANYKEY())//(KEYDOWN(W) || KEYDOWN(S) || KEYDOWN(A) || KEYDOWN(D) || KEYDOWN(Q) || KEYDOWN(E))
                readkeyboard();
            switch (i)
            {
            case 1: y--; break;
            case 2: y++; break;
            case 3: x--; break;
            case 4: x++; break;
            case 5: z--; break;
            case 6: z++; break;
            case 7: level++; unpack_map(level); x=29; break;
            case 8: if (level) { level--; unpack_map(level);} x=29; break;
            case 9: paper(); break;
            }
            j = 0xfff;
            if (x >= 0 && x < 8 &&
                y >= 0 && y < 8 &&
                z >= 0 && z < 8)
            {
                j = (z << 6) | (y << 3) | (x);
                if (map[j] < TILE_NODRAW)
                {
                    if (map[j] == TILE_DOOR && keys == totalkeys)
                    {
                        // next level
                        draw16x16(empty_png, 8 + (2*(playerofs & 7)), 8+((playerofs>>3)&7)*16);
                        playfx(4);
                        level++;
                        unpack_map(level);
                    }
                    else
                    {
                        playerofs = j;
                            
                        if (map[j] == TILE_KEY)
                        {
                            map[j] = 0;
                            keys++;
                            playfx(2);
                            update_keys();
                        }
                        else
                        if (map[j] == TILE_PAPER)
                        {
                            map[j] = 0;
                            playfx(3);
                            paper();
                        }
                        else
                        {
                            if (i>4)
                                playfx(3);
                            else
                                playfx(0);
                        }
                    }
                }
                else
                {
                    j = 0xfff;
                }
            }
            if (j == 0xfff)
            {
                playfx(1);
            }
            else
            {
                steps++;
                update_steps(steps);
            }
        }
    }    
}

void drawlogo()
{
    unsigned short i, j, p, c;
    c = 0;
    for (j = 0; j < 6*8; j++)
    {
        p = yofs[j];
        for (i = 0; i < 32; i++, c++)
        {
            *(unsigned char*)(p+i) = logo_small[c];
        }
    }
    for (i = 0; i < 32*6; i++, c++)
        *(unsigned char*)(0x5800+i) = logo_small[c];
}

void drawlogo_dist()
{
    unsigned short i, j, p, c, d;
    c = 0;
    for (j = 0; j < 6*8; j++)
    {
        p = yofs[j];
        d = c;
        c -= (((xorshift8() & 3) + (xorshift8() & 3) + (xorshift8() & 3) + (xorshift8() & 3))/4)-1;
        for (i = 0; i < 32; i++, c++)
        {
            *(unsigned char*)(p+i) = logo_small[c];
        }
        c = d + 32;
    }
}

void menu()
{
    unsigned short i, j, p, c;
    unsigned short eventcounter = 127 << 6;
    unsigned char code[5] = { 4, 'S', 'T', 'R', 'T' };
    for (i = 0; i < 32*24; i++)
        *(unsigned char*)(0x5800+i) = 0;
    for (i = 0; i < 192*32; i++)        
        *(unsigned char*)(0x4000+i) = 0;
    for (i = 0; i < 32*18; i++)
        *(unsigned char*)(0x5800+6*32+i) = 7;
            /*     12345670123456701234567012345670123456701234567012345670 */
    drawstring("\20Select controls and start game:", 8, 10*8);
    drawstring("\13 1) W,A,S,D", 10, 12*8);
    drawstring("\13 2) Q,A,O,P", 10, 13*8);

    drawstring("\13Enter code:", 8, 16*8);
    drawstring(code, 10, 18*8);
    
    drawstring("\52Mazeract by Jari Komppa, http://iki.fi/sol", 0, 22 * 8);
    drawstring("\66In-game art by Antti Tiihonen, twitter @antti_tiihonen", 0, 23 * 8);
    
    p = 0;
    j = 0;

    drawlogo();
            
    while(p == 0)
    {
        framecounter++;
        if (eventcounter == 0)
        {
            // distort
            drawlogo_dist();
            drawlogo();
            eventcounter = xorshift8() << 6;                
        }
        eventcounter--;
        
        readkeyboard();
        c = 0;
        if (KEYDOWN(1)) c = '1';
        if (KEYDOWN(2)) c = '2';

        if (KEYDOWN(A)) c = 'A';
        if (KEYDOWN(B)) c = 'B';
        if (KEYDOWN(C)) c = 'C';
        if (KEYDOWN(D)) c = 'D';
        if (KEYDOWN(E)) c = 'E';
        if (KEYDOWN(F)) c = 'F';
        if (KEYDOWN(G)) c = 'G';
        if (KEYDOWN(H)) c = 'H';
        if (KEYDOWN(I)) c = 'I';
        if (KEYDOWN(J)) c = 'J';
        if (KEYDOWN(K)) c = 'K';
        if (KEYDOWN(L)) c = 'L';
        if (KEYDOWN(M)) c = 'M';
        if (KEYDOWN(N)) c = 'N';
        if (KEYDOWN(O)) c = 'O';
        if (KEYDOWN(P)) c = 'P';
        if (KEYDOWN(Q)) c = 'Q';
        if (KEYDOWN(R)) c = 'R';
        if (KEYDOWN(S)) c = 'S';
        if (KEYDOWN(T)) c = 'T';
        if (KEYDOWN(U)) c = 'U';
        if (KEYDOWN(V)) c = 'V';
        if (KEYDOWN(W)) c = 'W';
        if (KEYDOWN(X)) c = 'X';
        if (KEYDOWN(Y)) c = 'Y';
        if (KEYDOWN(Z)) c = 'Z';
        
        while (ANYKEY()) readkeyboard();            
        
        if (c != 0)
        {
            if (c == '1')
            {
                p = 1;
                controlscheme = 1;              
            }
            else
            if (c == '2')
            {
                p = 1;
                controlscheme = 2;
            }
            else
            {
                code[j + 1] = c;
                j++;
                j &= 3;
                draw8x8(empty_png, 10, 18*8);
                draw8x8(empty_png, 11, 18*8);
                draw8x8(empty_png, 12, 18*8);
                draw8x8(empty_png, 13, 18*8);
                draw8x8(empty_png, 14, 18*8);
                drawstring(code, 10, 18*8);
            }
        }
    }
    
    for (i = 0; i < 50; i++)
    {
        unsigned char temp[4];
        level = i;
        get_code(temp);
        if (temp[0] == code[1] &&
            temp[1] == code[2] &&
            temp[2] == code[3] &&
            temp[3] == code[4])
        {
            return;
        }
    }
    {
        static const char mess[] = "Hi Andrew, I think instead of a poke, you might want to use the built-in cheat code.";
        if ('C' == code[1] &&
            'H' == code[2] &&
            'T' == code[3] &&
            'R' == code[4])
            cheater = 1;
    }
    
    level = 0; 
}

void main()
{       
    y8 = 1;
    cheater = 0;
    do_port254(0);
    menu();
    ingame();
}
