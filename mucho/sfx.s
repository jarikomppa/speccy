
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
	.dw #SoundEffect0Data
	.dw #SoundEffect1Data
	.dw #SoundEffect2Data
	.dw #SoundEffect3Data
	.dw #SoundEffect4Data
	.dw #SoundEffect5Data
	.dw #SoundEffect6Data
	.dw #SoundEffect7Data
	.dw #SoundEffect8Data
	.dw #SoundEffect9Data
	.dw #SoundEffect10Data
	.dw #SoundEffect11Data
	.dw #SoundEffect12Data
	.dw #SoundEffect13Data
	.dw #SoundEffect14Data
	.dw #SoundEffect15Data
	.dw #SoundEffect16Data
	.dw #SoundEffect17Data
	.dw #SoundEffect18Data
	.dw #SoundEffect19Data
	.dw #SoundEffect20Data
	.dw #SoundEffect21Data
	.dw #SoundEffect22Data
	.dw #SoundEffect23Data
	.dw #SoundEffect24Data
	.dw #SoundEffect25Data
	.dw #SoundEffect26Data
	.dw #SoundEffect27Data
	.dw #SoundEffect28Data
	.dw #SoundEffect29Data
	.dw #SoundEffect30Data
	.dw #SoundEffect31Data
	.dw #SoundEffect32Data
	.dw #SoundEffect33Data
	.dw #SoundEffect34Data
	.dw #SoundEffect35Data
	.dw #SoundEffect36Data
	.dw #SoundEffect37Data
	.dw #SoundEffect38Data
	.dw #SoundEffect39Data
	.dw #SoundEffect40Data
	.dw #SoundEffect41Data
	.dw #SoundEffect42Data
	.dw #SoundEffect43Data
	.dw #SoundEffect44Data
	.dw #SoundEffect45Data
	.dw #SoundEffect46Data
	.dw #SoundEffect47Data
	.dw #SoundEffect48Data
	.dw #SoundEffect49Data
	.dw #SoundEffect50Data
	.dw #SoundEffect51Data
	.dw #SoundEffect52Data
	.dw #SoundEffect53Data
	.dw #SoundEffect54Data
	.dw #SoundEffect55Data

SoundEffect0Data:
	.db 1 ;tone
	.dw 20,50,2000,65486,128
	.db 0
SoundEffect1Data:
	.db 2 ;noise
	.dw 20,50,257
	.db 0
SoundEffect2Data:
	.db 1 ;tone
	.dw 100,20,500,2,128
	.db 0
SoundEffect3Data:
	.db 1 ;tone
	.dw 100,20,500,2,16
	.db 0
SoundEffect4Data:
	.db 1 ;tone
	.dw 10,100,2000,100,128
	.db 0
SoundEffect5Data:
	.db 1 ;tone
	.dw 50,100,200,65531,128
	.db 0
SoundEffect6Data:
	.db 2 ;noise
	.dw 100,50,356
	.db 0
SoundEffect7Data:
	.db 2 ;noise
	.dw 1,1000,10
	.db 2 ;noise
	.dw 1,1000,1
	.db 0
SoundEffect8Data:
	.db 2 ;noise
	.dw 1,1000,20
	.db 1 ;pause
	.dw 1,1000,0,0,0
	.db 2 ;noise
	.dw 1,1000,1
	.db 0
SoundEffect9Data:
	.db 1 ;tone
	.dw 20,1000,200,0,63104
	.db 0
SoundEffect10Data:
	.db 1 ;tone
	.dw 400,50,200,0,63104
	.db 0
SoundEffect11Data:
	.db 1 ;tone
	.dw 2000,10,400,0,63104
	.db 0
SoundEffect12Data:
	.db 1 ;tone
	.dw 100,100,1000,0,32896
	.db 0
SoundEffect13Data:
	.db 1 ;tone
	.dw 1000,10,100,0,25728
	.db 0
SoundEffect14Data:
	.db 1 ;tone
	.dw 100,100,1000,0,32640
	.db 0
SoundEffect15Data:
	.db 1 ;tone
	.dw 100,20,400,1,25728
	.db 0
SoundEffect16Data:
	.db 2 ;noise
	.dw 2,2000,32776
	.db 0
SoundEffect17Data:
	.db 2 ;noise
	.dw 1,1000,10
	.db 1 ;tone
	.dw 20,100,400,65526,128
	.db 2 ;noise
	.dw 1,2000,1
	.db 0
SoundEffect18Data:
	.db 1 ;tone
	.dw 100,20,1000,65535,2176
	.db 0
SoundEffect19Data:
	.db 2 ;noise
	.dw 20,2000,1290
	.db 0
SoundEffect20Data:
	.db 2 ;noise
	.dw 100,400,2562
	.db 0
SoundEffect21Data:
	.db 2 ;noise
	.dw 5,1000,5124
	.db 1 ;tone
	.dw 50,100,200,65534,128
	.db 0
SoundEffect22Data:
	.db 2 ;noise
	.dw 8,200,20
	.db 2 ;noise
	.dw 4,2000,5220
	.db 0
SoundEffect23Data:
	.db 2 ;noise
	.dw 25,2500,28288
	.db 0
SoundEffect24Data:
	.db 2 ;noise
	.dw 100,40,20
	.db 1 ;tone
	.dw 100,40,400,65532,128
	.db 2 ;noise
	.dw 100,40,40
	.db 1 ;tone
	.dw 100,40,350,65532,128
	.db 2 ;noise
	.dw 100,40,80
	.db 1 ;tone
	.dw 100,40,320,65532,128
	.db 2 ;noise
	.dw 100,40,100
	.db 1 ;tone
	.dw 100,40,310,65532,128
	.db 2 ;noise
	.dw 100,40,120
	.db 1 ;tone
	.dw 100,40,300,65532,128
	.db 0
SoundEffect25Data:
	.db 2 ;noise
	.dw 5,1000,5130
	.db 1 ;tone
	.dw 20,100,200,65526,128
	.db 2 ;noise
	.dw 1,10000,200
	.db 0
SoundEffect26Data:
	.db 1 ;tone
	.dw 8,400,300,65511,128
	.db 2 ;noise
	.dw 6,5000,5270
	.db 0
SoundEffect27Data:
	.db 2 ;noise
	.dw 1,1000,4
	.db 1 ;tone
	.dw 4,1000,400,65436,128
	.db 2 ;noise
	.dw 1,5000,150
	.db 0
SoundEffect28Data:
	.db 1 ;tone
	.dw 10,400,1000,65136,2688
	.db 0
SoundEffect29Data:
	.db 1 ;tone
	.dw 10,400,1000,65336,2688
	.db 0
SoundEffect30Data:
	.db 1 ;tone
	.dw 4,1000,1000,400,128
	.db 0
SoundEffect31Data:
	.db 1 ;tone
	.dw 4,1000,1000,65136,128
	.db 0
SoundEffect32Data:
	.db 1 ;tone
	.dw 1,1000,1000,0,128
	.db 1 ;pause
	.dw 1,1000,0,0,0
	.db 1 ;tone
	.dw 1,2000,2000,0,128
	.db 1 ;tone
	.dw 1,2000,2000,0,16
	.db 0
SoundEffect33Data:
	.db 1 ;tone
	.dw 1,1000,2000,0,64
	.db 1 ;pause
	.dw 1,1000,0,0,0
	.db 1 ;tone
	.dw 1,1000,1500,0,64
	.db 0
SoundEffect34Data:
	.db 2 ;noise
	.dw 1,1000,4
	.db 1 ;tone
	.dw 1,1000,2000,0,128
	.db 0
SoundEffect35Data:
	.db 2 ;noise
	.dw 1,1000,8
	.db 1 ;tone
	.dw 1,1000,800,0,128
	.db 2 ;noise
	.dw 1,1000,16
	.db 1 ;tone
	.dw 1,1000,700,0,128
	.db 0
SoundEffect36Data:
	.db 1 ;tone
	.dw 10,400,400,65516,128
	.db 1 ;pause
	.dw 10,400,0,0,0
	.db 1 ;tone
	.dw 10,400,350,65516,96
	.db 1 ;pause
	.dw 10,400,0,0,0
	.db 1 ;tone
	.dw 10,400,300,65516,64
	.db 1 ;pause
	.dw 10,400,0,0,0
	.db 1 ;tone
	.dw 10,400,250,65516,32
	.db 1 ;pause
	.dw 10,400,0,0,0
	.db 1 ;tone
	.dw 10,400,200,65516,16
	.db 0
SoundEffect37Data:
	.db 1 ;tone
	.dw 5,1800,1000,1000,65408
	.db 0
SoundEffect38Data:
	.db 1 ;tone
	.dw 3500,10,2,0,25728
	.db 0
SoundEffect39Data:
	.db 1 ;tone
	.dw 20,200,3400,10,64
	.db 0
SoundEffect40Data:
	.db 1 ;tone
	.dw 1,2000,400,0,128
	.db 1 ;tone
	.dw 1,2000,400,0,16
	.db 1 ;tone
	.dw 1,2000,600,0,128
	.db 1 ;tone
	.dw 1,2000,600,0,16
	.db 1 ;tone
	.dw 1,2000,800,0,128
	.db 1 ;tone
	.dw 1,2000,800,0,16
	.db 0
SoundEffect41Data:
	.db 1 ;tone
	.dw 1,2000,400,0,128
	.db 1 ;tone
	.dw 1,2000,600,0,128
	.db 1 ;tone
	.dw 1,2000,800,0,128
	.db 1 ;tone
	.dw 1,2000,400,0,16
	.db 1 ;tone
	.dw 1,2000,600,0,16
	.db 1 ;tone
	.dw 1,2000,800,0,16
	.db 0
SoundEffect42Data:
	.db 1 ;tone
	.dw 4,1000,500,100,384
	.db 1 ;tone
	.dw 4,1000,500,100,64
	.db 1 ;tone
	.dw 4,1000,500,100,16
	.db 0
SoundEffect43Data:
	.db 1 ;tone
	.dw 5,2000,200,100,128
	.db 0
SoundEffect44Data:
	.db 1 ;tone
	.dw 4,1000,400,100,128
	.db 1 ;tone
	.dw 4,1000,400,100,64
	.db 1 ;tone
	.dw 4,1000,400,100,32
	.db 1 ;tone
	.dw 4,1000,400,100,16
	.db 0
SoundEffect45Data:
	.db 1 ;tone
	.dw 4,2000,600,65436,61504
	.db 1 ;tone
	.dw 4,2000,600,65436,8
	.db 1 ;tone
	.dw 4,2000,600,65436,4
	.db 0
SoundEffect46Data:
	.db 1 ;tone
	.dw 2,4000,400,200,64
	.db 1 ;tone
	.dw 2,4000,200,200,32
	.db 0
SoundEffect47Data:
	.db 1 ;tone
	.dw 2,1000,400,100,64
	.db 1 ;tone
	.dw 2,1000,400,100,64
	.db 1 ;tone
	.dw 2,1000,400,100,64
	.db 1 ;tone
	.dw 2,1000,400,100,64
	.db 0
SoundEffect48Data:
	.db 1 ;tone
	.dw 32,1000,2000,16384,320
	.db 0
SoundEffect49Data:
	.db 1 ;tone
	.dw 200,20,400,0,384
	.db 1 ;tone
	.dw 200,20,800,0,384
	.db 1 ;tone
	.dw 200,20,400,0,384
	.db 1 ;tone
	.dw 200,20,800,0,384
	.db 1 ;tone
	.dw 200,20,400,0,384
	.db 1 ;tone
	.dw 200,20,800,0,384
	.db 0
SoundEffect50Data:
	.db 1 ;tone
	.dw 20,100,200,10,1025
	.db 1 ;pause
	.dw 30,100,0,0,0
	.db 1 ;tone
	.dw 50,100,200,10,1025
	.db 0
SoundEffect51Data:
	.db 1 ;tone
	.dw 50,200,500,65516,128
	.db 0
SoundEffect52Data:
	.db 1 ;tone
	.dw 1,2000,200,0,128
	.db 1 ;pause
	.dw 1,2000,0,0,0
	.db 1 ;tone
	.dw 1,2000,200,0,32
	.db 1 ;pause
	.dw 1,2000,0,0,0
	.db 1 ;tone
	.dw 1,2000,200,0,16
	.db 1 ;pause
	.dw 1,2000,0,0,0
	.db 1 ;tone
	.dw 1,2000,200,0,8
	.db 0
SoundEffect53Data:
	.db 1 ;tone
	.dw 10,1000,200,2,272
	.db 1 ;pause
	.dw 1,4000,0,0,0
	.db 1 ;tone
	.dw 10,1000,200,65534,272
	.db 0
SoundEffect54Data:
	.db 1 ;tone
	.dw 20,500,200,5,272
	.db 1 ;pause
	.dw 1,1000,0,0,0
	.db 1 ;tone
	.dw 30,500,200,8,272
	.db 0
SoundEffect55Data:
	.db 1 ;tone
	.dw 40,2125,16384,45459,128
	.db 0
