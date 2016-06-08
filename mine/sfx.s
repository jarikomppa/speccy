
;BeepFX player by Shiru
;You are free to do whatever you want with this code



;   void playfx(unsigned short fx) __z88dk_fastcall;
; assuming fx = bc
    .module playfx
_playfx::
    push hl
    pop bc
    
	ld hl,#sfxData	;address of sound effects data

	push ix
	push iy

	add hl,bc
	add hl,bc
	ld e,(hl)
	inc hl
	ld d,(hl)
	push de
	pop ix			;put it into ix

readData:
	ld a, 0 (ix)		;read block type
	ld c, 1 (ix)		;read duration 1
	ld b, 2 (ix)
	ld e, 3 (ix)		;read duration 2
	ld d, 4 (ix)
	push de
	pop iy

	dec a
	jr z, sfxRoutineTone
	dec a
	jr z, sfxRoutineNoise
	dec a
	jr z, sfxRoutineSample
	pop iy
	pop ix
	ret

;play sample

sfxRoutineSample:
	ex de,hl
sfxRS0:
	ld e,#8
	ld d,(hl)
	inc hl
sfxRS1:
	ld a, 5 (ix)
sfxRS2:
	dec a
	jr nz,sfxRS2
	rl d
	sbc a,a
	and #16
sfxRoutineSampleBorder:
	or #0
	out (254),a
	dec e
	jr nz,sfxRS1
	dec bc
	ld a,b
	or c
	jr nz,sfxRS0

	ld c,#6
	
nextData:
	add ix,bc		;skip to the next block
	jr readData



;generate tone with many parameters

sfxRoutineTone:
	ld e, 5 (ix)		;freq
	ld d, 6 (ix)
	ld a, 9 (ix)		;duty
	ld (#sfxRoutineToneDuty+1),a
	ld hl,#0

sfxRT0:
	push bc
	push iy
	pop bc
sfxRT1:
	add hl,de
	ld a,h
sfxRoutineToneDuty:
	cp #0
	sbc a,a
	and #16
sfxRoutineToneBorder:
	or #0
	out (254),a

	dec bc
	ld a,b
	or c
	jr nz,sfxRT1

	ld a,(sfxRoutineToneDuty+1)	 ;duty change
	add a, 10 (ix)
	ld (sfxRoutineToneDuty+1),a

	ld c, 7 (ix)		;slide
	ld b, 8 (ix)
	ex de,hl
	add hl,bc
	ex de,hl

	pop bc
	dec bc
	ld a,b
	or c
	jr nz,sfxRT0

	ld c,#11
	jr nextData



;generate noise with two parameters

sfxRoutineNoise:
	ld e, 5 (ix)		;pitch

	ld d,#1
	ld h,d
	ld l,d
sfxRN0:
	push bc
	push iy
	pop bc
sfxRN1:
	ld a,(hl)
	and #16
sfxRoutineNoiseBorder:
	or #0
	out (254),a
	dec d
	jr nz,sfxRN2
	ld d,e
	inc hl
	ld a,h
	and #31
	ld h,a
sfxRN2:
	dec bc
	ld a,b
	or c
	jr nz,sfxRN1

	ld a,e
	add a, 6 (ix)	;slide
	ld e,a

	pop bc
	dec bc
	ld a,b
	or c
	jr nz,sfxRN0

	ld c,#7
	jr nextData


sfxData:

SoundEffectsData:
	.dw SoundEffect0Data
	.dw SoundEffect1Data

SoundEffect0Data:
	.db 1 ;tone
	.dw 1,2000,2000,0,128
	.db 1 ;pause
	.dw 1,1000,0,0,0
	.db 1 ;tone
	.dw 1,2000,1000,0,128
	.db 0
SoundEffect1Data:
	.db 2 ;noise
	.dw 50,100,768
	.db 0