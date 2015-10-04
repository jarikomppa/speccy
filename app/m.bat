sdasz80 -xlos -g unpack.rel unpack.s
sdasz80 -xlos -g sound.rel sound.s
sdasz80 -xlos -g crt0.rel crt0.s
sdcc -c -o app.rel app.c -mz80 --no-std-crt0 --opt-code-speed
sdcc -mz80 --no-std-crt0 --opt-code-speed --nostdlib --code-loc 0x6032 --data-loc 0xc000 -Wl -b_HEADER=0x6000 crt0.rel app.rel unpack.rel sound.rel
call objcopy -Iihex -Obinary crt0.ihx app.bin
appmake +zx --binfile app.bin --org 24576
speccy app.tap
