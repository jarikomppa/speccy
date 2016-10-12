/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org
 * (practically public domain)
*/

#include <string.h>

//unsigned char fbcopy_idx;

//unsigned char *data_ptr;
//unsigned char *screen_ptr;

//unsigned char port254tonebit;

#define FONTHEIGHT 8
#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

#include "yofstab.h"
#define HWIF_IMPLEMENTATION
#include "hwif.c"
#include "propfont.h"
#include "drawstring.c"

//extern void zx7_unpack(unsigned char *src, unsigned char *dst) __z88dk_callee __z88dk_fastcall;
extern void zx7_unpack(unsigned char *src)  __z88dk_fastcall;
extern void playfx(unsigned short fx) __z88dk_fastcall;  

#define KEY_PRESSED_UP (KEYDOWN(9) || KEYDOWN(Q) || KEYDOWN(W) || KEYDOWN(E) || KEYDOWN(R) || KEYDOWN(T) || KEYDOWN(Y) || KEYDOWN(U) || KEYDOWN(I) || KEYDOWN(O) || KEYDOWN(P))
#define KEY_PRESSED_DOWN (KEYDOWN(8) || KEYDOWN(A) || KEYDOWN(S) || KEYDOWN(D) || KEYDOWN(F) || KEYDOWN(G) || KEYDOWN(H) || KEYDOWN(J) || KEYDOWN(K) || KEYDOWN(L))
#define KEY_PRESSED_FIRE (KEYDOWN(0) || KEYDOWN(ENTER) || KEYDOWN(SPACE))

enum opcodeval
{
    OP_HAS,
    OP_NOT,
    OP_SET,
    OP_CLR,
    OP_XOR,
    OP_RND,
    OP_ATTR,
    OP_EXT,
    OP_IATTR,
    OP_DATTR
};

unsigned char state[256]; // game state
unsigned char y8; // prg state
unsigned char *answer[16];
unsigned char answers;
unsigned char attrib, iattrib, dattrib, attrib_c;

void cls()
{
    unsigned short i, j;

    for (i = 0; i < 20*32; i++)
        *(unsigned char*)(0x5800+i) = attrib_c; 
    for (j = 0; j < 20*8; j++)
        for (i = 0; i < 32; i++)
            *(unsigned char*)(yofs[j]+i) = 0;
}
   
const unsigned char pattern[8] = 
{ 
    0x00, 
    0xC1, 
    0x32, 
    0x18, 
    0x0C, 
    0x26, 
    0xC1, 
    0x00  
};

void clearbottom()
{
    unsigned short i, j;

    for (j = 20*8; j < 21*8; j++)
        for (i = 0; i < 32; i++)
            *(unsigned char*)(yofs[j]+i) = pattern[j & 7];

    for (j = 21*8; j < 24*8; j++)
        for (i = 0; i < 32; i++)
            *(unsigned char*)(yofs[j]+i) = 0;

    for (i = 20*32; i < 21*32; i++)
        *(unsigned char*)(0x5800+i) = dattrib; 

    for (i = 21*32; i < 24*32; i++)
        *(unsigned char*)(0x5800+i) = iattrib; 
}

unsigned char * unpack_resource(unsigned short id)
{
    // dest = 0xd000, ~4k of scratch. A bit tight?
    
    unsigned short v;

    for (v = 0; v < 4096; v++)
        *((unsigned char*)0xd000 + v) = 0;

    v = (((unsigned short)*(unsigned char*)(0x5b00 + id*2+1)) << 8) | *(unsigned char*)(0x5b00 + id*2);
    zx7_unpack((unsigned char*)v);
    

    return (unsigned char*)0xd000;
}

unsigned char xorshift8(void) 
{
    y8 ^= (y8 << 7);
    y8 ^= (y8 >> 5);
    return y8 ^= (y8 << 3);
}

unsigned char get_bit(unsigned short id)
{
    return !!(state[id & 0xff] & 1 << (id >> 8));
}

void set_ext(unsigned short id)
{
    if (id < 8)
    {
        port254(id);
    }
    if (id == 8)
    {
        cls();
        clearbottom();
    }
    if (id == 9)
    {
        cls();
    }
    if (id == 10)
    {
        clearbottom();
    }
    if (id >= 100 && id <= 155)
    {
        playfx(id-100);
    }
}

#define SET_BIT(x) state[(x) & 0xff] |= 1 << ((x) >> 8)

void exec(unsigned char *dataptr)
{
    unsigned char c = *dataptr;
    
    switch (dataptr[1])
    {
        case 'I':
        case 'A':
        case 'Q': 
            dataptr += 1+1+2; 
            c -= 3; 
            break;
        case 'O': 
            dataptr += 1+1; 
            c -= 1; 
            break;
    }
    
    while (c)
    {
        // ignore predicate ops
        unsigned short id = ((unsigned short)dataptr[2] << 8) | dataptr[1];
        switch (*dataptr)
        {
            case OP_SET:
                SET_BIT(id);
                break;
            case OP_CLR:
                state[id & 0xff] &= (1 << (id >> 8))^0xff;
                break;
            case OP_XOR:
                state[id & 0xff] ^= 1 << (id >> 8);
                break;
            case OP_ATTR:
                attrib = id;
                attrib_c = (id & 070) | (id >> 3);                
                break;
            case OP_IATTR:
                iattrib = id;
                break;
            case OP_DATTR:
                dattrib = id;
                break;                
            case OP_EXT:
                set_ext(id);
                break;
        }
        dataptr += 3;
        c -= 3;
    }    
}

unsigned char pred(unsigned char *dataptr)
{
    unsigned char c = *dataptr;
    unsigned char ret = 1;
    
    switch (dataptr[1])
    {
        case 'I':
        case 'A':
        case 'Q':
            dataptr += 1+1+2; 
            c -= 3; 
            break; 
        case 'O': 
            dataptr += 1+1; 
            c -= 1; 
            break;
    }
    
    while (c)
    {
        // ignore exec ops
        unsigned short id = ((unsigned short)dataptr[2] << 8) | dataptr[1];
        
        switch (*dataptr)
        {
            case OP_HAS:
                if (!get_bit(id)) ret = 0;
                break;
            case OP_NOT:
                if (get_bit(id)) ret = 0;
                break;
            case OP_RND:
                if (xorshift8() > id) ret = 0;
                break;
        }
        
        dataptr += 3;
        c -= 3;
    }
    return ret;
}

void add_answer(unsigned char *dataptr)
{
    answer[answers] = dataptr;
    answers++;
    if (answers == 16)
        answers = 15;
}


void hitkeytocontinue()
{
    clearbottom();
    //                       0123456789ABCDEF0123456789ABCDEF01
    drawstring_lr_pascal("\x19[Press enter to continue]", 6, 22*8);
    
    readkeyboard();            
    while (!KEY_PRESSED_FIRE)
    {
        readkeyboard();
    }            
    while (KEY_PRESSED_FIRE)
    {
        readkeyboard();
    }                
    clearbottom();
}

void drawattrib(unsigned char yofs, unsigned char attrib)
{
    unsigned char * dst = (unsigned char*)(0x5800 + yofs * 4);
    unsigned char i;
    for (i = 0; i < 32; i++)
    {
        *dst = attrib;
        dst++;
    }
}

void image(unsigned char *dataptr, unsigned char *aYOfs)
{
    unsigned short id = ((unsigned short)dataptr[3] << 8) | dataptr[2];
    unsigned char * dst;
    unsigned short yp, ayp;
    unsigned char x, y, rows;
    
    dataptr = unpack_resource(id);
    id = *dataptr; // scanlines
    yp = *aYOfs;
    rows = id / 8;
    for (y = 0; y < rows; y++)
        drawattrib(yp + y, attrib_c);
    if (id + yp > 20*8)
    {
        hitkeytocontinue();
        cls();
        yp = 0;
    }   
    
    ayp = yp * 4; // yp / 8 * 32 -> yp * 4
    
    dataptr++;
    for (y = 0; y < id; y++, yp++)
    {
        dst = (unsigned char*)yofs[yp];
        for (x = 0; x < 32; x++)
        {
            *dst = *dataptr;
            dataptr++;
            dst++;
        }
    }
    
    dst = (unsigned char*)(0x5800 + ayp);
    for (y = 0; y < rows; y++)
    {
        
        for (x = 0; x < 32; x++)
        {
            *dst = *dataptr;
            dataptr++;
            dst++;
        }
    }
    *aYOfs = yp;
}

void render_room(unsigned short room_id)
{
    unsigned char *dataptr = unpack_resource(room_id);
    unsigned char output_enable = 1;
    unsigned char yofs = 0;

    SET_BIT(room_id);

    cls();
    
    while (*dataptr)
    {       
        switch (dataptr[1])
        {
            case 'Q': 
                if (!output_enable) // the next room
                {
                    return;
                }
                if (pred(dataptr)) 
                {
                    exec(dataptr); 
                }
                answers = 0;
                break;
            case 'I': 
                if (pred(dataptr))
                {
                    exec(dataptr);
                    image(dataptr, &yofs); 
                    unpack_resource(room_id); // re-load the room data
                }
                break;
            case 'O': 
                if (pred(dataptr)) 
                {
                    exec(dataptr);
                    output_enable = 1;
                }
                else
                {
                    output_enable = 0;
                }
                break;
            case 'A':
                if (pred(dataptr))
                {
                    add_answer(dataptr);
                }
                output_enable = 0;
                break;
        }
        dataptr += *dataptr + 1;
        
        while (*dataptr)
        {
            if (output_enable)
            {
                if (yofs + 8 > 20*8) 
                {
                    hitkeytocontinue();
                    cls();
                    yofs = 0;                
                } 

                drawattrib(yofs, attrib_c);
                drawstring_lr_pascal(dataptr, 0, yofs);
                drawattrib(yofs, attrib);
                yofs += 8;
            }
            dataptr += *dataptr + 1;
        }
        dataptr++;
    }
    // if we get here, this was the last room in the data
}


void reset()
{
    unsigned short i;

    for (i = 0; i < 256; i++)
        state[i] = 0;

    attrib = 070;
    attrib_c = 077;
    iattrib = 071;
    dattrib = 072;
    cls();
    clearbottom();
    
    port254(7);

    y8 = 1;
    answers = 0;
}

void main()
{
    unsigned short i;
    unsigned short current_room = 0;    
    unsigned char current_answer = 0;
    unsigned char selecting;    

    reset();     
        
    while(1)
    { 
        /*
        - code string
        - 0..n strings
        - 0        
        */
        
        render_room(current_room);

        if (answers == 0)
        {
            unsigned short t;
            clearbottom();
            // delay..
            for (t = 0; t < 30000; t++);
            for (t = 0; t < 30000; t++);
            for (t = 0; t < 30000; t++);
            for (t = 0; t < 30000; t++);
            //                       0123456789ABCDEF0123456789ABCDEF01
            drawstring_lr_pascal("\x21[The end. Press enter to restart]", 6, 22*8);
            
            readkeyboard();            
            while (!KEY_PRESSED_FIRE)
            {
                readkeyboard();
            }            
            while (KEY_PRESSED_FIRE)
            {
                readkeyboard();
            }            
            
            current_room = 0;
            current_answer = 0;
            reset();            
        }
        else
        {
    
            if (current_answer >= answers)
                current_answer = 0;
                
            selecting = 1;
            while (selecting)
            {
                unsigned char answer_ofs;
                clearbottom();
                answer_ofs = current_answer;
                if (current_answer > 0)
                    answer_ofs -= 1;
                if (current_answer == answers-1)
                    answer_ofs -= 1;
                if (answers < 4) answer_ofs = 0;
                for (i = 0; i < 3; i++)
                {
                    if (answer_ofs + i < answers)
                    {
                        unsigned char *dataptr = answer[answer_ofs + i];
                        unsigned char roll = 0;
                        dataptr += *dataptr + 1;
                        drawstring_lr_pascal(dataptr, 1, 21*8 + i * 8);
                    }
                }
                                                    
                readkeyboard();

                while (!KEY_PRESSED_UP && !KEY_PRESSED_DOWN && !KEY_PRESSED_FIRE)
                {            
                    i = 22*8;
                    if (current_answer == 0) i = 21*8;
                    if (current_answer == answers-1 && answers > 2) i = 23*8;
                    drawstring_lr_pascal("\x03>>>", 0, i);
                    
                    readkeyboard();
                }
                if (KEY_PRESSED_FIRE)
                {
                    unsigned char *dataptr = answer[current_answer];
                    unsigned short id = ((unsigned short)dataptr[3] << 8) | dataptr[2];
                    while (KEY_PRESSED_FIRE)
                    {
                        readkeyboard();
                    }
                    selecting = 0;
                    exec(answer[current_answer]);
                    if (current_room != id)
                        current_answer = 0;
                    current_room = id;
                }
                if (KEY_PRESSED_UP)
                {
                    while (KEY_PRESSED_UP)
                    {
                        readkeyboard();
                    }
                    if (current_answer > 0)
                        current_answer--;
                }
                if (KEY_PRESSED_DOWN)
                {
                    while (KEY_PRESSED_DOWN)
                    {
                        readkeyboard();
                    }
                    if (current_answer+1 < answers)
                        current_answer++;
                }
                xorshift8();    
            }
        }
    }
}
