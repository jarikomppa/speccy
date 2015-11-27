extern void playtone(unsigned short delay) __z88dk_fastcall;

#define MAX_CHANNEL 4
unsigned short toneadder[MAX_CHANNEL];

const unsigned char musicdata[] = {
#include "tune.h"
20,0,0,0,0,0
};

unsigned char arpy;
unsigned char nexttone = 0;
unsigned char keeptone = 0;
unsigned short songidx;

void initmusic()
{
    toneadder[0] = 0;
    toneadder[1] = 0;
    toneadder[2] = 0;
    toneadder[3] = 0;
    arpy = 0;
    songidx = 0;

    port254tonebit = 0;
}

void music()
{
    arpy++;
    if (arpy == MAX_CHANNEL)
        arpy = 0;
    if (toneadder[arpy] == 0)
    {
        arpy++;
        if (arpy == MAX_CHANNEL)
            arpy = 0;
    }
    if (toneadder[arpy] == 0)
    {
        arpy++;
        if (arpy == MAX_CHANNEL)
            arpy = 0;
    }
    if (toneadder[arpy] == 0)
    {
        arpy++;
        if (arpy == MAX_CHANNEL)
            arpy = 0;
    }


    port254tonebit |= 5;
    playtone(toneadder[arpy]);
    
    port254tonebit &= ~5;
    port254(0);    

    if (!keeptone)
    {
        unsigned char note;
        unsigned char channel;
        keeptone = musicdata[songidx++];
        note = musicdata[songidx++];
        channel = musicdata[songidx++];
        toneadder[channel] = tonetab[note];
            
        if (keeptone == 0)
        {
            songidx = 0;
            keeptone = 1;
        }
    }
    keeptone--;
}