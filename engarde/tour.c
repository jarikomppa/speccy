#include "main.h"

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
