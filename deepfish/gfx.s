; -------------------------------------------------------------------------------------------------
; Draw the player's fish database, skipping on any zeros to leave the hints alone
drawfishdb:
	ld ix, fishdbmin
	ld hl, 5 + 256 * 26
	ld b, 10
.rep:
	push bc	
	ld a, (ix)	
	cp 0
	jr z, .skip
	push hl	
	ld bc, hl
	push bc
	call drawnum
	pop bc
	inc b	
	inc b
	ld a, '-'
	push bc
	call drawchar
	pop bc
	inc b	
	ld a, (ix+16)	
	call drawnum
	pop hl
.skip:	
	inc l
	inc l
	inc ix
	pop bc
	djnz .rep
	ret	

; -------------------------------------------------------------------------------------------------
; draw hints, "top" for near the top, "deep" for near the bottom

drawfishhints:
	ld ix, fishmin
	ld hl, 5 + 256 * 26
	ld b, 10
.rep:
	push bc	
	ld a, (ix)	
	cp 10
	jr nc, .nottop
    push hl
    push bc
    push de
    ld bc, hl
    ld hl, str_top
    call drawstring
    pop de
    pop bc
    pop hl
    jr .next
.nottop:
    cp 22
    jr c, .notdeep
    push hl
    push bc
    push de
    ld bc, hl
    ld hl, str_deep
    call drawstring
    pop de
    pop bc
    pop hl
.notdeep:
.next:	
	inc l
	inc l
	inc ix
	pop bc
	djnz .rep
	ret	

str_top:
    db "  top",0
str_deep:
    db " deep",0

; -------------------------------------------------------------------------------------------------
; Debug print stuff. Not used in in the final binary.

; ix = input
drawdebugnum:
	ld hl, 0
	ld b, 16
.rep:
	push bc	
	ld a, (ix)	
	push hl	
	ld bc, hl
	call drawnum
	pop hl
	inc l
	inc ix
	pop bc
	djnz .rep
	ret	


; -------------------------------------------------------------------------------------------------
; putpixel. the routine I've spent years telling people not to use. and here it is. uses xor, so it can used to undo.

; hl=pixel x/y
putpixel:
	ld bc, hl
	push hl
	call Get_Pixel_Address
	ld bc, hl
	pop hl
	ld a, l ; x bottom bits
	and 7
	ld de, bitofs
	ld h, 0
	ld l, a
	add hl, de
	ld a, (bc)
	ld d, (hl)
	xor d
	ld (bc), a 
	ret

bitofs:
	db 128,64,32,16,8,4,2,1

; -------------------------------------------------------------------------------------------------
; draw a two-digit number

; a=number
; bc=screen pos
drawnum:
	ld l, ' '	
	cp 10
	jr c, .lt9
	ld l, '0' - 1
.tens:
	inc l
	sub 10
	jr nc, .tens
	add 10
.lt9:
	push af
	push bc
	ld a, l
	call drawchar
	pop bc
	pop af
	ld l, '0'
.ones:
	inc l
	sub 1
	jr nz, .ones
	ld a, l
	inc b
	call drawchar
	ret

; -------------------------------------------------------------------------------------------------
; draw an asciiz string

; hl=asciiz
; bc=screen pos
drawstring:
	ld a, (hl)
	cp 0
	ret z
	push hl
	push bc
	call drawchar
	pop bc
	pop hl
	inc hl
	inc b
	jr drawstring

; -------------------------------------------------------------------------------------------------
; draw a character from the ROM font

; a=char
; bc=screen pos
drawchar:
	ld l, a
	ld h, 0
	add hl, hl ; *2
	add hl, hl ; *4
	add hl, hl ; *8
	ex de, hl
	call screenofs8x8
	ex de, hl
	ld bc, 0x3d00 - 32*8; ROM font from space
	add hl, bc
	ld b, 8
.rep:
	ld a, (hl)
	xor 255     ; assets and screen paper/ink are swapped, too lazy to fix assets, so here we go
	ld (de), a
	inc hl
	inc d
	djnz .rep
	ret


; -------------------------------------------------------------------------------------------------
; copy fish from screen to screen

; hl=src, de=dest
copyfish:
	ld bc, hl
	call screenofs8x8
	push hl
	ld bc, de
	call screenofs8x8
	ld de, hl
	pop hl
	ld b, 8
.rep:
	push bc
	ld bc, 7
	ldir
	ld bc, 256-7
	add hl, bc
	ex de,hl
	add hl, bc
	ex de,hl
	pop bc
	djnz .rep
	ret

; -------------------------------------------------------------------------------------------------
; draw enough empty spaces to clear fish from fishlist

; de=dest
clearfish:
	ld bc, 23+11*256
	call screenofs8x8
	push hl
	ld bc, de
	call screenofs8x8
	ld de, hl
	pop hl
	ld b, 8
.rep:
	push bc
	ld bc, 10
	ldir
	ld bc, 256-10
	add hl, bc
	ex de,hl
	add hl, bc
	ex de,hl
	pop bc
	djnz .rep
	ret

; -------------------------------------------------------------------------------------------------
; do the little animation while pulling the line up

tugrod:
	ld hl, (rodindex)
	ld a, l
	xor 16
	ld l, a
	ld (rodindex), hl
	ld bc, rodframes
	add hl, bc
	ld de, SCREEN+6+32*4; x=6, y=4
	ld b,8
.rep:
	ld a, (hl)
	xor 255
	ld (de), a
	inc hl
	inc d
	djnz .rep
	ld de, SCREEN+6+32*5; x=6, y=5
	ld b,8
.rep2:
	ld a, (hl)
	xor 255     ; assets and screen paper/ink are swapped, too lazy to fix assets, so here we go
	ld (de), a
	inc hl
	inc d
	djnz .rep2
	ret

; -------------------------------------------------------------------------------------------------
; draw tickmark for when fish is found. Could be a generic copy routine, but, eh.

drawtick:
	call screenofs8x8
	ld de, hl
	ld hl, tickimg
	ld b,8
.rep:
	ld a, (hl)
	xor 255     ; assets and screen paper/ink are swapped, too lazy to fix assets, so here we go
	ld (de), a
	inc hl
	inc d
	djnz .rep


; -------------------------------------------------------------------------------------------------
; fix the paper/ink for the faces. Should have just gone with the xor 255 here too for consistency.

facecolor:
	ld hl, 14 + COLOR
	ld de, 32-2
	ld b, 3
	ld a, 0x40+(7*8)
.rep:
	ld (hl), a
	inc hl
	ld (hl), a
	inc hl
	ld (hl), a
	add hl, de
	djnz .rep
	ret

; -------------------------------------------------------------------------------------------------
; a 3x3 cell copy routine. I could have just written one generic one.

drawface:
	; cycle faces
	ld hl, (faceindex)
	inc hl
	ld (faceindex), hl
	; restrict bound to 8 faces
	ld a, l
	and a, 7
	ld l, a
	ld h, 0
	; one face is 8*3*3 bytes
	ld bc, hl
	add hl, hl ; *2
	add hl, hl ; *4
	add hl, hl ; *8
    add hl, bc ; *9
	add hl, hl ; *9*2
	add hl, hl ; *9*4
	add hl, hl ; *9*8
	ld bc, faces
	add hl, bc
	ld de, 14 + SCREEN
	ld c, 3
.rerep:
	ld b, 8
.rep:
	push bc
	ldi
	ldi
	ldi
	ex de, hl
	ld bc, 256-3
	add hl, bc
	pop bc
	ex de, hl
	djnz .rep
	dec c	
	ret z

	push bc
	ex de,hl
	ld bc, 32-(8*256)
	add hl,bc
	ex de,hl
	pop bc
	jr .rerep

; -------------------------------------------------------------------------------------------------
; draw the list of fishes by calling copyfish

drawfishlist:
	ld ix, fishlist
	ld b, (ix)
	inc ix
	ld de, 7 + 256 * 13		
.rep:	
	ld a, (ix)
	add a, a
	add a, 2
	ld l, a
	ld h, 22
	push bc
	push de
	call copyfish: ; hl=src, de=dest
	pop de
	pop bc
	inc ix
	inc e
	inc e
	djnz .rep
	ret

; -------------------------------------------------------------------------------------------------
; clear the full fish list

clearfishlist:
	ld de, 7 + 256 * 12		
	ld b, 8
.rep:	
	push bc
	push de
	call clearfish: ; de=dest
	pop de
	pop bc
	inc e
	inc e
	djnz .rep
	ret
