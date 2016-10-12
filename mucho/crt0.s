		;;	zx spectrum ram startup code
		.module crt0
		.globl _heap
		.globl _stack

		.area _HEADER(ABS)
	
		ld sp, #0xffff ;#_stack

		;call gsinit			; init static vars (sdcc style)

		;; start the os
		;call _main			
		jp _main

		;;	(linker documentation:) where specific ordering is desired - 
		;;	the first linker input file should have the area definitions 
		;;	in the desired order
		.area _HOME
		.area _CODE
	        .area _GSINIT
	        .area _GSFINAL	
		.area _DATA
	        .area _BSS
	        .area _HEAP

		;;	this area contains data initialization code -
		;;	unlike gnu toolchain which generates data, sdcc generates 
		;;	initialization code for every initialized global 
		;;	variable. and it puts this code into _GSINIT area
        	.area _GSINIT
gsinit:	
        	.area _GSFINAL
        	;ret

		.area _DATA

		.area _BSS

		;; 2048 bytes of operating system stack
		;.ds	1024
_stack::
		.area _HEAP
_heap::
