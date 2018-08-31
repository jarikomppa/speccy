/* * Part of Jari Komppa's zx spectrum suite * 
https://github.com/jarikomppa/speccy * released under the unlicense, see 
http://unlicense.org * (practically public domain) */

#include "main.h"

#define HWIF_IMPLEMENTATION
#include "hwif.c"

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

#define TRIGGER(x) ((key_wasdown & (x)) && !(key_isdown & (x)))

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

void tour()
{
    // 32x24
    fillback();
    drawtextbox(1,2,4,6);
    drawtextbox(1,10,4,6);
    drawicon(4, 2, 4);    
    drawicon(5, 2, 12);    
       
    drawtextbox(5,1,4,16);
    drawicon(3, 6, 8);
    
    drawtextbox(9,1,4,8);
    drawtextbox(9,9,4,8);
    drawicon(3, 10, 4);
    drawicon(3, 10, 12);
    
    drawtextbox(13,1,4,4);
    drawtextbox(13,5,4,4);
    drawtextbox(13,9,4,4);
    drawtextbox(13,13,4,4);
    drawicon(3, 14, 2);
    drawicon(3, 14, 6);
    drawicon(3, 14, 10);
    drawicon(3, 14, 14);

    drawtextbox(17,1,4,8);
    drawtextbox(17,9,4,8);
    drawicon(3, 18, 4);
    drawicon(3, 18, 12);

    drawtextbox(21,1,4,16);
    drawicon(3, 22, 8);
   
    drawtextbox(1,18,30,5);
    
    drawtextbox(26,5,5,9);
    drawmug(3,26,5);
    drawstringz("Store", 27, 12);
    
    while (1) do_halt();
}

void heal()
{
    unsigned short i;
    fillback();
    drawtextbox(6,18,25,5);
    drawmug(3, 1, 17);
    drawstringz("You're quite hurt!", 7, 20);
    
    for (i = 0; i < 5; i++)
    {
        drawcard(CARD_HURT, 1 + i * 4, 1, COLOR(0,1,0,7));
        drawcard(CARD_HURT, 1 + i * 4, 7, COLOR(0,1,0,7));
    }
    
    drawtextbox(22,1,9,4);
    drawstringz("Heal", 25, 2);
    drawstringz("Cost:1", 25, 3);

    drawtextbox(22,6,9,4);
    drawstringz("Done", 25, 7);
    drawstringz("shopping", 25, 8);

    drawtextbox(7,14,20,3);
    drawstringz("You have xx crowns", 9, 15);

    while (1) do_halt();
}

void newcard(unsigned char newcardid)
{
    unsigned short i;
    unsigned char pos = 0;
    unsigned char commit = 0;
    unsigned char frame = 0;
    fillback();
    for (i = 0; i < 5; i++)
    {
        drawcard(i+1, 1 + i * 4, 0, COLOR(0,1,0,7));
        drawcard(i+2, 1 + i * 4, 6, COLOR(0,1,0,7));
        drawcard(i+3, 1 + i * 4, 12, COLOR(0,1,0,7));
        drawcard(i+4, 1 + i * 4, 18, COLOR(0,1,0,7));
    }
    drawtextbox(22,0,4,7);
    drawcard(newcardid, 22, 0, COLOR(0,1,0,7));
    drawstringz("New", 23, 6);

    drawtextbox(22, 13, 9, 10);
    drawstringz("Choose card", 23, 14);
    drawstringz("to discard.", 23, 15);
    
    drawstringz("You can also", 23, 17);
    drawstringz("choose the", 23, 18);
    drawstringz("new card if", 23, 19);
    drawstringz("you don't", 23, 20);
    drawstringz("want it.", 23, 21);

    while (1) 
    {
        static const unsigned char positionsx[21] = { 2, 6, 10, 14, 18, 23, 2, 6, 10, 14, 18, 2, 6, 10, 14, 18, 2, 6, 10, 14, 18 };            
        static const unsigned char positionsy[21] = { 2, 2, 2, 2, 2, 2, 8, 8, 8, 8, 8, 14, 14, 14, 14, 14, 20, 20, 20, 20, 20 };
        scan_input();
        {
            unsigned char triggered = 0;
            unsigned char oldpos = pos;
            if (TRIGGER(KEY_RIGHT)) { triggered = 1; pos++; if (pos == 21) pos = 0; }
            if (TRIGGER(KEY_LEFT)) { triggered = 1; pos--; if (pos == 255) pos = 20; }
            if (TRIGGER(KEY_DOWN)) { triggered = 1; if (pos < 5) pos += 6; else if (pos > 5) pos += 5; if (pos > 20) pos -= 21; }
            if (TRIGGER(KEY_UP)) { triggered = 1; if (pos < 5) pos += 16; else if (pos > 10) pos -= 5; else if (pos > 5) pos -= 6;}
            if (TRIGGER(KEY_FIRE)) { triggered = 1; commit = 1; }
            if (triggered) 
            {
                if (commit)
                {
                    // Todo: replace card 
                    return;
                }
                key_wasdown = 0;
                if (oldpos < 5)
                    drawcard(oldpos+1,oldpos * 4 + 1, 0 ,COLOR(0,1,0,7));
                if (oldpos == 5)
                    drawcard(newcardid, 22, 0, COLOR(0,1,0,7));
                if (oldpos > 5 && oldpos < 11)
                    drawcard(oldpos-4, (oldpos - 6) * 4 + 1, 6, COLOR(0,1,0,7));
                if (oldpos > 10 && oldpos < 16)
                    drawcard(oldpos-8, (oldpos - 11) * 4 + 1, 12, COLOR(0,1,0,7));
                if (oldpos > 15)
                    drawcard(oldpos-12, (oldpos - 16) * 4 + 1, 18, COLOR(0,1,0,7));                
            }
        }
        frame++;
        drawicon(littlesin[frame & 15], positionsx[pos], positionsy[pos]);
        do_halt();
        do_halt();
    }
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

void shop()
{
    unsigned short i;
    unsigned char pos = 5;
    unsigned char commit = 0;
    unsigned char frame = 0;
    static const unsigned char shoporder[10] = 
    {
        CARD_FOCUS,
        CARD_ATK1,
        CARD_DEF1,
        CARD_ATK2,
        CARD_DEF2,
        
        CARD_ATK3,
        CARD_DEF3,
        CARD_ATK1DEF1,
        CARD_ATK2DEF1,
        CARD_ATK1DEF2
    };
    static const unsigned char shopcost[10] =
    {
        1,2,2,6,6,14,14,18,22,22
    };
    fillback();
    drawtextbox(6,18,25,5);
    drawmug(3, 1, 17);
    drawstringz("Welcome to the card shop!", 7, 19);
    drawstringz("Here you can spend crowns to buy", 7, 20);
    drawstringz("better cards to improve your deck.", 7, 21);    
                         
    
    for (i = 0; i < 5; i++)
    {
        drawcard(shoporder[i], 1 + i * 4, 1, COLOR(0,1,0,7));
        drawcard(shoporder[i+5], 1 + i * 4, 7, COLOR(0,1,0,7));
    }
    
    drawtextbox(22,5,9,4);
    drawstringz("Done", 25, 6);
    drawstringz("shopping", 25, 7);
    
    drawtextbox(7,14,20,3);
    drawmoney(9, 15, player_money);

    while (1) 
    {
        static const unsigned char positionsx[11] = { 2, 6, 10, 14, 18, 22, 2, 6, 10, 14, 18 };            
        static const unsigned char positionsy[11] = { 3, 3, 3, 3, 3, 6, 9, 9, 9, 9, 9};
        scan_input();
        {
            unsigned char triggered = 0;
            unsigned char oldpos = pos;
            if (TRIGGER(KEY_RIGHT)) { triggered = 1; pos++; if (pos == 11) pos = 5; }
            if (TRIGGER(KEY_LEFT)) { triggered = 1; pos--; if (pos == 255) pos = 10; }
            if (TRIGGER(KEY_DOWN) || TRIGGER(KEY_UP)) { triggered = 1; if (pos < 5) pos += 6; else if (pos > 5) pos -= 6;}
            if (TRIGGER(KEY_FIRE)) { triggered = 1; commit = 1; }
            if (triggered) 
            {
                if (commit)
                {
                    if (pos == 5)
                        return;
                    // Todo: replace card 
                    return;
                }
                key_wasdown = 0;
                if (pos != oldpos)
                {
                    if (oldpos < 5)
                        drawcard(shoporder[oldpos],oldpos * 4 + 1, 1 ,COLOR(0,1,0,7));
                    if (oldpos == 5)
                    {
                        drawtextbox(22,5,9,4);
                        drawstringz("Done", 25, 6);
                        drawstringz("shopping", 25, 7);
                    }
                    if (oldpos > 5)
                        drawcard(shoporder[oldpos-1], (oldpos - 6) * 4 + 1, 7, COLOR(0,1,0,7));
                }
                cleartextbox(6,18,25,5);
                switch (pos)
                {
                    case 5:
                        drawstringz("Come back soon!", 7, 20);
                        break;
                }
                
                if (pos < 5)
                    drawcost(7,21,shopcost[pos]);
                if (pos > 5)
                    drawcost(7,21,shopcost[pos-1]);

            }
        }
        frame++;
        drawicon(littlesin[frame & 15], positionsx[pos], positionsy[pos]);
        do_halt();
        do_halt();
    }
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
