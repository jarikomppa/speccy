extern unsigned char *data_ptr;
extern unsigned char *screen_ptr;
extern unsigned short framecounter;
extern const unsigned short yofs[];
extern char gamestate;
extern char inputscheme;

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

extern void playfx(unsigned short fx) __z88dk_fastcall;  
extern void cp(unsigned char *dst, unsigned short len, unsigned char *src)  __z88dk_callee;
extern void drawstring(unsigned char *t, unsigned char x, unsigned char y);
extern void scroller(unsigned char y);

#include "hwif.c"

void gameover()
{
    unsigned short j;
    unsigned short i;    
    unsigned short o;
    for (j = 0; j < 16; j++)
    {        
        o = (framecounter * 3) & 15;
        for (i = 0; i < 32; i++, o += 16)
            *((unsigned char*)0x4000+192*32+ 8*32 + o) = 2;    
        scroller(0);
        do_halt();
        framecounter++;
    }
    for (j = 0; j < 16; j++)
    {
        o = (framecounter * 3) & 15;
        for (i = 0; i < 32; i++, o += 16)
            *((unsigned char*)0x4000+192*32+ 8*32 + o) = 1;    
        scroller(0);
        do_halt();
        framecounter++;
    }
    for (j = 0; j < 16; j++)
    {
        o = (framecounter * 3) & 15;
        for (i = 0; i < 32; i++, o += 16)
            *((unsigned char*)0x4000+192*32+ 8*32 + o) = 0;    
        scroller(0);
        do_halt();
        framecounter++;
    }
    
    for (i = 0; i < 128*32; i++)
        *((unsigned char*)0x4000+64*32 + i) = 0;
        
    drawstring("Game Over", 11, 120);
    
    for (j = 0; j < 16; j++)
    {
        o = (framecounter * 3) & 15;
        for (i = 0; i < 32; i++, o += 16)
            *((unsigned char*)0x4000+192*32+ 8*32 + o) = 7;    
        scroller(0);
        do_halt();
        framecounter++;
    }

    o = 0;
    
    while (o == 0)
    {
        readkeyboard();
        if (!ANYKEY())
            o = 1;
        for (i = 0; i < 256; i++)
            *((unsigned char*)0x4000+192*32+ 8*32 + i) = 7;    
        scroller(0);
        do_halt();
        framecounter++;           
    }
    
    while (o == 1)
    {
        readkeyboard();
        if (ANYKEY())
            o = 0;
        for (i = 0; i < 256; i++)
            *((unsigned char*)0x4000+192*32+ 8*32 + i) = 7;    
        scroller(0);
        do_halt();
        framecounter++;
    }    

    while (o == 0)
    {
        readkeyboard();
        if (!ANYKEY())
            o = 1;
        for (i = 0; i < 256; i++)
            *((unsigned char*)0x4000+192*32+ 8*32 + i) = 7;    
        scroller(0);
        do_halt();
        framecounter++;
    }    
}
