/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

void cp32(unsigned char *dst, const unsigned char *src)  __z88dk_callee
{
       dst; src;
    // de  hl
    __asm
    pop bc
    pop de
    pop hl
    push bc
    
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
    __endasm;
    
}

// copy N scanlines from linear memory to video memory
void fbcopy(const unsigned char * src, unsigned char start, unsigned char end)
{
    for (fbcopy_idx = start; fbcopy_idx < end; fbcopy_idx++, src+=32)
    {   
        //memcpy((void*)yofs[fbcopy_idx], src, 32);
        cp32((void*)yofs[fbcopy_idx], src);
    }
}

unsigned char fbcopy_i_idx;
unsigned char fbcopy_i_idxtab[256];
unsigned short fbcopy_i_lintab[512];
void fbcopy_i(unsigned short linofs, unsigned char lines)
{
    while (lines)
    {
        fbcopy_idx = fbcopy_i_idxtab[fbcopy_i_idx];
        //memcpy((void*)yofs[fbcopy_idx], src + fbcopy_idx*32, 32);
        cp32((void*)yofs[fbcopy_idx], (unsigned char*)fbcopy_i_lintab[fbcopy_idx + linofs]);
        lines--;
        fbcopy_i_idx++;
    }    
}

void initfbcopy()
{
    unsigned short i;
    for (i = 0; i < 256; i++)
    {
        unsigned char v = i * 13;
        if (v >= 192) v -= 192;
        if (v >= 160 && v <= 168) v -= 100;
        fbcopy_i_idxtab[i] = v;
    }
    for (i = 0; i < 512; i++)
        fbcopy_i_lintab[i] = (unsigned short)&s_png + i * 32;
}