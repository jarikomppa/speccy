;;	boot.s

		.module boot

		.area _HEADER(ABS)

        di 
	
        ld sp, #0x4400 ; put stack into video memory
        
        ; mask screen to black through attributes
        ld bc, #768
        ld hl, #0x5800
clearloop:
        ld a, #0
        ld (hl), a
        inc hl
        dec bc
        ld a, b
        or a, c
        jr NZ, clearloop

        ; Copy bootloader to video memory
        ld de, #0x4000 ; dst
        ld bc, #0x200  ; len, currently 304 bytes, but a little extra doesn't hurt
        ld hl, #bootloader ; src (needs patching)
        ldir      
        ld hl, #0x4000  
        jp (hl)

bootloader:        
    
    ld hl, #0x6000 ; destination position (needs patching)
    push hl
    ld hl, #0x2727 ; compressed data len (needs patching)
    push hl
    ld hl, #0xd000 ; source position (needs patching)
    push hl
    push hl ; needs another push to make the counts match
 	push ix
	ld	ix,#0
	add	ix,sp
	ld	hl,#-6
	add	hl,sp
	ld	sp,hl
	ld	bc,#0 ;idx
next_data:
	ld	a,c
	sub	a, 6 (ix) ; len
	ld	a,b
	sbc	a, 7 (ix) ; len
	jr	NC, done_island
	ld	l,4 (ix) ; src
	ld	h,5 (ix) ; src
	add	hl,bc
	ld	h,(hl)
	ld	a,h
	rlca
	rlca
	rlca
	and	a,#0x07
	ld	-2 (ix),a     ; run len
	ld	-1 (ix),#0x00 ; run len
	ld	l,-2 (ix)
	out (254), a
	inc	bc            ; idx
	ld	-4 (ix),c     ; idx
	ld	-3 (ix),b     ; idx
	ld	a,4 (ix)      ; src
	add	a, -4 (ix)    ; idx
	ld	c,a
	ld	a,5 (ix)      ; src
	adc	a, -3 (ix)    ; idx
	ld	b,a
	ld	a,h
	and	a, #0x1F
	ld	e,a
	ld	d,#0x00
	ld	a,-1 (ix) ; run len
	or	a,-2 (ix) ; run len
	jr	NZ, not_literal
	inc	de
	; prep for memcpy
	push	de
	push	bc
	push	de
	ld	l,8 (ix) ; dst
	ld	h,9 (ix) ; dst
	push	hl
	pop de
	pop bc
	pop hl
	ldir
	; / memcpy
	pop	de
	ld	a,8 (ix) ; dst
	add	a, e
	ld	8 (ix),a
	ld	a,9 (ix) ; dst
	adc	a, d
	ld	9 (ix),a  ; dst
	ld	a,-4 (ix) ; idx
	add	a, e
	ld	c,a
	ld	a,-3 (ix) ; idx
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
	ld	-6 (ix), h
	ld	-5 (ix), a
	ld	c,-4 (ix)
	ld	b,-3 (ix)
	inc	bc
	ld	a,-2 (ix)
	sub	a, #0x07
	jr	NZ,do_run
	ld	a,-1 (ix)
	or	a, a
	jr	NZ,do_run
	ld	l,4 (ix) ; src
	ld	h,5 (ix) ; src
	add	hl,bc
	ld	e,(hl)
	ld	d,#0x00
	ld	hl,#0x0007
	add	hl,de
	ld	-2 (ix),l
	ld	-1 (ix),h
	inc	bc
do_run:
	ld	e,-2 (ix)
	ld	d,-1 (ix)
	inc	de
	inc	de
	ld	a,8 (ix) ; dst
	sub	a, -6 (ix)
	ld	l,a
	ld	a,9 (ix) ; dst
	sbc	a, -5 (ix)
	ld	h,a
	dec	hl
	; memcpy prep
	push	bc
	push	de
	push	hl
	push	de
	ld	l,8 (ix) ; dst
	ld	h,9 (ix) ; dst
	push	hl
	pop de
	pop bc
	pop hl
	ldir	
	; / memcpy
	pop	de
	pop	bc
	ld	a,8 (ix) ; dst
	add	a, e
	ld	8 (ix),a
	ld	a,9 (ix) ; dst
	adc	a, d
	ld	9 (ix),a ; dst
	jr	next_data_island
done:
	ld	sp, ix
	pop	ix
    ld hl, #0x6000 ; destination position, needs patching  
    jp (hl)
