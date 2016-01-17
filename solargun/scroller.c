/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

const unsigned char scrolltext[] = ". .. ...   . .. SolarGun work in progress ... by Jari Komppa ... http://iki.fi/sol ... blah blah ... .. .   ... .. .    ";
unsigned char scrolltext_idx = 0;
unsigned char scrollcycle = 0;
unsigned char nextdata;
unsigned char vloop;


void scroller(unsigned char y)
{   
    if (scrollcycle == 0)
    {
        scrolltext_idx++;
        scrollcycle = 7;
        if (scrolltext[scrolltext_idx] == 0)
            scrolltext_idx = 0;
    }    
    
    data_ptr = (unsigned char*)(15616 - 32 * 8 + scrolltext[scrolltext_idx] * 8);
    //data_ptr = (unsigned char*)(fona_png - 32 * 8 + scrolltext[scrolltext_idx] * 8);

    for (vloop = 0; vloop < 8; vloop++)
    {
        screen_ptr = (unsigned char*)(yofs[y] + 31);
        nextdata = *data_ptr;
        data_ptr++;
        // shift by variable - causes variable timing
        nextdata = (nextdata >> scrollcycle) & 3;
        // a bc de hl ix iy
        __asm
//            push bc
//            push hl
            push de
            ld bc, (_screen_ptr)
            ld hl, (_nextdata)

; a - workhorse
; bc - screen ptr
; d - temp
; e - leftovers to next round
; l - leftovers from previous round
; h - value 0x03 
            ld h, #0x03
#define REPBLOCK \
            ld a,(bc) \
            rlca \
            rlca \
            ld d, a \
            and a, h; #0x03 \
            ld e, a \
            ld a, d \
            and a, #0xfc \
            or a, l \
            ld (bc), a \
            ld l, e \
            dec bc 
            
            REPBLOCK
            REPBLOCK
            REPBLOCK
            REPBLOCK

            REPBLOCK
            REPBLOCK
            REPBLOCK
            REPBLOCK

            REPBLOCK
            REPBLOCK
            REPBLOCK
            REPBLOCK

            REPBLOCK
            REPBLOCK
            REPBLOCK
            REPBLOCK

            REPBLOCK
            REPBLOCK
            REPBLOCK
            REPBLOCK

            REPBLOCK
            REPBLOCK
            REPBLOCK
            REPBLOCK

            REPBLOCK
            REPBLOCK
            REPBLOCK
            REPBLOCK

            REPBLOCK
            REPBLOCK
            REPBLOCK
#undef REPBLOCK            
            
            ld a,(bc)
            rlca
            rlca
            and a, #0xfc
            or a,l
            ld (bc), a
            pop de
//            pop hl
//            pop bc

        __endasm;
        
        y++;
    }
    scrollcycle--;
}
