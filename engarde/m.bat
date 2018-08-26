cls
@del engarde.tap
@del *.rel
@del crt0.ihx
..\cc\sdcc350\bin\sdasz80 -xlos -g crt0.rel crt0.s
..\cc\sdcc350\bin\sdcc -c -o app.rel app.c -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return 
@if NOT EXIST "app.rel" goto end
..\cc\sdcc350\bin\sdcc -mz80 --no-std-crt0 --opt-code-speed --nostdlib --code-loc 0x6007 --data-loc 0x5b00  -Wl -b_HEADER=0x6000 crt0.rel app.rel
@if NOT EXIST "crt0.ihx" goto end
..\tools\mackarel crt0.ihx engarde.tap Engarde loader.scr -nosprestore -noei
@if NOT EXIST "engarde.tap" goto end
speccy engarde.tap
:end
