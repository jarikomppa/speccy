;; Part of Jari Komppa's zx spectrum suite
;; https://github.com/jarikomppa/speccy
;; released under the unlicense, see http://unlicense.org 
;; (practically public domain)

;;	screen_unpacker.s

	.module boot

	.area _HEADER(ABS)
	.org 23810

    di
        
    ld de, #0x4000 ; destination position
    ld hl, #datastart ; source position ofs+basic+unpacker=23759+69+262
    call dzx7_smartrcs
    ei
    ret
.include "dzx7_smartRCS.s"
datastart: