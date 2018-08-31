cls
@del engarde.tap
@del *.rel
@del crt0.ihx
..\cc\sdcc360\bin\sdasz80 -xlos -g crt0.rel crt0.s
@if NOT EXIST "crt0.rel" goto end
..\cc\sdcc372_10507\bin\sdcc -c -o primdraw.rel primdraw.c -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return 
@if NOT EXIST "primdraw.rel" goto end
..\cc\sdcc372_10507\bin\sdcc -c -o data.rel data.c -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return 
@if NOT EXIST "data.rel" goto end
..\cc\sdcc372_10507\bin\sdcc -c -o drawstring.rel drawstring.c -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return
@if NOT EXIST "drawstring.rel" goto end
..\cc\sdcc372_10507\bin\sdcc -c -o app.rel app.c -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return 
@if NOT EXIST "app.rel" goto end
..\cc\sdcc372_10507\bin\sdcc -mz80 --no-std-crt0 --opt-code-speed --nostdlib --code-loc 0x6007 --data-loc 0x5b00  -Wl -b_HEADER=0x6000 crt0.rel data.rel app.rel primdraw.rel drawstring.rel
@if NOT EXIST "crt0.ihx" goto end
..\tools\mackarel crt0.ihx engarde.tap Engarde loader.scr -nosprestore -noei
@if NOT EXIST "engarde.tap" goto end
speccy engarde.tap
:end
