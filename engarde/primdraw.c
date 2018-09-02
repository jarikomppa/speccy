#include "main.h"

void drawmoney(unsigned char x, unsigned char y, unsigned char v)
{
    char temp[20];
    temp[0] = 0;
    strcat(temp, "You have ");
    int2str(v, &temp[9]);
    strcat(temp, " crowns.");
    drawstringz(temp, x, y);
}

void drawcost(unsigned char x, unsigned char y, unsigned char v)
{
    char temp[40];
    temp[0] = 0;
    strcat(temp, "It costs ");
    int2str(v, &temp[9]);
    strcat(temp, " crowns.");
    drawstringz(temp, x, y);
}
void colorbox(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned char c)
{
    unsigned short o, i, j;
    unsigned char ws;
    ws = 32 - w;
    o = 0x4000 + (32*192) + x + y * 32;
    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            *((unsigned char*)o) = c;
            o++;
        }
        o += ws;
    }    
}

void fillback()
{
    unsigned short i;
    colorbox(0,0,32,24,0);
    for (i = 0; i < 192; i++)
    {
        unsigned char d = backtile[i & 7];
        unsigned short t = yofs[i];
        unsigned char c;
        for (c = 0; c < 32; c++)
        {
            *((unsigned char*)t) = d; t++;
        }
    }
    colorbox(0,0,32,24,COLOR(0, 0, 5, 4));
}

void drawcard(unsigned char cardno,  unsigned char x, unsigned char y, unsigned char c)
{
    //32x48
    unsigned short o;
    unsigned char i, j;
    const char *sp = &artassets[0];
    static const unsigned short cardofs[16] = 
    {
        0, 4, 8, 12, 16, 20, 24, 28, 
        1536, 1540, 1544, 1548, 1552, 1556, 1560, 1564
    };
    if (cardno == CARD_NOCARD) return;
    cardno--;
    sp += cardofs[cardno];
    colorbox(x,y,4,6,c);

    y *= 8;
    for (j = 0; j < 48; j++)
    {
        o = yofs[y] + x; y++;    
        for (i = 0; i < 4; i++)
        {
            *((unsigned char*)o) = *sp; 
            sp++; 
            o++;
        }
        sp += 28;
    }
}

void drawmug(unsigned char mugno,  unsigned char x, unsigned char y)
{
    //40x48
    unsigned short o;
    unsigned char i, j;
    const char *sp = (char*)&artassets[0];
    static const unsigned short mugofs[12] = {3072, 3077, 3082, 3087, 3092, 3097, 4608, 4613, 4618, 4623, 4628, 4633 };
    sp += mugofs[mugno];
    colorbox(x,y,5,6,COLOR(0,1,0,7));
    y *= 8;
    for (j = 0; j < 48; j++)
    {
        o = yofs[y] + x; y++;    
        for (i = 0; i < 5; i++)
        {
            *((unsigned char*)o) = *sp; 
            sp++; 
            o++;
        }
        sp += 27;
    }
}

void drawicon(unsigned char iconno,  unsigned char x, unsigned char y)
{
    //16x16
    unsigned short o;
    unsigned char i, j;
    const char *sp = (char*)&artassets[0];
    static const unsigned short iconofs[6] = {3102, 3614, 4126, 4638, 5150, 5662};
    sp += iconofs[iconno];
    //colorbox(x,y,2,2,COLOR(0,1,0,7));

    y *= 8;
    for (j = 0; j < 16; j++)
    {
        o = yofs[y] + x; y++;    
        for (i = 0; i < 2; i++)
        {
            *((unsigned char*)o) = *sp; 
            sp++; 
            o++;
        }
        sp += 30;
    }
}


void cleartextbox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    unsigned short o, i, j;
    unsigned char oh, oy;
    oh = h;
    oy = y;
    colorbox(x+1,oy+1,w-2,oh-2,COLOR(0,1,7,7));

    h *= 8;
    y *= 8;
    
    for (j = y + 1; j < y + h - 1; j++)
    {
        o = yofs[j] + x + 1;
        for (i = 1; i < w-1; i++)
        {
            *((unsigned char*)o) = 0;
            o++;
        }
    }
    
    colorbox(x+1,oy+1,w-2,oh-2,COLOR(0,1,7,0));
}

void drawtextbox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    unsigned short o, o2, i, j;
    unsigned char oh, oy;
    oh = h;
    oy = y;
    colorbox(x,oy,w,oh,COLOR(0,1,7,7));

    h *= 8;
    y *= 8;
    
    o = yofs[y] + x;
    o2 = yofs[y+h-1] + x;
    for (i = 0; i < w; i++)
    {
        *((unsigned char*)o) = 0xff;
        *((unsigned char*)o2) = 0xff;
        o++;
        o2++;
    }
    
    for (j = y + 1; j < y + h - 1; j++)
    {
        o = yofs[j] + x + 1;
        for (i = 1; i < w-1; i++)
        {
            *((unsigned char*)o) = 0;
            o++;
        }
    }
    
    for (j = y + 1; j < y + h - 1; j++)
    {
        o = yofs[j] + x;
        *((unsigned char*)o) = 0x80;
        *((unsigned char*)o + w - 1) = 0x01;
    }    
    colorbox(x,oy,w,oh,COLOR(0,1,7,0));

}
