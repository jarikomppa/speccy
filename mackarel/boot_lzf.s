;;	boot.s

		.module boot

		.area _HEADER(ABS)
        .org #0x4100
    
.include "lzfunpack.s"