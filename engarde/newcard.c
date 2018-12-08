#include "main.h"

void newcard(unsigned char newcardid)
{
    unsigned short i;
    unsigned char pos = 0;
    unsigned char commit = 0;
    unsigned char frame = 0;
    fillback();
    for (i = 0; i < 5; i++)
    {
        drawcard(playerdeck[i   ], 1 + i * 4, 0, COLOR(0,1,0,7));
        drawcard(playerdeck[i+ 5], 1 + i * 4, 6, COLOR(0,1,0,7));
        drawcard(playerdeck[i+10], 1 + i * 4, 12, COLOR(0,1,0,7));
        drawcard(playerdeck[i+15], 1 + i * 4, 18, COLOR(0,1,0,7));
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
                key_wasdown = 0;
                if (commit)
                {
					if (pos > 4) pos--;
					playerdeck[pos] = newcardid;
                    return;
                }
                if (oldpos < 5)
                    drawcard(playerdeck[oldpos],oldpos * 4 + 1, 0 ,COLOR(0,1,0,7));
                if (oldpos == 5)
                    drawcard(newcardid, 22, 0, COLOR(0,1,0,7));
                if (oldpos > 5 && oldpos < 11)
                    drawcard(playerdeck[oldpos-1], (oldpos - 6) * 4 + 1, 6, COLOR(0,1,0,7));
                if (oldpos > 10 && oldpos < 16)
                    drawcard(playerdeck[oldpos-1], (oldpos - 11) * 4 + 1, 12, COLOR(0,1,0,7));
                if (oldpos > 15)
                    drawcard(playerdeck[oldpos-1], (oldpos - 16) * 4 + 1, 18, COLOR(0,1,0,7));                
            }
        }
        frame++;
        drawicon(littlesin[frame & 15], positionsx[pos], positionsy[pos]);
        do_halt();
        do_halt();
    }
}
