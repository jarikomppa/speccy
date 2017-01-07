//#define DRAWNXN_C

#ifdef DRAWNXN_C
 void draw8x8(unsigned char *sp,  unsigned char x, unsigned char y)
{
    unsigned short o;
    unsigned char i;
    for (i = 0; i < 8; i++)
    {
        o = yofs[y] + x; y++;    
        *((unsigned char*)o) = *sp; sp++;
    }
}
#else

void draw8x8(unsigned char *sp, unsigned char x, unsigned char y) __naked
{
    sp; x; y;
    /*
    Stack:
    --   01
    [ix] 23
    [spr] 45
    [x]  6
    [y]  7
    [ret] 89
    
    af bc de hl
    
    hl = screen ptr
    bc = x
    de = data ptr
    
    */
    __asm
        push ix
    	ld	ix,#0
    	add	ix,sp
    	push bc
        push hl
        push de
        
    	ld l, 7 (ix) // y
    	ld h, #0
    	add hl, hl
    	ld de, #_yofs
    	add hl, de
    	ld sp, hl
    	ld c, 6 (ix) // x
    	ld b, #0
    	ld d, 5(ix)
    	ld e, 4(ix)

#define REP \
    	pop hl \
    	add hl, bc \
        ld a, (de) \
        ld (hl), a \
        inc de
        
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP

        ld bc, #-6
        add ix, bc
        ld sp,ix  
        pop bc
        pop de
        pop hl
        pop ix
        ret
    __endasm;
}
#undef REP
#endif

#ifdef DRAWNXN_C
void draw32x16(unsigned char *sp, unsigned char x, unsigned char y)
{
    unsigned short o;
    unsigned char i;
    for (i = 0; i < 16; i++)
    {
        o = yofs[y] + x; y++;    
        *((unsigned char*)o) = *sp; o++; sp++;
        *((unsigned char*)o) = *sp; o++; sp++;
        *((unsigned char*)o) = *sp; o++; sp++;
        *((unsigned char*)o) = *sp; sp++;
    }
}
#else
void draw32x16(unsigned char *sp, unsigned char x, unsigned char y) __naked
{
    sp; x; y;
    /*
    Stack:
    --   01
    [ix] 23
    [spr] 45
    [x]  6
    [y]  7
    [ret] 89
    
    af bc de hl
    
    hl = screen ptr
    bc = x
    de = data ptr
    
    */
    __asm
        push ix
    	ld	ix,#0
    	add	ix,sp
    	push bc
        push hl
        push de
        
    	ld l, 7 (ix) // y
    	ld h, #0
    	add hl, hl
    	ld de, #_yofs
    	add hl, de
    	ld sp, hl
    	ld c, 6 (ix) // x
    	ld b, #0
    	ld d, 5(ix)
    	ld e, 4(ix)

#define REP \
    	pop hl \
    	add hl, bc \
        ld a, (de) \
        ld (hl), a \
        inc hl\
        inc de\
        ld a, (de) \
        ld (hl), a \
        inc hl\
        inc de\
        ld a, (de) \
        ld (hl), a \
        inc hl\
        inc de\
        ld a, (de) \
        ld (hl), a \
        inc hl\
        inc de
        
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP        
        ld bc, #-6
        add ix, bc
        ld sp,ix  
        pop bc
        pop de
        pop hl
        pop ix
        ret
    __endasm;
}
#undef REP
#endif

#ifdef DRAWNXN_C
void draw32x32(unsigned char *sp, unsigned char x, unsigned char y)
{
    unsigned short o;
    unsigned char i;
    for (i = 0; i < 32; i++)
    {
        o = yofs[y] + x; y++;    
        *((unsigned char*)o) = *sp; o++; sp++;
        *((unsigned char*)o) = *sp; o++; sp++;
        *((unsigned char*)o) = *sp; o++; sp++;
        *((unsigned char*)o) = *sp; sp++;
    }
}
#else

void draw32x32(unsigned char *sp, unsigned char x, unsigned char y) __naked
{
    sp; x; y;
    /*
    Stack:
    --   01
    [ix] 23
    [spr] 45
    [x]  6
    [y]  7
    [ret] 89
    
    af bc de hl
    
    hl = screen ptr
    bc = x
    de = data ptr
    
    */
    __asm
        push ix
    	ld	ix,#0
    	add	ix,sp
    	push bc
        push hl
        push de
        
    	ld l, 7 (ix) // y
    	ld h, #0
    	add hl, hl
    	ld de, #_yofs
    	add hl, de
    	ld sp, hl
    	ld c, 6 (ix) // x
    	ld b, #0
    	ld d, 5(ix)
    	ld e, 4(ix)

#define REP \
    	pop hl \
    	add hl, bc \
        ld a, (de) \
        ld (hl), a \
        inc hl\
        inc de\
        ld a, (de) \
        ld (hl), a \
        inc hl\
        inc de\
        ld a, (de) \
        ld (hl), a \
        inc hl\
        inc de\
        ld a, (de) \
        ld (hl), a \
        inc hl\
        inc de
        
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
                
        ld bc, #-6
        add ix, bc
        ld sp,ix  
        pop bc
        pop de
        pop hl
        pop ix
        ret
    __endasm;
}
#undef REP
#endif

#ifdef DRAWNXN_C
void draw16x32(unsigned char *sp, unsigned char x, unsigned char y)
{
    unsigned short o;
    unsigned char i;
    for (i = 0; i < 32; i++)
    {
        o = yofs[y] + x; y++;    
        *((unsigned char*)o) = *sp; o++; sp++;
        *((unsigned char*)o) = *sp; sp++;
    }
}

#else

void draw16x32(unsigned char *sp, unsigned char x, unsigned char y) __naked
{
    sp; x; y;
    /*
    Stack:
    --   01
    [ix] 23
    [spr] 45
    [x]  6
    [y]  7
    [ret] 89
    
    af bc de hl
    
    hl = screen ptr
    bc = x
    de = data ptr
    
    */
    __asm
        push ix
    	ld	ix,#0
    	add	ix,sp
    	push bc
        push hl
        push de
        
    	ld l, 7 (ix) // y
    	ld h, #0
    	add hl, hl
    	ld de, #_yofs
    	add hl, de
    	ld sp, hl
    	ld c, 6 (ix) // x
    	ld b, #0
    	ld d, 5(ix)
    	ld e, 4(ix)

#define REP \
    	pop hl \
    	add hl, bc \
        ld a, (de) \
        ld (hl), a \
        inc hl\
        inc de\
        ld a, (de) \
        ld (hl), a \
        inc hl\
        inc de
        
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP        
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP        

        ld bc, #-6
        add ix, bc
        ld sp,ix  
        pop bc
        pop de
        pop hl
        pop ix
        ret
    __endasm;
}
#undef REP
#endif

#ifdef DRAWNXN_C
void draw16x16(unsigned char *sp, unsigned char x, unsigned char y)
{
    unsigned short o;
    unsigned char i;
    for (i = 0; i < 16; i++)
    {
        o = yofs[y] + x; y++;    
        *((unsigned char*)o) = *sp; o++; sp++;
        *((unsigned char*)o) = *sp; sp++;
    }
}

#else

void draw16x16(unsigned char *sp, unsigned char x, unsigned char y) __naked
{
    sp; x; y;
    /*
    Stack:
    --   01
    [ix] 23
    [spr] 45
    [x]  6
    [y]  7
    [ret] 89
    
    af bc de hl
    
    hl = screen ptr
    bc = x
    de = data ptr
    
    */
    __asm
        push ix
    	ld	ix,#0
    	add	ix,sp
    	push bc
        push hl
        push de
        
    	ld l, 7 (ix) // y
    	ld h, #0
    	add hl, hl
    	ld de, #_yofs
    	add hl, de
    	ld sp, hl
    	ld c, 6 (ix) // x
    	ld b, #0
    	ld d, 5(ix)
    	ld e, 4(ix)

#define REP \
    	pop hl \
    	add hl, bc \
        ld a, (de) \
        ld (hl), a \
        inc hl\
        inc de\
        ld a, (de) \
        ld (hl), a \
        inc hl\
        inc de
        
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP
        REP        
        ld bc, #-6
        add ix, bc
        ld sp,ix  
        pop bc
        pop de
        pop hl
        pop ix
        ret
    __endasm;
}
#undef REP
#endif
