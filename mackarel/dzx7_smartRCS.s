; -----------------------------------------------------------------------------
; "Smart" integrated RCS+ZX7 decoder by Einar Saukas (110 bytes)
; -----------------------------------------------------------------------------
; Parameters:
;   HL: source address (compressed data)
;   DE: destination address (decompressing)
; -----------------------------------------------------------------------------

dzx7_smartrcs:
        ld      a, #0x80
dzx7r_copy_byte_loop:
        call    dzx7r_copy_byte         ; copy literal byte
dzx7r_main_loop:
        call    dzx7r_next_bit
        jr      nc, dzx7r_copy_byte_loop ; next bit indicates either literal or sequence

; determine number of bits used for length (Elias gamma coding)
        push    de
        ld      bc, #0
        ld      d, b
dzx7r_len_size_loop:
        inc     d
        call    dzx7r_next_bit
        jr      nc, dzx7r_len_size_loop

; determine length
dzx7r_len_value_loop:
        call    nc, dzx7r_next_bit
        rl      c
        rl      b
        jr      c, dzx7r_exit           ; check end marker
        dec     d
        jr      nz, dzx7r_len_value_loop
        inc     bc                      ; adjust length

; determine offset
        ld      e, (hl)                 ; load offset flag (1 bit) + offset value (7 bits)
        inc     hl
        .db    #0xcb, #0x33            ; opcode for undocumented instruction "SLL E" aka "SLS E"
        jr      nc, dzx7r_offset_end    ; if offset flag is set, load 4 extra bits
        ld      d, #0x10                  ; bit marker to load 4 bits
dzx7r_rld_next_bit:
        call    dzx7r_next_bit
        rl      d                       ; insert next bit into D
        jr      nc, dzx7r_rld_next_bit  ; repeat 4 times, until bit marker is out
        inc     d                       ; add 128 to DE
        srl	d			; retrieve fourth bit from D
dzx7r_offset_end:
        rr      e                       ; insert fourth bit into E

; copy previous sequence
        ex      (sp), hl                ; store source, restore destination
        push    hl                      ; store destination
        sbc     hl, de                  ; HL = destination - offset - 1
        pop     de                      ; DE = destination
dzx7r_copy_bytes:
        push    hl
        ex      de, hl
        call    dzx7r_convert
        ex      de, hl
        call    dzx7r_copy_byte
        pop     hl
        inc     hl
        jp      pe, dzx7r_copy_bytes
        pop     hl                      ; restore source address (compressed data)
        jr      dzx7r_main_loop

dzx7r_next_bit:
        add     a, a                    ; check next bit
        ret     nz                      ; no more bits left?
        ld      a, (hl)                 ; load another group of 8 bits
        inc     hl
        rla
        ret

dzx7r_copy_byte:
        push    de
        call    dzx7r_convert
        ldi
dzx7r_exit:
        pop     de
        inc     de
        ret

; Convert an RCS address 010RRccc ccrrrppp to screen address 010RRppp rrrccccc
; (note: replace both EX AF,AF' with PUSH AF/POP AF if you want to preserve AF')
dzx7r_convert:
        ex      af, af'
        ld      a, d                    ; A = 010RRccc
        cp      #0x58
        jr      nc, dzx7r_skip
        xor     e
        and     #0xf8
        xor     e                       ; A = 010RRppp
        push    af
        xor     d
        xor     e                       ; A = ccrrrccc
        rlca
        rlca                            ; A = rrrccccc
        pop     de                      ; D = 010RRppp
        ld      e, a                    ; E = rrrccccc
dzx7r_skip:
        ex      af, af'
        ret

; -----------------------------------------------------------------------------
