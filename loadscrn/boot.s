;;	boot.s

		.module boot

		.area _HEADER(ABS)

        di
        
bootloader:        
    ld hl, #0x4000 ; destination position
    push hl
    ld hl, #1300 ; compressed data len (needs patching)
    push hl
    ld hl, #24092 ; source position (needs patching?) 23828+264
    push hl
    push hl ; needs another push to make the counts match
 	push ix
 	
	ld	ix,#10
	add	ix,sp
	ld	hl,#-6
	add	hl,sp
	ld	sp,hl
	ld	bc,#0 ;idx
next_data:
	ld	a,c
	sub	a, -4 (ix) ; len
	ld	a,b
	sbc	a, -3 (ix) ; len
	jr	NC, done_island
	ld	l,-6 (ix) ; src
	ld	h,-5 (ix) ; src
	add	hl,bc
	ld	h,(hl)
	ld	a,h
	rlca
	rlca
	rlca
	and	a,#0x07
	ld	-12 (ix),a     ; run len
	ld	-11 (ix),#0x00 ; run len
	ld	l,-12 (ix)
	inc	bc            ; idx
	ld	-14 (ix),c     ; idx
	ld	-13 (ix),b     ; idx
	ld	a,-6 (ix)      ; src
	add	a, -14 (ix)    ; idx
	ld	c,a
	ld	a,-5 (ix)      ; src
	adc	a, -13 (ix)    ; idx
	ld	b,a
	ld	a,h
	and	a, #0x1F
	ld	e,a
	ld	d,#0x00
	ld	a,-11 (ix) ; run len
	or	a,-12 (ix) ; run len
	jr	NZ, not_literal
	inc	de
	; prep for memcpy
	push	de
	push	bc
	push	de
	ld	l,-2 (ix) ; dst
	ld	h,-1 (ix) ; dst
	push	hl
	pop de
	pop bc
	pop hl
	ldir
	; / memcpy
	pop	de
	ld	a,-2 (ix) ; dst
	add	a, e
	ld	-2 (ix),a
	ld	a,-1 (ix) ; dst
	adc	a, d
	ld	-1 (ix),a  ; dst
	ld	a,-14 (ix) ; idx
	add	a, e
	ld	c,a
	ld	a,-13 (ix) ; idx
	adc	a, d
	ld	b,a
next_data_island:
	jr	next_data
done_island:	
    jr done
not_literal:
	ld	d,e
	ld	e,#0x00
	ld	a,(bc)
	ld	c,a
	ld	b,#0x00
	ld	a,e
	or	a, c
	ld	h,a
	ld	a,d
	or	a, b
	ld	-16 (ix), h
	ld	-15 (ix), a
	ld	c,-14 (ix)
	ld	b,-13 (ix)
	inc	bc
	ld	a,-12 (ix)
	sub	a, #0x07
	jr	NZ,do_run
	ld	a,-11 (ix)
	or	a, a
	jr	NZ,do_run
	ld	l,-6 (ix) ; src
	ld	h,-5 (ix) ; src
	add	hl,bc
	ld	e,(hl)
	ld	d,#0x00
	ld	hl,#0x0007
	add	hl,de
	ld	-12 (ix),l
	ld	-11 (ix),h
	inc	bc
do_run:
	ld	e,-12 (ix)
	ld	d,-11 (ix)
	inc	de
	inc	de
	ld	a,-2 (ix) ; dst
	sub	a, -16 (ix)
	ld	l,a
	ld	a,-1 (ix) ; dst
	sbc	a, -15 (ix)
	ld	h,a
	dec	hl
	; memcpy prep
	push	bc
	push	de
	push	hl
	push	de
	ld	l,-2 (ix) ; dst
	ld	h,-1 (ix) ; dst
	push	hl
	pop de
	pop bc
	pop hl
	ldir	
	; / memcpy
	pop	de
	pop	bc
	ld	a,-2 (ix) ; dst
	add	a, e
	ld	-2 (ix),a
	ld	a,-1 (ix) ; dst
	adc	a, d
	ld	-1 (ix),a ; dst
	jr	next_data_island
done:
	ld sp, ix
	ei
    ret
