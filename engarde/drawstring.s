	.module drawstring
	.globl _drawstringz
	.area _CODE

	
	;           7(ix) = aY
	;           6(ix) = aX
	;   4(ix)   5(ix) = aS
	;  -1(ix)  -2(ix) = bd			5 -> stack
	;  -3(ix)  -4(ix) = datap		5
	;  -5(ix)  -6(ix) = s			3
	;          -7(ix) = g			4 -> b
	;          -8(ix) = i			2
	;          -9(ix) = pixofs		11 -> c
	; -10(ix) -11(ix) = d			7 -> iy
	;         -12(ix) = w			2
	
	
; void drawstringz(unsigned char *aS, unsigned char aX, unsigned char aY)
_drawstringz::
	push ix
	ld	ix,#0
	add	ix,sp
	ld	hl,#-13
	push iy
	add	hl,sp
	ld	sp,hl
                        ;drawstring.c:5: const unsigned char *datap = (unsigned char*)(propfont + 94 - 32); // font starts from space (32)
	ld	-4 (ix),#<((_propfont + 0x003e))
	ld	-3 (ix),#>((_propfont + 0x003e))
                        ;drawstring.c:6: const unsigned char *widthp = (unsigned char*)(propfont - 32);
                        ;drawstring.c:7: const unsigned char *shiftp = (unsigned char*)(propfont + 846);
                        ;drawstring.c:8: unsigned char *bd = (unsigned char*)yofs[aY * 8] + aX;
	ld	bc,#_yofs+0
	ld	l,7 (ix)
	ld	h,#0x00
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl,bc
	ld	c,(hl)
	inc	hl
	ld	b,(hl)
	ld	a,c
	add	a, 6 (ix)
	ld l, a
	ld	a,b
	adc	a, #0x00
	ld h, a
	push hl
                        ;drawstring.c:10: for (i = 0; i < 8; i++)
	ld	-8 (ix),#0x08

rowloop:
                        ;drawstring.c:12: unsigned char *d = bd;
	pop iy
	push iy

                        ;drawstring.c:13: unsigned char *s = aS;        
	ld	a,4 (ix)
	ld	-6 (ix),a
	ld	a,5 (ix)
	ld	-5 (ix),a
                        ;drawstring.c:19: unsigned char ch = *s;
	ld	l,-6 (ix)
	ld	h,-5 (ix)
	ld	c,(hl)
                        ;drawstring.c:20: unsigned char w = widthp[ch];
	ld	hl,#(_propfont - 0x0020)
	ld	b,#0x00
	add	hl, bc
	ld	a,(hl)
	ld	-12 (ix),a
                        ;drawstring.c:21: unsigned char g = datap[ch];
	ld	l,-4 (ix)
	ld	h,-3 (ix)
	ld	b,#0x00
	add	hl, bc
	ld	a,(hl)
	ld	b,a
                        ;drawstring.c:22: if (g)
	or	a, a
	jr	Z,fast_emptydata
                        ;drawstring.c:24: unsigned short si = (unsigned short)g * 2 + pixofs * 2;
	ld l,a
	ld	h,#0x00
	add	hl, hl
	ex	de,hl
                        ;drawstring.c:25: *d |= shiftp[si];
	pop hl
	push hl
	ld	c,(hl)
	ld	hl,#(_propfont + 0x034e)
	add	hl,de
	ld	a,(hl)
	or	a, c
	pop hl
	push hl	
	ld	(hl),a
                        ;drawstring.c:26: pixofs += w;
fast_emptydata:
	ld	a,-12 (ix)
	ld	c,a
                        ;drawstring.c:32: s++;                    
                        ;drawstring.c:35: while (*s)
	ld	l,-6 (ix)
	ld	h,-5 (ix)
	push hl
charloop:
	pop hl
	inc	hl
	push hl
	ld	a,(hl)
	ld	e,a
	or	a, a
	jp	Z,endofstring
                        ;drawstring.c:37: unsigned char ch = *s;
                        ;drawstring.c:38: unsigned char w = widthp[ch];
	ld	hl,#(_propfont - 0x0020)
	ld	d,#0x00
	add	hl, de
	ld	d,(hl)
                        ;drawstring.c:39: unsigned char g = datap[ch];
	ld	a,-4 (ix)
	add	a, e
	ld	l,a
	ld	a,-3 (ix)
	adc	a, #0x00
	ld	h,a
	ld	e,(hl)
	ld	l,e
                        ;drawstring.c:44: pixofs += w;
	ld	a,c
	add	a, d
	ld	b,a
                        ;drawstring.c:40: if (g)
	ld	a,e
	or	a, a
	jr	Z,emptydata
                        ;drawstring.c:42: unsigned short si = (unsigned short)g * 2 + pixofs * 2;
	ld	h,#0x00
	add	hl, hl
	ex	de,hl
	ld	l,c
	ld	h,#0x00
	add	hl, hl
	add	hl,de
	ex	de,hl
                        ;drawstring.c:43: *d |= shiftp[si];
	ld	a,(iy)
	ld	-6 (ix),a
	ld	hl,#(_propfont + 0x034e)
	add	hl,de
	ld	a,(hl)
	or	a, -6 (ix)
	ld	(iy),a
                        ;drawstring.c:44: pixofs += w;
	ld	c,b
                        ;drawstring.c:45: if (pixofs > 7)
	bit 3, b
	jr Z, withinbyte
                        ;drawstring.c:47: d++; 
	inc iy	
						;drawstring.c:48: *d |= shiftp[si+1];
	ld	a,(iy)
	ld	-6 (ix),a
	inc	de
	ld	hl,#(_propfont + 0x034e)
	add	hl,de
	ld	a,(hl)
	or	a, -6 (ix)
	ld	(iy),a
                        ;drawstring.c:49: pixofs &= 7;
	ld	a,c
	and	a, #0x07
	ld	c,a
	jr	withinbyte
emptydata:
                        ;drawstring.c:54: pixofs += w;
	ld	c,b
                        ;drawstring.c:55: if (pixofs > 7)
	bit 3, b
	jr Z, withinbyte
                        ;drawstring.c:57: pixofs &= 7;
	ld	a,c
	and	a, #0x07
	ld	c,a
                        ;drawstring.c:58: d++;
	inc	iy
withinbyte:
                        ;drawstring.c:61: s++;                    
	jp	charloop
endofstring:
	pop hl
                        ;drawstring.c:63: datap += 94;
	ld	a,-4 (ix)
	add	a, #0x5e
	ld	-4 (ix),a
	ld	a,-3 (ix)
	adc	a, #0x00
	ld	-3 (ix),a
                        ;drawstring.c:64: bd += 0x0100;
	pop de
	inc d
	push de
                        ;drawstring.c:10: for (i = 0; i < 8; i++)
	dec	-8 (ix)
	jp	NZ,rowloop
	pop iy
	ld	sp, ix
	pop	ix
	ret
