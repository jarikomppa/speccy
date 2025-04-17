
; -------------------------------------------------------------------------------------------------
; basically a "any key" routine. Considered also supporting kempston, but that's tricky.

; return a=1 if key down, a=0 if not.
; Could do early out, but prefer about constant runtime
scankeys:
    push hl
    push bc
	ld l, 0xff
	ld bc, 0xfefe
	in a, (c)
	and l
	ld l, a
	ld bc, 0xfdfe
	in a, (c)
	and l
	ld l, a
	ld bc, 0xfbfe
	in a, (c)
	and l
	ld l, a
	ld bc, 0xf7fe
	in a, (c)
	and l
	ld l, a
	ld bc, 0xeffe
	in a, (c)
	and l
	ld l, a
	ld bc, 0xdffe
	in a, (c)
	and l
	ld l, a
	ld bc, 0xbffe
	in a, (c)
	and l
	ld l, a
	ld bc, 0x7ffe
	in a, (c)
	and l
	and 31
    pop bc
    pop hl
	cp 31
	jr nz, .keydown
	ld a, 0
	ret
.keydown:
	ld a, 1
	ret

; -------------------------------------------------------------------------------------------------
; pseudorandom generation routine. no reason to reinvent the wheel... this time

; from https://wikiti.brandonw.net/index.php?title=Z80_Routines:Math:Random
prng16:
;Inputs:
;   (seed1) contains a 16-bit seed value
;   (seed2) contains a NON-ZERO 16-bit seed value
;Outputs:
;   HL is the result
;   BC is the result of the LCG, so not that great of quality
;   DE is preserved
;Destroys:
;   AF
;cycle: 4,294,901,760 (almost 4.3 billion)
;160cc
;26 bytes
    ld hl,(.seed1)
    ld b,h
    ld c,l
    add hl,hl
    add hl,hl
    inc l
    add hl,bc
    ld (.seed1),hl
    ld hl,(.seed2)
    add hl,hl
    sbc a,a
    and %00101101
    xor l
    ld l,a
    ld (.seed2),hl
    add hl,bc
    ret
.seed1:
	db 0x08, 0x04
.seed2:
	db 0x20, 0x25


; -------------------------------------------------------------------------------------------------
; generate a random list of fishes to give player a goal

genfishlist:
	call prng16
	ld a, h
	and 7 
	inc a ; let's say 1-8 fishes..
	ld ix, fishlist
	ld (ix), a
    ld (fishlistsize), a
	inc ix
	ld b, a
.rep:
	push bc
	call prng16
	ld a, l
	and 15
	cp 9
	pop bc
	jr nc, .rep ; over 9, try again
	inc a ; fish numbers start from 1
    ld (ix), a
	inc ix
	djnz .rep
	ret


; -------------------------------------------------------------------------------------------------
; finding screen offset based on 8x8 cell address

; b = x, c = y
; out: hl=screen position
screenofs8x8:
; screen coordinate bits go like this:
;           H          |            L
; 0 1 0 Y7 Y6 Y2 Y1 Y0 | Y5 Y4 Y3 X4 X3 X2 X1 X0
	ld a, c
	; rotate the coordinate right three bits (to get to Y3, Y4, Y5 in L)	
	rrca
	rrca
	rrca        
	; AND any additional bits off
	and 0xe0
	; Add in the x offset
	add a, b        
	ld l, a  ; coordinate bottom byte done
	; next we do the same for Y6 and Y7; no need to shift because we're in the
	; right place.
	ld a, c
	and 0x18 ; AND extra bits off, and h is done.
	ld h, a
	ld bc, SCREEN
	add hl, bc  ; now hl points at the screen offset we want
	ret


; -------------------------------------------------------------------------------------------------
; and for putpixel we need 1x1 pixel accuracy... fought with the routine for a while, figured someone already wrote it better.

; from http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/screen-memory-layout
; Get screen address
;  B = Y pixel position
;  C = X pixel position
; Returns address in HL
;
Get_Pixel_Address:      LD A,B          ; Calculate Y2,Y1,Y0
                        AND %00000111   ; Mask out unwanted bits
                        OR %01000000    ; Set base address of screen
                        LD H,A          ; Store in H
                        LD A,B          ; Calculate Y7,Y6
                        RRA             ; Shift to position
                        RRA
                        RRA
                        AND %00011000   ; Mask out unwanted bits
                        OR H            ; OR with Y2,Y1,Y0
                        LD H,A          ; Store in H
                        LD A,B          ; Calculate Y5,Y4,Y3
                        RLA             ; Shift to position
                        RLA
                        AND %11100000   ; Mask out unwanted bits
                        LD L,A          ; Store in L
                        LD A,C          ; Calculate X4,X3,X2,X1,X0
                        RRA             ; Shift into position
                        RRA
                        RRA
                        AND %00011111   ; Mask out unwanted bits
                        OR L            ; OR with Y5,Y4,Y3
                        LD L,A          ; Store in L
                        RET
