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
    OP_DATTR,

    OP_GO,
    OP_GOSUB,
    
    OP_GT,
    OP_GTC,
    OP_LT,
    OP_LTC,
    OP_GTE,
    OP_GTEC,
    OP_LTE,
    OP_LTEC,
    OP_EQ,
    OP_EQC,
    OP_IEQ,
    OP_IEQC,
    
    OP_ASSIGN,
    OP_ASSIGNC,
    OP_ADD,
    OP_ADDC,
    OP_SUB,
    OP_SUBC    
};

unsigned char gState[128]; // game state - 128x8=1024 bits, which "shold" be enough
unsigned char gNumber[32]; // number store - 32 variables "should" be enough
unsigned char y8; // prg state
unsigned char *answer[16]; // collected answers
unsigned char answers; // count of answers
unsigned char attrib, iattrib, dattrib, attrib_c, iattrib_c;
unsigned short go, gosub; // room id:s for go and gosub values
unsigned short current_resource; // currently decompressed resource

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

const unsigned char selectpattern[8] = 
{ 
    0x00, 
    0x88, 
    0xcc, 
    0xee, 
    0xcc, 
    0x88, 
    0x00, 
    0x00  
};

char decimal(unsigned char *d, unsigned char v, unsigned char step)
{
    unsigned char c;
    c = '0';
    while (v >= step) 
    {
        v -= step;
        c++;
    }
    *d = c;
    return v;
}

void patchstring(unsigned char *aDataPtr)
{    
    unsigned char c = aDataPtr[0];
    
    aDataPtr++; // skip len byte
    
    while (c)
    {
        if (aDataPtr[0] == 1)
        {
            unsigned char ov, v;     
            current_resource = 0; // reset current resource so re-patching works
            v = gNumber[aDataPtr[1]];
            ov = v;
            if (ov > 99) v = decimal(aDataPtr, v, 100); else aDataPtr[0] = 128;
            if (ov > 9)  v = decimal(aDataPtr+1, v, 10); else aDataPtr[1] = 128;
            decimal(aDataPtr + 2, v, 1);
        }
        aDataPtr++; c--;
    }
}

void clearbottom()
{
    unsigned short i, j;

    for (i = 21*32; i < 24*32; i++)
        *(unsigned char*)(0x5800+i) = iattrib_c; 

    for (i = 20*32; i < 21*32; i++)
        *(unsigned char*)(0x5800+i) = dattrib; 

    for (j = 21*8; j < 24*8; j++)
        for (i = 0; i < 32; i++)
            *(unsigned char*)(yofs[j]+i) = 0;


}

void unpack_resource(unsigned short id)
{
    // dest = 0xd000, ~4k of scratch. A bit tight?
       
    unsigned short res = *((unsigned short*)(unsigned char*)(0x5b00 + id * 2));

    if (res != current_resource)
    {
    unsigned short v;
    for (v = 0; v < 4096; v++)
        *((unsigned char*)0xd000 + v) = 0;    

    zx7_unpack((unsigned char*)res);        
        current_resource = res;
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
    return !!(gState[id & 0xff] & 1 << (id >> 8));
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
}

#define SET_BIT(x) gState[(x) & 0xff] |= 1 << ((x) >> 8)

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
        case 'C':
            dataptr += 1+1+2+2;
            c -= 5;
            break;
    }
    
    while (c)
    {
        // ignore predicate ops
        unsigned short id = *((unsigned short*)&dataptr[1]);
        switch (*dataptr)
        {
            case OP_SET:
                SET_BIT(id);
                break;
            case OP_CLR:
                gState[id & 0xff] &= (1 << (id >> 8))^0xff;
                break;
            case OP_XOR:
                gState[id & 0xff] ^= 1 << (id >> 8);
                break;
            case OP_ATTR:
                attrib = id;
                attrib_c = (id & 070) | (id >> 3);                
                break;
            case OP_IATTR:
                iattrib = id;
                iattrib_c = (id & 070) | (id >> 3);                
                break;
            case OP_DATTR:
                dattrib = id;
                break;                
            case OP_EXT:
                set_ext(id);
                break;                
            case OP_ASSIGN:
                gNumber[dataptr[1]] = gNumber[dataptr[2]];
                break;
            case OP_ASSIGNC:
                gNumber[dataptr[1]] = dataptr[2];
                break;
            case OP_ADD:
                gNumber[dataptr[1]] += gNumber[dataptr[2]];
                break;
            case OP_ADDC:
                gNumber[dataptr[1]] += dataptr[2];
                break;
            case OP_SUB:
                gNumber[dataptr[1]] -= gNumber[dataptr[2]];
                break;
            case OP_SUBC:    
                gNumber[dataptr[1]] -= dataptr[2];
                break;
            case OP_GO:
                go = id;
                break;
            case OP_GOSUB:
                gosub = id;
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
        case 'C':
            dataptr += 1+1+2+2;
            c -= 5;
            break;
    }
    
    while (c)
    {
        // ignore exec ops
        unsigned short id = *((unsigned short*)&dataptr[1]);
        unsigned char first = gNumber[dataptr[1]];
        unsigned char secondc = dataptr[2];
        unsigned char second = gNumber[secondc];
        switch (*dataptr)
        {
            case OP_HAS: if (!get_bit(id))        ret = 0; break;
            case OP_NOT: if ( get_bit(id))        ret = 0; break;
            case OP_RND: if (xorshift8() > id)    ret = 0; break;
            case OP_GT:  if (!(first >  second))  ret = 0; break;
            case OP_GTC: if (!(first >  secondc)) ret = 0; break;
            case OP_LT:  if (!(first <  second))  ret = 0; break;
            case OP_LTC: if (!(first <  secondc)) ret = 0; break;
            case OP_GTE: if (!(first >= second))  ret = 0; break;
            case OP_GTEC:if (!(first >= secondc)) ret = 0; break;
            case OP_LTE: if (!(first <= second))  ret = 0; break;
            case OP_LTEC:if (!(first <= secondc)) ret = 0; break;
            case OP_EQ:  if (!(first == second))  ret = 0; break;
            case OP_EQC: if (!(first == secondc)) ret = 0; break;
            case OP_IEQ: if (!(first != second))  ret = 0; break;
            case OP_IEQC:if (!(first != secondc)) ret = 0; break;
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


void hitkeytocontinue()
{
    clearbottom();
    //              0123456789ABCDEF0123456789ABCDEF01
    drawstring("\x19[Press enter to continue]", 6, 22*8);
    drawattrib(22*8, iattrib);
    
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

unsigned short hlreg;

void codeblock(unsigned char *dataptr)
{
    unsigned short id = *((unsigned short*)&dataptr[2]);
    hlreg = *((unsigned short*)&dataptr[4]);
    unpack_resource(id);
    //*((unsigned short*)(((char*)&codeblock) + 0x35)) = hlreg;
    asmstuff:
    __asm
        push hl
        ld hl, (#_hlreg)
        call #0xd000
        pop hl
    __endasm;

}

void image(unsigned char *dataptr, unsigned char *aYOfs)
{
    unsigned short id = *((unsigned short*)&dataptr[2]);
    unsigned char * dst;
    unsigned short yp, ayp;
    unsigned char x, y, rows;
    
    unpack_resource(id);
    dataptr = (unsigned char*)0xd000;
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

unsigned char * find_room(unsigned short id)
{
    unsigned char * dataptr = (unsigned char*)0xd000;
    unpack_resource(id);
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

void render_room(unsigned short room_id)
{
    unsigned char *dataptr;
    unsigned char *subreturn;
    unsigned char output_enable;
    unsigned char yofs;
    unsigned char p;
    unsigned char q;

restart:
    p = 0;
    q = 0;
    go = 0xffff;
    gosub = 0xffff;
    output_enable = 1;
    subreturn = 0;
    yofs = 0;
    dataptr = find_room(room_id);    
    SET_BIT(room_id);

    cls();
    
    while (*dataptr)
    {       
        p = pred(dataptr);
        switch (dataptr[1])
        {
            case 'Q': 
                if (q) // the next room
                {
                    return;
                }
                q = 1;
                if (p) 
                {
                    exec(dataptr); 
                }
                answers = 0;
                break;
            case 'I': 
                if (p)
                {
                    exec(dataptr);
                    image(dataptr, &yofs); 
                    unpack_resource(room_id); // re-load the room data
                }
                break;
            case 'C': 
                if (p)
                {
                    exec(dataptr);
                    codeblock(dataptr); 
                    unpack_resource(room_id); // re-load the room data
                }
                break;
            case 'O': 
                if (p) 
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
                if (p && subreturn == 0) // no A:s from subpages
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
                patchstring(dataptr);
                drawstring(dataptr, 0, yofs);
                drawattrib(yofs, attrib);
                yofs += 8;
            }
            dataptr += *dataptr + 1;
        }
        dataptr++;
        
        if (go != 0xffff)
        {
            room_id = go;
            goto restart;
        }
                
        if ((*dataptr == 0 || dataptr[1] == 'Q') && subreturn != 0)
        {
            dataptr = subreturn;
            subreturn = 0;
            unpack_resource(room_id);
        }

        if (gosub != 0xffff && subreturn == 0)
        {
            output_enable = 1;
            subreturn = dataptr;
            dataptr = find_room(gosub);
            SET_BIT(gosub);
            gosub = 0xffff;
            q = 0;
        }                
    }
    // if we get here, this was the last room in the data
}


void reset()
{
    unsigned short i;

    for (i = 0; i < 256; i++)
        gState[i] = 0;
    for (i = 0; i < 32; i++)
        gNumber[i] = 0;

    attrib = 070;
    attrib_c = 077;
    iattrib = 071;
    iattrib_c = 077;
    dattrib = 072;
    cls();
    clearbottom();
    go = 0xffff;
    gosub = 0xffff;
    
    port254(7);

    y8 = 1;
    answers = 0;
    
    current_resource = 0;
}

unsigned char roller;

void drawselector(unsigned char y)
{    
    unsigned char i;   
    unsigned char v;
    roller++;
    v = (roller >> 5) & 7;
    for (i = 0; i < 8; i++,y++)
    {
        unsigned char p = 
            (selectpattern[i & 7] >> v) | (selectpattern[i & 7] << (8-v));
        *(unsigned char*)(yofs[y]+0) = p & 0x1f;
        *(unsigned char*)(yofs[y]+1) = p & 0xf8;
            
            //pattern[i & 7];
    }
}

void main()
{
    unsigned short i;
    unsigned char j;
    unsigned short current_room = 0;    
    unsigned char current_answer = 0;
    unsigned char selecting;    
    unsigned char *widthp = (unsigned char*)(int*)builtin_width - 32;
    widthp[128] = 0;

    reset();     

    for (j = 20*8; j < 21*8; j++)
        for (i = 0; i < 32; i++)
            *(unsigned char*)(yofs[j]+i) = pattern[j & 7];
        
    while(1)
    { 
        /*
        - code string
        - 0..n strings
        - 0        
        */
        clearbottom();        
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
            //              0123456789ABCDEF0123456789ABCDEF01
            drawstring("\x21[The end. Press enter to restart]", 6, 22*8);
            drawattrib(22*8, iattrib);
            
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
                unsigned char yofs;
                clearbottom();
                answer_ofs = current_answer;
                if (current_answer > 0)
                    answer_ofs -= 1;
                if (current_answer == answers-1)
                    answer_ofs -= 1;
                if (answers < 4) answer_ofs = 0;
                yofs = 21*8;
                for (i = 0; i < 3; i++, yofs += 8)
                {
                    if (answer_ofs + i < answers)
                    {
                        unsigned char *dataptr = answer[answer_ofs + i];
                        unsigned char roll = 0;
                        dataptr += *dataptr + 1;
                        patchstring(dataptr);
                        drawstring(dataptr, 1, yofs);
                    }
                }
                drawattrib(21*8, iattrib);
                drawattrib(22*8, iattrib);
                drawattrib(23*8, iattrib);
                                                    
                readkeyboard();

                while (!KEY_PRESSED_UP && !KEY_PRESSED_DOWN && !KEY_PRESSED_FIRE)
                {            
                    i = 22*8;
                    if (current_answer == 0) i = 21*8;
                    if (current_answer == answers-1 && answers > 2) i = 23*8;
                    
                    drawselector(i);
                    
                    readkeyboard();
                }
                if (KEY_PRESSED_FIRE)
                {
                    unsigned char *dataptr = answer[current_answer];
                    unsigned short id = *((unsigned short*)&dataptr[2]);
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
