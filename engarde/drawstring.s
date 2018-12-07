	.module drawstring
	.globl _drawstringz
	.area _CODE
	;  original temp    var name   uses   change/note
	;           7(ix) = aY                -> only in preamble
	;           6(ix) = aX                -> only in preamble
	;   4(ix)   5(ix) = aS                -> copy to local variable
	;  -1(ix)  -2(ix) = bd			5     -> local memory variable
	;  -3(ix)  -4(ix) = datap		5     -> de´   (hl´ needed for math)
	;  -5(ix)  -6(ix) = s			3     -> ix
	;          -7(ix) = g			4     -> b
	;          -8(ix) = i			2     -> local memory variable
	;          -9(ix) = pixofs		11    -> c
	; -10(ix) -11(ix) = d			7     -> bc´
	;         -12(ix) = w			2     -> juggled through h´
	; unused: iy
		
; void drawstringz(unsigned char *aS, unsigned char aX, unsigned char aY)
_drawstringz::
	push ix
	ld	ix,#0
	add	ix,sp

	; move datap to de´ (glyph data indices)
	exx
	ld	e,#<((_propfont + 0x003e))
	ld	d,#>((_propfont + 0x003e))
	exx
	
	; calculate pointer to screen, yofs from table
	ld	bc,#_yofs+0
	ld	l,7 (ix)
	ld	h,#0x00	
	add	hl, hl      ; aY * 8
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl,bc
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	
	ld	a,c
	add	a, 6 (ix)   ; + aX
	ld l, a
	ld	a,b
	adc	a, #0x00
	ld h, a
	
	; base target pointer calculated, store:
	ld (drawstringz_local_bd + 1), hl
	
	ld	l,4 (ix)
	ld	h,5 (ix)
	; copy of the string start pointer:
	ld (drawstringz_local_as + 2), hl

	; loop for 8 scanlines
	ld a, #0x08
	ld (drawstringz_local_i + 1), a

rowloop:
						   
	exx 
drawstringz_local_bd: ; start bc´ from destination base pointer
	ld bc, #1234 ; stored as self-modifying code
	exx
	
drawstringz_local_as: ; start ix from string start
	ld ix, #0x1234  ; stored as self-modifying code
												
	; get glyph data pattern number
	exx
	push de
	exx
	pop hl
	ld	c,(ix) ; c is our glyph
	ld	b,#0x00
	add	hl, bc
	ld	a,(hl)
	ld	b,a

	; if it's the first pattern, we can skip actually drawing it because it's empty
	or	a, a
	jr	Z,fast_emptydata

	; get the pre-shifted ghyph data from pattern index                       
	ld  l,a
	ld	h,#0x00
	add	hl, hl
	ex	de,hl        
	ld	hl,#(_propfont + 0x034e)
	add	hl,de
	ld	a,(hl)
	
	; this is the first thing to be plotted, no or:ing needed
	ld  hl, (drawstringz_local_bd + 1)
	ld	(hl),a

fast_emptydata:

	; get glyph width
	ld	hl,#(_propfont - 0x0020)
	ld	c,(ix) ; c is our glyph
	ld	b,#0x00
	add	hl, bc
	ld	c,(hl)

charloop:
	inc	ix              ; next char
	ld	a,(ix)
	ld	e,a
	; if zero, we're at end of string
	or	a, a
	jp	Z,endofstring

	; get glyph width
	ld	hl,#(_propfont - 0x0020)
	ld	d,#0x00
	add	hl, de
	ld	a,(hl)
						
	; get the glyph pattern number
	push de
	exx
	pop hl
	add hl, de
	push hl
	exx
	pop hl
	ld  d,a      ; d = width
	ld	l,(hl)   ; l = index
	
	; increment pixel offset 
	ld	a, c
	add	a, d
	ld	b, a ; store pixel offset in b temporarily

	; if glyph pattern is 0, skip drawing                        
	ld	a, l
	or	a, a
	jr	Z,emptydata

	; get the actual pixel data for the pattern, pre-shifted                        
	ld	h,#0x00
	add	hl, hl
	ex	de,hl
	ld	l,c ; old pixel offset
	ld	h,#0x00
	add	hl, hl
	add	hl, de
	ex	de, hl
	exx
	ld	a,(bc)
	exx
	ld	c, a
	ld	hl,#(_propfont + 0x034e)
	add	hl,de

	; "or" the pixels to screen                        
	ld	a,(hl)
	or	a, c
	exx
	ld	(bc),a
	exx

	ld	c,b ; set the new pixel offset
	; if pixel offset goes over byte boundary, plot the second pixel too
	bit 3, b
	jr Z, withinbyte
							
	exx
	inc bc	; move forward in screen 						
	ld	a,(bc) ; get current pixels
	exx
	ld	c, a
	inc	de
	ld	hl, #(_propfont + 0x034e)
	add	hl, de
	ld	a, (hl)
	or	a, c
	exx
	ld	(bc),a ; plot
	exx
	; keep pixel offset within a byte                        
	ld	a,b
	and	a, #0x07
	ld	c,a
	jr	withinbyte
emptydata:
	
	ld	c,b ; new pixel offset                            
	bit 3, b  ; check if we moved to next byte
	jr Z, withinbyte                        
	ld	a,c     ; new byte, keep pixel offset within it
	and	a, #0x07
	ld	c,a                       
	exx
	inc	bc ; move to next byte
	exx
withinbyte:

	jp	charloop

endofstring:

	; end of string, move to the next scanline

	exx
	ex de, hl
	ld de, #0x5e  ; next line of glyph data
	add hl, de
	ex de, hl
	exx

	; move destination base pointer one scanline down
	; (we're within 8 pixels so simple +256 is enough)
	ld hl, #drawstringz_local_bd + 2
	inc (hl)
	
	; was this the last scanline?

drawstringz_local_i:
	ld a, #0x12 ; stored as self-modifying code
	dec	a
	ld (drawstringz_local_i + 1),a
	jp	NZ,rowloop
	
	pop	ix
	ret
