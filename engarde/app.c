/* * Part of Jari Komppa's zx spectrum suite * 
https://github.com/jarikomppa/speccy * released under the unlicense, see 
http://unlicense.org * (practically public domain) */

#define HWIF_IMPLEMENTATION
#include "main.h"

unsigned char xorshift8(void) 
{
    y8 ^= (y8 << 7);
    y8 ^= (y8 >> 5);
    return y8 ^= (y8 << 3);
}

void int2str(unsigned char val, char s[4])
{
    if (val > 99)
    {
        s[0] = '0';
        while (val > 99)
        {
            s[0]++;
            val -= 100; 
        }
        s[1] = '0';
        while (val > 9)
        {
            s[1]++;
            val -= 10; 
        }
        s[2] = '0' + val;
        s[3] = 0;
    }
    else
    if (val > 9)
    {
        s[0] = '0';
        while (val > 9)
        {
            s[0]++;
            val -= 10; 
        }
        s[1] = '0' + val;
        s[2] = 0;
    }
    else
    {
        s[0] = '0' + val;
        s[1] = 0;
    }
}

void scan_input()
{
    unsigned char down;
    readkeyboard();
    down = 0;
    switch (input_mode)
    {
        case INPUT_WASD:
            if (KEYDOWN(S)) down |= KEY_DOWN;
            if (KEYDOWN(W)) down |= KEY_UP;
            if (KEYDOWN(A)) down |= KEY_LEFT;
            if (KEYDOWN(D)) down |= KEY_RIGHT;
            if (KEYDOWN(SPACE) || KEYDOWN(ENTER) || KEYDOWN(M)) down |= KEY_FIRE;
            break;
        case INPUT_QAOP:
            if (KEYDOWN(S)) down |= KEY_DOWN;
            if (KEYDOWN(W)) down |= KEY_UP;
            if (KEYDOWN(A)) down |= KEY_LEFT;
            if (KEYDOWN(D)) down |= KEY_RIGHT;
            if (KEYDOWN(SPACE) || KEYDOWN(ENTER) || KEYDOWN(M)) down |= KEY_FIRE;
            break;
        case INPUT_KEMPSTON:
            if (KEYDOWN(KEMPD)) down |= KEY_DOWN;
            if (KEYDOWN(KEMPU)) down |= KEY_UP;
            if (KEYDOWN(KEMPL)) down |= KEY_LEFT;
            if (KEYDOWN(KEMPR)) down |= KEY_RIGHT;
            if (KEYDOWN(KEMPF)) down |= KEY_FIRE;
            break;
        case INPUT_SINCLAIR:
            if (KEYDOWN(8)) down |= KEY_DOWN;
            if (KEYDOWN(9)) down |= KEY_UP;
            if (KEYDOWN(6)) down |= KEY_LEFT;
            if (KEYDOWN(7)) down |= KEY_RIGHT;
            if (KEYDOWN(0)) down |= KEY_FIRE;
            break;
        case INPUT_CURSOR:
            if (KEYDOWN(6)) down |= KEY_DOWN;
            if (KEYDOWN(7)) down |= KEY_UP;
            if (KEYDOWN(5)) down |= KEY_LEFT;
            if (KEYDOWN(8)) down |= KEY_RIGHT;
            if (KEYDOWN(0)) down |= KEY_FIRE;
            break;
    }
    key_isdown = down;
    key_wasdown |= down;    
}


void cardfx()
{
    do_halt();
    do_halt();
    // TODO: play sound
}

void strcat(char *tgt, char *src)
{
    while (*tgt)
        tgt++;
    while (*src)
    {
        *tgt = *src;
        tgt++;
        src++;
    }
    *tgt = 0;
}






void drawmoney(unsigned char x, unsigned char y, unsigned char v)
{
    char temp[20];
    temp[0] = 0;
    strcat(temp, "You have ");
    int2str(v, &temp[9]);
    strcat(temp, " crowns.");
    drawstringz(temp, x, y);
 //   while(1) {};
}

void drawcost(unsigned char x, unsigned char y, unsigned char v)
{
    char temp[40];
    temp[0] = 0;
    strcat(temp, "It costs ");
    int2str(v, &temp[9]);
    strcat(temp, " crowns.");
    drawstringz(temp, x, y);
}



void main()
{       
    y8 = 1;
    key_isdown = 0;
    key_wasdown = 0;
    input_mode = INPUT_WASD;
    player_money = 10;

    do_port254(0);
    //ingame();
    //tour();
    //heal();
    //newcard(CARD_FOCUS);
    shop();
    while (1);
}
