	.module drawstring
	.optsdcc -mz80
	
	.globl _drawstringz
	.area _DATA
	.area _INITIALIZED
	.area _DABS (ABS)
	.area _HOME
	.area _GSINIT
	.area _GSFINAL
	.area _GSINIT
	.area _HOME
	.area _HOME
	.area _CODE

;drawstring.c:3: void drawstringz(unsigned char *aS, unsigned char aX, unsigned char aY)
_drawstringz::
	push	ix
	push iy
	ld	ix,#0
	add	ix,sp
	ld	hl,#-15
	add	hl,sp
	ld	sp,hl
;drawstring.c:6: const unsigned char *datap = (unsigned char*)propfont_data - 32 * 8; // font starts from space (32)
	ld	-9 (ix),#<((_propfont_data - 0x0100))
	ld	-8 (ix),#>((_propfont_data - 0x0100))
;drawstring.c:7: const unsigned char *widthp = (unsigned char*)propfont_width - 32;
;drawstring.c:8: aY *= 8;
	ld	a,9 (ix)
	rlca
	rlca
	rlca
	and	a,#0xf8
	ld	9 (ix),a
;drawstring.c:9: for (i = 0; i < 8; i++)
	ld	a,6 (ix)
	ld	-11 (ix),a
	ld	a,7 (ix)
	ld	-10 (ix),a
	ld	a,9 (ix)
	ld	-1 (ix),a
	ld	-12 (ix),#0x08
rowloop:
;drawstring.c:11: unsigned char ch = *aS;
	ld	c,-11 (ix)
	ld	b,-10 (ix)
	ld iy, #0x00
	add iy, bc
	
;drawstring.c:13: s = aS;
;drawstring.c:14: sx = 0;
	ld	c,#0x08
;drawstring.c:15: pd = 0;
;	ld	b,#0x00 ; not actually needed 
;drawstring.c:16: d = (unsigned char*)yofs[aY] + aX;
	ld	l,-1 (ix)
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
;drawstring.c:17: while (ch)
nextchar:
;drawstring.c:19: unsigned char data = datap[ch * 8];
	ld	l,(iy)
	ld	h,#0x00
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	e,-9 (ix)
	ld	d,-8 (ix)
	add	hl,de
	ld	a,(hl)
	ld d, a
;drawstring.c:20: unsigned char width = widthp[ch];
	ld	a,#<((_propfont_width - 0x0020))
	add	a, (iy)
	ld	l,a
	ld	a,#>((_propfont_width - 0x0020))
	adc	a, #0x00
	ld	h,a
	ld	a,(hl)
	ld	e,a
	
;drawstring.c:22: while (width)
	ld	l,-6 (ix)
	ld	h,-5 (ix)
	
widthloop:
	
;;drawstring.c:24: pd <<= 1;
;;drawstring.c:25: pd |= (data & 0x80) != 0;				
;;drawstring.c:26: data <<= 1;
	rl d
	rl b
	
;drawstring.c:28: sx++;
	dec	c
;drawstring.c:29: if (sx == 8)
;drawstring.c:31: sx = 0;
	jr	NZ,bytefull
	ld	c,#0x08
;drawstring.c:32: *d = pd;
	ld	(hl),b
;drawstring.c:33: d++;
	inc	hl
bytefull:
;drawstring.c:37: width--;
	dec e
	jr NZ, widthloop
nowidth:
	ld	-6 (ix),l
	ld	-5 (ix),h
;drawstring.c:40: s++;

;drawstring.c:41: ch = *s;
	inc iy
	ld a, (iy)
	or	a, a
	jr	NZ,nextchar
lastchar:


;drawstring.c:44: if (sx)
	ld	a,c
	sub	a, #0x08
	jr	Z,evenbits
;drawstring.c:46: if (sx < 4)
	ld	a,c
	sub	a, #0x05
	jr	C,lessthan4
;drawstring.c:48: sx += 4;
	dec	c
	dec	c
	dec	c
	dec	c
;drawstring.c:49: pd <<= 4;
	ld	a,b
	rlca
	rlca
	rlca
	rlca
	and	a,#0xf0
	ld	b,a
;drawstring.c:51: while (sx != 7)
lessthan4:
lastshifts:
;drawstring.c:53: pd <<= 1;
	sla	b
;drawstring.c:54: sx++;
	dec	c
	jr	NZ,lastshifts
shiftdone:


;drawstring.c:56: *d = pd;
	ld	l,-6 (ix)
	ld	h,-5 (ix)
	ld	(hl),b
evenbits:


;drawstring.c:59: aY++;
	inc	-1 (ix)
;drawstring.c:60: datap++;
	inc	-9 (ix)
	jr	NZ,dataadd16
	inc	-8 (ix)
dataadd16:
;drawstring.c:9: for (i = 0; i < 8; i++)
	dec	-12 (ix)
	jr	Z, done
	jp  rowloop
done:
	ld	sp, ix
	pop iy
	pop	ix
	ret
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
