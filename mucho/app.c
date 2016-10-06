/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org
 * (practically public domain)
*/

#include <string.h>

unsigned char fbcopy_idx;

unsigned char *data_ptr;
unsigned char *screen_ptr;

unsigned char port254tonebit;

#define FONTHEIGHT 8
#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

#include "yofstab.h"
#define HWIF_IMPLEMENTATION
#include "hwif.c"
#include "propfont.h"
#include "drawstring.c"


enum opcodeval
{
    OP_HAS,
    OP_NOT,
    OP_SET,
    OP_CLR,
    OP_XOR,
    OP_RND,
    OP_ATTR,
    OP_EXT
};

unsigned char state[256]; // game state
unsigned char y8; // prg state
unsigned char *answer[16];
unsigned char answers;
unsigned char attrib;

unsigned char * find_room(unsigned short id)
{
    unsigned char * dataptr = (unsigned char*)0x5b00;
        /*
        - code string
        - 0..n strings
        - 0        
        */
    while (1)
    {
        if (dataptr[1] == 'Q')
        {
            if (dataptr[2] == (id & 0xff) &&
                dataptr[3] == (id >> 8))
            {
                return dataptr;
            }
        }
        dataptr += *dataptr + 1;
        while (*dataptr)
        {
            dataptr += *dataptr + 1;
        }
        dataptr++;
    }
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

void set_bit(unsigned short id)
{
    state[id & 0xff] |= 1 << (id >> 8);
}

void clear_bit(unsigned short id)
{
    state[id & 0xff] &= (1 << (id >> 8))^0xff;
}

void toggle_bit(unsigned short id)
{
    state[id & 0xff] ^= 1 << (id >> 8);
}

void set_attr(unsigned short id)
{
    attrib = id;
}

void set_ext(unsigned short id)
{
    switch (id)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            port254(id);
            break;
        case 8:
            // cls
            break;                    
    }
}


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
                set_bit(id);
                break;
            case OP_CLR:
                clear_bit(id);
                break;
            case OP_XOR:
                toggle_bit(id);
                break;
            case OP_ATTR:
                set_attr(id);
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

void image(unsigned char *dataptr, unsigned char *yofs)
{
    if (pred(dataptr))
    {
        exec(dataptr);
        *yofs += 8;
        drawstring("[image]", 0, *yofs); // TODO
        *yofs += 16;
    }
}

void add_answer(unsigned char *dataptr)
{
    answer[answers] = dataptr;
    answers++;
    if (answers == 16)
        answers = 15;
}

void render_room(unsigned short room_id)
{
    unsigned char *dataptr = find_room(room_id);
    unsigned char output_enable = 1;
    unsigned char yofs = 0;
    unsigned short i;
    set_bit(room_id);

    for (i = 0; i < 192*32; i++)
      *(unsigned char*)(0x4000+i) = 0;
    
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
            case 'I': image(dataptr, &yofs); break;
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
                add_answer(dataptr);
                output_enable = 0;
                break;
        }
        dataptr += *dataptr + 1;
        
        while (*dataptr)
        {
            if (output_enable)
            {
                drawstring_lr_pascal(dataptr, 0, yofs);
                yofs += 8;
            }
            dataptr += *dataptr + 1;
        }
        dataptr++;
    }
    // if we get here, this was the last room in the data
}

void main()
{
    unsigned short i;
    unsigned short current_room = 0;    
    unsigned char current_answer = 0;
    unsigned char selecting;

    y8 = 1;
    answers = 0;
    attrib = 7 << 3;
    port254(7);

    for (i = 0; i < 256; i++)
        state[i] = 0;

    for (i = 0; i < 192*32; i++)
        *(unsigned char*)(0x4000+i) = 0;
    for (i = 0; i < 24*32; i++)
        *(unsigned char*)(0x5800+i) = 7 << 3;

    
    for (i = 0; i < 192*32; i++)
      *(unsigned char*)(0x4000+i) = 0;
      
    
    
    while(1)
    {
        unsigned char * dataptr = (unsigned char*)0x5b00;

        //do_halt(); // halt waits for interrupt - or vertical retrace
  
        /*
        - code string
        - 0..n strings
        - 0        
        */
        
        render_room(current_room);

        selecting = 1;
        while (selecting)
        {
            unsigned char *dataptr = answer[current_answer];
            dataptr += *dataptr + 1;
            drawstring_lr_pascal(dataptr, 0, 21*8);
            readkeyboard();
            if (KEYDOWN(ENTER))
                selecting = 0;
            xorshift8();    
        }                       
    }
}
