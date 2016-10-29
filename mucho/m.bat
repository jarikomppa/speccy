cls
del mucho.tap
del *.rel
del crt0.ihx
sdasz80 -xlos -g crt0.rel crt0.s
sdasz80 -xlos -g dzx7_standard.rel dzx7_standard.s
sdcc -c -o app.rel app.c -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return 
rem 0xfe00 leaves 512 bytes for data & stack
sdcc -mz80 --no-std-crt0 --opt-code-speed --nostdlib --code-loc 0xe006 --data-loc 0xfe00 -Wl -b_HEADER=0xe000 crt0.rel app.rel dzx7_standard.rel
copy crt0.ihx out\kit_03\
mc test.txt test.dat
..\tools\mackarel crt0.ihx mucho.tap mucho -nosprestore -noei -lowblock test.dat 0x5b00 -q -screencodec zx7
speccy mucho.tap
