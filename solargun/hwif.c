/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

// Use HWIF_IMPLEMENTATION once in project

#define MKEYBYTE(x) KEYBYTE_ ## x
#define MKEYBIT(x) KEYBIT_ ## x
#define KEYUP(x) (keydata[MKEYBYTE(x)] & MKEYBIT(x))

enum KEYS
{
    KEYBIT_SHIFT = (1 << 0),
    KEYBIT_Z     = (1 << 1),
    KEYBIT_X     = (1 << 2),
    KEYBIT_C     = (1 << 3),
    KEYBIT_V     = (1 << 4),

    KEYBIT_A     = (1 << 0),
    KEYBIT_S     = (1 << 1),
    KEYBIT_D     = (1 << 2),
    KEYBIT_F     = (1 << 3),
    KEYBIT_G     = (1 << 4),

    KEYBIT_Q     = (1 << 0),
    KEYBIT_W     = (1 << 1),
    KEYBIT_E     = (1 << 2),
    KEYBIT_R     = (1 << 3),
    KEYBIT_T     = (1 << 4),

    KEYBIT_1     = (1 << 0),
    KEYBIT_2     = (1 << 1),
    KEYBIT_3     = (1 << 2),
    KEYBIT_4     = (1 << 3),
    KEYBIT_5     = (1 << 4),

    KEYBIT_0     = (1 << 0),
    KEYBIT_9     = (1 << 1),
    KEYBIT_8     = (1 << 2),
    KEYBIT_7     = (1 << 3),
    KEYBIT_6     = (1 << 4),

    KEYBIT_P     = (1 << 0),
    KEYBIT_O     = (1 << 1),
    KEYBIT_I     = (1 << 2),
    KEYBIT_U     = (1 << 3),
    KEYBIT_Y     = (1 << 4),

    KEYBIT_ENTER = (1 << 0),
    KEYBIT_L     = (1 << 1),
    KEYBIT_K     = (1 << 2),
    KEYBIT_J     = (1 << 3),
    KEYBIT_H     = (1 << 4),

    KEYBIT_SPACE = (1 << 0),
    KEYBIT_SYM   = (1 << 1),
    KEYBIT_M     = (1 << 2),
    KEYBIT_N     = (1 << 3),
    KEYBIT_B     = (1 << 4),


    KEYBYTE_SHIFT = 0,
    KEYBYTE_Z     = 0,
    KEYBYTE_X     = 0,
    KEYBYTE_C     = 0,
    KEYBYTE_V     = 0,

    KEYBYTE_A     = 1,
    KEYBYTE_S     = 1,
    KEYBYTE_D     = 1,
    KEYBYTE_F     = 1,
    KEYBYTE_G     = 1,

    KEYBYTE_Q     = 2,
    KEYBYTE_W     = 2,
    KEYBYTE_E     = 2,
    KEYBYTE_R     = 2,
    KEYBYTE_T     = 2,

    KEYBYTE_1     = 3,
    KEYBYTE_2     = 3,
    KEYBYTE_3     = 3,
    KEYBYTE_4     = 3,
    KEYBYTE_5     = 3,

    KEYBYTE_0     = 4,
    KEYBYTE_9     = 4,
    KEYBYTE_8     = 4,
    KEYBYTE_7     = 4,
    KEYBYTE_6     = 4,

    KEYBYTE_P     = 5,
    KEYBYTE_O     = 5,
    KEYBYTE_I     = 5,
    KEYBYTE_U     = 5,
    KEYBYTE_Y     = 5,

    KEYBYTE_ENTER = 6,
    KEYBYTE_L     = 6,
    KEYBYTE_K     = 6,
    KEYBYTE_J     = 6,
    KEYBYTE_H     = 6,

    KEYBYTE_SPACE = 7,
    KEYBYTE_SYM   = 7,
    KEYBYTE_M     = 7,
    KEYBYTE_N     = 7,
    KEYBYTE_B     = 7 
};

#ifndef HWIF_IMPLEMENTATION

extern unsigned char keydata[];
extern void do_halt();
extern void port254(const unsigned char color) __z88dk_fastcall;
extern void readkeyboard();

#else

unsigned char keydata[8];

void readkeyboard()
{
    __asm
        ld bc, #0xfefe
        in a, (c)
        ld (#_keydata+0), a
        ld bc, #0xfdfe
        in a, (c)
        ld (#_keydata+1), a
        ld bc, #0xfbfe
        in a, (c)
        ld (#_keydata+2), a
        ld bc, #0xf7fe
        in a, (c)
        ld (#_keydata+3), a
        ld bc, #0xeffe
        in a, (c)
        ld (#_keydata+4), a
        ld bc, #0xdffe
        in a, (c)
        ld (#_keydata+5), a
        ld bc, #0xbffe
        in a, (c)
        ld (#_keydata+6), a
        ld bc, #0x7ffe
        in a, (c)
        ld (#_keydata+7), a
    __endasm;
}

void port254(const unsigned char color) __z88dk_fastcall
{
    color;
}
/*
// xxxsmbbb
// where b = border color, m is mic, s is speaker
void port254(const unsigned char color) __z88dk_fastcall
{
    color; // color is in l
    // Direct border color setting
    __asm
        ld a,l
        ld hl, #_port254tonebit
        or a, (hl)
        out (254),a
    __endasm;    
    
}
*/

// practically waits for retrace
void do_halt()
{
    __asm
        ei
        halt
        di
    __endasm;
}

#endif