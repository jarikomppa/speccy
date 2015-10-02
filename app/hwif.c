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

// practically waits for retrace
void do_halt()
{
    __asm
        halt
    __endasm;
}
