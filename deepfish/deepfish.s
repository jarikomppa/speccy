        DEVICE ZXSPECTRUM48         ; Device setting for sjasmplus (.tap writing etc)
SCREEN  EQU $4000                   ; Location of screen
COLOR   EQU $5800                   ; Location of color array

        ORG $8000                   ; Let's start our code at 32k

; -------------------------------------------------------------------------------------------------
; Bootup
        di                          ; Disable interrupts
        ld  sp,     0x8000          ; Set stack to grow down from our code
        ld  de,     0xfe00          ; im2 vector table start right after color table
        ld  hl,     0xfdfd          ; where interrupt will point at
        ld  a,      d 
        ld  i,      a               ; interrupt will hop to 0xfe?? where ?? is random 
        ld  a,      l               ; we need 257 copies of the address
rep_isr_setup:
        ld  (de),   a 
        inc e
        jr  nz,     rep_isr_setup
        inc d                       ; just one more
        ld  (de),   a
        ld de,      isr
        ld  (hl),   0xc3            ; 0xc3 = JP
        inc hl
        ld  (hl),   e
        inc hl
        ld  (hl),   d
        im  2                       ; set the interrupt mode
        ei                          ; Enable interrupt

; -------------------------------------------------------------------------------------------------
; at this point we have loading screen up

main:
	; draw credits
	ld bc, 19 + 256 * 20
	ld hl, cred1
	call drawstring
	ld bc, 20 + 256 * 20
	ld hl, cred2
	call drawstring
	ld bc, 21 + 256 * 20
	ld hl, cred3
	call drawstring

	; draw "press any key"
	ld bc, 6 + 256 * 2
	ld hl, pressanykey
	call drawstring

; -------------------------------------------------------------------------------------------------
; Start key loop

.startkeyloop:
	call prng16 ; random state depends on how fast user hits a key
	call scankeys
	cp 1
	jr nz, .startkeyloop

	; Randomize	game values
	call prng16
	ld (faceindex), hl

	; give all fishes random ranges

	ld ix, fishrange
	ld b, 16
.fishrangeloop:
	push bc
	call prng16
	ld a,l
	and 7
	inc a
	inc a
	pop bc
	ld (ix), a
	inc ix
	djnz .fishrangeloop

	; randomize the offset based on the ranges

	ld ix, fishofs
	ld b, 16
.fishofsloop:
	push bc
	call prng16
	ld a, l
	and 31
	pop bc
	ld l, (ix-16) ; range
	add a, l
	cp 32
	jr nc, .fishofsloop ; ofs+range>32, try again
	sub l
	inc a
	ld (ix), a
	inc ix
	djnz .fishofsloop

	; data init done, prep visuals

	; copy bg to screen
	ld hl, bg
	ld de, SCREEN
	ld bc, 6912
	ldir

	; asset has paper and ink reversed, so swap that for the faces..

	call facecolor

	; turn ofs/range to min/max for easier checking later

	ld b, 16
	ld ix, fishofs
	ld hl, fishmax
.calcranges:
	ld c, (ix)
	ld a, (ix-16)
	add a, c
	ld (hl), a
	inc ix
	inc hl
	djnz .calcranges

; ..debugging stuff..
;	ld hl, fishmin
;	ld de, fishdbmin
;	ld bc, 32
;	ldir
;	call drawfishdb

	call drawfishhints

; ..more debugging stuff..

;	ld bc, 7 + 256 * 12	
;	call drawtick
	
;	ld hl, 4 + 256 * 22
;	ld de, 7 + 256 * 13	
;	call copyfish

;	ld bc, 7 + 256 * 13	
;	ld a, 4
;	call drawnum
;	ld hl, helloworld
;	call drawstring

; make sure key is up before the game actually starts

startkeyup:
	call scankeys
	cp 1
	jr z, startkeyup

; -------------------------------------------------------------------------------------------------
; game outer loop starts here

newclient:
	call drawface
	call genfishlist
	call clearfishlist

; ...debugging stuff...
;	ld a, 4
;	ld (fishlist), a
;	ld (fishlist+1), a
;	ld (fishlist+2), a
;	ld (fishlist+3), a
;	ld (fishlist+4), a
;	ld (fishlistsize), a
;	ld a, 0
;	ld (fishmin+3), a
;	ld a, 32
;	ld (fishmax+3), a

	call drawfishlist

; -------------------------------------------------------------------------------------------------
; actual main loop starts here

mainloop:

mainkeydown:
	call scankeys
	cp 1
	jr nz, mainkeydown
; while key is down, draw line down slowly, counting how deep we go
	ld hl, 60*256+56
	ld b, 0
.hookdown:
	call snd_click
	halt
	halt
	halt
	halt
	halt
	call scankeys
	cp 1
	jr nz, .hookrelease
	inc h
	inc h
	inc h
	inc h
	push hl
	push bc
	call putpixel
	pop bc
	pop hl
	inc b
	ld a, b
	cp 31
	jr nz, .hookdown
.maxdepthkeyup
	call scankeys
	cp 1
	jr z, .maxdepthkeyup
; when key is released, draw the line back without checking the key
.hookrelease:
	ld (hookdepth), bc
	ld a,b
	cp 0
	jp z, score
.hookup:
	push hl
	push bc
	ld a, b
	bit 0, b
	jr z, .skiptug
	call tugrod
.skiptug:	
	pop bc
	pop hl
	call snd_click
	halt
	halt
	push hl
	push bc
	call putpixel
	pop bc
	pop hl
	dec h
	dec h
	dec h
	dec h
	djnz .hookup
; once line is back up, do the scoring
score:
	ld bc, (hookdepth)
	; b = hook depth
	; populate Potential Fish Set
	call find_pfs
	; pick random fish (or none)
	call pick_fish
	ld (potentialfishset), a
	ld ix, potentialfishset
	;call drawdebugnum
	; update fishdb
	call updatefishdb
	; show found fish above player (blink a couple of times)
	call blinkfish
	; draw fishdb
	call drawfishdb
	; check fish against fishlist
	; tick a fish if matches
	call checkfishlist
	; if last fish to be found, jump to newclient
	ld a, (fishlistsize)
	cp 0
	jp z, newclient_snd
    jp mainloop

; main loop ends here
; -------------------------------------------------------------------------------------------------

; -------------------------------------------------------------------------------------------------
; play sound before generating new set of goals
newclient_snd:
	halt ; a bit of a pause to give more space for the sound
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	halt
	ld b, 5
	ld de, 0
.sndrep:
	halt
	halt
	halt
	push bc
	ld hl, 0x2060
	add hl, de
	call snd_beep
	ld hl, 0x3050
	add hl, de
	call snd_beep
	ld hl, 0x4040	
	add hl, de
	call snd_beep
	ld hl, 0x5030	
	add hl, de
	call snd_beep
	ex de, hl
	ld de, 0x00fc
	add hl, de
	ex de, hl
	pop bc
	djnz .sndrep
	jp newclient

; -------------------------------------------------------------------------------------------------
; click sound for the line going down and up
snd_click:
	push af
	push bc
	push hl
	ld hl, 0x0701
	call snd_beep
	pop hl
	pop bc
	pop af
	ret

; -------------------------------------------------------------------------------------------------
; generic beep routine
; hl = params
snd_beep:
	ld a, 16+7
	di
	out (0xfe), a
	ld b, h
.rep:
	cpl
	or 7
	out (0xfe), a
	ld c, l
.dlay:
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	dec c
	jr nz, .dlay
	djnz .rep
	ld a, 7
	out (0xfe), a
	ei
	ret

; -------------------------------------------------------------------------------------------------
; check if the caught fish is on the fishlist, check it 

checkfishlist:
	ld ix, fishlist
	ld a, (ix)
	ld b, a
	inc ix
	ld a, (potentialfishset)
	cp 0
	ret z ; no fish
	ld hl, 7 + 256 * 12	
.rep:
	cp (ix)
	jr z, .match
	inc ix
	inc l
	inc l
	djnz .rep
	ret
.match:
	ld (ix), 0 ; this fish is cooked	
	ld bc, hl
	call drawtick	
	ld a, (fishlistsize)
	dec a
	ld (fishlistsize), a
	ld b, 2
.sndrep:
	halt
	halt
	halt
	halt
	push bc
	ld hl, 0x2040
	call snd_beep
	ld hl, 0x4020	
	call snd_beep
	pop bc
	djnz .sndrep
	ret

; -------------------------------------------------------------------------------------------------
; blink the caught fish above the player

blinkfish:
	ld a, (potentialfishset)
	cp 0
	ret z ; no fish

	ld hl, 0x4080
	call snd_beep
	ld hl, 0x8040
	call snd_beep

	ld b, 5
.rep:
	ld de, 2*256+2
	ld a, (potentialfishset)
	add a, a
	add a, 2
	ld l, a
	ld h, 22
	push bc
	push de
	push hl
	call copyfish ; hl=src, de=dest
	pop hl
	pop de
	pop bc

	halt
	halt
	halt
	halt
	halt
	halt
	halt

	push bc
	push de
	push hl
	call clearfish
	pop hl
	pop de
	pop bc

	halt

	djnz .rep
	ret

; -------------------------------------------------------------------------------------------------
; Update player's fish database based on caught fish & depth

updatefishdb:
	ld a, (potentialfishset)
	cp 0
	ret z ; no fish, no update
	dec a
	ld bc, (hookdepth)
	inc b
	ld e, a
	ld d, 0
	ld ix, fishdbmin
	add ix, de
	ld a, (ix)
	cp 0
	jr z, .firsthit
	cp b
	jr c, .notmin
	ld a, b
	ld (ix), a
	ret ; can't grow both down and up
.notmin:
	ld a, (ix+16)
	cp b
	ret nc
	ld a, b
	ld (ix+16), a
	ret
.firsthit:
	ld a, b
	ld (ix), a
	ld (ix+16), a
	ret
	

; -------------------------------------------------------------------------------------------------
; pick a fish from the potential fish set.

pick_fish:
	ld a, (potentialfishset)
	cp 0
	ret z ; if no fish, no fish
	cp 1
	jr nz, .morethanone
	ld a, (potentialfishset+1)
	ret
.morethanone: ; if more than one fish, pick random one
	ld d, a ; 1..10
	dec d   ; 0..9
.retry:
	call prng16
	ld a, l
	and 15
	cp d  ; a(0..15) - d(0..9)
	jr nc, .retry ; rng result was too high, retry
	ld hl, potentialfishset
	inc hl
	ld e, a
	ld d, 0
	add hl, de
	ld a, (hl)
	ret

; -------------------------------------------------------------------------------------------------
; build potential fish set for a depth (this could be calced once for 32*11 bytes of storage, but who cares)

; b = hook depth
find_pfs:
; ..guess what? debug stuff..
;	ld a, 99
;	ld (potentialfishset), a
;	ld (potentialfishset+1), a
;	ld (potentialfishset+2), a
;	ld (potentialfishset+3), a
;	ld (potentialfishset+4), a
;	ld (potentialfishset+5), a
;	ld (potentialfishset+6), a
;	ld (potentialfishset+7), a
;	ld (potentialfishset+8), a
;	ld (potentialfishset+9), a
	
	ld a, b
	ld ix, fishmin
	ld hl, potentialfishset
	inc hl
	ld b, 10
	ld c, 1
	ld d, 0
.rep:
	cp (ix)
	jr c, .next ; less than min
	cp (ix+16)
	jr nc, .next ; more than max
	; fish found
	ld (hl), c
	inc hl
	inc d
.next:
	inc c
	inc ix
	djnz .rep
	ld a, d
	ld (potentialfishset), a
	ld ix, fishlist
	;call drawdebugnum
	ret

; -------------------------------------------------------------------------------------------------
; mostly data (and includes) after this point..

potentialfishset:
	block 16,0
hookpos:
	db 0,0
hookdepth:
	db 0,0

; -------------------------------------------------------------------------------------------------
; our super useful interrupt service routine

isr:                    ; This will be called ~50Hz
        ei
        reti            ; Return from interrupt


	INCLUDE "gfx.s"
	INCLUDE "utils.s"

fishrange:
	block 16,0
fishofs:
fishmin:
	block 16,0

fishmax:
	block 16,0

fishdbmin:
	block 16,0
fishdbmax:
	block 16,0	

fishlist:
	block 16,0
fishlistsize:
	db 0,0

pressanykey:
	db "PRESS ANY KEY",0
cred1:
	db "(C)2025",0
cred2:
	db "JARI KOMPPA",0
cred3:
	db "SOLHSA.COM",0
faceindex:
	db 0,0
rodindex:
	db 0,0
tickimg:
	INCBIN "tick.bin"
faces:
	INCBIN "faces.bin"
rodframes:
	INCBIN "rodframes.bin"
bg:
	INCBIN "bg.scr" ; todo: compress?
    	
    	SAVETAP "deepfish.tap", $8000   ; Save the assembled program as a tap file
