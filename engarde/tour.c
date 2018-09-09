#include "main.h"

const char stagecolors[3] = {
	COLOR(0,0,7,0), // locked
	COLOR(0,1,7,0), // fight
	COLOR(0,0,0,2)  // done
};


void tour()
{
	char pos = 0;
	char frame = 0;
	char commit = 0;
    // 32x24
    fillback();
    drawtextbox(1,2,4,6);
    drawtextbox(1,10,4,6);
    drawicon(4, 2, 4);    
    drawicon(5, 2, 12);    
       
    drawtextbox_color(5,1,4,16, stagecolors[stage[0]]);
    drawicon(3, 6, 8);
    
    drawtextbox_color(9,1,4,8, stagecolors[stage[1]]);
    drawtextbox_color(9,9,4,8, stagecolors[stage[2]]);
    drawicon(3, 10, 4);
    drawicon(3, 10, 12);
    
    drawtextbox_color(13,1,4,4, stagecolors[stage[3]]);
    drawtextbox_color(13,5,4,4, stagecolors[stage[4]]);
    drawtextbox_color(13,9,4,4, stagecolors[stage[5]]);
    drawtextbox_color(13,13,4,4, stagecolors[stage[6]]);
    drawicon(3, 14, 2);
    drawicon(3, 14, 6);
    drawicon(3, 14, 10);
    drawicon(3, 14, 14);

    drawtextbox_color(17,1,4,8, stagecolors[stage[7]]);
    drawtextbox_color(17,9,4,8, stagecolors[stage[8]]);
    drawicon(3, 18, 4);
    drawicon(3, 18, 12);

    drawtextbox_color(21,1,4,16, stagecolors[stage[9]]);
    drawicon(3, 22, 8);
   
    drawtextbox(1,18,30,5);
    
    drawtextbox(26,5,5,9);
    drawmug(3,26,5);
    drawstringz("Store", 27, 12);
    
    while (1) 
    {
        static const unsigned char positionsx[12] = { 
/*
		2,  6, 10, 14, 18, 22, 
		2,     10, 14, 18,
		           14,
                   14	 };      
*/
		2, 2, 6, 10, 10, 14, 14, 14, 14, 18, 18, 22
	};
				   
        static const unsigned char positionsy[12] = { 
		/*
		4,  8,  4,  2,  4,  8, 
		12,    12,  6, 12, 
		           10,
				   14	};
				   */
		4, 12, 8, 4, 12, 2, 6, 10, 14, 4, 12, 8
		};
				   /*
				    5
		0   2   3   6   9  11
		1       4   7  10
		            8
					
				   */
		static const unsigned char r[12] = {
	/*  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11 */
		2,  2,  3,  6,  7,  9,  9, 10, 10, 11, 11,  0
		};
		static const unsigned char l[12] = {
	/*   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11 */
		11, 11,  0,  2,  2,  3,  3,  4,  4,  6,  7,  9
		};
		static const unsigned char u[12] = {
	/*   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11 */
		 1,  0,  2,  4,  3,  8,  5,  6,  7, 10,  9, 11
		};
		static const unsigned char d[12] = {
	/*   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11 */
		 1,  0,  2,  4,  3,  6,  7,  8,  5, 10,  9, 11
		};

		static const unsigned char posmug[12] = {
		3, 3, 5, 4, 0, 7, 11, 9, 10, 8, 2, 1
		};
		
        scan_input();
        {
            unsigned char triggered = 0;
            unsigned char oldpos = pos;
            if (TRIGGER(KEY_RIGHT)) { triggered = 1; pos = r[pos]; }
            if (TRIGGER(KEY_LEFT)) { triggered = 1; pos = l[pos]; }
            if (TRIGGER(KEY_UP)) { triggered = 1; pos = u[pos]; }
            if (TRIGGER(KEY_DOWN)) { triggered = 1; pos = d[pos]; }
            if (TRIGGER(KEY_FIRE)) { triggered = 1; commit = 1; }
            if (triggered) 
            {
                if (commit)
                {
                    if (pos == 5)
                        return;
                    return;
                }
                key_wasdown = 0;
                if (pos != oldpos)
                {
					char oldico = 3;
					if (oldpos == 0) oldico = 4;
					if (oldpos == 1) oldico = 5;
					drawicon(oldico, positionsx[oldpos], positionsy[oldpos]);

					cleartextbox(26,11,5,3);

					if (pos == 0)
						drawstringz("Store", 27, 12);
					else
					if (pos == 1)
						drawstringz("Heal", 27, 12);
					else
					{
						if (stage[pos-2] == 0)
							drawstringz("Lock!", 27, 12);
						if (stage[pos-2] == 1)
							drawstringz("Fight", 27, 12);
						if (stage[pos-2] == 2)
							drawstringz("Done", 27, 12);
					}

					drawmug(posmug[pos],26,5);						

					cleartextbox(1,18,30,5);
					switch (pos)
					{
						case 0:
							drawstringz("Here you can spend crowns to improve your deck.", 2, 19);
							drawmoney(2, 21, player_money);
							break;
						case 1:
							drawstringz("Here you can spend crowns to heal your wounds.", 2, 19);
							drawmoney(2, 21, player_money);
							break;
						case 2:
							drawstringz("Lorem ipsum1", 2, 19);
							drawstringz("dolor set amet.1", 2, 20);
							break;
						case 3:
							drawstringz("Lorem ipsum2", 2, 19);
							drawstringz("dolor set amet.2", 2, 20);
							break;
						case 4:
							drawstringz("Lorem ipsum3", 2, 19);
							drawstringz("dolor set amet.3", 2, 20);
							break;
						case 5:
							drawstringz("Lorem ipsum4", 2, 19);
							drawstringz("dolor set amet.4", 2, 20);
							break;
						case 6:
							drawstringz("Lorem ipsum5", 2, 19);
							drawstringz("dolor set amet.5", 2, 20);
							break;
						case 7:
							drawstringz("Lorem ipsum6", 2, 19);
							drawstringz("dolor set amet.6", 2, 20);
							break;
						case 8:
							drawstringz("Lorem ipsum7", 2, 19);
							drawstringz("dolor set amet.7", 2, 20);
							break;
						case 9:
							drawstringz("Lorem ipsum8", 2, 19);
							drawstringz("dolor set amet.8", 2, 20);
							break;
						case 10:
							drawstringz("Lorem ipsum9", 2, 19);
							drawstringz("dolor set amet.9", 2, 20);
							break;
						case 11:
							drawstringz("Lord Masdevalja.", 2, 19);
							drawstringz("Your ultimate goal.", 2, 20);
							break;
					}
					if (pos > 1)
					{
						if (stage[pos-2] == 0)
							drawstringz("Locked - beat earlier stages to open.", 2, 21);
						if (stage[pos-2] == 1)
							drawstringz("Open for your challenge.", 2, 21);
						if (stage[pos-2] == 2)
							drawstringz("You've beaten this stage already.", 2, 21);
					}
				}
            }
        }
        frame++;
        drawicon(littlesin[frame & 15], positionsx[pos], positionsy[pos]);
        do_halt();
        do_halt();
    }
}
