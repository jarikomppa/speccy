/* * Part of Jari Komppa's zx spectrum suite * 
https://github.com/jarikomppa/speccy * released under the unlicense, see 
http://unlicense.org * (practically public domain) */

#include "main.h"

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


void main()
{       
    y8 = 1;
    key_isdown = 0;
    key_wasdown = 0;
    input_mode = INPUT_WASD;
    player_money = 10;
    player_hurt = 12;
	stage[0] = 1;
	stage[1] = 0;
	stage[2] = 0;
	stage[3] = 0;
	stage[4] = 0;
	stage[5] = 0;
	stage[6] = 0;
	stage[7] = 0;
	stage[8] = 0;
	stage[9] = 0;

	game_state = 0;
	game_stage = 0;

    port254(0);
	/*
	cleartextbox_color(0, 0, 20, 5, 0x17);
	while(1)
	{
		do_halt();
		port254(1);
		drawstringz("brown fox",3,2);
		port254(0);
	}
	*/
	#if 1
	while(1)
	switch (game_state)
	{
//		case 0:
		default:
			tour();
			break;
		case 1:
			heal();
			break;
		case 2:
			shop();
			break;
		case 3:
			ingame();
			break;
    //newcard(CARD_FOCUS);
	}
	#endif
}
