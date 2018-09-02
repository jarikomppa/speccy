#include "main.h"

void update_info(unsigned char selected, unsigned char hand[5], unsigned char pos)
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
    selected;
    cleartextbox(1, 8, 19, 8);
    drawstringz("Select cards to play.", 2, 9);
    temp[0] = 0;
    strcat(temp, "Selected effects: ");
    if (selected == 0)
        strcat(temp, "rest");
    for (i = 0; i < 5; i++)
    {
        if (selected & (1 << i))
        {
            if (gCardTypes[hand[i]].mFlags & CARDFLAG_FOCUS) focus = 1;
            if (gCardTypes[hand[i]].mFlags & CARDFLAG_LEAP) leap = 1;
            attack += gCardTypes[hand[i]].mAttack;
            defend += gCardTypes[hand[i]].mDefend;
            count++;
        }
        if (gCardTypes[hand[i]].mFlags & CARDFLAG_FATIGUE)
            restcount--;
    }
          
    if (focus)
    {
        strcat(temp, "focus");
        first = 1;
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
    drawstringz(temp, 2, 12);
    if (pos == 5)
    {
        drawstringz(">>> Play selected cards <<<", 2, 14);
    }
    else
    {
        temp[0] = 0;    
        strcat(temp, "Card: ");
        strcat(temp, gCardTypes[hand[pos]].mName);
        drawstringz(temp, 2, 14);
    }
}

void ingame()
{
    unsigned char frame;
    unsigned char pos;
    unsigned char selected = 0;
    unsigned char commit = 0;
    unsigned char c;
    unsigned char hand[5];
    hand[0] = 1;
    hand[1] = 2;
    hand[2] = 3;
    hand[3] = CARD_ATK1DEF2;
    hand[4] = 5;
    pos = 0;
    frame = 0;

    while (1)
    {
        fillback();
        drawmug(0,26,1);
        drawmug(5,26,17);
        drawtextbox(1, 8, 19, 8);
        drawtextbox(21, 8, 10, 8);
        drawstringz("Enemy HP 7", 22, 9);
        drawstringz("stamina 13", 22, 10);
        //drawstringz("Stamina 3", 22, 11);
        drawstringz("Your HP 8", 22, 12);
        drawstringz("stamina 17", 22, 13);
        drawstringz("deck left 22", 22, 14);

        drawstringz("Dealing..", 2, 9);

        drawcard(CARD_BACK,1,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(hand[0],1,17,COLOR(0,0,0,7));
        cardfx();
        drawcard(CARD_BACK,6,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(hand[1],6,17,COLOR(0,0,0,7));
        cardfx();
        drawcard(CARD_BACK,11,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(hand[2],11,17,COLOR(0,0,0,7));
        cardfx();
        drawcard(CARD_BACK,16,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(hand[3],16,17,COLOR(0,0,0,7));
        cardfx();
        drawcard(CARD_BACK,21,1,COLOR(0,0,0,7));
        cardfx();
        drawcard(hand[4],21,17,COLOR(0,0,0,7));
        cardfx();
        
        update_info(0,hand,0);
       
        while (1)
        {
            static const unsigned char positions[6] = { 2, 7, 12, 17, 22, 27 };            
            scan_input();
            {
                unsigned char triggered = 0;
                unsigned char oldpos = pos;
                if (TRIGGER(KEY_RIGHT)) { triggered = 1; pos++; if (pos == 6) pos = 0; }
                if (TRIGGER(KEY_LEFT)) { triggered = 1; pos--; if (pos == 255) pos = 5; }
                if (TRIGGER(KEY_FIRE)) { triggered = 1; if (pos != 5) selected ^= 1 << pos; else commit = 1; }
                if (triggered) 
                {
                    key_wasdown = 0;
                    c = COLOR(0,0,0,7);
                    switch (oldpos)
                    {
                        case 0: if (selected &  1) c = COLOR(0,1,0,7); drawcard(hand[0], 1,17,c); break;
                        case 1: if (selected &  2) c = COLOR(0,1,0,7); drawcard(hand[1], 6,17,c); break;
                        case 2: if (selected &  4) c = COLOR(0,1,0,7); drawcard(hand[2],11,17,c); break;
                        case 3: if (selected &  8) c = COLOR(0,1,0,7); drawcard(hand[3],16,17,c); break;
                        case 4: if (selected & 16) c = COLOR(0,1,0,7); drawcard(hand[4],21,17,c); break;
                        case 5: drawmug(5,26,17); break;
                    }
                    if (!commit)
                    {
                        update_info(selected,hand,pos);
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
