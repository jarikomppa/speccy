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
unsigned short framecounter = 0;

#define FONTHEIGHT 8

#include "yofstab.h"
#include "hwif.c"
#include "propfont.h"
#include "drawstring.c"

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))

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

unsigned char state[256];
unsigned char y8;
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
    id;// tbd
}

void set_ext(unsigned short id)
{
    id;// tbd
}

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

void answer(unsigned char *dataptr)
{
    dataptr; // TODO
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
                    return;
                if (pred(dataptr)) exec(dataptr); break;
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
                answer(dataptr);
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
}

void main()
{
    unsigned short i;
    unsigned short current_room = 0;    

    y8 = 1;

    for (i = 0; i < 256; i++)
        state[i] = 0;

    for (i = 0; i < 192*32; i++)
        *(unsigned char*)(0x4000+i) = 0;
    for (i = 0; i < 24*32; i++)
        *(unsigned char*)(0x5800+i) = 7 << 3;

    framecounter = 0;

    port254(7);
    
    for (i = 0; i < 192*32; i++)
      *(unsigned char*)(0x4000+i) = 0;
      
    
    
    while(1)
    {
        unsigned char * dataptr = (unsigned char*)0x5b00;

        framecounter++;
        //do_halt(); // halt waits for interrupt - or vertical retrace
  
        /*
        - code string
        - 0..n strings
        - 0        
        */
        
        render_room(current_room);

/*        
        i = 0;
        while (*dataptr)
        {
            
            dataptr += *dataptr + 1;            
            while (*dataptr != 0)
            {
                drawstring_lr_pascal(dataptr, 0, i);
                i += 8;
                dataptr += *dataptr + 1;                
            }
            dataptr++;       
        }        
*/        
        xorshift8();
    }
}
