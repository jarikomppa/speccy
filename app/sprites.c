/*
void drawsprite(char *aData, short aX, short aY)
{    
    unsigned char y;
    y = (aX & 7) * 16;
    aData += y;
    aData += y;
    aData += y;
    aData += y;
    aData += y;
    aData += y;
    aX /= 8;
    for (y = 0; y < 16; y++, aY++)
    {
        data_ptr = aData;
        screen_ptr = (unsigned char*)yofs[aY] + aX;
    __asm
        push bc
        push hl
        ld bc,(_screen_ptr)
        ld hl,(_data_ptr)

#define HPASS = \
        ld a, (bc)  \
        and (hl)    \
        inc hl      \
        or (hl)     \
        ld (bc),a   \
        inc hl      \
        inc bc      \
                    \
        ld a, (bc)  \
        and (hl)    \
        inc hl      \
        or (hl)     \
        ld (bc),a   \
        inc hl      \
        inc bc      \
                    \
        ld a, (bc)  \
        and (hl)    \
        inc hl      \
        or (hl)     \
        ld (bc),a

        HPASS

        pop hl
        pop bc
    __endasm;   
        aData += 3*2;             
    }    
}
*/


/*
void drawsprite(char *aData, short aX, short aY)
{    
    aData; aX; aY;
     
    __asm
        push ix
    	ld	ix,#0
    	add	ix,sp
    
        push iy
        push hl
        push bc
        push de
        
        ld a, 6 (ix) ; aX
        and a, #7
        ld c, a
        ld b, #0
        
        ; now we need to multiply bc by 16*6
        push bc
        pop hl     ; hl = 1*bc       
        add hl, bc ; hl = 2*bc
        add hl, bc ; hl = 3*bc
        add hl, hl ; hl = 6*bc
        add hl, hl ; hl = 2*6*bc
        add hl, hl ; hl = 4*6*bc
        add hl, hl ; hl = 8*6*bc
        add hl, hl ; hl = 16*6*bc        
        push hl
        pop bc

        ld d, 9 (ix) ; aY
        ld e, 8 (ix) 
        ld hl, #(_yofs)
        add hl, de      
        add hl, de      
        push hl
        pop iy
        
        ld h, 5 (ix) ; aData
        ld l, 4 (ix)
        add hl, bc ; inside-byte shift added
        
        ex de,hl
        ld h, 7 (ix) ; aX
        ld l, 6 (ix)
        // divide aX by 8
        srl h
        rr l
        srl h
        rr l
        srl h
        rr l
        ex de,hl
        push de
        pop bc
              
#define HPASS  \
        ld a, (de)  \
        and (hl)    \
        inc hl      \
        or (hl)     \
        ld (de),a   \
        inc hl      \
        inc de      \
                    \
        ld a, (de)  \
        and (hl)    \
        inc hl      \
        or (hl)     \
        ld (de),a   \
        inc hl      \
        inc de      \
                    \
        ld a, (de)  \
        and (hl)    \
        inc hl      \
        or (hl)     \
        ld (de),a   \
        inc hl

        ld e, 0 (iy)
        ld d, 1 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 2 (iy)
        ld d, 3 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 4 (iy)
        ld d, 5 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 6 (iy)
        ld d, 7 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 8 (iy)
        ld d, 9 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 10 (iy)
        ld d, 11 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 12 (iy)
        ld d, 13 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 14 (iy)
        ld d, 15 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 16 (iy)
        ld d, 17 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 18 (iy)
        ld d, 19 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 20 (iy)
        ld d, 21 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 22 (iy)
        ld d, 23 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 24 (iy)
        ld d, 25 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 26 (iy)
        ld d, 27 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 28 (iy)
        ld d, 29 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
        ld e, 30 (iy)
        ld d, 31 (iy)
        ex de,hl
        add hl, bc
        ex de,hl
        HPASS
#undef HPASS        
        pop de
        pop bc
        pop hl
        pop iy
        pop ix            
    __endasm;   
}
*/


void drawsprite(char *aData, short aX, short aY)
{    
    aData; aX; aY;
     
    __asm
        push ix
    	ld	ix,#0
    	add	ix,sp
    
        push iy
        push hl
        push bc
        push de
        
        ld a, 6 (ix) ; aX
        and a, #7
        ld c, a
        ld b, #0
        
        ; now we need to multiply bc by 16*6
        push bc
        pop hl     ; hl = 1*bc       
        add hl, bc ; hl = 2*bc
        add hl, bc ; hl = 3*bc
        add hl, hl ; hl = 6*bc
        add hl, hl ; hl = 2*6*bc
        add hl, hl ; hl = 4*6*bc
        add hl, hl ; hl = 8*6*bc
        add hl, hl ; hl = 16*6*bc        
        push hl
        pop bc

        ld d, 9 (ix) ; aY
        ld e, 8 (ix) 
        ld hl, #(_yofs)
        add hl, de      
        add hl, de      
        push hl
        pop iy
        
        ld h, 5 (ix) ; aData
        ld l, 4 (ix)
        add hl, bc ; inside-byte shift added
        
        ex de,hl
        ld h, 7 (ix) ; aX
        ld l, 6 (ix)
        // divide aX by 8
        srl h
        rr l
        srl h
        rr l
        srl h
        rr l
        ex de,hl
        push de
        pop bc
        ld sp, hl
              
#define HPASS       \
        pop hl      \
        ld a, (de)  \
        and l       \
        or h        \
        ld (de),a   \
        inc de      \
                    \
        pop hl      \
        ld a, (de)  \
        and l       \
        or h        \
        ld (de),a   \
        inc de      \
                    \
        pop hl      \
        ld a, (de)  \
        and l       \
        or h        \
        ld (de),a   

        ld l, 0 (iy)
        ld h, 1 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 2 (iy)
        ld h, 3 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 4 (iy)
        ld h, 5 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 6 (iy)
        ld h, 7 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 8 (iy)
        ld h, 9 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 10 (iy)
        ld h, 11 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 12 (iy)
        ld h, 13 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 14 (iy)
        ld h, 15 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 16 (iy)
        ld h, 17 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 18 (iy)
        ld h, 19 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 20 (iy)
        ld h, 21 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 22 (iy)
        ld h, 23 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 24 (iy)
        ld h, 25 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 26 (iy)
        ld h, 27 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 28 (iy)
        ld h, 29 (iy)
        add hl, bc
        ex de,hl
        HPASS
        ld l, 30 (iy)
        ld h, 31 (iy)
        add hl, bc
        ex de,hl
        HPASS
#undef HPASS
        ld sp, ix
        dec sp
        dec sp
        dec sp
        dec sp
        dec sp
        dec sp
        dec sp
        dec sp

        pop de
        pop bc
        pop hl
        pop iy
        pop ix            
    __endasm;   
}


void clearsprite(short aX, short aY)
{    
    unsigned char y;
    aX /= 8;
    for (y = 0; y < 16; y++, aY++)
    {
        unsigned char *scr = (unsigned char*)yofs[aY] + aX;
        *scr = 0;
        scr++; 
        *scr = 0;
        scr++; 
        *scr = 0;
        scr++; 
    }    
}

/*
#include "..\tools\bubble.c"

void drawsprite(char *aData, short aX, short aY)
{
    drawsprite_bubble(aX, aY);
}
*/


//unsigned char oldx, oldy;

void spritetest()
{
    unsigned char x = sinofs[(framecounter + framecounter) & 0xff] & 127;
    unsigned char y = sinofs[framecounter & 0xff] & 127;
    //clearsprite(oldx,oldy);
    //clearsprite(oldy,oldx);
    //clearsprite(127-oldx, 127-oldy);
    //oldx = x;    oldy = y;
    x = 0; y = 0;
/*    drawsprite_bubble(0,0);
    drawsprite_bubble(1,0);
    drawsprite_bubble(2,0);
    drawsprite_bubble(3,0);
    drawsprite_bubble(4,0);

    drawsprite_bubble(5,0);
    drawsprite_bubble(6,0);
    drawsprite_bubble(7,0);
    drawsprite_bubble(0,0);
    drawsprite_bubble(1,0);


    drawsprite_bubble(2,0);
    drawsprite_bubble(3,0);
    drawsprite_bubble(4,0);
    drawsprite_bubble(5,0);
    drawsprite_bubble(6,0);

    drawsprite_bubble(7,0);
    drawsprite_bubble(0,0);
    drawsprite_bubble(1,0);
    drawsprite_bubble(2,0);
    drawsprite_bubble(3,0);


    drawsprite_bubble(4,0);
    drawsprite_bubble(5,0);
    drawsprite_bubble(6,0);
    drawsprite_bubble(7,0);
    drawsprite_bubble(0,0);

    drawsprite_bubble(1,0);
    */
/*    
    drawsprite(bubble, 0, 0);    
    drawsprite(bubble, 16, 0);    
    drawsprite(bubble, 32, 0);    
    drawsprite(bubble, 48, 0);    
    drawsprite(bubble, 64, 0);    

    drawsprite(bubble, 80, 0);    
    drawsprite(bubble, 96, 0);    
    drawsprite(bubble, 112, 0);    
    drawsprite(bubble, 128, 0);    
    drawsprite(bubble, 144, 0);    

    drawsprite(bubble, 160, 0);    
    drawsprite(bubble, 176, 0);    
    drawsprite(bubble, 192, 0);    
    drawsprite(bubble, 208, 0);    
    drawsprite(bubble, 224, 0);    

    drawsprite(bubble, 240, 0);    
    drawsprite(bubble, 0, 16);    
    drawsprite(bubble, 16, 16);    
    drawsprite(bubble, 32, 16);    
    drawsprite(bubble, 48, 16);    
*/

    
    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10; port254(2);    
    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10; port254(0);
    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10; port254(2);
    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10;port254(0);

    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10; port254(2);
    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10;port254(0);
    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10;port254(2);
    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10;port254(0);

    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10;port254(2);
    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10;port254(0);
    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10;port254(2);
    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10;port254(0);

    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10;port254(2);
    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10;port254(0);
    drawsprite(bubble, (sinofs[(framecounter + framecounter+x) & 0xff] & 255) + 16, (sinofs[(framecounter+y) & 0xff] & 127)+24); x += 10; y -= 10;port254(2);
    
}
