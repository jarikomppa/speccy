cls
sdasz80 -xlos -g crt0.rel crt0.s
sdcc -c -o app.rel app.c -mz80 --no-std-crt0 --opt-code-speed
sdcc -mz80 --no-std-crt0 --opt-code-speed --nostdlib --code-loc 0x8032 --data-loc 0xc000 -Wl -b_HEADER=0x8000 crt0.rel app.rel
..\tools\mackarel crt0.ihx app.tap SolarGun -nosprestore -noei
speccy app.tap
