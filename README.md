# Speccy

Here's a bunch of stuff I've cobbled together to make a zx spectrum toolchain. It's (still) (well, probably forever) a work in progress, but 
someone may find it useful.

Note that I'm doing this on windows, but what I've got together here may be helpful for figuring these things out on other platforms too.

## Dependencies

- z88dk toolchain - http://www.z88dk.org
- SDCC toolchain - http://sdcc.sourceforge.net/
- cygwin or mingw, something with objcopy (or some other way to convert ihx to raw binary)
- a zx spectrum emulator, I like speccy - http://fms.komkon.org/Speccy/

In speccy emulator, be sure to set hardware->fast tape loader on unless you want to die prematurely of old age.

## Handy pages

- http://wordpress.animatez.co.uk/computers/zx-spectrum/memory-map/
- http://wordpress.animatez.co.uk/computers/zx-spectrum/screen-memory-layout/

## Tools

- mktab.cpp - generate video memory offset table and a sine wave table
- png2c.cpp - load up a .png file and output its monochrome bitmap as a .h file

## App

The app directory contains a few files that will most likely not work as is on your system.

- app.c is a simple demo application that can be used as a template for greater things
- tab.h and s.h contain data for app.c
- crt0.s is a startup module I stole from https://github.com/tstih/yx/blob/master/apps/template/crt0.s
- setenv.bat sets the paths to z88dk and sdcc as well as z88dk's enviroment variables
- m.bat builds the application as a .tap file and launches it in emulator

m.bat in particular has a bunch of things you will want to know about;

1. Compile crt0.s to object file
2. Compile app.c to object file
3. Link to intel hex (ihx) file
4. Convert ihx to raw binary
5. Use z88dk's appmake to turn binary to a .tap file
6. Run the result in emulator

Link time and appmake have some constants that will probably require tweaking from one application to another.

- "--code-loc 0x6032" - Offset of the code in the image. Image itself starts 0x32 bytes earlier, with the startup code going in the beginning.
- "-Wl -b_HEADER=0x6000" - Tell linker the start of the image.
- "--org 24576" - this is 0x6000 in hex. This and the two above values must correlate.
- "--data-loc 0xc000" - Offset of mutable variables. This shouldn't overlap with anything else or bad things will happen. Move as needed.

After compilation you will most likely want to look at crt0.map, as it shows how big everything is and what the offsets are set to, so you can spot issues like --data-loc being wrong.

## Musing on audio

The speccy has a piezo speaker that is controlled through high bit of port 254. It's entirely bit-banged. The speccy has one interrupt that triggers at 50hz and can't be changed. 

Sooo.. how does one do audio then?

By busy looping.

Sooo... background audio is out?

Pretty much. After pondering about this I checked a bunch of speccy gameplay videos on youtube and realized that all of the background audio is pretty choppy, meaning that they play a little bit of audio each frame but spend most of the time with the speaker silent. Kinda like arpeggio with most of the arpeggiated notes silent.
