;;	boot.s

		.module boot

		.area _HEADER(ABS)
        .org #0x4000-0x1e

        di
        	        
        ; mask screen to black through attributes
        ld a, #0
        ld de, #0x5801
        ld hl, #0x5800
        ld bc, #767
        ld (hl), a        
        ldir       

        ; Copy bootloader to video memory
        ld de, #0x4000 ; dst
        ld bc, #0x200  ; len, currently ~140 bytes, but a little extra doesn't hurt
        ld hl, #0xb007 ; src (needs patching)
        ldir
        ld hl, #0x4000          
        
        jp (hl)

bootloader:
    ;.org 0x4000
    
    ld de, #0xde57 ; destination position (needs patching)
    ld hl, #0x5052 ; source position (needs patching)   
    ld ix, #0
    add ix, sp
    ld sp, #0x4400 ; put stack into video memory
    push de ; put dest into stack
    push ix ; save basic stack pointer..
    call lzfunpack
    pop ix 
    pop hl ; dest address
    ld sp, ix ; restore BASIC stack
    ei
    jp (hl) ; jumps to dest position
    
.include "lzfunpack.s"