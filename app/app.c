#pragma preproc_asm -
#include <string.h>

unsigned char fbcopy_idx;
unsigned char sin_idx;

#include "tab.h"
#include "s.h"
const unsigned char *bufp;

// xxxsmbbb
// where b = border color, m is mic, s is speaker
void port254(const unsigned char color) __z88dk_fastcall
{
    color; // color is in l
    // Direct border color setting
    __asm
        ld a,l
        out (254),a
    __endasm;    
}

void do_halt()
{
    __asm
        halt
    __endasm;
}

void fbcopy(const unsigned char * src, unsigned char start, unsigned char end)
{
    for (fbcopy_idx = start; fbcopy_idx < end; fbcopy_idx++, src+=32)
    {   
        memcpy((void*)yofs[fbcopy_idx], src, 32);
    }
}

void main()
{       
    sin_idx = 0;
    
    while(1)
    {
        sin_idx++;
        bufp = s_png;
        bufp += sinofs[sin_idx];
        do_halt(); // halt waits for interrupt - or vertical retrace
        // can do about 64 scanlines in a frame
        port254(1);
        fbcopy(bufp, 64, 120);
        port254(0);
    }    
}
