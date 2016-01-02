sdasz80 -xlos -g screen_unpacker.rel screen_unpacker.s
sdld -i -b _HEADER=23828 screen_unpacker.rel
..\tools\ihx2bin screen_unpacker.ihx screen_unpacker.bin
..\tools\bin2h screen_unpacker.bin screen_unpacker.h -noconst

sdasz80 -xlos -g boot.rel boot.s
sdld -i -b _HEADER=16354 boot.rel
..\tools\ihx2bin boot.ihx boot.bin
..\tools\bin2h boot.bin boot.h -noconst
