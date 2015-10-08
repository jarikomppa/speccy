;; Part of Jari Komppa's zx spectrum suite
;; https://github.com/jarikomppa/speccy
;; released under the unlicense, see http://unlicense.org 
;; (practically public domain)

;   void playtone(unsigned short delay) __z88dk_fastcall;
    .module sound

playtone_savesp:
    .dw 0
playtone_tone1store:
    .dw 0    

_playtone::    
    ld a, (_port254tonebit) ; port254	
	ld de, (playtone_tone1store)
	ld ix, #0
	add ix, de
	ld (playtone_savesp), sp
	ex de, hl
	
	ld b, #0xff 
	ld c, #16
	
	; de - increment value
	; b  - audio cycles
	; c  - value 16
	; a - port 254 data
	; ix - accumulator
	; hl - unused
	; iy - unused
loop:

	add ix, de
	jp	C, delayends
    jp out254 
delayends:	
	xor	a, c ; 4 clocks
	ld sp, hl
	
out254:   
	out (254), a
	dec	b
	jr	NZ, loop

    ld (_port254tonebit), a
    ld (playtone_tone1store), ix
    ld sp, (playtone_savesp)
    ret
    