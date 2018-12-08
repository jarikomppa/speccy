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
	unsigned char i;
    y8 = 1;
    key_isdown = 0;
    key_wasdown = 0;
    input_mode = INPUT_WASD;
    player_money = 10;
    player_hurt = 12;
	stage[0] = 1;
	for (i = 1; i < 10; i++)
		stage[i] = 0;

	for (i = 0; i < 5; i++)
	{
		playerdeck[i   ] = CARD_ATK1;
		playerdeck[i+ 5] = CARD_DEF1;
		playerdeck[i+10] = CARD_LEAP;
		playerdeck[i+15] = CARD_IDLE;
	}
	playerdeck[0] = CARD_ATK2;
	playerdeck[9] = CARD_DEF2;
	playerdeck[10] = CARD_FOCUS;
	
	game_state = 0;
	game_stage = 0;

    port254(0);
	while(1)
	{
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
#if 0
    	cleartextbox_color(0, 0, 20, 5, 0x17);
    	while(1)
    	{
    		do_halt();
    		port254(1);
    		drawstringz("brown fox",3,2);
    		port254(0);
    	}
#endif
    }
}
