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

#include "yofstab.h"
#include "hwif.c"
#include "textout.c"

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

unsigned char *dict[120];
unsigned char tempq[128];
unsigned char tempa1[32];
unsigned char tempa2[32];
unsigned char tempa3[32];
unsigned char tempa4[32];

unsigned char decode_string(unsigned char *src, unsigned char *dst)
{
    unsigned char dstlen = 0;
    unsigned char len = *src;
    src++;
    
    while (len)
    {
        if (*src & 128)
        {
            unsigned char *sub = dict[*src & 127];
            unsigned char sublen = *sub;
            sub++;
            while (sublen)
            {
                *dst = *sub;
                sub++;
                dst++;
                dstlen++;
                sublen--;
            }
        }
        else
        {
            *dst = *src;
            dst++;
            dstlen++;
        }
        src++;
        len--;
    }
    return dstlen;
}

void get_string(unsigned short stringno, unsigned char *dst)
{
    unsigned char *src = dict[119]+*dict[119]+1;
    while (stringno)
    {
        src = src + *src + 1;
        stringno--;
    }
    *dst = decode_string(src, dst+1);
}

void get_question(unsigned short questionno)
{
    questionno *= 5;
    get_string(questionno, tempq); questionno++;
    get_string(questionno, tempa1); questionno++;
    get_string(questionno, tempa2); questionno++;
    get_string(questionno, tempa3); questionno++;
    get_string(questionno, tempa4);
}

void print_keys(unsigned char player, unsigned char ofs)
{
    const char * k1;
    const char * k2;
    const char * k3;
    const char * k4;
    char c;
    switch(player)
    {
        default:
        case 0:
            k1 = "1";
            k2 = "2";
            k3 = "3";
            k4 = "4";
            c = 2 << 3;
            break;
        case 1:            
            k1 = "7";
            k2 = "8";
            k3 = "9";
            k4 = "0";
            c = 4 << 3;
            break;
        case 2:            
            k1 = "Z";
            k2 = "X";
            k3 = "C";
            k4 = "V";
            c = 5 << 3;
            break;
        case 3:            
            k1 = "H";
            k2 = "J";
            k3 = "K";
            k4 = "L";
            c = 6 << 3;
            break;
    }
    drawstringfancy(k1, 3+11-ofs,17,c,1);
    drawstringfancy(k2, 17+11-ofs,17,c,1);
    drawstringfancy(k3, 3+11-ofs,21,c,1);
    drawstringfancy(k4, 17+11-ofs,21,c,1);
}

void mouth(unsigned char v)
{
    unsigned char v1 = v | 0xc0;
    unsigned char v2 = v | 0x07;
    *((unsigned char*)(yofs[31]+5)) = v2;
    *((unsigned char*)(yofs[32]+5)) = v2;
    *((unsigned char*)(yofs[33]+5)) = v2;
    *((unsigned char*)(yofs[34]+5)) = v2;

    *((unsigned char*)(yofs[31]+4)) = v1;
    *((unsigned char*)(yofs[32]+4)) = v1;
    *((unsigned char*)(yofs[33]+4)) = v1;
    *((unsigned char*)(yofs[34]+4)) = v1;
    
}

void main()
{   
    short i;
    framecounter = 0;
    for (i = 0; i < 192*32; i++)
        *((char*)0x4000+i) = 0;
    for (i = 0; i < 32*24; i++)
        *((char*)0x4000+192*32+i) = 7;
    dict[0] = (char*)(0x5b00 + (192*32+24*32));
    for (i = 1; i < 120; i++)
    {
        dict[i] = dict[i-1] + *dict[i-1] + 1;
    }
    
    get_question(50);
        
    cp((unsigned char*)0x4000, 6912, (char*)0x5b00);
                
    y8 = 1;
/*
    drawstringfancy("1234567890123456", 15, 2, 7, 5);
    drawstringfancy("1234567890123456", 15, 3, 7, 25);
    drawstringfancy("1234567890123456", 15, 4, 7, 255);
    drawstringfancy("1234567890123456", 15, 5, 7, 255);
    drawstringfancy("1234567890123456", 15, 6, 7, 255);
    drawstringfancy("1234567890123456", 15, 7, 7, 255);
    drawstringfancy("1234567890123456", 15, 8, 7, 255);
    drawstringfancy("1234567890123456", 15, 9, 7, 255);
*/
    
    drawstringfancy("000", 6, 13, 7, 255);
    drawstringfancy("000", 12, 13, 7, 255);
    drawstringfancy("000", 18, 13, 7, 255);
    drawstringfancy("000", 24, 13, 7, 255);
    
    drawstringfancy(tempa1+1,3,16,7,*tempa1);
    //drawstringfancy("123456789012", 3, 16, 7, 255);
    //drawstringfancy(dict[0]+1,3,16,7,*dict[0]);
    //drawstringfancy("123456789012", 3, 17, 7, 255);
    //drawstringfancy(dict[1]+1,3,17,7,*dict[1]);

    drawstringfancy(tempa2+1,17,16,7,*tempa2);
    //drawstringfancy("123456789012", 17, 16, 7, 255);
    //drawstringfancy("123456789012", 17, 17, 7, 255);

    drawstringfancy(tempa3+1,3,20,7,*tempa3);
    //drawstringfancy("123456789012", 3, 20, 7, 255);
    //drawstringfancy("123456789012", 3, 21, 7, 255);

    drawstringfancy(tempa4+1,17,20,7,*tempa4);
    //drawstringfancy("123456789012", 17, 20, 7, 255);
    //drawstringfancy("123456789012", 17, 21, 7, 255);

/*
    print_keys(0,3);
    print_keys(1,2);
    print_keys(2,1);
    print_keys(3,0);
*/
    print_keys(0,0);
    while(1) 
    {
        static const blinky[4] = {7,3,1,5};
        framecounter++;
        i = framecounter;
        
        if (i > *tempq) i = *tempq;
        drawstringfancy(tempq+1,15,1,7,i);
        if (i == *tempq)
        {
            mouth(0x66);
        }
        else
        {
            mouth(tempq[i]);
        }
        
        
        /*
        drawstringfancy("This is some\n"
                        "rather fancy\n"
                        "writing that\n"
                        "happens to have\n"
                        "mostly same\n"
                        "length lines.", 15, 1, blinky[framecounter & 3], framecounter);
                        */
        do_halt();
        do_halt();
        
    }

}
