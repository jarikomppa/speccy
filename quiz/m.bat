cls
del qt48k01.tap
del *.rel
del crt0.ihx
sdasz80 -xlos -g crt0.rel crt0.s
sdasz80 -xlos -g sfx.rel sfx.s
sdcc -c -o app.rel app.c -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return 
sdcc -mz80 --no-std-crt0 --opt-code-speed --nostdlib --code-loc 0xc032 --data-loc 0xf000 -Wl -b_HEADER=0xc000 crt0.rel sfx.rel app.rel 
copy /b bg.scr+entertainment1.txt.packed lowblock.dat 
..\tools\mackarel crt0.ihx qt48k01.tap QT48k01 -nosprestore -noei -lowblock lowblock.dat 0x5b00
speccy qt48k01.tap
