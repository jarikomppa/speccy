#include "main.h"

unsigned char p_hand[5];
unsigned char o_hand[5];

void update_info(unsigned char selected, unsigned char pos, unsigned char oldselected)
{
    char temp[40];
    char numtemp[3];
    char focus = 0;
    char leap = 0;
    char attack = 0;
    char defend = 0;
    signed char fatigue = 0;
    char count = 0;
    char restcount = 0;
    char first = 0;
    char i;

    for (i = 0; i < 5; i++)
    {
        if (selected & (1 << i))
        {
            if (gCardTypes[p_hand[i]].mFlags & CARDFLAG_FOCUS) focus = 1;
            if (gCardTypes[p_hand[i]].mFlags & CARDFLAG_LEAP) leap = 1;
            attack += gCardTypes[p_hand[i]].mAttack;
            defend += gCardTypes[p_hand[i]].mDefend;
            count++;
        }
        if (gCardTypes[p_hand[i]].mFlags & CARDFLAG_FATIGUE)
            restcount--;
    }

	if (oldselected == 100)
	{
		cleartextbox(1+1, 8+1, 19-2, 8-2);
		drawstringz("Select cards to play.", 2, 9);
	}
	
	if (oldselected != selected || oldselected == 100)
	{
		cleartextbox(1+1, 10+1, 19-2, 4-2);
	
		temp[0] = 0;
		strcat(temp, "Selected effects: ");
		if (selected == 0)
			strcat(temp, "rest");
			  
		if (focus)
		{
			strcat(temp, "focus");
			first = 0;
		}
			
		if (leap)
		{
			if (!first)
				strcat(temp, ", ");
			strcat(temp, "leap");        
		}
		
		if (count == 0)
		{
			fatigue = restcount;
		}
		else
		{
			fatigue = fatigue_for_cards[count];
		}
		
		if (*temp)
			drawstringz(temp, 2, 11);
		
		first = 0;
		temp[0] = 0;
		if (attack)
		{
			strcat(temp, "attack ");
			numtemp[0] = attack + '0';
			numtemp[1] = 0;
			strcat(temp, numtemp);
			first = 1;
		}
		
		if (defend)
		{
			if (first)
				strcat(temp, ", ");
			strcat(temp, "defend ");
			numtemp[0] = defend + '0';
			numtemp[1] = 0;
			strcat(temp, numtemp);
			first = 1;
		}
		
		if (fatigue)
		{
			if (first)
				strcat(temp, ", ");
			strcat(temp, "fatigue ");
			if (fatigue < 0 || fatigue > 9)
			{
				if (fatigue < 0) { numtemp[0] = '-'; fatigue = -fatigue; }
				if (fatigue > 9) { numtemp[0] = '1'; fatigue -= 10; }
				numtemp[1] = fatigue + '0';
				numtemp[2] = 0;
			}
			else
			{
				numtemp[0] = fatigue + '0';
				numtemp[1] = 0;
			}
			strcat(temp, numtemp);
			first = 1;
		}
		
		if (*temp)
			drawstringz(temp, 2, 12);
	}

	if (selected == oldselected || oldselected == 100)
	{
		cleartextbox(1+1, 13+1, 19-2, 3-2);
		if (pos == 5)
		{
			drawstringz(">>> Play selected cards <<<", 2, 14);
		}
		else
		{
			temp[0] = 0;    
			strcat(temp, "Card: ");
			strcat(temp, gCardTypes[p_hand[pos]].mName);
			drawstringz(temp, 2, 14);
		}
	}	
}

char p_deck[40];
char p_discard[40];
unsigned char p_decktop;
char p_discardtop;

char o_deck[40];
char o_discard[40];
unsigned char o_decktop;
char o_discardtop;

#define P_ADDCARD(card) p_deck[p_decktop++] = (card) 
#define P_GETCARD() p_deck[--p_decktop]
#define P_DISCARD(card) p_discard[p_discardtop++] = (card)
#define P_GETDISCARD() p_discard[--p_discardtop]

#define O_ADDCARD(card) o_deck[o_decktop++] = (card) 
#define O_GETCARD() o_deck[--o_decktop]
#define O_DISCARD(card) o_discard[o_discardtop++] = (card)
#define O_GETDISCARD() o_discard[--o_discardtop]

void opponentdeck()
{
	char i, c7, c5, c4, c3, c1;
	
	switch (game_stage)
	{
		default:
		case 0:
			c7 = CARD_IDLE;
			c5 = CARD_DEF1;
			c4 = CARD_ATK1;
			c3 = CARD_FOCUS;
			c1 = CARD_ATK2;
			break;
		case 1:
			c7 = CARD_DEF1;
			c5 = CARD_ATK1;
			c4 = CARD_FOCUS;
			c3 = CARD_DEF2;
			c1 = CARD_ATK2;
			break;
		case 2:
			c7 = CARD_ATK1;
			c5 = CARD_DEF1;
			c4 = CARD_FOCUS;
			c3 = CARD_ATK2;
			c1 = CARD_DEF2;
			break;
		case 3:
			c7 = CARD_ATK1;
			c5 = CARD_DEF1;
			c4 = CARD_ATK2;
			c3 = CARD_FOCUS;
			c1 = CARD_DEF2;
			break;
		case 4:
			c7 = CARD_ATK1;
			c5 = CARD_ATK2;
			c4 = CARD_DEF1;
			c3 = CARD_DEF2;
			c1 = CARD_ATK1DEF1;
			break;
		case 5:
			c7 = CARD_ATK1;
			c5 = CARD_ATK2;
			c4 = CARD_DEF1;
			c3 = CARD_DEF2;
			c1 = CARD_ATK1DEF2;
			break;
		case 6:
			c7 = CARD_ATK2;
			c5 = CARD_DEF2;
			c4 = CARD_DEF1;
			c3 = CARD_ATK3;
			c1 = CARD_ATK2DEF1;
			break;
		case 7:
			c7 = CARD_ATK2;
			c5 = CARD_ATK3;
			c4 = CARD_ATK1DEF2;
			c3 = CARD_DEF2;
			c1 = CARD_ATK2DEF1;
			break;
		case 8:
			c7 = CARD_ATK2;
			c5 = CARD_ATK1DEF2;
			c4 = CARD_DEF2;
			c3 = CARD_ATK2DEF1;
			c1 = CARD_DEF3;
			break;
		case 9:
			c7 = CARD_ATK3;
			c5 = CARD_ATK1DEF2;
			c4 = CARD_ATK2DEF1;
			c3 = CARD_DEF3;
			c1 = CARD_DEF3;
			break;
	}
	for (i = 0; i < 7; i++) O_ADDCARD(c7);
	for (i = 0; i < 5; i++) O_ADDCARD(c5);
	for (i = 0; i < 4; i++) O_ADDCARD(c4);
	for (i = 0; i < 3; i++) O_ADDCARD(c3);
    O_ADDCARD(c1);
}


void player_shuffle()
{
	unsigned char i;
	while (p_discardtop)
		P_ADDCARD(P_GETDISCARD());

	for (i = 0; i < p_decktop; i++)
	{
		unsigned char c1, c2;
		c1 = i;
		c2 = 200;
		while (c2 >= p_decktop)
			c2 = xorshift8() & 63;
		{
			char t = p_deck[c1];
			p_deck[c1] = p_deck[c2];
			p_deck[c2] = t;
		}
	}	
}

void opponent_shuffle()
{
	unsigned char i;
	while (o_discardtop)
		O_ADDCARD(O_GETDISCARD());

	for (i = 0; i < o_decktop; i++)
	{
		unsigned char c1, c2;
		c1 = i;
		c2 = 200;
		while (c2 >= o_decktop)
			c2 = xorshift8() & 63;
		{
			char t = o_deck[c1];
			o_deck[c1] = o_deck[c2];
			o_deck[c2] = t;
		}
	}	
}

char focus()
{
	return 0b11111; // cards to keep
}

char resolve(unsigned char selected)
{
	char player_focus = selected;
	unsigned char c;
	
	selected = 0b11111;
	if (player_focus)
		selected = focus();
	for (c = 0; c < 5; c++)
		if (selected & (1 << c))
			P_DISCARD(p_hand[c]);
	for (c = 0; c < 5; c++)
	{
		if (p_decktop == 0)
			player_shuffle();
		p_hand[c] = P_GETCARD();
		if (o_decktop == 0)
			opponent_shuffle();
		o_hand[c] = O_GETCARD();
	}

	return 0; // no game over
//	return -1; // player won
//	return -2; // player lost
}

void ingame()
{
	static const unsigned char stagemug[10] = {
	5, 4, 0, 7, 11, 9, 10, 8, 2, 1
	};	

    unsigned char frame;
    unsigned char pos;
    unsigned char selected = 0;
    unsigned char commit = 0;
    unsigned char c;
	p_decktop = 0;
	p_discardtop = 0;
	o_decktop = 0;
	o_discardtop = 0;
	opponentdeck();
	for (c = 0; c < 20; c++)
		P_ADDCARD(playerdeck[c]);
	for (c = 0; c < player_hurt; c++)
		P_ADDCARD(CARD_HURT);
	player_shuffle();
	opponent_shuffle();
	for (c = 0; c < 5; c++)
	{
		p_hand[c] = P_GETCARD();
		o_hand[c] = O_GETCARD();
	}
    pos = 0;
    frame = 0;

    while (1)
    {
        fillback();
        drawmug(stagemug[game_stage],26,1);
        drawmug(6,26,17);
        drawtextbox(1, 8, 19, 8);
        drawtextbox(21, 8, 10, 8);
        drawstringz("Enemy HP 7", 22, 9);
        drawstringz("stamina 13", 22, 10);
        //drawstringz("Stamina 3", 22, 11);
        drawstringz("Your HP 8", 22, 12);
        drawstringz("stamina 17", 22, 13);
        drawstringz("deck left 22", 22, 14);

        drawstringz("Dealing..", 2, 9);

#if 0
        drawcard(CARD_BACK,1,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(p_hand[0],1,17,COLOR(0,0,0,7));
        cardfx();
        drawcard(CARD_BACK,6,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(p_hand[1],6,17,COLOR(0,0,0,7));
        cardfx();
        drawcard(CARD_BACK,11,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(p_hand[2],11,17,COLOR(0,0,0,7));
        cardfx();
        drawcard(CARD_BACK,16,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(p_hand[3],16,17,COLOR(0,0,0,7));
        cardfx();
        drawcard(CARD_BACK,21,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(p_hand[4],21,17,COLOR(0,0,0,7));
        cardfx();
#else
        drawcard(o_hand[0],1,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(p_hand[0],1,17,COLOR(0,0,0,7));
        cardfx();
        drawcard(o_hand[1],6,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(p_hand[1],6,17,COLOR(0,0,0,7));
        cardfx();
        drawcard(o_hand[2],11,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(p_hand[2],11,17,COLOR(0,0,0,7));
        cardfx();
        drawcard(o_hand[3],16,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(p_hand[3],16,17,COLOR(0,0,0,7));
        cardfx();
        drawcard(o_hand[4],21,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(p_hand[4],21,17,COLOR(0,0,0,7));
        cardfx();
#endif
        update_info(0,0, 100);
       
        while (1)
        {
            static const unsigned char positions[6] = { 2, 7, 12, 17, 22, 27 };            
            scan_input();
            {
                unsigned char triggered = 0;
                unsigned char oldpos = pos;
				unsigned char oldselected = selected;
                if (TRIGGER(KEY_RIGHT)) { triggered = 1; pos++; if (pos == 6) pos = 0; }
                if (TRIGGER(KEY_LEFT)) { triggered = 1; pos--; if (pos == 255) pos = 5; }
                if (TRIGGER(KEY_FIRE)) 
				{ 
					triggered = 1; 
					if (pos != 5) 
					{
						if (!(gCardTypes[p_hand[pos]].mFlags & CARDFLAG_IDLE))
							selected ^= 1 << pos; 
					}
					else 
					{
						commit = 1; 
					}
				}
                if (triggered) 
                {
                    key_wasdown = 0;
                    c = COLOR(0,0,0,7);
                    switch (oldpos)
                    {
                        case 0: if (selected &  1) c = COLOR(0,1,0,7); drawcard(p_hand[0], 1,17,c); break;
                        case 1: if (selected &  2) c = COLOR(0,1,0,7); drawcard(p_hand[1], 6,17,c); break;
                        case 2: if (selected &  4) c = COLOR(0,1,0,7); drawcard(p_hand[2],11,17,c); break;
                        case 3: if (selected &  8) c = COLOR(0,1,0,7); drawcard(p_hand[3],16,17,c); break;
                        case 4: if (selected & 16) c = COLOR(0,1,0,7); drawcard(p_hand[4],21,17,c); break;
                        case 5: drawmug(6,26,17); break;
                    }
                    if (!commit)
                    {
                        update_info(selected,pos,oldselected);
                    }
					else
					{
						signed char result = resolve(selected);
						commit = 0;
						selected = 0;
						if (result == -1)
							game_state = 0;
						if (result == -2)
							game_state = 0;
						if (result < 0)
							return;
					}						
                }
            }
            frame++;
            drawicon(littlesin[frame & 15], positions[pos], 19);
            do_halt();
            do_halt();
        }        
    }       
}
