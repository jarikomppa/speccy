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

unsigned char max_intro;
unsigned char max_question;

unsigned short framecounter;

unsigned short score[4];

#define HWIF_IMPLEMENTATION

#include "yofstab.h"
#include "hwif.c"
#include "textout.c"

void playfx(unsigned short fx) __z88dk_fastcall;
enum SFX
{
    SFX_SELECT = 0,
    SFX_FAIL,
    SFX_CORRECT,
    SFX_E,
    SFX_A,
    SFX_O,
    SFX_K,
    SFX_P,
    SFX_S,
    SFX_PT,
    SFX_X    
};

char playchar(char c)
{
    switch (c)
    {
        case 0x66:
        case ' ':
        case '.':
        case '?':
        case '!':
        case '/':
        case '&':
        case '*':
        case '+':
        case '-':
        case '\n':
            return 0;
            break;
        case 'e':
        case 'E':
            playfx(SFX_E);
            break;
        case 'a':
        case 'A':
            playfx(SFX_A);
            break;
        case 'o':
        case 'O':
            playfx(SFX_O);
            break;
        case 'c':
        case 'C':
        case 'k':
        case 'K':
            playfx(SFX_K);
            break;
        case 'p':
        case 'P':
            playfx(SFX_P);
            break;
        case 's':
        case 'S':
            playfx(SFX_S);
            break;
        case 't':
        case 'T':
            playfx(SFX_PT);
            break;
        case 'x':
        case 'X':
            playfx(SFX_X);
            break;
        default:
            playfx(SFX_E + (c & 7));
            break;
    }
    return 1;
}


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
    questionno += max_intro;
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

char mouth(unsigned char v)
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
    return playchar(v);
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
    {
        for (i = 0; i < 16; i++)
        {
            st((unsigned char*)yofs[16 * 8 + i] +  3, 0, 12);
            st((unsigned char*)yofs[16 * 8 + i] + 17, 0, 12);
            st((unsigned char*)yofs[20 * 8 + i] +  3, 0, 12);
            st((unsigned char*)yofs[20 * 8 + i] + 17, 0, 12);
        }
        st((unsigned char*)0x5800+16*32+3,7,12);
        st((unsigned char*)0x5800+17*32+3,7,12);
    
        st((unsigned char*)0x5800+20*32+3,7,12);
        st((unsigned char*)0x5800+21*32+3,7,12);
    
        st((unsigned char*)0x5800+16*32+17,7,12);
        st((unsigned char*)0x5800+17*32+17,7,12);
    
        st((unsigned char*)0x5800+20*32+17,7,12);
        st((unsigned char*)0x5800+21*32+17,7,12);
    }
}

void setplayerbright(unsigned char player, char bright)
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
    if (bright < 0)
        v = 0;
    // y = 11
    st((unsigned char*)0x5800+11*32+xofs,v,3);
    st((unsigned char*)0x5800+12*32+xofs,v,5);
    *((unsigned char*)0x5800+13*32+xofs) = v;
    *((unsigned char*)0x5804+13*32+xofs) = v;
    st((unsigned char*)0x5800+14*32+xofs,v,5);
}

void setanswerbright(unsigned char answer, char bright)
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
    if (bright < 0)
        v = 0;
    st((unsigned char*)0x5800+yofs*32+xofs,v,14);
    *((unsigned char*)0x5820+yofs*32+xofs) = v;
    *((unsigned char*)0x582d+yofs*32+xofs) = v;
    *((unsigned char*)0x5840+yofs*32+xofs) = v;
    *((unsigned char*)0x584d+yofs*32+xofs) = v;
    st((unsigned char*)0x5860+yofs*32+xofs,v,14);
}

char gamemode;
char players;

void mainmenu()
{
    char keydown = 0;
    char done = 0;
    char menumode = 0;
    unsigned short ticker = 0;
    unsigned short robotalk = 0;
    unsigned char blurbno = 0;
    clearfields(1,1);
    setanswerbright(0, 0);
    setanswerbright(1, 0);
    setanswerbright(2, 0);
    setanswerbright(3, 0);                       
    setplayerbright(0, 0);
    setplayerbright(1, 0);
    setplayerbright(2, 0);
    setplayerbright(3, 0);
    
    get_string(0, tempq);
    
    while (!done)
    {
        readkeyboard();
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
                if (keydown)
                    playfx(SFX_SELECT);
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
                if (keydown)                    
                    playfx(SFX_SELECT);                
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
            drawstringfancy(tempq+1, 15, 1, 7, robotalk);
            if (robotalk >= *tempq)
            {
                robotalk = 170;
                mouth(0x66);
                do_halt();
                do_halt();
                eyes(1);
            }
            else
            {
                if (!mouth(tempq[robotalk]))
                {
                    do_halt();
                    do_halt();
                }                    
            }            
        }
        else
        {
            do_halt();
            do_halt();
            do_halt();
            do_halt();
        }

        if (robotalk == 199)
        {
            clearfields(1, 0);
            eyes(0);
            robotalk = 0;
            blurbno++;
            if (blurbno == max_intro)
                blurbno = 0;
            get_string(blurbno, tempq);
        }
        
        robotalk++;
        xorshift16();        
    }    
    score[0] = 0;
    score[1] = 0;
    score[2] = 0;
    score[3] = 0;
}

void printscore(char player)
{
    char temp[4];
    char xofs;
    unsigned short sum = score[player];
    temp[0] = '0';
    temp[1] = '0';
    temp[2] = '0';
    temp[3] = 0;
    switch (player)
    {
    default:
    case 0:
        xofs = 6;
        break;
    case 1:
        xofs = 12;
        break;
    case 2:
        xofs = 18;
        break;
    case 3:
        xofs = 24;
        break;
    }
    while (sum >= 100 && temp[0] != '9')
    {
        temp[0]++;
        sum -= 100;
    }
    while (sum >= 100)
    {
        sum -= 100;
    }
    while (sum >= 10)
    {
        temp[1]++;
        sum -= 10;
    }
    while (sum >= 1)
    {
        temp[2]++;
        sum -= 1;
    }
    
    drawstringfancy(temp, xofs, 13, 7, 255);
}

void ingame()
{
    char done = 0;
    char currentplayer = 0;
    unsigned short turn = 0;
    unsigned short maxturn = 0;
    unsigned short robotalk = 0;
    unsigned char keydown = 0;
    char i;
    for (i = 0; i < players; i++)
        maxturn += 12;
    
    printscore(0);
    printscore(1);
    printscore(2);
    printscore(3);
    
    while (!done)
    {
        char *a[4];
        char correct;
        char what;
        char who;
        unsigned short q;
        what = 0;
        who = 0;
        
        if (gamemode != 4)
        {
            setplayerbright(0, 0);
            setplayerbright(1, 0);
            setplayerbright(2, 0);
            setplayerbright(3, 0);
            setplayerbright(currentplayer, 1);
        }
            

        clearfields(1,1);
        q = xorshift16() & 255;
        while (q >= max_question) q = xorshift16() & 255;
        get_question(q);
        a[0] = tempa1;
        a[1] = tempa2;
        a[2] = tempa3;
        a[3] = tempa4;
        for (i = 0; i < 8; i++)
        {
            char s1 = xorshift8() & 3;
            char s2 = xorshift8() & 3;
            char *t = a[s1];
            a[s1] = a[s2];
            a[s2] = t;
        }
        correct = 0;
        if (a[0] == tempa1) correct = 0;
        if (a[1] == tempa1) correct = 1;
        if (a[2] == tempa1) correct = 2;
        if (a[3] == tempa1) correct = 3;
        robotalk = 0;

        while (keydown == 0)
        {
            readkeyboard();
            drawstringfancy(a[0]+1,3,16,7,(robotalk < *a[0]) ? robotalk : *a[0]);
            drawstringfancy(a[1]+1,17,16,7,(robotalk < *a[1]) ? robotalk : *a[1]);
            drawstringfancy(a[2]+1,3,20,7,(robotalk < *a[2]) ? robotalk : *a[2]);
            drawstringfancy(a[3]+1,17,20,7,(robotalk < *a[3]) ? robotalk : *a[3]);
            drawstringfancy(tempq+1,15,1,7,(robotalk < *tempq) ? robotalk : *tempq);
            if (robotalk >= *tempq)
            {
                mouth(0x66);
                eyes(!(robotalk & 0x80));
                do_halt();
                do_halt();
            }
            else
            {
                if (!mouth(tempq[robotalk]))
                {
                    do_halt();
                    do_halt();
                }
                eyes(!(robotalk & 0xc));
            }
            if (gamemode == 4)
            {
                switch (players)
                {
                    default:
                    case 1:
                        print_keys(0,0);
                        break;
                    case 2:
                        print_keys(0,1);
                        print_keys(1,0);
                        break;
                    case 3:
                        print_keys(0,2);
                        print_keys(1,1);
                        print_keys(2,0);
                        break;
                    case 4:
                        print_keys(0,3);
                        print_keys(1,2);
                        print_keys(2,1);
                        print_keys(3,0);
                        break;
                }
            }
            else
            {
                print_keys(currentplayer, 0);
            }
            
            if (gamemode < 3 || gamemode == 3 && currentplayer == 0 || gamemode == 4)
            {
                if (KEYDOWN(1))
                {
                    what = 0;
                    who = 0;
                    keydown = 1;
                }
                if (KEYDOWN(2))
                {
                    what = 1;
                    who = 0;
                    keydown = 1;
                }
                if (KEYDOWN(3))
                {
                    what = 2;
                    who = 0;
                    keydown = 1;
                }
                if (KEYDOWN(4))
                {
                    what = 3;
                    who = 0;
                    keydown = 1;
                }
            }
            if (gamemode == 3 && currentplayer == 1 || gamemode == 4)
            {
                if (KEYDOWN(7))
                {
                    what = 0;
                    who = 1;
                    keydown = 1;
                }
                if (KEYDOWN(8))
                {
                    what = 1;
                    who = 1;
                    keydown = 1;
                }
                if (KEYDOWN(9))
                {
                    what = 2;
                    who = 1;
                    keydown = 1;
                }
                if (KEYDOWN(0))
                {
                    what = 3;
                    who = 1;
                    keydown = 1;
                }
            }       
            if (gamemode == 3 && currentplayer == 2 || gamemode == 4)
            {
                if (KEYDOWN(Z))
                {
                    what = 0;
                    who = 2;
                    keydown = 1;
                }
                if (KEYDOWN(X))
                {
                    what = 1;
                    who = 2;
                    keydown = 1;
                }
                if (KEYDOWN(C))
                {
                    what = 2;
                    who = 2;
                    keydown = 1;
                }
                if (KEYDOWN(V))
                {
                    what = 3;
                    who = 2;
                    keydown = 1;
                }
            }       
            if (gamemode == 3 && currentplayer == 3 || gamemode == 4)
            {
                if (KEYDOWN(H))
                {
                    what = 0;
                    who = 3;
                    keydown = 1;
                }
                if (KEYDOWN(J))
                {
                    what = 1;
                    who = 3;
                    keydown = 1;
                }
                if (KEYDOWN(K))
                {
                    what = 2;
                    who = 3;
                    keydown = 1;
                }
                if (KEYDOWN(L))
                {
                    what = 3;
                    who = 3;
                    keydown = 1;
                }
            }       
            robotalk++;
            xorshift16();
        }
        drawstringfancy(a[0]+1,3,16,7,*a[0]);
        drawstringfancy(a[1]+1,17,16,7,*a[1]);
        drawstringfancy(a[2]+1,3,20,7,*a[2]);
        drawstringfancy(a[3]+1,17,20,7,*a[3]);
        drawstringfancy(tempq+1,15,1,7,*tempq);
        mouth(0x66);
        eyes(0);
        
        if (what == correct)
        {
            score[who]++;
            printscore(who);
            setplayerbright(who, 1);       
            setanswerbright(correct, 1);
            playfx(SFX_CORRECT);
            setplayerbright(who, 0);       
            setanswerbright(correct, -1);
            playfx(SFX_CORRECT);
            setplayerbright(who, 1);       
            setanswerbright(correct, 1);
            playfx(SFX_CORRECT);
            setplayerbright(who, 0);       
            setanswerbright(correct, 0);
            if (gamemode < 2)
                setplayerbright(who, 1);       
        }
        else
        {
            if (gamemode == 4 && score[who] > 0)
                score[who]--;
            printscore(who);
            setplayerbright(who, 1);       
            setanswerbright(correct, 1);
            playfx(SFX_FAIL);
            setanswerbright(correct, 1);
            playfx(SFX_FAIL);
            setanswerbright(correct, -1);
            playfx(SFX_FAIL);
            setanswerbright(correct, 1);
            playfx(SFX_FAIL);
            setanswerbright(correct, 1);
            playfx(SFX_FAIL);
            setanswerbright(correct, 0);
            if (gamemode > 1)
                setplayerbright(who, 0);       
        }
        
        while (ANYKEY())
        {
            readkeyboard();
        }
        
        // if a[x] == tempa1, we have correct answer
        
        keydown = 0;
        
        currentplayer++;
        if (currentplayer == players)
            currentplayer = 0;
        
        turn++;
        if (turn == maxturn && gamemode != 2)
            done = 1;
    }
    clearfields(1,1);
    eyes(0);
    
    robotalk = 0;
    for (robotalk = 0; robotalk < 200; robotalk++)
    {
        static const char blurb[] =
        //1234567890123456
         "And that was\n"
         "our game.\n"
         "\n"
         "What fun we\n"
         "had.\n"
         "\n"
         "Play again\n"
         "sometime,\n"
         "I won't mind.";
        drawstringfancy(blurb, 15, 1, 7, robotalk);
        if (robotalk < sizeof(blurb))
        {
            if (!mouth(blurb[robotalk]))
            {
                do_halt();
                do_halt();
            }
        }
        else
        {
            mouth(0x66);
            do_halt();
            do_halt();
        }
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

    max_intro = *(char*)(0x5b00 + (192*32+24*32) + 0);
    max_question = *(char*)(0x5b00 + (192*32+24*32) + 1);

    dict[0] = (char*)(0x5b00 + (192*32+24*32) + 2);
    for (i = 1; i < 120; i++)
    {
        dict[i] = dict[i-1] + *dict[i-1] + 1;
    }
    
    get_question(50);
        
    cp((unsigned char*)0x4000, 6912, (char*)0x5b00);

    drawstringfancy("000", 6, 13, 7, 255);
    drawstringfancy("000", 12, 13, 7, 255);
    drawstringfancy("000", 18, 13, 7, 255);
    drawstringfancy("000", 24, 13, 7, 255);
               
    y8 = 1;
    y16 = 1;
      
    while(1)
    {         
        mainmenu();
        ingame();
    }
}
