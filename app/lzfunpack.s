;; Part of Jari Komppa's zx spectrum suite
;; https://github.com/jarikomppa/speccy
;; released under the unlicense, see http://unlicense.org 
;; (practically public domain)

; void lzf_unpack(unsigned char *src, unsigned char *dst)
; {
;     unsigned short len = *(unsigned short*)src;
;     unsigned short idx = 0;
;     src++;
;     src++;
;     while (idx < len)
;     {
;         unsigned char op = src[idx];
;         unsigned short runlen = op >> 5;
;         port254(runlen);
;         idx++;
;         if (runlen == 0)
;         {
;             // literals
;             runlen = (op & 31) + 1;
;             
;             cp(dst,runlen, src+idx);
;             dst += runlen;
;             idx += runlen;                      
;         }
;         else
;         {
;             // run
;             unsigned short ofs = ((op & 31) << 8) | src[idx];
;             unsigned char * runsrc;
;             idx++;
;             if (runlen == 7)
;             {
;                 // long run
;                 runlen = src[idx] + 7;
;                 idx++;
;             }
;             runlen += 2;
;             runsrc = dst - ofs - 1;
;             cp(dst, runlen, runsrc);
;             dst += runlen;
;         }            
;     }
; }

;   HL: source address (compressed data)
;   DE: destination address (decompressing)
lzfunpack:
    ld c, (hl) ; lsb
    inc hl
    ld b, (hl) ; msb
    inc hl
    ; bc = data len
    push hl
    pop ix
    add ix, bc    
    push ix
    pop bc
    ; bc = source address + data len (i.e, end of data address)
nextdata:
    push bc
    ld a, (hl)
; no hl inc, we'll re-read the op later
    and #0xe0 ; top 3 bits
    cp #0xe0 ; are top bits full?
    jp z, longrun
    cp #0 ; are top bits zero?
    jp nz, run
; top bits zero - literals
    ld a, (hl)
    inc hl
    and #0x1f ; bottom 5 bits
    inc a
    ld c, a
    ld b, #0
    ; bc run length, hl source, de dest
    ldir
    jp next
run:
; run length: 3 top bits of op + 2
    rlca
    rlca
    rlca
    ld c, a
    ld b, #0
    ld ix, #2
    add ix, bc 
; run length now in ix (top bits, which aren't 0 nor 7, + 2)
; offset: ((bottom 5 bits << 8) | (next byte)) + 1
    ld a, (hl)
    inc hl
    and #0x1f ; bottom 5 bits
    ld b, a
    ld c, (hl)
    inc hl
    push hl
    inc bc
; offset now in bc, source data saved in stack
    ld hl, #0
    add hl, de    
    and a, a ; should clear carry
    sbc hl, bc ; carry should be zero
; hl is destination - offset    
    push ix
    pop bc
; bc is run length
; de remains dest    
    ldir
    pop hl ; restore source data
    jp next
longrun:
; offset: ((bottom 5 bits << 8) | (next byte)) + 1
    ld a, (hl)
    inc hl
    and #0x1f ; bottom 5 bits
    ld b, a
    ld c, (hl)
    inc hl
    inc bc
; bc now has offset, but we need bc, so move it aside
    push bc
    ld b, #0
    ld c, (hl)
    inc hl
    ld ix, #9 ; run count is x + 7 + 2
    add ix, bc
; ix is now long run count byte + 9
    pop bc   
; bc now has offset
    push hl
    ld hl, #0
    add hl, de
; hl is now dest    
    and a, a ; should clear carry
    sbc hl, bc ; carry should be zero
; hl is dest - offset    
    push ix
    pop bc
; bc is count
; de remains dest    
    ldir
    pop hl
; todo    
next: 
    pop bc
    ld a, c
    cp a, l    
    jp nz, nextdata
    ld a, b
    cp a, h
    jp nz, nextdata    
    ret

; C interface
    .module lzfunpack
_lzf_unpack::
    pop ix
    pop hl
    pop de
    push de
    push hl
    push ix
    
    call lzfunpack
    ret
