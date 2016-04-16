/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

#define FX_STEP 0
#define FX_ROBOTROBOT 1
#define FX_PLAYERROBOT 2
#define FX_ROBOTPLAYER 3
#define FX_SHAPE 4

#define ANYKEYX() ANYKEY()

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

#include <string.h>

unsigned char *data_ptr;
unsigned char *screen_ptr;

unsigned short framecounter;

unsigned char gamestate;
char inputscheme;

#define HWIF_IMPLEMENTATION

#include "yofstab.h"
#include "bobot.h"
#include "player.h"
#include "hwif.c"
#include "textout.c"
#include "sprites.c"

const unsigned char bobotcolor[3] = { 0x2, 0x4, 0x1 };
const unsigned char playercolor[3] = { 0x42, 0x44, 0x41 };
    // 0x41 = blue
    // 0x42 = red
    // 0x44 = green

extern void playfx(unsigned short fx) __z88dk_fastcall;  

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

extern void initfbcopy();
extern void ingame();
extern void init_ingame();
extern void gameover();
extern void mainmenu();

unsigned short score;
unsigned char level, levelbcd;
unsigned short turn;

void cleartile(char x, char y)
{
    char i;
    for (i = 0; i < 16; i++)
    {
        unsigned short ofs = yofs[y * 16 + i] + x * 2;
        *((char*)ofs) = *((char*)ofs + 0x1b00);
        ofs++;
        *((char*)ofs) = *((char*)ofs + 0x1b00);
    }
}

void cleartilecolor(char x, char y)
{
    unsigned short sofs = 0x5b00+192*32 + x * 2 + y * 32 * 2;
    unsigned short dofs = 0x4000+192*32 + x * 2 + y * 32 * 2;
   *((char*)dofs     ) = *((char*)sofs     );
   *((char*)dofs +  1) = *((char*)sofs +  1);
   *((char*)dofs + 32) = *((char*)sofs + 32);
   *((char*)dofs + 33) = *((char*)sofs + 33);
}

void settilecolor(char x, char y, unsigned char c)
{
    unsigned short ofs = 0x4000+192*32 + x * 2 + y * 64;
   *((char*)ofs     ) = c;
   *((char*)ofs +  1) = c;
   *((char*)ofs + 32) = c;
   *((char*)ofs + 33) = c;
}

void drawbobot(char x, char y, unsigned char c)
{
    drawsprite_16(bobot, x * 16, y * 16);
    settilecolor(x, y, bobotcolor[c]);
}

void drawplayer(char x, char y, unsigned char c)
{
    drawsprite_16(player, x * 16, y * 16);
    settilecolor(x, y, playercolor[c]);
}

void setcolor(char x, char y, unsigned char c)
{
   *((char*)0x4000+192*32+(unsigned short)y*32+x) = c;
}

typedef struct bobot_type
{
    char x, y, c, live;
} Bobot;

// returns values from 1 to 255 inclusive, period is 255
unsigned char y8;
unsigned char xorshift8(void) {
    y8 ^= (y8 << 7);
    y8 ^= (y8 >> 5);
    return y8 ^= (y8 << 3);
}

char enemies;
Bobot enemy[64];
char playerx, playery, playerc;

char rand12()
{
    unsigned char res = 13;
    while (res >= 12)
    {
        res = xorshift8() & 15;
    }
    return res;
}

char rand3()
{
    unsigned char res = 13;
    while (res >= 3)
    {
        res = xorshift8() & 3;
    }
    return res;
}

void spawn_enemy()
{
    char valid = 0;
    char i;
    do
    {
        enemy[enemies].x = rand12();
        enemy[enemies].y = rand12();
        enemy[enemies].c = rand3();
        enemy[enemies].live = 1;
        valid = 1;
        for (i = 0; i < enemies; i++)
        {            
            if (enemy[i].x == enemy[enemies].x &&
                enemy[i].y == enemy[enemies].y)
                valid = 0;
        }
    }
    while (!valid);
    enemies++;
}



void init_ingame()
{
    char i;
    char enemycount;
    cp((unsigned char*)0x4000, 6912, (char*)0x5b00);
    setcolor(28,11,0x5);
    setcolor(29,11,0x5);
    setcolor(27,11+1+1+1,0x5);
    setcolor(28,11+1+1+1,0x5);
    setcolor(29,11+1+1+1,0x5);
    setcolor(30,11+1+1+1,0x5);
    
    turn = 0;
    enemies = 0;
    playerx = 6;
    playery = 6;
    playerc = 0;

    playfx(FX_STEP);
    playfx(FX_STEP);
    playfx(FX_STEP);
    playfx(FX_STEP);
    playfx(FX_STEP);

    playfx(FX_SHAPE);
    drawplayer(playerx, playery, 2);
    playfx(FX_SHAPE);
    drawplayer(playerx, playery, 1);
    playfx(FX_SHAPE);
    drawplayer(playerx, playery, 0);
    playfx(FX_SHAPE);
    drawplayer(playerx, playery, 2);
    playfx(FX_SHAPE);
    drawplayer(playerx, playery, 1);
    playfx(FX_SHAPE);
    drawplayer(playerx, playery, 0);
    playfx(FX_PLAYERROBOT);

    if (level > 31)
        enemycount = 64;
    else
        enemycount = level * 2 + 1;

    for (i = 0; i < enemycount; i++)
    {
        spawn_enemy();
    }

    for (i = 0; i < enemies; i++)
    {    
        drawbobot(enemy[i].x, enemy[i].y, enemy[i].c);
        playfx(FX_ROBOTROBOT);        
    }
}

const char decimals[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void add_score(char val)
{
    score += val;
    if ((score & 0xf) >= 0xa) score += 0x6;
    if ((score & 0xf0) >= 0xa0) score += 0x60;
    if ((score & 0xf00) >= 0xa00) score += 0x600;    
    if (score > 0x9999) score = 0x9999;    
}

char dominate_color(char a, char b)
{
    if (a == 0 && b == 1) return 1;
    if (a == 1 && b == 2) return 1;
    if (a == 2 && b == 0) return 1;
    return 0;
}

char validate(char dx, char dy, char c)
{
    char i;
    if (playerx == dx && playery == dy)
    {
        if (dominate_color(playerc, c))
            return -3; // game over
        return -2; // no move
    }
    for (i = 0; i < enemies; i++)
    {
        if (enemy[i].live && enemy[i].x == dx && enemy[i].y == dy)
        {
            if (dominate_color(enemy[i].c, c))
                return i; // eat the target
            return -2; // no move
        }
    }
    return -1; // normal move    
}

void ingame()
{
    char dx, dy, res, player_moved = 0, livebots = 0, input;
    char i;    
    char numbuf[5];
    numbuf[0] = decimals[(levelbcd >> 4) & 0xf];
    numbuf[1] = decimals[(levelbcd >> 0) & 0xf];
    numbuf[2] = 0;
    drawstring(numbuf, 28, 88); // level
    numbuf[0] = decimals[(score >> 12) & 0xf];
    numbuf[1] = decimals[(score >> 8) & 0xf];
    numbuf[2] = decimals[(score >> 4) & 0xf];
    numbuf[3] = decimals[(score >> 0) & 0xf];
    numbuf[4] = 0;
    drawstring(numbuf, 27, 88+8+8+8); // score

    for (i = 0; i < enemies; i++)
    {
        if (enemy[i].live)
        {
            char dxi, dyi;
            livebots++;
            cleartilecolor(enemy[i].x, enemy[i].y);
            cleartile(enemy[i].x, enemy[i].y);
            
            // AI here
            
            dx = playerx - enemy[i].x;
            dy = playery - enemy[i].y;
            
            dxi = dx;
            dyi = dy;
            
            if (dxi < 0) dxi = -dxi;
            if (dyi < 0) dyi = -dyi;
                
            if (dxi > dyi)
            {
                dy = enemy[i].y;
                if (dx > 0) 
                    dx = enemy[i].x + 1;
                else
                    dx = enemy[i].x - 1;
            }
            else
            {
                dx = enemy[i].x;
                if (dy > 0) 
                    dy = enemy[i].y + 1;
                else
                    dy = enemy[i].y - 1;
            }
            
            res = validate(dx, dy, enemy[i].c);
            if (res == -2 || turn == 0)
            {
                // no move
                dx = enemy[i].x;
                dy = enemy[i].y;
            }
            else
            if (res == -3)
            {
                playfx(FX_ROBOTPLAYER);
                cleartile(dx, dy);
                drawbobot(dx, dy, enemy[i].c);
                playfx(FX_ROBOTPLAYER);
                cleartile(dx, dy);
                drawplayer(dx,dy,playerc);
                playfx(FX_ROBOTPLAYER);
                cleartile(dx, dy);
                drawbobot(dx, dy, enemy[i].c);
                playfx(FX_ROBOTPLAYER);
                cleartile(dx, dy);
                drawplayer(dx,dy,playerc);
                playfx(FX_ROBOTPLAYER);
                cleartile(dx, dy);
                drawbobot(dx, dy, enemy[i].c);
                gamestate = 2;
                return;
                // game over
            }
            else
            if (res == -1)
            {
                // normal move
                playfx(FX_STEP);
            }
            else
            {
                enemy[res].live = 0;
                add_score(2);
                playfx(FX_ROBOTROBOT);
                drawbobot(dx, dy, enemy[res].c);
                playfx(FX_ROBOTROBOT);
                drawbobot(dx, dy, enemy[i].c);
                playfx(FX_ROBOTROBOT);
                drawbobot(dx, dy, enemy[res].c);
                playfx(FX_ROBOTROBOT);
                drawbobot(dx, dy, enemy[i].c);
                playfx(FX_ROBOTROBOT);
                drawbobot(dx, dy, enemy[res].c);
                playfx(FX_ROBOTROBOT);
            }
            
            enemy[i].x = dx;
            enemy[i].y = dy;        
            
            drawbobot(enemy[i].x, enemy[i].y, enemy[i].c);
        }
    }

    if (livebots == 0)
    {
        level++;
        levelbcd++;
        if ((levelbcd & 0xf) >= 0xa) levelbcd += 0x6;
        if (levelbcd > 0x99) levelbcd = 0x99;

        gamestate = 0;
        add_score(0x1);
        return;
    }
            
    player_moved = 0;
    while (player_moved == 0)
    {
        dx = 0;
        while (!ANYKEYX()) 
        { 
            do_halt();
            readkeyboard(); 
            dx++;
            if (dx == 90)
                settilecolor(playerx, playery, 0);
            if (dx == 100)
            {
                settilecolor(playerx, playery, playercolor[playerc]);
                dx = 0;
            }
                
        }
        
        dx = playerx;
        dy = playery;
        
        input = 0;
        
        if (inputscheme == 1)
        {
            if (!KEYUP(O)) input = 1;
            if (!KEYUP(P)) input = 2;
            if (!KEYUP(Q)) input = 3;
            if (!KEYUP(A)) input = 4;
            if (!KEYUP(SPACE)) input = 5;
            if (!KEYUP(M)) input = 5;
        }
        if (inputscheme == 2)
        {
            if (!KEYUP(A)) input = 1;
            if (!KEYUP(D)) input = 2;
            if (!KEYUP(W)) input = 3;
            if (!KEYUP(S)) input = 4;
            if (!KEYUP(SPACE)) input = 5;
        }
        if (inputscheme == 3)
        {
            if (!KEYUP(6)) input = 1;
            if (!KEYUP(7)) input = 2;
            if (!KEYUP(9)) input = 3;
            if (!KEYUP(8)) input = 4;
            if (!KEYUP(0)) input = 5;                
        }
        if (input == 1) dx = playerx - 1;
        if (input == 2) dx = playerx + 1;
        if (input == 3) dy = playery - 1;
        if (input == 4) dy = playery + 1;
        
        if (dx < 0) dx = 0;
        if (dx > 11) dx = 11;
        if (dy < 0) dy = 0;
        if (dy > 11) dy = 11;
        
        if (input == 5)
        {
            playerc++;
            if (playerc == 3) playerc = 0;
            drawplayer(playerx, playery, playerc);
            playfx(FX_SHAPE);
        }
        while (ANYKEYX()) { readkeyboard(); }
        res = validate(dx, dy, playerc);
        if (dx != playerx || dy != playery)
            player_moved = 1;
        if (res < -1)
        {
            player_moved = 0;
            dx = playerx;
            dy = playery;
        }
        else
        if (res == -1)
        { // normal move
            playfx(FX_STEP);
            cleartilecolor(playerx, playery);
            cleartile(playerx, playery);
            drawplayer(dx, dy, playerc);            
        }
        else
        {
            add_score(1);
            enemy[res].live = 0;
            cleartilecolor(playerx, playery);
            cleartile(playerx, playery);
            playfx(FX_PLAYERROBOT);
            drawbobot(dx, dy, enemy[res].c);
            playfx(FX_PLAYERROBOT);
            drawplayer(dx, dy, playerc);
            playfx(FX_PLAYERROBOT);
            drawbobot(dx, dy, enemy[res].c);
            playfx(FX_PLAYERROBOT);
            drawplayer(dx, dy, playerc);
            playfx(FX_PLAYERROBOT);
            drawbobot(dx, dy, enemy[res].c);
            playfx(FX_PLAYERROBOT);
            cleartile(dx, dy);
            drawplayer(dx, dy, playerc);        
        }
        
        playerx = dx;
        playery = dy;
    }
    turn++;
        
}

void gameover()
{
    char i;
    for (i = 0; i < 24; i++)
        memset((char*)yofs[11*8+i],0,24);
    drawstring("Game Over", 8,12*8);
    setcolor(8, 12, 2);
    setcolor(9, 12, 2);
    setcolor(10, 12, 2);
    setcolor(11, 12, 2);
    setcolor(13, 12, 2);
    setcolor(14, 12, 2);
    setcolor(15, 12, 2);
    setcolor(16, 12, 2);
    while (!ANYKEYX()) { readkeyboard(); }
    while (ANYKEYX()) { readkeyboard(); }
}

void mainmenu()
{
    level = 1;
    levelbcd = 1;
    score = 0;
    
    
    
    cp((unsigned char*)0x4000, 6912, (char*)0x5b00+6912);    
    
    while (ANYKEYX()) { readkeyboard(); }
    inputscheme = -1;
    
    while (inputscheme == -1)
    {
        readkeyboard();
        if (!KEYUP(1))
            inputscheme = 1;
        if (!KEYUP(2))
            inputscheme = 2;
        if (!KEYUP(3))
            inputscheme = 3;
    }
    
}


void main()
{         
    y8 = 1;

    do_halt();
    do_port254(0);
    readkeyboard();
        
    cp((unsigned char*)0x4000, 6912, (char*)0x5b00);
    //cp((unsigned char*)0x4000+(32*192), 32*8, (char*)0x5b00+32*64);
    
    drawstring("00", 28, 88); // level
    setcolor(28,11,0x5);
    setcolor(29,11,0x5);

    drawstring("0000", 27, 88+8+8+8); // score
    setcolor(27,11+1+1+1,0x5);
    setcolor(28,11+1+1+1,0x5);
    setcolor(29,11+1+1+1,0x5);
    setcolor(30,11+1+1+1,0x5);
    
    // grid = 12x12
    // 0x41 = blue
    // 0x42 = red
    // 0x44 = green
    // | 0x80 = blink
        
    //drawbobot(3,4,0x42);
    //drawplayer(7,1,0x41);
    

    framecounter = 0;
    
    gamestate = 3;
    
    inputscheme = 0;
    
    while(1)
    {
        switch (gamestate)
        {
        case 0:
            init_ingame();
            gamestate = 1;
            break;
        case 1:             
            ingame();
            break;
        case 2:
            gameover();
            gamestate = 3;
            break;
        case 3:
            mainmenu();
            gamestate = 0;
            break;
        }    
        framecounter++;
    }       
}
