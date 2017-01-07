cls
del mazeract.tap
del *.rel
del crt0.ihx
sdasz80 -xlos -g crt0.rel crt0.s
sdasz80 -xlos -g sfx.rel sfx.s
sdcc -c -o app.rel app.c -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return 
sdcc -mz80 --no-std-crt0 --opt-code-speed --nostdlib --code-loc 0x7032 --data-loc 0x5b00 -Wl -b_HEADER=0x7000 crt0.rel app.rel sfx.rel
..\tools\mackarel crt0.ihx mazeract.tap Mazeract loader.scr -nosprestore -noei
speccy mazeract.tap
