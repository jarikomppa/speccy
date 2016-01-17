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

void mainmenu()
{
    unsigned short i;
    for (i = 0; i < 128*32; i++)
        *((unsigned char*)0x4000+64*32 + i) = 0;
    for (i = 0; i < 32*8; i++)
        *((unsigned char*)0x4000+192*32+ 8*32 + i) = 7;    
    for (i = 0; i < 32*8; i++)
        *((unsigned char*)0x4000+192*32+ 16*32 + i) = 3;    
        
    drawstring("Pick controls:", 2, 80);
    drawstring("1) OPQA space", 2, 96);
    drawstring("2) Interface-2 joystick", 2, 104);
    drawstring("3) Kempston joystick", 2, 112);
        
              //12345678901234567890123456789012
    drawstring("  Navigate through an unending", 0, 136);
    drawstring("         asteroid field.",0,144);
    drawstring("     Avoid a rock = 1 point", 0, 152);
    drawstring("    Shoot a rock = 10 points", 0, 160);
    drawstring("   - never tell me the odds -", 0, 176);
        
    while (1)
    {
        readkeyboard();
        if (!ANYKEY())
            break;
        for (i = 0; i < 256; i++)
            *((unsigned char*)0x4000+192*32+ 8*32 + i) = 7;    
        scroller(0);
        do_halt();
        framecounter++;           
    }
    
    while (1)
    {
        readkeyboard();
        if (KEYDOWN(1))
        {
            inputscheme = 0;
            return;
        }
        if (KEYDOWN(2))// || KEYDOWN(0))
        {
            inputscheme = 1;
            return;
        }
        if (KEYDOWN(3))// || !KEYDOWN(KEMPF))
        {
            inputscheme = 2;
            return;
        }
        
        for (i = 0; i < 256; i++)
            *((unsigned char*)0x4000+192*32+ 8*32 + i) = 7;    
        scroller(0);
        do_halt();
        framecounter++;           
    }    
}
