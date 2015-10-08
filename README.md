# Speccy

Here's a bunch of stuff I've cobbled together to make a zx spectrum toolchain. It's (still) (well, probably forever) a work in progress, but 
someone may find it useful.

Note that I'm doing this on windows, but what I've got together here may be helpful for figuring these things out on other platforms too.

## Dependencies

- z88dk toolchain - http://www.z88dk.org
- SDCC toolchain - http://sdcc.sourceforge.net/
- a zx spectrum emulator, I like speccy - http://fms.komkon.org/Speccy/

In speccy emulator, be sure to set hardware->fast tape loader on unless you want to die prematurely of old age.

## Handy pages and stuff

- http://wordpress.animatez.co.uk/computers/zx-spectrum/memory-map/
- http://wordpress.animatez.co.uk/computers/zx-spectrum/screen-memory-layout/
- http://www.z80.info/z80-op.txt
- http://www.z80.info/z80code.txt
- http://randomflux.info/1bit/viewtopic.php?id=21

Notes..

- rom character set is at 0x3d00 - 0x3ff and contain 8 bytes per glyph, starting from ascii 32 (space)
- i/o port 254 controls border color (bottom bits 0,1,2) and speaker (bit 4)
- scanline is 224 clocks
- after frame interrupt, 64 scanlines before first pixel
- 192 scanlines of pixels
- 56 scanlines before retrace
- 69888 clocks per frame
- border is 48 pixels wide
- one scanline: 128 clocks pixels, 24 clocks right border, 48 clocks retrace, 24 clocks left border
- different machines may vary by one clock
- contended memory and contended io-port 254 make the above values largely useless

### Memory map

Start | End  | Area
----- | ---- | --------
0000  | 3fff | ROM
4000  | 57ff | Screen bitmap memory
5800  | 5aff | Screen color memory
5b00  | 5bff | Printer buffer
5c00  | 5cbf | System variables
5cc0  | 5cca | Reserved
5ccb  | ff57 | Available memory
ff59  | ffff | Reserved

So we have about 40k of usable RAM normally. It should be possible to use from 5b00 to ffff though (~41k).

Memory below 0x8000 is "slow" because the video hardware has priority to it; fast(er) routines and data should
be in the upper portion.

### Bitmap data

There's 32*8=256 x 192 pixels on the screen.

The screen bitmap memory isn't linear. The addresses map to (see above links for prettier graph):

H                       | L
----------------------- | -----------------------
0 1 0 Y7 Y6 Y2 Y1 Y0 | Y5 Y4 Y3 X4 X3 X2 X1 X0

Each scanline is stored linearily. The Y coordinate is split so that printing single glyphs would be faster - just increment H by one and you're on the next scanline. That only works up to 8 scanlines though. I have no idea whatsoever why they would split the rest of the bits the way they did, but there you go.

Easiest way to deal with this is with a scanline start lookup table, but you could do the swizzling with code too.

### Color data

Color data is stored linearly, where each byte overlays a 8x8 block of bitmap data.

F B P2 P1 P0 I2 I1 I0

Where:
- F is for FLASH, swaps the PAPER and INK every 16 frames.
- B is for BRIGHT, changes both PAPER and INK.
- P2 to P0 is PAPER color (bitmap 0)
- I2 to I0 is INK color (bitmap 1)

Color number | Bright 0 | Bright 1 | Color name
------------ | -------- | -------- | ----------
0            | 0x000000 | 0x000000 | Black
1            | 0x0000CD | 0x0000FF | Blue
2            | 0xCD0000 | 0xFF0000 | Red
3            | 0xCD00CD | 0xFF00FF | Magenta
4            | 0x00CD00 | 0x00FF00 | Green
5            | 0x00CDCD | 0x00FFFF | Cyan
6            | 0xCDCD00 | 0xFFFF00 | Yellow
7            | 0xCDCDCD | 0xFFFFFF | White


## Tools

- bin2h.cpp - raw binary to c header file converter
- ihx2bin.cpp - intel hex file to raw binary converter (needed by m.bat in app directory)
- lzfpack.cpp - compressor for lzf format
- lzfunpack.cpp - decompressor for lzf format
- midi2h.cpp - tool that massages midi files into a format used by the experimental music routines, outputs c header file
- png2bin.cpp - PNG file to bitmap raw binary converter (doesn't deal with color)
- png2c.cpp - load up a .png file and output its monochrome bitmap as a .h file
- sinetab.cpp - tool that generates the sine table used by the effect in app.c
- tonetab.cpp - tool that generates frequency adder table used by the experimental music routines
- yofstab.cpp - tool that generates y offset table used by many routines in app.c
- bootpack.cpp - tool that patches bootloader to work with a compressed image

## App

The app directory contains a few files that will most likely not work as is on your system.

- app.c is a simple demo application that can be used as a template for greater things
- tab.h and s.h contain data for app.c
- crt0.s is a startup module I stole from https://github.com/tstih/yx/blob/master/apps/template/crt0.s
- setenv.bat sets the paths to z88dk and sdcc as well as z88dk's enviroment variables
- m.bat builds the application as a .tap file and launches it in emulator

m.bat in particular has a bunch of things you will want to know about;

1. Compile crt0.s and other assembly files to object files
2. Compile app.c to object file
3. Link objects into a intel hex (ihx) file
4. Convert ihx to raw binary using ihx2bin in tools directory
5. Use z88dk's appmake to turn binary to a .tap file
6. Run the result in emulator

Link time and appmake have some constants that will probably require tweaking from one application to another.

- "--code-loc 0x6032" - Offset of the code in the image. Image itself starts 0x32 bytes earlier, with the startup code going in the beginning.
- "-Wl -b_HEADER=0x6000" - Tell linker the start of the image.
- "--org 24576" - this is 0x6000 in hex. This and the two above values must correlate.
- "--data-loc 0xc000" - Offset of mutable variables. This shouldn't overlap with anything else or bad things will happen. Move as needed.

After compilation you will most likely want to look at crt0.map, as it shows how big everything is and what the offsets are set to, so you can spot issues like --data-loc being wrong.

## Bootloader

With the bootloader, the application is stored in a compressed form on tape, reducing the load times.

When the bootloader is in use, the image is loaded to the end of the memory. The bootloader first copies itself to the video memory and
continues running there. It decompresses the image to the desired base address and then jumps to it.

By default, the bootloader also sets the graphics attribute memory to zero to hide the garbage on the screen, and blinks the borders to see that progress is being made. The decompression is such a fast phase of the loading (1 second of unpacking, 1+ minutes of loading) that it may be desirable to disable one or both of these features - see commandline options for bootpack.exe for details.

## Musing on audio

The speccy has a piezo speaker that is controlled through high bit of port 254. It's entirely bit-banged. The speccy has one interrupt that triggers at 50hz and can't be changed. 

Sooo.. how does one do audio then?

By busy looping.

Sooo... background audio is out?

Pretty much. After pondering about this I checked a bunch of speccy gameplay videos on youtube and realized that all of the background audio is pretty choppy, meaning that they play a little bit of audio each frame but spend most of the time with the speaker silent. Kinda like arpeggio with most of the arpeggiated notes silent.

As a result, the audio routine now takes a fixed amount of frame time and outputs the desired frequency as many times as possible during that fixed time. The result kinda works. There's still room for improvement (even without taking more frame time) though.
