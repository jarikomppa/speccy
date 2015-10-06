sdasz80 -xlos -g boot.rel boot.s
sdld -i -b _HEADER=52945 boot.rel
..\tools\ihx2bin boot.ihx boot.bin
..\tools\lzfpack ..\app\app.bin app.bin.lzf
copy /b boot.bin+app.bin.lzf combined.bin
..\tools\bootpack combined.bin -nc
call call_appmake.bat
speccy combined.tap
