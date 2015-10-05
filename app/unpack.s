;; Part of Jari Komppa's zx spectrum suite
;; https://github.com/jarikomppa/speccy
;; released under the unlicense, see http://unlicense.org 
;; (practically public domain)

;   void lzf_unpack(unsigned char *src, unsigned short len, unsigned char *dst)
;   {
;       unsigned short idx = 0;
;       while (idx < len)
;       {
;           unsigned char op = src[idx];
;           unsigned short runlen = op >> 5;
;           port254(runlen);
;           idx++;
;           if (runlen == 0)
;           {
;               // literals
;               runlen = (op & 31) + 1;
;               
;               cp(dst,runlen, src+idx);
;               dst += runlen;
;               idx += runlen;                      
;           }
;           else
;           {
;               // run
;               unsigned short ofs = ((op & 31) << 8) | src[idx];
;               unsigned char * runsrc;
;               idx++;
;               if (runlen == 7)
;               {
;                   // long run
;                   runlen = src[idx] + 7;
;                   idx++;
;               }
;               runlen += 2;
;               runsrc = dst - ofs - 1;
;               cp(dst, runlen, runsrc);
;               dst += runlen;
;           }            
;       }
;   }
;   
    .module unpack
_lzf_unpack::
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
	ld	l,4 (ix)
	ld	h,5 (ix)
	add	hl,bc
	ld	h,(hl)
	ld	a,h
	rlca
	rlca
	rlca
	and	a,#0x07
	ld	-2 (ix),a
	ld	-1 (ix),#0x00
	ld	l,-2 (ix)
	out (254), a
	inc	bc
	ld	-4 (ix),c
	ld	-3 (ix),b
	ld	a,4 (ix)
	add	a, -4 (ix)
	ld	c,a
	ld	a,5 (ix)
	adc	a, -3 (ix)
	ld	b,a
	ld	a,h
	and	a, #0x1F
	ld	e,a
	ld	d,#0x00
	ld	a,-1 (ix)
	or	a,-2 (ix)
	jr	NZ, not_literal
	inc	de
	push	de
	push	bc
	push	de
	ld	l,8 (ix)
	ld	h,9 (ix)
	push	hl
	pop de
	pop bc
	pop hl
	ldir	
	pop	de
	ld	a,8 (ix)
	add	a, e
	ld	8 (ix),a
	ld	a,9 (ix)
	adc	a, d
	ld	9 (ix),a
	ld	a,-4 (ix)
	add	a, e
	ld	c,a
	ld	a,-3 (ix)
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
	jr	NZ,00102$
	ld	a,-1 (ix)
	or	a, a
	jr	NZ,00102$
	ld	l,4 (ix)
	ld	h,5 (ix)
	add	hl,bc
	ld	e,(hl)
	ld	d,#0x00
	ld	hl,#0x0007
	add	hl,de
	ld	-2 (ix),l
	ld	-1 (ix),h
	inc	bc
00102$:
	ld	e,-2 (ix)
	ld	d,-1 (ix)
	inc	de
	inc	de
	ld	a,8 (ix)
	sub	a, -6 (ix)
	ld	l,a
	ld	a,9 (ix)
	sbc	a, -5 (ix)
	ld	h,a
	dec	hl
	push	bc
	push	de
	push	hl
	push	de
	ld	l,8 (ix)
	ld	h,9 (ix)
	push	hl
	pop de
	pop bc
	pop hl
	ldir	
	pop	de
	pop	bc
	ld	a,8 (ix)
	add	a, e
	ld	8 (ix),a
	ld	a,9 (ix)
	adc	a, d
	ld	9 (ix),a
	jr	next_data_island
done:
	ld	sp, ix
	pop	ix
    ret
_lzf_unpack_ends::    
