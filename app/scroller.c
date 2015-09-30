
const unsigned char scrolltext[] = ". .. ...   . .. Hello world, let's do this thing..  ";
unsigned char scrolltext_idx = 0;
unsigned char scrollcycle = 0;
unsigned char nextdata;
unsigned char vloop;

void scroller(unsigned char y, unsigned char nextglyph)
{   
    data_ptr = (unsigned char*)(15616 - 32 * 8 + nextglyph * 8);
    for (vloop = 0; vloop < 8; vloop++)
    {
        screen_ptr = (unsigned char*)(yofs[y] + 31);
        nextdata = *data_ptr;
        data_ptr++;
        nextdata = (nextdata >> scrollcycle) & 3;
        for (fbcopy_idx = 0; fbcopy_idx < 4; fbcopy_idx++)
        {
            // a bc de hl
            __asm
                ld bc, (_screen_ptr)
                ld hl, #_nextdata
                
                ld a,(bc)
                rlca
                rlca
                ld d, a
                and a, #0x03
                ld e, a
                ld a, d
                and a, #0xfc
                or a,(hl)
                ld (bc), a
                ld l, e
                dec bc
                
                ld a,(bc)
                rlca
                rlca
                ld d, a
                and a, #0x03
                ld e, a
                ld a, d
                and a, #0xfc
                or a,l
                ld (bc), a
                ld l, e
                dec bc

                ld a,(bc)
                rlca
                rlca
                ld d, a
                and a, #0x03
                ld e, a
                ld a, d
                and a, #0xfc
                or a,l
                ld (bc), a
                ld l, e
                dec bc

                ld a,(bc)
                rlca
                rlca
                ld d, a
                and a, #0x03
                ld e, a
                ld a, d
                and a, #0xfc
                or a,l
                ld (bc), a
                ld l, e
                dec bc

                ld a,(bc)
                rlca
                rlca
                ld d, a
                and a, #0x03
                ld e, a
                ld a, d
                and a, #0xfc
                or a,l
                ld (bc), a
                ld l, e
                dec bc
                
                ld a,(bc)
                rlca
                rlca
                ld d, a
                and a, #0x03
                ld e, a
                ld a, d
                and a, #0xfc
                or a,l
                ld (bc), a
                ld l, e
                dec bc

                ld a,(bc)
                rlca
                rlca
                ld d, a
                and a, #0x03
                ld e, a
                ld a, d
                and a, #0xfc
                or a,l
                ld (bc), a
                ld l, e
                dec bc

                ld a,(bc)
                rlca
                rlca
                ld d, a
                and a, #0x03
                ld e, a
                ld a, d
                and a, #0xfc
                or a,l
                ld (bc), a
                ld hl, #_nextdata
                ld (hl), e

            __endasm;
            /*
            unsigned char carry = *screen_ptr >> 7;
            *screen_ptr = (*screen_ptr << 1) | nextdata;
            nextdata = carry;
            */
            screen_ptr-=8;
        }
        y++;
    }
    scrollcycle--;
}
