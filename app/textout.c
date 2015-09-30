
// draw a single 8x8 character (x in 8 pixel steps, y in 1 pixel steps)
void drawchar(unsigned char c, unsigned char x, unsigned char y)
{
    data_ptr = (unsigned char*)(15616-32*8+c*8);
    screen_ptr = (unsigned char*)yofs[y]+x;
    __asm
        ld bc,(_screen_ptr)
        ld hl,(_data_ptr)
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    	inc hl
    	inc b
        ld a,(hl)
    	ld (bc),a
    __endasm;
}

// draw string, x in 8 pixel steps, y in 1 pixel steps
void drawstring(unsigned char *t, unsigned char x, unsigned char y)
{
    screen_ptr = (unsigned char*)yofs[y]+x;
    while (*t)
    {
        data_ptr = (unsigned char*)(15616-32*8+*t*8);
        __asm
            ld bc,(_screen_ptr)
	        ld hl,(_data_ptr)
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        	inc hl
        	inc b
	        ld a,(hl)
        	ld (bc),a
        __endasm;
        x++;        
        screen_ptr++;
        t++;
    }
}
