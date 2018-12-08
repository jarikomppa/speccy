#include "main.h"

void heal()
{
    char i;
    char pos = 1;
    char commit = 0;
    char frame = 0;
    fillback();
    drawtextbox(6,18,25,5);
    drawmug(3, 1, 17);
	
	if (player_hurt>0) pos = 0;
    
    if (player_hurt == 0)
    {
        drawstringz("This is a place of healing. You're fine.", 7, 20);
    }
    else
    if (player_hurt < 5)
    {
        drawstringz("Ah, that's just a scratch.", 7, 20);
    }
    else
    if (player_hurt < 9)
    {
        drawstringz("Ouch, that must have hurt.", 7, 20);
    }
    else
    {
        drawstringz("It's a miracle you're still standing!", 7, 20);
    }
    
    
    for (i = 0; i < player_hurt; i++)
    {
        if (i < 5)
            drawcard(CARD_HURT, 1 + i * 4, 1, COLOR(0,1,0,7));
        else
        if (i < 9)
            drawcard(CARD_HURT, 3 + (i - 5) * 4, 4, COLOR(0,1,0,7));
        else
            drawcard(CARD_HURT, 5 + (i - 9) * 4, 7, COLOR(0,1,0,7));
    }
    
    drawtextbox(22,1,9,4);
    drawstringz("Heal", 25, 2);
    drawstringz("2 hurt", 25, 3);

    drawtextbox(22,6,9,4);
    drawstringz("Done", 25, 7);
    drawstringz("shopping", 25, 8);

    drawtextbox(7,14,20,3);
    drawmoney(9, 15, player_money);

    while (1) 
    {
        static const unsigned char positionsy[2] = { 2, 7 };
        scan_input();
        {
            unsigned char triggered = 0;
            unsigned char oldpos = pos;
            if (TRIGGER(KEY_LEFT) || TRIGGER(KEY_RIGHT) || TRIGGER(KEY_DOWN) || TRIGGER(KEY_UP)) { triggered = 1; pos = 1 - pos; }
            if (TRIGGER(KEY_FIRE)) { triggered = 1; commit = 1; }
            if (triggered) 
            {
                key_wasdown = 0;
                if (commit)
                {
                    if (pos == 1)
					{
						game_state = 0;
                        return;
					}
					if (player_money > 0 && player_hurt > 0)
					{
						player_hurt--;
						if (player_hurt>0) player_hurt--;
						player_money--;
						return;
                    }
                }
                if (pos != oldpos)
                {
                    if (oldpos == 1)
                    {
                        drawtextbox(22,6,9,4);
                        drawstringz("Done", 25, 7);
                        drawstringz("shopping", 25, 8);
                    }
                    else
                    {
                        drawtextbox(22,1,9,4);
                        drawstringz("Heal", 25, 2);
                        drawstringz("2 hurt", 25, 3);
                    }
                }
                cleartextbox(6+1,18+1,25-2,5-2);

                switch (pos)
                {
                    case 0: // heal
                        drawstringz("Heal wounds by spending crowns.", 7, 19);
                        drawstringz("(You heal 2 cards after every match)", 7, 20);
                        drawcost(7,21,1);
                        break;
                    case 1: // done
                        drawstringz("You'll be back..", 7, 20);
                        break;
                }
                

            }
        }
        frame++;
        drawicon(littlesin[frame & 15], 22, positionsy[pos]);
        do_halt();
        do_halt();
    }
}
