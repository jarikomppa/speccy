#include "main.h"

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
        2,5,5,15,15,35,35,45,55,55
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
                    case 0: // focus
                        drawstringz("Focus: remove cards from the game.", 7, 19);
                        drawstringz("Cards come back in the next match.", 7, 20);
                        break;
                    case 1: // attack1                        
                        drawstringz("The basic attack.", 7, 19);
                        drawstringz("Better than nothing.", 7, 20);
                        break;
                    case 2: // defend1
                        drawstringz("Basic defense.", 7, 19);
                        break;
                    case 3: // attack2
                        drawstringz("Mid-range attack.", 7, 19);
                        drawstringz("Formidable.", 7, 20);
                        break;
                    case 4: // defend2
                        drawstringz("Mid-range defense.", 7, 19);
                        drawstringz("Helps keep you alive.", 7, 20);
                        break;
                    case 5: // exit
                        drawstringz("Come back soon!", 7, 19);
                        break;                        
                    case 6: // attack3
                        drawstringz("Heavy attack.", 7, 19);
                        drawstringz("Dodge this!", 7, 20);
                        break;
                    case 7: // defend3
                        drawstringz("Heavy defense.", 7, 19);
                        drawstringz("Almost impenetrable.", 7, 20);
                        break;
                    case 8: // atk1def1
                        drawstringz("Attack while you defend!", 7, 19);
                        drawstringz("Literally worth two cards.", 7, 20);
                        break;
                    case 9: // atk2def1
                        drawstringz("Heavy attack while you defend!", 7, 19);
                        drawstringz("It's an advanced technique.", 7, 20);
                        break;
                    case 10: // atk1def2
                        drawstringz("Heavy defense while you attack!", 7, 19);
                        drawstringz("Great long-term strategy.", 7, 20);
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
