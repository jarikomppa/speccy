#include "main.h"

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
