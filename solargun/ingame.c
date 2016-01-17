unsigned char fbcopy_idx;
extern unsigned char *data_ptr;
extern unsigned char *screen_ptr;
extern unsigned short framecounter;
extern const unsigned short yofs[];
extern char gamestate;
extern char inputscheme;

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

extern void playfx(unsigned short fx) __z88dk_fastcall;  
extern void cp(unsigned char *dst, unsigned short len, unsigned char *src)  __z88dk_callee;
extern void drawstring(unsigned char *t, unsigned char x, unsigned char y);

#include "hwif.c"
#include "bg.h"
#include "fbcopy.c"
#include "scroller.c"
#include "ship.h"
#include "rock_16.h"
#include "sprites.c"

struct Enemy
{
    unsigned char x;
    unsigned char y;
    char xi;
    char yi;
    char life;
};

struct Enemy enemy[18];
unsigned char player_y;
unsigned char player_x;
unsigned char playsound;
char recharge;  
char player_xm;
char player_ym;
char bgi;
unsigned short bgo;
char spritemux;
char next_enemyslot;
char needclean;
char hp;

unsigned short score;
unsigned char spawnbuf;
unsigned char spawninc;
unsigned char scoredirty;

void addscore(unsigned char val)
{
    if (score != 0x9999)
    {
        score+=val;
        if ((score & 0xf) >= 0xa) score += 0x6;
        if ((score & 0xf0) >= 0xa0) score += 0x60;
        if ((score & 0xf00) >= 0xa00) score += 0x600;    
        if (score > 0x9999) score = 0x9999;
        scoredirty = 1;
    }
}

void printscore()
{
    if (scoredirty)
    {
        char temp[5] = "0000";
        temp[3] += (score >> 0) & 0xf;
        temp[2] += (score >> 4) & 0xf;
        temp[1] += (score >> 8) & 0xf;
        temp[0] += (score >> 12) & 0xf;
        drawstring(temp, 26, 56);
        scoredirty = 0;
    }
}

const unsigned char spanattr[32] = { 
    0x01, 0x41, 0x05, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47,
    0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x05, 0x41, 0x01
};

void attribclean()
{    
    unsigned short i;    
    unsigned short o;
    if (needclean == 0)
        return;
    needclean--;
    o = (framecounter * 3) & 15;
    for (i = 0; i < 32; i++, o += 16)
        *((unsigned char*)0x4000+192*32+ 8*32 + o) = spanattr[o & 31];    
}

void attribline(char x, char y, unsigned char c)
{
    unsigned short i;
    char d = 32 - x;
    i = 0x4000+192*32+ 7*32 + y * 32 + x;
    while (d)
    {
        *((unsigned char*)i) = c;
        d--;
        i++;
    }
    needclean = 16;
}

void attribmess(char x, char y, char c)
{
    unsigned short i;
    i = 0x4000+192*32+ 8*32 + y * 32 + x;

    *((unsigned char*)i) = c;    
    *((unsigned char*)i + 1) = c;    
    *((unsigned char*)i + 2) = c;    

    *((unsigned char*)i + 31) = c;    
    *((unsigned char*)i + 32) = c;    
    *((unsigned char*)i + 33) = c;    
    *((unsigned char*)i + 34) = c;    
    *((unsigned char*)i + 35) = c;    

    *((unsigned char*)i + 64) = c;    
    *((unsigned char*)i + 65) = c;    
    *((unsigned char*)i + 66) = c;    

    needclean = 16;
}

void kill(char i)
{
    enemy[i].life = 0;
    attribmess(enemy[i].x/8,enemy[i].y/8,COLOR(0,1,0,2));
    playsound = 2;
    next_enemyslot = i;
}

void kill_enemies(unsigned char y)
{
    char i;
    for (i = 0; i < 18; i++)
    {
        if (enemy[i].life)
        {
            if (enemy[i].x > player_x/2 &&
                enemy[i].y / 16 == y)
            {
                kill(i);
                addscore(0x10);
            }
        }
    }
}

void hurtplayer()
{
    if (hp == 0)
    {
        gamestate = 2;
    }
    else    
    {
        hp--;
        *((char*)0x4000+192*32+7*32+15+hp) = 1;
    }
}

void enemy_physics(char spriteofs)
{
    char i;
    char spriteidx = spriteofs * 6;
    for (i = 0; i < 6; i++, spriteidx++)
    {
        if (enemy[spriteidx].life)
        {
            unsigned char xe = enemy[spriteidx].x;
            unsigned char ye = enemy[spriteidx].y;
            
            xe += enemy[i].xi;
            if (xe > (256 - 16)) xe = 8;        
            if (xe < 4) 
            { 
                enemy[spriteidx].life = 0;//xe = 240; //(256 - 16);
                addscore(1);
            }
    
            ye += enemy[spriteidx].yi;
            if (ye < 8) enemy[spriteidx].yi = -enemy[spriteidx].yi;//176;
            if (ye > 104) enemy[spriteidx].yi = -enemy[spriteidx].yi;//64;
                
            enemy[spriteidx].x = xe;
            enemy[spriteidx].y = ye;
                
            if (enemy[spriteidx].x < 128)
            {
                
/*

 x----+
 |    |
 |  a----+
 |  | |  |
 +--|-y  |
    |    |
    +----+

*/                
                char x = player_x / 2 + 4;
                char y = player_y / 2 + 4;

                if (x + 8 > xe &&
                    y + 8 > ye &&
                    x < xe + 16 &&
                    y < ye + 16)
                {
                    hurtplayer();
                    kill(spriteidx);
                }
            }
        }               
    }
}

void clear_guncharge()
{
    char c;
    for (c = 0; c < 11; c++)
    {
        *((char*)0x4000+192*32+7*32+2+c) = 2;
    }
}

void guncharge(char pos)
{
    *((char*)0x4000+192*32+7*32+2+pos/4) = 0x40 + (2<<3);
}


char spawncount;

void init_ingame()
{
    unsigned short i;
    recharge = 0;  
    player_xm = 0;
    player_ym = 0;
    bgi = 1;
    bgo = 0;
    spritemux = 0;
    needclean = 16;
    score = 0;
    scoredirty = 1;

    player_y = 112;
    player_x = 10;
    playsound = 0;
    
    spawnbuf = 0;
    spawninc = 3;
    spawncount = 0;
    
    hp = 3;
    *((char*)0x4000+192*32+7*32+15+0) = 0x44;
    *((char*)0x4000+192*32+7*32+15+1) = 0x44;
    *((char*)0x4000+192*32+7*32+15+2) = 0x44;

    next_enemyslot = 0;

    for (i = 0; i < 18; i++)
    {
        enemy[i].life = 0;
    }
    
    enemy_physics(0);
    enemy_physics(1);
    enemy_physics(2);
    
    clear_guncharge();
}

void spawn_enemy()
{
    struct Enemy *e;
    char c = 0;
    while (c < 18 && enemy[next_enemyslot].life != 0)
    {
        c++;
        next_enemyslot++;
        if (next_enemyslot == 18)
            next_enemyslot = 0;
    }
    if (c == 18)
        return;
    e = enemy + next_enemyslot;
    e->x = 240;        
    e->y = ((framecounter * 17) & 127);        
    if (e->y > 104) e->y -= 60;
    if (e->y < 8) e->y += 60;
    e->xi = -(((framecounter & 7) + 1));
    e->yi = ((framecounter * 3) & 7) - 4;
    e->life = 1;
    spawncount++;
    if (spawncount > 10)
    {
        spawncount = 0;
        spawninc++;
        if (spawninc > 50)
            spawninc = 50;
    }
}

void ingame()
{
    unsigned short i;
    unsigned char ci;
    char fire = 0;

    char spriteofs = 0;
    do_halt(); // halt waits for interrupt - or vertical retrace     
    
    port254(1);
    // can do about 64 scanlines in a frame (with nothing else)
    //fbcopy(s_png + sinofs[framecounter & 0xff] * 32, 64, 110);
    // Let's do interlaced copy instead =)
    i = ((player_y) + (bgo << 1)) / 16;
    fbcopy_i(i, (framecounter >> 2) & 31, 13);
        
    bgo += bgi;
    if (bgo == 0) bgi = 1;
    if (bgo == (126 << 1)) bgi = -1;
    port254(2);

    spriteofs = 6 * spritemux;
    spritemux++;
    if (spritemux == 3) spritemux = 0;

    if (playsound)
    {
        playfx(playsound - 1);
    }
    else
    {
        spriteofs = 6 * spritemux;
        spritemux++;
        if (spritemux == 3) spritemux = 0;
        
        for (ci = 0; ci < 6; ci++, spriteofs++)
        {
            if (enemy[spriteofs].life != 0)
            {
                drawsprite_16(rock_16, enemy[spriteofs].x, enemy[spriteofs].y + 64);
            }
        }        
    }
    
    playsound = 0;

    drawsprite_16(ship, player_x / 2, player_y / 2 + 64);

    port254(1);
    
    enemy_physics(spritemux);
    
    readkeyboard();
 
    if (inputscheme == 0)
    {       
        if (!KEYUP(Q)) player_ym -= 5;
        if (!KEYUP(A)) player_ym += 5;
        if (!KEYUP(O)) player_xm -= 5;
        if (!KEYUP(P)) player_xm += 5;
        if (!KEYUP(SPACE)) fire = 1;
    }
    else
    if (inputscheme == 1)
    {       
        if (!KEYUP(9)) player_ym -= 5;
        if (!KEYUP(8)) player_ym += 5;
        if (!KEYUP(6)) player_xm -= 5;
        if (!KEYUP(7)) player_xm += 5;
        if (!KEYUP(0)) fire = 1;
    }
    else
    if (inputscheme == 2)
    {       
        if (!KEYUP(KEMPU)) player_ym -= 5;
        if (!KEYUP(KEMPD)) player_ym += 5;
        if (!KEYUP(KEMPL)) player_xm -= 5;
        if (!KEYUP(KEMPR)) player_xm += 5;
        if (KEYUP(KEMPF)) fire = 1;
    }        
   
    player_xm = (player_xm * 3) / 4;
    player_ym = (player_ym * 3) / 4;
                    
    player_x += player_xm / 2;
    player_y += player_ym / 2;
    
    if (player_x < 8) 
    {
        player_x = 8;
    }
    if (player_x > 240) 
    {
        player_x = 240;               
    }
    if (player_y < 8) 
    {
        player_y = 8;
    }
    if (player_y > 208) 
    {
        player_y = 208;
    }

    if (recharge == 40 && fire)
    {
        kill_enemies(player_y/32);
        attribline(player_x/16+2,player_y/16+2,COLOR(1,1,6,2));
        recharge = 0;
        playsound = 1;
        clear_guncharge();
    }
    
    if (recharge < 40 && fire == 0)
    {
        recharge++;
        guncharge(recharge);
    }

    port254(5);
    attribclean();
    port254(0);                                             
    scroller(0);    
    spawnbuf += spawninc;
    if (spawnbuf > 220)
    {
        spawn_enemy();
        spawnbuf = 0;        
    }
    
    printscore();
}