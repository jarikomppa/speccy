sdasz80 -xlos -g screen_unpacker_lzf.rel screen_unpacker_lzf.s
sdld -i -b _HEADER=23828 screen_unpacker_lzf.rel
..\tools\ihx2bin screen_unpacker_lzf.ihx screen_unpacker_lzf.bin
..\tools\bin2h screen_unpacker_lzf.bin screen_unpacker_lzf.h -noconst

sdasz80 -xlos -g screen_unpacker_zx7.rel screen_unpacker_zx7.s
sdld -i -b _HEADER=23828 screen_unpacker_zx7.rel
..\tools\ihx2bin screen_unpacker_zx7.ihx screen_unpacker_zx7.bin
..\tools\bin2h screen_unpacker_zx7.bin screen_unpacker_zx7.h -noconst

sdasz80 -xlos -g screen_unpacker_rcs.rel screen_unpacker_rcs.s
sdld -i -b _HEADER=23828 screen_unpacker_rcs.rel
..\tools\ihx2bin screen_unpacker_rcs.ihx screen_unpacker_rcs.bin
..\tools\bin2h screen_unpacker_rcs.bin screen_unpacker_rcs.h -noconst

sdasz80 -xlos -g boot_lzf.rel boot_lzf.s
sdld -i -b _HEADER=16354 boot_lzf.rel
..\tools\ihx2bin boot_lzf.ihx boot_lzf.bin
..\tools\bin2h boot_lzf.bin boot_lzf.h -noconst

sdasz80 -xlos -g boot_zx7.rel boot_zx7.s
sdld -i -b _HEADER=16354 boot_zx7.rel
..\tools\ihx2bin boot_zx7.ihx boot_zx7.bin
..\tools\bin2h boot_zx7.bin boot_zx7.h -noconst
