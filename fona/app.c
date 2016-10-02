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

#define FONTHEIGHT 8

#include "yofstab.h"
#include "hwif.c"
#include "test.h"

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

void drawstring(unsigned char *aS, unsigned char aX, unsigned char aY)
{
    unsigned char i, *s, *d, sx, x;
    unsigned char *datap = (unsigned char*)(int*)test_data - 32*FONTHEIGHT;
    unsigned char *widthp = (unsigned char*)(int*)test_width - 32;
    for (i = 0; i < FONTHEIGHT; i++)
    {
        s = aS;
        sx = 0;
        x = aX;
        d = (unsigned char*)yofs[aY];
        while (*s)
        {
            unsigned char ch = *s;
            unsigned char data = datap[ch * FONTHEIGHT];
            unsigned char width = widthp[ch];
            *d |= data >> sx;
            
            sx += width;
            if (sx > 8)
            {
                d++;
                sx -= 8;
                *d |= data << (width - sx);
            }
            s++;
        }
        aY++;
        datap++;
    }
}

void main()
{
    unsigned short i;
    do_halt();

    for (i = 0; i < 192*32; i++)
        *(unsigned char*)(0x4000+i) = 0;
    for (i = 0; i < 24*32; i++)
        *(unsigned char*)(0x5800+i) = (i&1)?7:7+8;

    framecounter = 0;

    port254(0);
    
    while(1)
    {
        /*
        for (i = 0; i < 192*32; i++)
            *(unsigned char*)(0x4000+i) = 0;
*/
        framecounter++;
        do_halt(); // halt waits for interrupt - or vertical retrace
        for (i = 0; i < 600; i++);

        port254(1);
        drawstring("Test", 0, 0);
/*                           
        drawstring("Venison frankfurter turducken boudin. Filet", 0, 0);
        drawstring("mignon meatloaf ham hamburger, pork chop ", 0, 8*1);
        drawstring("drumstick kevin prosciutto t-bone chuck short ", 0, 8*2);
        drawstring("loin. Tenderloin burgdoggen bacon porchetta", 0, 8*3);
        drawstring("pork chop leberkas. Filet mignon pork", 0, 8*4);
        drawstring("andouille flank, shankle brisket turkey ", 0, 8*5);
        drawstring("bacon bresaola. Prosciutto pork chop", 0, 8*6);
        drawstring("hamburger pastrami strip steak pork belly pig ", 0, 8*7);
        drawstring("frankfurter rump cupim biltong shoulder ", 0, 8*8);
        drawstring("andouille chicken kevin. Pork belly ", 0, 8*9);
        drawstring("meatloaf kielbasa tri-tip meatball.", 0, 8*10);
        */
        //drawstring_dumb("What a fixed world it is!", 0, 9);
        port254(0);
    }
}
