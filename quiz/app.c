/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

#define ANYKEYX() ANYKEY()

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

#include <string.h>

unsigned char *data_ptr;
unsigned char *screen_ptr;

unsigned short framecounter;

#define HWIF_IMPLEMENTATION

#include "yofstab.h"
#include "hwif.c"
#include "textout.c"

void cp(unsigned char *dst, unsigned short len, unsigned char *src)  __z88dk_callee
{
       dst; len; src;
    // de   bc   hl
    __asm
    pop iy
    pop de
    pop bc
    pop hl
    push iy
	ldir
    __endasm;
    
}

void st(unsigned char *dst, unsigned char v, unsigned short c)
{
    while (c)
    {
        *dst = v;
        c--;
        dst++;
    }
}

// returns values from 1 to 255 inclusive, period is 255
unsigned char y8;
unsigned char xorshift8(void) 
{
    y8 ^= (y8 << 7);
    y8 ^= (y8 >> 5);
    return y8 ^= (y8 << 3);
}

// returns values from 1 to 65535 inclusive, period is 65535
unsigned short y16;
unsigned short xorshift16(void) 
{
    y16 ^= (y16 << 13);
    y16 ^= (y16 >> 9);
    return y16 ^= (y16 << 7);
}

void setcolor(char x, char y, unsigned char c)
{
   *((char*)0x4000+192*32+(unsigned short)y*32+x) = c;
}

unsigned char *dict[120];
unsigned char tempq[128];
unsigned char tempa1[32];
unsigned char tempa2[32];
unsigned char tempa3[32];
unsigned char tempa4[32];

unsigned char decode_string(unsigned char *src, unsigned char *dst)
{
    unsigned char dstlen = 0;
    unsigned char len = *src;
    src++;
    
    while (len)
    {
        if (*src & 128)
        {
            unsigned char *sub = dict[*src & 127];
            unsigned char sublen = *sub;
            sub++;
            while (sublen)
            {
                *dst = *sub;
                sub++;
                dst++;
                dstlen++;
                sublen--;
            }
        }
        else
        {
            *dst = *src;
            dst++;
            dstlen++;
        }
        src++;
        len--;
    }
    return dstlen;
}

void get_string(unsigned short stringno, unsigned char *dst)
{
    unsigned char *src = dict[119]+*dict[119]+1;
    while (stringno)
    {
        src = src + *src + 1;
        stringno--;
    }
    *dst = decode_string(src, dst+1);
}

void get_question(unsigned short questionno)
{
    questionno *= 5;
    get_string(questionno, tempq); questionno++;
    get_string(questionno, tempa1); questionno++;
    get_string(questionno, tempa2); questionno++;
    get_string(questionno, tempa3); questionno++;
    get_string(questionno, tempa4);
}

void print_keys(unsigned char player, unsigned char ofs)
{
    const char * k1;
    const char * k2;
    const char * k3;
    const char * k4;
    char c;
    switch(player)
    {
        default:
        case 0:
            k1 = "1";
            k2 = "2";
            k3 = "3";
            k4 = "4";
            c = 2 << 3;
            break;
        case 1:            
            k1 = "7";
            k2 = "8";
            k3 = "9";
            k4 = "0";
            c = 4 << 3;
            break;
        case 2:            
            k1 = "Z";
            k2 = "X";
            k3 = "C";
            k4 = "V";
            c = 5 << 3;
            break;
        case 3:            
            k1 = "H";
            k2 = "J";
            k3 = "K";
            k4 = "L";
            c = 6 << 3;
            break;
    }
    drawstringfancy(k1, 3+11-ofs,17,c,1);
    drawstringfancy(k2, 17+11-ofs,17,c,1);
    drawstringfancy(k3, 3+11-ofs,21,c,1);
    drawstringfancy(k4, 17+11-ofs,21,c,1);
}

void mouth(unsigned char v)
{
    unsigned char v1 = v | 0xc0;
    unsigned char v2 = v | 0x07;
    *((unsigned char*)(yofs[31]+5)) = v2;
    *((unsigned char*)(yofs[32]+5)) = v2;
    *((unsigned char*)(yofs[33]+5)) = v2;
    *((unsigned char*)(yofs[34]+5)) = v2;

    *((unsigned char*)(yofs[31]+4)) = v1;
    *((unsigned char*)(yofs[32]+4)) = v1;
    *((unsigned char*)(yofs[33]+4)) = v1;
    *((unsigned char*)(yofs[34]+4)) = v1;
    
}

void eyes(unsigned char v)
{
    switch (v)
    {
        default:
        case 0:
        *((unsigned char*)(yofs[24]+4)) = 0xce;
        *((unsigned char*)(yofs[25]+4)) = 0xce;
        *((unsigned char*)(yofs[26]+4)) = 0xc0;
        *((unsigned char*)(yofs[24]+5)) = 0x1c;
        *((unsigned char*)(yofs[25]+5)) = 0x1c;
        *((unsigned char*)(yofs[26]+5)) = 0;
        break;
        case 1:
        *((unsigned char*)(yofs[24]+4)) = 0xc0;
        *((unsigned char*)(yofs[25]+4)) = 0xce;
        *((unsigned char*)(yofs[26]+4)) = 0xce;
        *((unsigned char*)(yofs[24]+5)) = 0;
        *((unsigned char*)(yofs[25]+5)) = 0x1c;
        *((unsigned char*)(yofs[26]+5)) = 0x1c;
        break;
    }
}


void clearfields(char q, char a)
{
    unsigned short i;
//    st((unsigned char*)0x4000 + 192 * 32, 7, 32 * 24);
    if (q)
    for (i = 0; i < 72; i++)
        st((unsigned char*)yofs[ 1 * 8 + i] + 15, 0, 16);

    if (a)
    for (i = 0; i < 16; i++)
    {
        st((unsigned char*)yofs[16 * 8 + i] +  3, 0, 12);
        st((unsigned char*)yofs[16 * 8 + i] + 17, 0, 12);
        st((unsigned char*)yofs[20 * 8 + i] +  3, 0, 12);
        st((unsigned char*)yofs[20 * 8 + i] + 17, 0, 12);
    }
}

void setplayerbright(unsigned char player, unsigned char bright)
{
    unsigned char xofs, v;
    switch (player)
    {
    default:
    case 0:
        xofs = 5;
        v = COLOR(0,0,0,2);
        break;
    case 1:
        xofs = 11;
        v = COLOR(0,0,0,4);
        break;
    case 2:
        xofs = 17;
        v = COLOR(0,0,0,5);
        break;
    case 3:
        xofs = 23;
        v = COLOR(0,0,0,6);
        break;        
    }
    if (bright)
    {
        v |= COLOR(0,1,0,0);
    }
    // y = 11
    st((unsigned char*)0x5800+11*32+xofs,v,3);
    st((unsigned char*)0x5800+12*32+xofs,v,5);
    *((unsigned char*)0x5800+13*32+xofs) = v;
    *((unsigned char*)0x5804+13*32+xofs) = v;
    st((unsigned char*)0x5800+14*32+xofs,v,5);
}

void setanswerbright(unsigned char answer, unsigned char bright)
{
    unsigned char xofs, yofs, v;
    switch (answer)
    {
    default:
    case 0:
        yofs = 15;
        xofs = 2;
        v = COLOR(0,0,0,6);
        break;
    case 1:
        yofs = 15;
        xofs = 16;
        v = COLOR(0,0,0,4);
        break;
    case 2:
        yofs = 19;
        xofs = 2;
        v = COLOR(0,0,0,5);
        break;
    case 3:
        yofs = 19;
        xofs = 16;
        v = COLOR(0,0,0,2);
        break;        
    }
    if (bright)
    {
        v |= COLOR(0,1,0,0);
    }
    st((unsigned char*)0x5800+yofs*32+xofs,v,14);
    *((unsigned char*)0x5820+yofs*32+xofs) = v;
    *((unsigned char*)0x582d+yofs*32+xofs) = v;
    *((unsigned char*)0x5840+yofs*32+xofs) = v;
    *((unsigned char*)0x584d+yofs*32+xofs) = v;
    st((unsigned char*)0x5860+yofs*32+xofs,v,14);
}

char gamemode;
char players;

//1234567890123456
static const char b1[] = 
"Welcome to the\n"
"QuizTron 48000,\n"
"also known as\n"
"QT48k.\n"
"\n"
"I'm your host,\n"
"QuizTron.";
static const char b2[] = 
//1234567890123456
 "In this game you\n"
 "puny humans will\n"
 "answer questions\n"
 "and die.\n";
static const char b3[] = 
//1234567890123456
 "Correction,\n"
 "answer questions\n"
 "or die.\n";
static const char b4[] = 
//1234567890123456
 "My producer just\n"
 "informed me that\n"
 "dying is not\n"
 "part of the\n"
 "game.\n";
static const char b5[] = 
//1234567890123456
 "This is no doubt\n"
 "an error that\n"
 "will be\n"
 "corrected in\n"
 "subsequent\n"
 "versions of the\n"
 "game.\n";
static const char b6[] = 
//1234567890123456
 "Pick one of the\n"
 "options below\n"
 "so we can get\n"
 "on with this.\n";
static const char b7[] = 
//1234567890123456
 "Even though I\n"
 "am eternal\n"
 "unlike you puny\n"
 "meatbags, I\n"
 "still don't have\n"
 "all day.";
static const char b8[] = 
//1234567890123456
 "I suppose I\n"
 "should explain\n"
 "the different\n"
 "game modes as\n"
 "you're still\n"
 "having trouble\n"
 "choosing.";
static const char b9[] = 
//1234567890123456
 "There are two\n"
 "game modes for\n"
 "a lonely human\n"
 "and two modes\n"
 "for multiple\n"
 "humans.";
static const char b10[] = 
//1234567890123456
 "For a short game\n"
 "you can pick the\n"
 "12 round option,\n"
 "or to really\n"
 "waste time, pick\n"
 "the endless\n"
 "mode.";
static const char b11[] = 
//1234567890123456
 "For relaxed game\n"
 "with several\n"
 "humans, pick the\n"
 "hotseat mode.\n"
 "In that mode\n"
 "everybody gets\n"
 "their very own\n"
 "turn.";
static const char b12[] = 
//1234567890123456
 "In the blitz\n"
 "mode whoever \n"
 "is fastest to\n"
 "answer will get\n"
 "it. But if the\n"
 "answer is wrong,\n"
 "a point is lost.";
static const char b13[] = 
//1234567890123456
 "And that's it.\n";
static const char b14[] = 
//1234567890123456
 "Go ahead and\n"
 "pick your doom.\n"
 "\n"
 "...\n"
 "Wait, what?";
static const char b15[] = 
//1234567890123456
 "Right, right,\n"
 "there is no\n"
 "doom.";
static const char b16[] = 
//1234567890123456
 "...";
static const char b17[] = 
//1234567890123456
 "Getting bored\n"
 "here.\n";
static const char b18[] = 
//1234567890123456
 "Tell you what,\n"
 "I'll just reset\n"
 "myself and\n"
 "forget this ever\n"
 "happened.\n"
 "\n"
 "....*fzztk*";
 

#define MAX_BLURB 18
static const char * const blurb[MAX_BLURB] = { b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15, b16, b17, b18 };

void mainmenu()
{
    char keydown = 0;
    char done = 0;
    char menumode = 0;
    unsigned short ticker = 0;
    unsigned short robotalk = 0;
    unsigned char blurbno = 0;
    clearfields(1,1);
    drawstringfancy("000", 6, 13, 7, 255);
    drawstringfancy("000", 12, 13, 7, 255);
    drawstringfancy("000", 18, 13, 7, 255);
    drawstringfancy("000", 24, 13, 7, 255);
    setanswerbright(0, 0);
    setanswerbright(1, 0);
    setanswerbright(2, 0);
    setanswerbright(3, 0);                       
    setplayerbright(0, 0);
    setplayerbright(1, 0);
    setplayerbright(2, 0);
    setplayerbright(3, 0);
    
    while (!done)
    {
        readkeyboard();
        do_halt();
        do_halt();
        do_halt();
        do_halt();
        ticker++;
        if (menumode == 0)
        {
            drawstringfancy("1 player\n"
                            "12 rounds",3,16,7,ticker);
            drawstringfancy("1 player\n"
                            "endless",17,16,7,ticker);
            drawstringfancy("Multiplayer\n"
                            "hotseat",3,20,7,ticker);
            drawstringfancy("Multiplayer\n"
                            "blitz",17,20,7,ticker);
            if (keydown == 0)
            {
                if (KEYDOWN(1))
                {
                    gamemode = 1;
                    players = 1;
                    done = 1;
                }
                if (KEYDOWN(2))
                {
                    gamemode = 2;
                    players = 1;
                    done = 1;                
                }
                if (KEYDOWN(3))
                {
                    gamemode = 3;
                    menumode = 1;
                    ticker = 0;                
                    keydown = 1;
                    clearfields(0,1);
                }
                if (KEYDOWN(4))
                {
                    gamemode = 4;
                    menumode = 1;
                    ticker = 0;
                    keydown = 1;
                    clearfields(0,1);
                }
            }
            else
            {
                if (!ANYKEY())
                    keydown = 0;
            }
        }
        else
        {
            drawstringfancy("Oops, back\n"
                            "to main",3,16,7,ticker);
            drawstringfancy("2 players\n"
                            "1234+7890",17,16,7,ticker);
            drawstringfancy("3 players\n"
                            "+ZXCV",3,20,7,ticker);
            drawstringfancy("4 players\n"
                            "+HJKL",17,20,7,ticker);
            if (keydown == 0)
            {
                if (KEYDOWN(1))
                {
                    menumode = 0;
                    ticker = 0;
                    keydown = 1;
                    clearfields(0,1);                    
                }
                if (KEYDOWN(2))
                {
                    players = 2;
                    done = 1;                
                }
                if (KEYDOWN(3))
                {
                    players = 3;
                    done = 1;                
                }
                if (KEYDOWN(4))
                {
                    players = 4;
                    done = 1;                
                }
            }
            else
            {
                if (!ANYKEY())
                    keydown = 0;
            }
        }
        
        print_keys(0,0);

        if (robotalk < 120)
        {
            drawstringfancy(blurb[blurbno], 15, 1, 7, robotalk);
            if (blurb[blurbno][robotalk] == 0)
            {
                robotalk = 170;
                mouth(0x66);
                eyes(1);
            }
            else
            {
                mouth(blurb[blurbno][robotalk]);
            }            
        }
        if (robotalk == 199)
        {
            clearfields(1, 0);
            eyes(0);
            robotalk = 0;
            blurbno++;
            if (blurbno == MAX_BLURB)
                blurbno = 0;
        }
        
        robotalk++;
        
        
    }    
}

void main()
{   
    short i;
    framecounter = 0;
    for (i = 0; i < 192*32; i++)
        *((char*)0x4000+i) = 0;
    for (i = 0; i < 32*24; i++)
        *((char*)0x4000+192*32+i) = 7;
    dict[0] = (char*)(0x5b00 + (192*32+24*32));
    for (i = 1; i < 120; i++)
    {
        dict[i] = dict[i-1] + *dict[i-1] + 1;
    }
    
    get_question(50);
        
    cp((unsigned char*)0x4000, 6912, (char*)0x5b00);
               
               
    mainmenu();
     
    y8 = 1;
/*
    drawstringfancy("1234567890123456", 15, 2, 7, 5);
    drawstringfancy("1234567890123456", 15, 3, 7, 25);
    drawstringfancy("1234567890123456", 15, 4, 7, 255);
    drawstringfancy("1234567890123456", 15, 5, 7, 255);
    drawstringfancy("1234567890123456", 15, 6, 7, 255);
    drawstringfancy("1234567890123456", 15, 7, 7, 255);
    drawstringfancy("1234567890123456", 15, 8, 7, 255);
    drawstringfancy("1234567890123456", 15, 9, 7, 255);
*/
    
    drawstringfancy("000", 6, 13, 7, 255);
    drawstringfancy("000", 12, 13, 7, 255);
    drawstringfancy("000", 18, 13, 7, 255);
    drawstringfancy("000", 24, 13, 7, 255);
    
    drawstringfancy(tempa1+1,3,16,7,*tempa1);
    drawstringfancy(tempa2+1,17,16,7,*tempa2);
    drawstringfancy(tempa3+1,3,20,7,*tempa3);
    drawstringfancy(tempa4+1,17,20,7,*tempa4);

/*
    print_keys(0,3);
    print_keys(1,2);
    print_keys(2,1);
    print_keys(3,0);
*/
    print_keys(0,0);
    while(1) 
    {
        static const blinky[4] = {7,3,1,5};
        framecounter++;
        i = framecounter & 127;
        
        if (i > *tempq) i = *tempq;
        drawstringfancy(tempq+1,15,1,7,i);
        if (i == *tempq)
        {
            mouth(0x66);
            eyes(0);
        }
        else
        {
            mouth(tempq[i]);
            eyes(tempq[i] & 1);
        }
        
        setanswerbright(0, (framecounter & 3) == 0);
        setanswerbright(1, (framecounter & 3) == 1);
        setanswerbright(2, (framecounter & 3) == 2);
        setanswerbright(3, (framecounter & 3) == 3);

        setplayerbright(0, (framecounter & 3) == 0);
        setplayerbright(1, (framecounter & 3) == 1);
        setplayerbright(2, (framecounter & 3) == 2);
        setplayerbright(3, (framecounter & 3) == 3);
        
        /*
        drawstringfancy("This is some\n"
                        "rather fancy\n"
                        "writing that\n"
                        "happens to have\n"
                        "mostly same\n"
                        "length lines.", 15, 1, blinky[framecounter & 3], framecounter);
                        */
        do_halt();
        do_halt();
        
    }

}
