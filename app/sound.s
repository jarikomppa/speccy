;; Part of Jari Komppa's zx spectrum suite
;; https://github.com/jarikomppa/speccy
;; released under the unlicense, see http://unlicense.org 
;; (practically public domain)

;   void playtone(unsigned short delay) __z88dk_fastcall;
    .module sound

_playtone::    
    ld a, (_port254tonebit) ; port254
	ld de, #0
	ex de, hl
	ld b, #0xff 
	ld c, #16
	
	; de - increment value
	; b  - audio cycles
	; c  - value 16
	; a - port 254 data
	; hl - accumulator
loop:

	add hl, de
	jr	C, delayends
    jp out254 
delayends:	
	xor	a, c ; 4 clocks
out254:   
	out (254), a
	dec	b
	jr	NZ, loop

    ld (_port254tonebit), a    
    ret
    