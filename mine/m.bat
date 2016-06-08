cls
del rgbobots.tap
del *.rel
del crt0.ihx
..\tools\spriteproc 0.png c0.h
..\tools\spriteproc 1.png c1.h
..\tools\spriteproc 2.png c2.h
..\tools\spriteproc 3.png c3.h
..\tools\spriteproc 4.png c4.h
..\tools\spriteproc 5.png c5.h
..\tools\spriteproc 6.png c6.h
..\tools\spriteproc 7.png c7.h
..\tools\spriteproc 8.png c8.h
sdasz80 -xlos -g crt0.rel crt0.s
sdasz80 -xlos -g sfx.rel sfx.s
sdcc -c -o app.rel app.c -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return 
sdcc -mz80 --no-std-crt0 --opt-code-speed --nostdlib --code-loc 0xa032 --data-loc 0xd000 -Wl -b_HEADER=0xa000 crt0.rel sfx.rel app.rel 
copy /b bg.scr lowblock.dat
..\tools\mackarel crt0.ihx remines.tap R.E.Mines -nosprestore -noei -lowblock lowblock.dat 0x5b00
speccy remines.tap
