cls
del mucho.tap
del *.rel
del crt0.ihx
sdasz80 -xlos -g crt0.rel crt0.s
sdasz80 -xlos -g dzx7_standard.rel dzx7_standard.s
sdasz80 -xlos -g sfx.rel sfx.s
sdcc -c -o app.rel app.c -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return 
sdcc -mz80 --no-std-crt0 --opt-code-speed --nostdlib --code-loc 0xe032 --data-loc 0xfa00 -Wl -b_HEADER=0xe000 crt0.rel app.rel dzx7_standard.rel sfx.rel
..\tools\mackarel crt0.ihx mucho.tap mucho -nosprestore -noei -lowblock simple.dat 0x5b00
speccy mucho.tap
