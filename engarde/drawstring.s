	.module drawstring
	.globl _drawstringz
	.area _CODE

	
	;           7(ix) = aY
	;           6(ix) = aX
	;   4(ix)   5(ix) = aS
	;  -1(ix)  -2(ix) = bd
	;  -3(ix)  -4(ix) = datap
	;  -5(ix)  -6(ix) = s
	;          -7(ix) = g
	;          -8(ix) = i
	;          -9(ix) = pixofs
	; -10(ix) -11(ix) = d
	;         -12(ix) = w
	
	
; void drawstringz(unsigned char *aS, unsigned char aX, unsigned char aY)
_drawstringz::
	push ix
	ld	ix,#0
	add	ix,sp
	ld	hl,#-13
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
	ld	-2 (ix),a
	ld	a,b
	adc	a, #0x00
	ld	-1 (ix),a
                        ;drawstring.c:10: for (i = 0; i < 8; i++)
	ld	-8 (ix),#0x08

rowloop:
                        ;drawstring.c:12: unsigned char *d = bd;
	ld	a,-2 (ix)
	ld	-11 (ix),a
	ld	a,-1 (ix)
	ld	-10 (ix),a
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
	ld	-7 (ix),a
                        ;drawstring.c:22: if (g)
	or	a, a
	jr	Z,fast_emptydata
                        ;drawstring.c:24: unsigned short si = (unsigned short)g * 2 + pixofs * 2;
	ld l,a
	ld	h,#0x00
	add	hl, hl
	ex	de,hl
                        ;drawstring.c:25: *d |= shiftp[si];
	ld	l,-2 (ix)
	ld	h,-1 (ix)
	ld	c,(hl)
	ld	hl,#(_propfont + 0x034e)
	add	hl,de
	ld	a,(hl)
	or	a, c
	ld	l,-2 (ix)
	ld	h,-1 (ix)
	ld	(hl),a
                        ;drawstring.c:26: pixofs += w;
fast_emptydata:
	ld	a,-12 (ix)
	ld	-9 (ix),a
                        ;drawstring.c:32: s++;                    
	ld	c,-6 (ix)
	ld	b,-5 (ix)
	inc	bc
                        ;drawstring.c:35: while (*s)
charloop:
	ld	a,(bc)
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
	ld	a,-9 (ix)
	add	a, d
	ld	-7 (ix),a
                        ;drawstring.c:40: if (g)
	ld	a,e
	or	a, a
	jr	Z,emptydata
                        ;drawstring.c:42: unsigned short si = (unsigned short)g * 2 + pixofs * 2;
	ld	h,#0x00
	add	hl, hl
	ex	de,hl
	ld	l,-9 (ix)
	ld	h,#0x00
	add	hl, hl
	add	hl,de
	ex	de,hl
                        ;drawstring.c:43: *d |= shiftp[si];
	ld	l,-11 (ix)
	ld	h,-10 (ix)
	ld	a,(hl)
	ld	-6 (ix),a
	ld	hl,#(_propfont + 0x034e)
	add	hl,de
	ld	a,(hl)
	or	a, -6 (ix)
	ld	l,-11 (ix)
	ld	h,-10 (ix)
	ld	(hl),a
                        ;drawstring.c:44: pixofs += w;
	ld	a,-7 (ix)
	ld	-9 (ix),a
                        ;drawstring.c:45: if (pixofs > 7)
	ld	a,#0x07
	sub	a, -9 (ix)
	jr	NC,withinbyte
                        ;drawstring.c:47: d++; 
	inc	-11 (ix)
	jr	NZ,add16bit
	inc	-10 (ix)
add16bit:
                        ;drawstring.c:48: *d |= shiftp[si+1];
	ld	l,-11 (ix)
	ld	h,-10 (ix)
	ld	a,(hl)
	ld	-6 (ix),a
	inc	de
	ld	hl,#(_propfont + 0x034e)
	add	hl,de
	ld	a,(hl)
	or	a, -6 (ix)
	ld	l,-11 (ix)
	ld	h,-10 (ix)
	ld	(hl),a
                        ;drawstring.c:49: pixofs &= 7;
	ld	a,-9 (ix)
	and	a, #0x07
	ld	-9 (ix),a
	jr	withinbyte
emptydata:
                        ;drawstring.c:54: pixofs += w;
	ld	a,-7 (ix)
	ld	-9 (ix),a
                        ;drawstring.c:55: if (pixofs > 7)
	ld	a,#0x07
	sub	a, -9 (ix)
	jr	NC,withinbyte
                        ;drawstring.c:57: pixofs &= 7;
	ld	a,-9 (ix)
	and	a, #0x07
	ld	-9 (ix),a
                        ;drawstring.c:58: d++;
	inc	-11 (ix)
	jr	NZ,add16bit2
	inc	-10 (ix)
add16bit2:
withinbyte:
                        ;drawstring.c:61: s++;                    
	inc	bc
	jp	charloop
endofstring:
                        ;drawstring.c:63: datap += 94;
	ld	a,-4 (ix)
	add	a, #0x5e
	ld	-4 (ix),a
	ld	a,-3 (ix)
	adc	a, #0x00
	ld	-3 (ix),a
                        ;drawstring.c:64: bd += 0x0100;
	inc -1 (ix)
                        ;drawstring.c:10: for (i = 0; i < 8; i++)
	dec	-8 (ix)
	jp	NZ,rowloop
	ld	sp, ix
	pop	ix
	ret
