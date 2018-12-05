	.module drawstring
	
	.globl _drawstringz
	.area _CODE

;   9(ix) = aY
;   8(ix) = aX
;  -7(ix) = i (row)
;   6(ix) &   7(ix) = aS
;  -3(ix) &  -4(ix) = datap
;  -1(ix) &  -2(ix) = dest pointer
;  -5(ix) &  -6(ix) = dest base

; void drawstringz(unsigned char *aS, unsigned char aX, unsigned char aY)
_drawstringz::
	push	ix
	push iy
	ld	ix,#0
	add	ix,sp
	ld	hl,#-15
	add	hl,sp
	ld	sp,hl
; font data pointer (-32*8 because data starts from space)
	ld	-4 (ix),#<((_propfont_data - 0x0100))
	ld	-3 (ix),#>((_propfont_data - 0x0100))
; Multiply input y by 8
	ld	a,9 (ix)
	rlca
	rlca
	rlca
	and	a,#0xf8
	ld	9 (ix),a
	
; row loop counter 
	ld	-7 (ix),#0x08

; calculate destination offset (from lookup table)
	ld	l,9 (ix)
	ld	h,#0x00
	add	hl, hl
	ld	de,#_yofs
	add	hl,de
	ld	e,(hl)
	inc	hl
	ld	b,(hl)
	ld	a,8 (ix)
	add	a, e
	ld	-6 (ix),a
	ld	a,#0x00
	adc	a, b
	ld	-5 (ix),a

; Row loop - do this 8 times (for 8 pixel high chars)
rowloop:
; set iy to point at input string
	ld	c, 6 (ix)
	ld	b, 7 (ix)
	ld iy, #0x00
	add iy, bc

; destination byte can fit 8 bits
	ld	c,#0x08
; destination byte
;	ld	b,#0x00 ; not actually needed 

; set up destination pointer (base ptr increments per row, copy runs with string)
    ld a, -5 (ix)
    ld -1 (ix), a
    ld a, -6 (ix)
    ld -2 (ix), a	
	  		  		
; Loop while input char is not zero
nextchar:
; Grab char and take data slice.
; This could be optimized by reorganizing data to be linear slice-wise,
; instead of all 8 bytes of a glyph at once.
	ld	l, (iy)
	ld	h, #0x00
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	e,-4 (ix)
	ld	d,-3 (ix)
	add	hl,de
	ld	a, (hl)
	ld  d, a
	
; Find glyph's width
	ld	a, #<((_propfont_width - 0x0020))
	add	a, (iy)
	ld	l, a
	ld	a, #>((_propfont_width - 0x0020))
	adc	a, #0x00
	ld	h, a
	ld	a, (hl)
	ld	e, a
	
;drawstring.c:22: while (width)
	ld	l,-2 (ix)
	ld	h,-1 (ix)

; does this glyph fit completely in byte?
    sub a, c
    jr NC, prewidthloop   
; pre-decrement target space
    ld a, c
    sub a, e
    ld c, a   
    ld a, d
; Loop for glyph's width
fastwidthloop:	
	; Rotate pixel through carry to output byte
	rla
	rl b
	
	dec e
	jr NZ, fastwidthloop	
    jr chardone    

prewidthloop:
    ld a, d
; Loop for glyph's width	
widthloop:
	
	; Rotate pixel through carry to output byte
	rla
	rl b
	
; Decrement pixels that fit in target
	dec	c
	jr	NZ,bytefull
; we're full, flush byte to screen
	ld	c, #0x08
	ld	(hl), b
	inc	hl
bytefull:
; Decrement width, loop if not done with glyph
	dec e
	jr NZ, widthloop

chardone:
; store hl (dest ptr)
	ld	-2 (ix),l
	ld	-1 (ix),h

; next char, check if we're at end of string
	inc iy
	ld a, (iy)
	or	a, a
	jr	NZ,nextchar
lastchar:

; do we have leftover data?
	ld	a, c
	sub	a, #0x08
	jr	Z, evenbits
; Do we have at least 5 pixels to go?
	ld	a,c
	sub	a, #0x05
	jr	C, lastshifts
; Do 4 pixels on one go
	dec	c
	dec	c
	dec	c
	dec	c
	ld a,b
	add a,a
	add a,a
	add a,a
	add a,a
	ld	b,a
; Do the last pixels one at a time
lastshifts:
	sla	b
	dec	c
	jr	NZ,lastshifts
; Put last byte on screen
	ld	(hl),b
evenbits:


; Move destination base pointer down one line
	inc	-5 (ix)
; 16 bit inc of the data pointer to access the next layer    
	inc	-4 (ix)	
	jr	NZ,dataadd16
	inc	-3 (ix)
dataadd16:
; Check if we've done all 8 lines
	dec	-7 (ix)
	jp NZ, rowloop
	ld	sp, ix
	pop iy
	pop	ix
	ret
