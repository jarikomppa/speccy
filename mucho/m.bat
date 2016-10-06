cls
del mucho.tap
del *.rel
del crt0.ihx
sdasz80 -xlos -g crt0.rel crt0.s
sdcc -c -o app.rel app.c -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return 
sdcc -mz80 --no-std-crt0 --opt-code-speed --nostdlib --code-loc 0xd032 --data-loc 0xf000 -Wl -b_HEADER=0xd000 crt0.rel app.rel 
..\tools\mackarel crt0.ihx mucho.tap mucho -nosprestore -noei -lowblock lowblock.dat 0x5b00
speccy mucho.tap
