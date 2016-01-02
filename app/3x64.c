const unsigned char testimg[] = {
#include "testimg.h"
};


void cp24x32(unsigned char *dst, const unsigned char *src)  __z88dk_callee
{
       dst; src;
    // de  hl
    __asm
    pop bc ; return address
    pop de ; dst
    pop hl ; src
    push bc ; put return address back
    
    ; 24x32x [de++]=[hl++], bc--
    #define REP_LDI_32 \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi \
	ldi
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
	REP_LDI_32
    __endasm;    
}

    
    dst = (unsigned char*)(0x4000);
    src = (unsigned char*)testimg;
    
    for (i = 0; i < (192*32+32*24); i++)
        *(unsigned char*)(0x4000 + i) = testimg[i];
    
    while(1) 
    {
        port254(1);
        dst = (unsigned char*)(0x4000 + 192*32);
        src = (unsigned char*)testimg + 192*32;
        cp24x32(dst, src);
        port254(0);
        //    *(unsigned char*)(0x4000 + 192*32 + i) = testimg[i + 192*32];
        do_halt();
        port254(2);
        dst = (unsigned char*)(0x4000 + 192*32);
        src = (unsigned char*)testimg + 192*32+32*24;
        cp24x32(dst, src);
        port254(0);
        do_halt();
    }
    