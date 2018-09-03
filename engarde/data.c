#include "main.h"

unsigned char *data_ptr;
unsigned char *screen_ptr;

unsigned char port254tonebit;
#include "yofstab.h"
#include "propfont.h"
const unsigned char artassets[] = {
#include "artassets.h"
};

const unsigned char backtile[] = {
0b01100100,
0b00001101,
0b11001001,
0b01100000,
0b00000110,
0b10010011,
0b10110000,
0b00100110
};


const Card gCardTypes[] =
{
	{ 0, 0, 0b00000, "No card"  }, // No card (kludge)
	{ 1, 0, 0b00000, "Attack +1"}, // Basic attack
	{ 2, 0, 0b00000, "Attack +2"}, // Strong attack
	{ 3, 0, 0b00000, "Attack +3"}, // Very strong attack
	{ 0, 1, 0b00000, "Defend +1"}, // Basic defend
	{ 0, 2, 0b01000, "Defend +2, Focus"}, // Strong defend
	{ 0, 3, 0b01000, "Defend +3, Focus"}, // Very strong defend
	{ 0, 0, 0b01000, "Focus"    }, // Basic focus
	{ 0, 0, 0b11000, "Leap"     }, // Basic dodge
	{ 1, 1, 0b00000, "Attack +1, Defend +1" }, // Attack & defend
	{ 2, 1, 0b00000, "Attack +2, Defend +1" }, // Attack & defend
	{ 1, 2, 0b00000, "Attack +1, Defend +2" }, // Attack & defend
	{ 0, 0, 0b00000, "tbd"      }, // tbd
	{ 0, 0, 0b00100, "Idle"     }, // idle
	{ 0, 0, 0b00110, "Fatigue"  }, // fatigue
	{ 0, 0, 0b00101, "Hurt"     }  // Wound (can't be focused away)
};

const unsigned char littlesin[16] = { 0,0,0,0,0,0,1,1,2,2,2,2,2,2,1,1 };
const unsigned char fatigue_for_cards[6] = { 0, 0, 1, 3, 6, 10 };


unsigned char y8;
unsigned char key_isdown;
unsigned char key_wasdown;
unsigned char input_mode;
unsigned char player_money;
unsigned char player_hurt;
