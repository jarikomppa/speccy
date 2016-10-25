//#define DUMP_BINARY_BLOBS


#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "propfont.h"
#include "../common/pack.h"
#include "../common/zx7pack.h"
#include "yofstab.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"
#include "../common/ihxtools.h"

#define strdup _strdup
#define stricmp _stricmp

#define DATA_AREA_SIZE 29952
#define MAX_SYMBOLS (8*256) // 2k symbols should be enough for everybody
#define MAX_NUMBERS 16

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


const unsigned char gDividerPattern[8] = 
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

const unsigned char gSelectorPattern[8] = 
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

struct Symbol
{
    char *mName;
    int mHits;
};

struct RoomBuf
{
    char *mName;
    unsigned char *mData;
    int mLen;
    int mUsed;
};

struct Token
{
    char * mData;
    int mHits;
    int mHash;

    int mUsed;

    int mNexts;
    int mNext[128];
    int mNextHit[128];    
};

#define MAXTOKENS 1024
Token gToken[MAXTOKENS];
int gTokens = 0;
int gPrevToken = -1;

int gPageData = 0;
int gImageData = 0;
int gCodeData = 0;
int gTrainers = 0;
#define MAX_TRAINER (1024)
unsigned char gTrainer[MAX_TRAINER];

int gSymbols = 0;
Symbol gSymbol[MAX_SYMBOLS*2];
int gImages = 0;
Symbol gImage[MAX_SYMBOLS];
int gCodes = 0;
Symbol gCode[MAX_SYMBOLS];
int gNumbers =0;
Symbol gNumber[MAX_SYMBOLS];

#define MAX_ROOMS 1024
int gRooms = 0;
RoomBuf gRoom[MAX_ROOMS];

int gMaxRoomSymbol = 0;

unsigned char *propfont_data = (unsigned char *)&builtin_data[0];
unsigned char *propfont_width = (unsigned char *)&builtin_width[0];
unsigned char *divider_data = (unsigned char*)&gDividerPattern[0];
unsigned char *selector_data = (unsigned char*)&gSelectorPattern[0];

int verbose = 0;
int quiet = 0;

unsigned char commandbuffer[1024];
int commandptr = 0;
int commandptropofs = 0;

int line = 0;
int image_id_start = 0;
int code_id_start = 0;

char outbuf[1024*1024];
int outlen = 0;

char databuf[1024*1024];
int datalen = 0;

char scratch[64 * 1024];
char stringlit[64 * 1024];
int stringptr;
int lits = 0;

ZX7Pack pack;

int roomno = 0;
int totaldata = 0;
char packbuf[8192];
int packbufofs = 0;

void patchword(unsigned short w, int pos)
{
    outbuf[pos*2] = w & 0xff;
    outbuf[pos*2+1] = w >> 8;
}

void outbyte(unsigned char d)
{
    outbuf[outlen] = d;
    outlen++;    
}

void outdata(unsigned char *d, int len, int skip = 0)
{
    while (len)
    {
        if (!skip)
            outbyte(*d);
        else
            skip--;
        len--;
        d++;
    }
}

void putbyte(unsigned char d)
{
    databuf[datalen] = d;
    datalen++;
}

void putbuf(unsigned char *d, int len)
{
    putbyte(len);
    while (len)
    {
        putbyte(*d);
        len--;
        d++;
    }
}

void putstring(char *d)
{
    putbyte(strlen((const char*)d));
    while (*d)
    {
        if (*d < 32 || *d > 126)
        {
            printf("Invalid character \"%c\" found near line %d\n", *d, line);
            exit(-1);
        }
        putbyte(*d);
        d++;
    }
}

int whitespace(char c)
{
    switch (c)
    {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            return 1;
    }
    return 0;
}

int is_numeric(char *s)
{
    if (!*s) return 0;
    while (*s)
    {
        if (*s < '0' || *s > '9')
            return 0;
        s++;
    }
    return 1;
}

void readrawline(char *buf, FILE * f)
{
    int i = 0;
    int c;
    do
    {
        c = fgetc(f);
        if (c == '\r')
            c = fgetc(f);
        if (c == '\t')
            c = ' ';
        if (feof(f))
            c = '\n';
        buf[i] = c;
        if (!feof(f) && c != '\n')
            i++;
    }
    while (!feof(f) && c > 31);
    
    // trim trailing spaces:
    while (i >= 0 && whitespace(buf[i])) i--;
    i++;
    
    // terminate
    buf[i] = 0;
}

void readline(char *buf, FILE * f)
{
    do
    {
        readrawline(buf, f);
        line++;
    }
    while (!feof(f) && buf[0] == '#' && buf[0] > 31);
}

void token(int n, char *src, char *dst)
{    
    while (n && *src)
    {
        while (*src && !whitespace(*src)) src++;
        while (*src && whitespace(*src)) src++;
        n--;
    }
    while (*src && !whitespace(*src))
    {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = 0;
}


int get_symbol_id(char * s)
{
    int i;
    for (i = 0; i < gSymbols; i++)
    {
        if (stricmp(gSymbol[i].mName, s) == 0)
        {
            gSymbol[i].mHits++;
            return i;
        }
    }
    gSymbol[gSymbols].mName = strdup(s);
    gSymbol[gSymbols].mHits = 1;
    gSymbols++;
    return gSymbols-1;            
}

int get_number_id(char * s)
{
    int i;
    for (i = 0; i < gNumbers; i++)
    {
        if (stricmp(gNumber[i].mName, s) == 0)
        {
            gNumber[i].mHits++;
            return i;
        }
    }
    gNumber[gNumbers].mName = strdup(s);
    gNumber[gNumbers].mHits = 1;
    gNumbers++;
    return gNumbers-1;            
}

int get_image_id(char * s)
{
    int i;
    for (i = 0; i < gImages; i++)
    {
        if (stricmp(gImage[i].mName, s) == 0)
        {
            gImage[i].mHits++;
            return i + image_id_start;
        }
    }
    gImage[gImages].mName = strdup(s);
    gImage[gImages].mHits = 1;
    gImages++;
    return gImages - 1 + image_id_start;            
}

int get_code_id(char * s)
{
    int i;
    for (i = 0; i < gCodes; i++)
    {
        if (stricmp(gCode[i].mName, s) == 0)
        {
            gCode[i].mHits++;
            return i + code_id_start;
        }
    }
    gCode[gCodes].mName = strdup(s);
    gCode[gCodes].mHits = 1;
    gCodes++;
    return gCodes - 1 + code_id_start;            
}


void flush_packbuf()
{
    if (packbufofs > gTrainers)
    {
        pack.mMax = 0;
        pack.pack((unsigned char*)&packbuf[0], packbufofs, gTrainers);
#ifdef DUMP_BINARY_BLOBS        
        char temp[64];
        static int blobno = 0;
        sprintf(temp, "blob%02d.bin", blobno);
        FILE * f = fopen(temp, "wb");
        fwrite(packbuf+gTrainers,1,packbufofs-gTrainers,f);
        fclose(f);
#endif
    
        if (!quiet)
            printf("zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", packbufofs-gTrainers, pack.mMax, ((pack.mMax)*100.0f)/(packbufofs-gTrainers), 0x5b00+outlen);
    
        outdata(pack.mPackedData, pack.mMax);
        packbufofs = gTrainers;
    }
}

void flush_room()
{
    if (datalen > 0)
    {
        putbyte(0); // add a zero byte for good measure.
        if (datalen > 4096)
        {
            printf("Room %s data too large; max 4096 bytes, has %d bytes\n", gSymbol[roomno].mName, datalen);
            exit(-1);
        }
        gRoom[gRooms].mName = gSymbol[roomno].mName;
        gRoom[gRooms].mData = new unsigned char[datalen];
        gRoom[gRooms].mLen = datalen;
        gRoom[gRooms].mUsed = 0;
        memcpy(gRoom[gRooms].mData, databuf, datalen);
        gRooms++;
        datalen = 0;
                
        roomno++;
    }        
}



void flush_sect()
{
    if (datalen > 0)  // skip end if we're in the beginning
    {
        if (verbose) printf("End of section\n");
        putbyte(0);
    }
    
}

void flush_cmd()
{
    if (commandptr)
    {
        int ops = (commandptr-1-(commandptropofs-1)*2) / 3;
        if (verbose) 
        {
            printf("  Command buffer '%c' (%d bytes) with %d bytes payload (%d ops) %d\n", 
            commandbuffer[0], 
            //commandptropofs+1,
            commandptr,
            commandptr-1-(commandptropofs-1)*2, 
            ops, commandptropofs);
        }
                
        if (commandptr > 255)
        {
            printf("Syntax error - too many operations on one statement, line %d\n", line);
            exit(-1);
        }
        putbuf(commandbuffer, commandptr);
    }
    commandptr = 0;
}

void store_cmd(int op, int param)
{
    commandbuffer[commandptr] = op; commandptr++;
    commandbuffer[commandptr] = param & 0xff; commandptr++;
    commandbuffer[commandptr] = param >> 8; commandptr++;
}

void store_number_cmd(int op, int param1, int param2)
{
    commandbuffer[commandptr] = op; commandptr++;
    commandbuffer[commandptr] = param1 & 0xff; commandptr++;
    commandbuffer[commandptr] = param2 & 0xff; commandptr++;
}

void store_section(int section)
{
    flush_cmd();
    flush_sect();
    commandbuffer[commandptr] = section; commandptr++;
}

void store_section(int section, int param)
{
    flush_cmd();
    flush_sect();
    store_cmd(section, param);
}

void store_section(int section, int param, int param2)
{
    flush_cmd();
    flush_sect();
    commandbuffer[commandptr] = section; commandptr++;
    commandbuffer[commandptr] = param & 0xff; commandptr++;
    commandbuffer[commandptr] = param >> 8; commandptr++;
    commandbuffer[commandptr] = param2 & 0xff; commandptr++;
    commandbuffer[commandptr] = param2 >> 8; commandptr++;
}

void set_op(int opcode, int value)
{
    if (value > 255)
    {
        printf("Parameter value out of range, line %d\n", line);
        exit(-1);
    }
    if (verbose) printf("    Opcode: ");
    switch(opcode)
    {
        case OP_HAS: if (verbose) printf("HAS(%s)", gSymbol[value].mName); break;
        case OP_NOT: if (verbose) printf("NOT(%s)", gSymbol[value].mName); break;
        case OP_SET: if (verbose) printf("SET(%s)", gSymbol[value].mName); break;
        case OP_CLR: if (verbose) printf("CLR(%s)", gSymbol[value].mName); break;
        case OP_XOR: if (verbose) printf("XOR(%s)", gSymbol[value].mName); break;
        case OP_RND: if (verbose) printf("RND(%d)", value); break;
        case OP_ATTR: if (verbose) printf("ATTR(%d)", value); break;
        case OP_EXT: if (verbose) printf("EXT(%d)", value); break;
        case OP_IATTR: if (verbose) printf("IATTR(%d)", value); break;
        case OP_DATTR: if (verbose) printf("DATTR(%d)", value); break;
        case OP_GO:  if (verbose) printf("GO(%s)", gSymbol[value].mName); break;
        case OP_GOSUB:  if (verbose) printf("GOSUB(%s)", gSymbol[value].mName); break;
    }
    if (verbose) printf("\n");
    store_cmd(opcode, value);
}

void set_number_op(int opcode, int value1, int value2)
{
    if (value1 > 255 || value2 > 255)
    {
        printf("Parameter value out of range, line %d\n", line);
        exit(-1); 
    }
    if (verbose) printf("    Opcode: ");
    switch(opcode)
    {
        case OP_GT: if (verbose) printf("GT(%s,%s)", gNumber[value1].mName, gNumber[value2].mName); break;
        case OP_GTC: if (verbose) printf("GTC(%s,%d)", gNumber[value1].mName, value2); break;
        case OP_LT: if (verbose) printf("LT(%s,%s)", gNumber[value1].mName, gNumber[value2].mName); break;
        case OP_LTC: if (verbose) printf("LTC(%s,%d)", gNumber[value1].mName, value2); break;
        case OP_GTE: if (verbose) printf("GTE(%s,%s)", gNumber[value1].mName, gNumber[value2].mName); break;
        case OP_GTEC: if (verbose) printf("GTEC(%s,%d)", gNumber[value1].mName, value2); break;
        case OP_LTE: if (verbose) printf("LTE(%s,%s)", gNumber[value1].mName, gNumber[value2].mName); break;
        case OP_LTEC: if (verbose) printf("LTEC(%s,%d)", gNumber[value1].mName, value2); break;
        case OP_EQ: if (verbose) printf("EQ(%s,%s)", gNumber[value1].mName, gNumber[value2].mName); break;
        case OP_EQC: if (verbose) printf("EQC(%s,%d)", gNumber[value1].mName, value2); break;
        case OP_IEQ: if (verbose) printf("IEQ(%s,%s)", gNumber[value1].mName, gNumber[value2].mName); break;
        case OP_IEQC: if (verbose) printf("IEQC(%s,%d)", gNumber[value1].mName, value2); break;
        case OP_ASSIGN: if (verbose) printf("ASSIGN(%s,%s)", gNumber[value1].mName, gNumber[value2].mName); break;
        case OP_ASSIGNC: if (verbose) printf("ASSIGNC(%s,%d)", gNumber[value1].mName, value2); break;
        case OP_ADD: if (verbose) printf("ADD(%s,%s)", gNumber[value1].mName, gNumber[value2].mName); break;
        case OP_ADDC: if (verbose) printf("ADDC(%s,%d)", gNumber[value1].mName, value2); break;
        case OP_SUB: if (verbose) printf("SUB(%s,%s)", gNumber[value1].mName, gNumber[value2].mName); break;
        case OP_SUBC: if (verbose) printf("SUBC(%s,%d)", gNumber[value1].mName, value2); break;
    }
    if (verbose) printf("\n");
    store_number_cmd(opcode, value1, value2);
}

void set_opgo(int op, int value)
{
    if (value >= gMaxRoomSymbol)
    {
        printf("Invalid GO%s parameter: symbol \"%s\" is not a room, line %d\n", 
            op==OP_GOSUB?"SUB":"",
            gSymbol[value].mName,
            line);
        exit(-1);
    }
    set_op(op, value);
}

void set_eop(int value, int maxvalue)
{
    if (value > maxvalue)
    {
        printf("Parameter value out of range, line %d\n", line);
        exit(-1);
    }
    set_op(OP_EXT, value);
}

void parse_op(char *op)
{
    // Op may be of form "foo" "!foo" or "foo:bar" or "foo[intop]bar" where intop is <,>,<=,>=,==,!=,=,+,-
    if (op[0] == 0)
    {
        printf("Syntax error (op=null), line %d\n", line);
        exit(-1);
    }
    if (op[0] == ':' || op[0] == '>' || op[0] == '<' || op[0] == '=' || op[0] == '+' || op[0] == '-' || (op[0] == '!' && op[1] == '='))
    {
        printf("Syntax error (op starting with '%c') \"%s\", line %d\n", op[0], op, line);
        exit(-1);
    }

    int i = 0;
    int operations = 0;
    while (op[i]) 
    { 
        if (op[i] == ':' || 
            op[i] == '-' || 
            op[i] == '+' ||
            (op[i] == '<' && op[i+1] != '=') ||
            (op[i] == '>' && op[i+1] != '=') ||
            (op[i] == '=' && op[i+1] != '='))
        {
            operations++;
        }
        if ((op[i] == '<' && op[i+1] == '=') ||
            (op[i] == '>' && op[i+1] == '=') ||
            (op[i] == '=' && op[i+1] == '=') ||
            (op[i] == '!' && op[i+1] == '='))
        {
            operations++;
            i++;
        }
        
        i++; 
    }

    if (operations > 1)
    {
        printf("Syntax error (op with more than one instruction) \"%s\", line %d\n", op, line);
        exit(-1);
    }
    
    if (operations == 0)
    {
        if (op[0] == '!')
        {        
            set_op(OP_NOT, get_symbol_id(op+1));
        }
        else
        {
            set_op(OP_HAS, get_symbol_id(op));
        }
    }
    else
    {
        char cmd[256];
        char *sym;
        i = 0;
        while (op[i] != ':' && 
               op[i] != '<' && 
               op[i] != '>' && 
               op[i] != '=' && 
               op[i] != '!' && 
               op[i] != '+' && 
               op[i] != '-') 
        {
            cmd[i] = op[i];
            i++;
        }
        cmd[i] = 0;
        if (op[i] == ':')
        {
            sym = op+i+1;
    
            if (stricmp(cmd, "has") == 0) set_op(OP_HAS, get_symbol_id(sym)); else
            if (stricmp(cmd, "need") == 0) set_op(OP_HAS, get_symbol_id(sym)); else
            if (stricmp(cmd, "not") == 0) set_op(OP_NOT, get_symbol_id(sym)); else
            if (stricmp(cmd, "set") == 0) set_op(OP_SET, get_symbol_id(sym)); else
            if (stricmp(cmd, "clear") == 0) set_op(OP_CLR, get_symbol_id(sym)); else
            if (stricmp(cmd, "clr") == 0) set_op(OP_CLR, get_symbol_id(sym)); else
            if (stricmp(cmd, "toggle") == 0) set_op(OP_XOR, get_symbol_id(sym)); else
            if (stricmp(cmd, "xor") == 0) set_op(OP_XOR, get_symbol_id(sym)); else
            if (stricmp(cmd, "flip") == 0) set_op(OP_XOR, get_symbol_id(sym)); else
            if (stricmp(cmd, "random") == 0) set_op(OP_RND, atoi(sym)); else
            if (stricmp(cmd, "rand") == 0) set_op(OP_RND, atoi(sym)); else
            if (stricmp(cmd, "rnd") == 0) set_op(OP_RND, atoi(sym)); else
            if (stricmp(cmd, "attr") == 0) set_op(OP_ATTR, atoi(sym)); else
            if (stricmp(cmd, "iattr") == 0) set_op(OP_IATTR, atoi(sym)); else
            if (stricmp(cmd, "dattr") == 0) set_op(OP_DATTR, atoi(sym)); else
            if (stricmp(cmd, "attrib") == 0) set_op(OP_ATTR, atoi(sym)); else
            if (stricmp(cmd, "iattrib") == 0) set_op(OP_IATTR, atoi(sym)); else
            if (stricmp(cmd, "dattrib") == 0) set_op(OP_DATTR, atoi(sym)); else
            if (stricmp(cmd, "color") == 0) set_op(OP_ATTR, atoi(sym)); else
            if (stricmp(cmd, "ext") == 0) set_op(OP_EXT, atoi(sym)); else
            if (stricmp(cmd, "border") == 0) set_eop(atoi(sym), 7); else
            if (stricmp(cmd, "cls") == 0) set_eop(atoi(sym)+8, 10); else
            if (stricmp(cmd, "go") == 0) set_opgo(OP_GO,get_symbol_id(sym)); else
            if (stricmp(cmd, "goto") == 0) set_opgo(OP_GO,get_symbol_id(sym)); else
            if (stricmp(cmd, "gosub") == 0) set_opgo(OP_GOSUB,get_symbol_id(sym)); else
            if (stricmp(cmd, "call") == 0) set_opgo(OP_GOSUB,get_symbol_id(sym)); else
            {
                printf("Syntax error: unknown operation \"%s\", line %d\n", cmd, line);
                exit(-1);
            }                
        }
        else
        {
            int first = get_number_id(cmd);
            // numeric op <,>,<=,>=,==,!=,=,+,-
            int v = 0;
            if (op[i] == '<' && op[i+1] != '=') v = OP_LT;
            if (op[i] == '<' && op[i+1] == '=') v = OP_LTE;
            if (op[i] == '>' && op[i+1] != '=') v = OP_GT;
            if (op[i] == '>' && op[i+1] == '=') v = OP_GTE;
            if (op[i] == '=' && op[i+1] != '=') v = OP_ASSIGN;
            if (op[i] == '=' && op[i+1] == '=') v = OP_EQ;
            if (op[i] == '!' && op[i+1] == '=') v = OP_IEQ;
            if (op[i] == '+' && op[i+1] != '=') v = OP_ADD;
            if (op[i] == '-' && op[i+1] != '=') v = OP_SUB;
                
            if (v == 0)
            {
                printf("Parse error near \"%s\" (\"%s\"), line %d\n", op+i, op, line);
                exit(-1);
            }
            
            sym = op + i + 1;
            if (op[i+1] == '=') sym++;
            int second = 0;
            if (is_numeric(sym)) 
            {   
                v++;
                second = atoi(sym);
            }
            else
            {
                second = get_number_id(sym);
            }
            set_number_op(v, first, second);
        }
    }
}

int previous_section = 0;
int previous_stringlits = 0;

void parse()
{
    if (previous_section == 'A' && previous_stringlits != 1)
    {
        printf("Statement A must have exactly one line of printable text (%d found)\n"
               "(Multiple lines may be caused by word wrapping; see verbose output\n"
               "to see what's going on), near line %d\n", previous_stringlits, line);
               exit(-1);
    }

    previous_stringlits = 0;
    
    // parse statement
    commandptropofs = 0;
    int i;
    char t[256];
    switch (scratch[1])
    {
    case 'Q':
        flush_room(); // flush previous room
        token(1, scratch, t);
        i = get_symbol_id(t);
        commandptropofs = 2; // $Q + roomno
        store_section('Q', i);  
        if (verbose) printf("Room: \"%s\" (%d)\n", t, i);
        previous_section = 'Q';
        break;
    case 'A':
        token(1, scratch, t);
        i = get_symbol_id(t);
        commandptropofs = 2; // $A + roomno
        store_section('A', i);          
        if (verbose) printf("Choice: %s (%d)\n", t, i);
        previous_section = 'A';
        break;
    case 'P':
        if (previous_section == 'A')
        {
            printf("Syntax error - statement P may not be included in statement A, line %d\n", line);
            exit(-1);
        }
        if (verbose) printf("Empty paragraph\n");
        putstring(" ");
        break;
    case 'O':
        if (previous_section == 'A')
        {
            printf("Syntax error - statement O may not be included in statement A, line %d\n", line);
            exit(-1);
        }
        commandptropofs = 1; // $O
        store_section('O');
        if (verbose) printf("Predicated section\n");
        previous_section = 'O';
        break;
    case 'I':
        if (previous_section == 'A')
        {
            printf("Syntax error - statement I may not be included in statement A, line %d\n", line);
            exit(-1);
        }
        token(1, scratch, t);
        commandptropofs = 2; // $I + imageid
        store_section('I', get_image_id(t));
        if (verbose) printf("Image: \"%s\"\n", t);
        previous_section = 'I';
        break;
    case 'C':
        if (previous_section == 'A')
        {
            printf("Syntax error - statement C may not be included in statement A, line %d\n", line);
            exit(-1);
        }
        token(2, scratch, t);
        i = strtol(t, 0, 0);
        token(1, scratch, t);
        commandptropofs = 3; // $C + codeblock + HL
        store_section('C', get_code_id(t), i);
        if (verbose) printf("Code: \"%s\", %d\n", t, i);
        previous_section = 'C';
        break;
    default:
        printf("Syntax error: unknown statement \"%s\", line %d\n", scratch, line);
        exit(-1);            
    }
    
    i = commandptropofs;    
    do
    {
        token(i, scratch, t);
        if (t[0]) parse_op(t);
        i++;
    }
    while (t[0]);
}

void store(char *lit)
{
    putstring(lit);
    if (verbose) printf("  lit %d: \"%s\"\n", lits++, lit);
    previous_stringlits++;
}

void process()
{
    flush_cmd();
    if (stringptr != 0)
    {
        char temp[256];
//        temp[0] = 0;
        int c = 0;
        int width = 0;
        char *s = stringlit;
        temp[0] = ' ';
        temp[1] = ' ';
        c = 2;
        width = propfont_width[' '-32] * 2;
        while (*s)
        {
            temp[c] = *s;
            c++;
            width += propfont_width[*s-32];
            if (width > 248)
            {
                c--;
                s--;
                while (temp[c] != ' ')
                {
                    c--;
                    s--;
                }
                s++;
                temp[c] = 0;
                store(temp);
                c = 0;
                width = 0;
            }
            s++;
        }
        temp[c] = 0;
        store(temp);
    
        stringptr = 0;
        stringlit[0] = 0;
    }
}

void capture()
{
    // capture string literal
    char *s = scratch;
    
    if (*s == 0)
    {
        // Empty line, cut to separate string lit
        process();
        return;
    }
    
    int was_whitespace = 1;
    
    if (stringptr && *s)
    {
        stringlit[stringptr] = ' ';
        stringptr++;
    }
    
    while (*s)
    {
        int ws = whitespace(*s);
        if (ws)
        {
            if (!was_whitespace)
            {
                ws = 0;
                *s = ' ';
            }
            was_whitespace = 1;
        }
        else
        {
            was_whitespace = 0;
        }
                
        if (!ws)
        {
            stringlit[stringptr] = *s;
            stringptr++;
        }
        
        s++;
    }
    stringlit[stringptr] = 0;
}

void scan(char *aFilename)
{
    FILE * f = fopen(aFilename, "rb");
    if (!f)
    {
        printf("File \"%s\" not found.\n", aFilename);
        exit(-1);
    }
    
    
    stringptr = 0;
    stringlit[0] = 0;
    
    while (!feof(f))
    {
        readline(scratch, f);
        if (scratch[0] == '$')
        {
            // process last string literal
            process();
            // opcode
            parse();
        }
        else
        {
            // string literal
            capture();
        }
    }
    // process final string literal
    process();
    // flush any pending commands
    flush_cmd();
    flush_sect();
    // end with empty section
    flush_sect();
    flush_room(); 
    fclose(f);
}


int calchash(char *s)
{
    unsigned int i = 0;
    while (*s)
    {
        i = (i << 11) | (i >> 21);
        i ^= *s;
        s++;
    }    
    
    return 0;
}    

void tokenref(int cur)
{
    int prev = gPrevToken;
    gPrevToken = cur;
    if (prev != -1)
    {
        int i;
        for (i = 0; i < gToken[prev].mNexts; i++)
        {
            if (gToken[prev].mNext[i] == cur)
            {
                gToken[prev].mNextHit[i]++;
                return;
            }
        }
        if (gToken[prev].mNexts < 128)
        {
            i = gToken[prev].mNexts;
            gToken[prev].mNext[i] = cur;
            gToken[prev].mNextHit[i] = 1;
            gToken[prev].mNexts++;
        }
    }
}


void addwordcounttoken(char *aToken)
{    
    int h = calchash(aToken);
    int i;	
	if (aToken == NULL || *aToken == 0)
		return;
    for (i = 0; i < gTokens; i++)
    {
        if (gToken[i].mHash == h && strcmp(gToken[i].mData, aToken) == 0)
        {
            tokenref(i);
            gToken[i].mHits++;
            return;
        }
    }
    if (gTokens < MAXTOKENS)
    {
        tokenref(gTokens);
        gToken[gTokens].mData = strdup(aToken);
        gToken[gTokens].mHash = h;
        gToken[gTokens].mHits = 1;
        gToken[gTokens].mUsed = 0;
        gToken[gTokens].mNexts = 0;
        gTokens++;        
    }
	else
		gPrevToken = -1;
}

void wordcount(char *aString)
{
    char temp[256];
    int p = 0;
    while (*aString)
    {
        temp[p] = *aString;
        if (*aString == ' ' || *aString == 0 || *aString == '\t' || *aString == '\r' || *aString == '\n')
        {
            temp[p] = 0;
            if (p > 0)
            {
                addwordcounttoken(temp);
            }
            p = 0;
        }
        else
        {
            p++;
        }
        aString++;
    }
	temp[p] = 0;
	addwordcounttoken(temp);
}

int tokencmp (const void * a, const void * b)
{
    int idx1 = *(int*)a;
    int idx2 = *(int*)b;
    return gToken[idx2].mHits - gToken[idx1].mHits;
}

int findidx(int *idx, int i)
{
    int c = 0;
    while (c < MAXTOKENS && idx[c] != i) c++;
    return c;
}

void maketrainer()
{
    int idx[MAXTOKENS];
    int i;
    for (i = 0; i < MAXTOKENS; i++)
        idx[i] = i;
        
    qsort(idx, gTokens, sizeof(int), tokencmp);

    if (verbose)
    {    
        printf("Most frequent words in source material:\n");
        for (i = 0; i < ((gTokens < 25)?gTokens:25); i++)
        {
            printf("%d. \"%s\"\t(%d)\n", i, gToken[idx[i]].mData, gToken[idx[i]].mHits);
        }
    }
        
    int c = 0;
    int done = 0;
    i = 0;
    
    while (c < MAX_TRAINER && i < gTokens)
    {
        c += strlen(gToken[idx[i]].mData) + 1;
        i++;
    }
    int maxtoken = i;
    c = 0;
    i = 0;
    while (c < MAX_TRAINER && !done)
    {   
        if (gToken[idx[i]].mUsed) i = 0;
        while (gToken[idx[i]].mUsed && i < gTokens) i++;
        if (i >= gTokens) 
		{ 
			done = 1; 
			i = 0; 
		}
		else
		{
            
			char * s = gToken[idx[i]].mData;
			while (*s && c < MAX_TRAINER)
			{
				gTrainer[c] = *s;
				s++;
				c++;
			}
        
			if (c < MAX_TRAINER)
			{
				gTrainer[c] = ' ';
				c++;
			}
        
			gToken[idx[i]].mUsed = 1;
        
			// Try to chain the words together
			int min = -1;
			int mini = 0;
			int j;
			for (j = 0; j < gToken[idx[i]].mNexts; j++)
			{
				int n = gToken[idx[i]].mNext[j];
				int h = gToken[idx[i]].mNextHit[j];
				int nextidx = findidx(idx, n);
				if (nextidx < maxtoken && 
					h > min && 
					gToken[n].mUsed == 0)
				{
					min = h;
					mini = nextidx;
				}
			}
		
			if (verbose)
			{    
				printf("%s", gToken[idx[i]].mData);
				if (min == -1)
				{
					printf(" # ");
				}
				else
				{
					printf("[%d]->", min);
				}
			}
			i = mini;
		}
    }
    gTrainers = c;
    
    if (!quiet)
    {
        printf(
            "Trainer data: %d bytes\n" 
            "|---------1---------2---------3---------4---------5---------6----|\n ",          
            gTrainers);
        for (c = 0; c < gTrainers; c++)
        {
            printf("%c", gTrainer[c]);
            if (c % 64 == 63) printf("\n ");
        }
        printf("\n");
    }
}

void scan_first_pass(char *aFilename)
{
    FILE * f = fopen(aFilename, "rb");
    if (!f)
    {
        printf("File \"%s\" not found.\n", aFilename);
        exit(-1);
    }
    
    // Register beepfx as a possible code block. 
    // Won't be stored if not actually in use.
    int i;
    i = get_code_id("beepfx.ihx");
    gCode[i].mHits--;
	
    while (!feof(f))
    {
        readline(scratch, f);
        if (scratch[0] == '$')
        {
            char t[256];
            token(1, scratch, t);
            if (scratch[1] == 'Q')
            {
                i = get_symbol_id(t);
                if (gSymbol[i].mHits > 1)
                {
                    printf("syntax error: room id \"%s\" used more than once, line %d\n", t, line);
                    exit(-1);
                }
                gSymbol[i].mHits--; // clear the hit, as it'll be scanned again
            }
            if (scratch[1] == 'I')
            {
                i = get_image_id(t);
                gImage[i].mHits--; // clear the hit, as it'll be scanned again
            }
            if (scratch[1] == 'C')
            {
                i = get_code_id(t);
                gCode[i].mHits--; // clear the hit, as it'll be scanned again
            }
        }
        else
        {
			// string literal
			wordcount(scratch);
        }
    }
    gMaxRoomSymbol = gSymbols;
    fclose(f);
}


void report()
{
    int i;
    printf("\n");
    printf("Token Hits Symbol\n");
    for (i = 0; i < gSymbols; i++)
    {
        printf("%5d %4d \"%s\"%s\n", i, gSymbol[i].mHits, gSymbol[i].mName, gSymbol[i].mHits < 2 ? " <- Warning":"");
    }

    if (gNumbers)
    {
	    printf("\n");
        printf("Token Hits Number\n");
        for (i = 0; i < gNumbers; i++)
        {
            printf("%5d %4d \"%s\"\n", i, gNumber[i].mHits, gNumber[i].mName);
        }
    }

    if (gImages)
    {
	    printf("\n");
        printf("Token Hits Image\n");
        for (i = 0; i < gImages; i++)
        {
            printf("%5d %4d \"%s\"\n", i, gImage[i].mHits, gImage[i].mName);
        }
    }

    if (gCodes > 1 || (gCodes == 1 && gCode[0].mHits > 0))
    {
	    printf("\n");
        printf("Token Hits Code\n");
        for (i = 0; i < gCodes; i++)
        {
            printf("%5d %4d \"%s\"\n", i, gCode[i].mHits, gCode[i].mName);
        }
    }

    //      123456789012345678901234567890123456789012345678901234567890
    printf("\n");
    printf("Memory map:\n\n");
    printf("         5         10        15        20        25      29\n");
    printf("---------.---------|---------.---------|---------.--------\n");
    int o = 0;
    for (i = 0; i < gPageData / 512; i++)
        o += printf("P");
    for (i = 0; i < gImageData / 512; i++)
        o += printf("I");
    for (i = 0; i < gCodeData / 512; i++)
        o += printf("C");
    for (i = o; i < 29*2-(gTrainers/512); i++)
        o += printf(".");
    for (i = o; i < 29*2; i++)
        o += printf("t");
    printf("\n\n");
    printf("Page data : %5d bytes\n", gPageData);
    printf("Image data: %5d bytes\n", gImageData);
    printf("Code data : %5d bytes\n", gCodeData);
    printf("Free      : %5d bytes\n", DATA_AREA_SIZE - gTrainers - gPageData - gImageData);
    printf("Trainer   : %5d bytes (used to improve compression by \"training\" it)\n", gTrainers);
        
}

int scan_row(int row, unsigned char*d)
{
    int i, j;
    int ret = 0;
    int ref = 0;
    if (d[yofs[row*8]-0x4000] == 0xff)
        ref = 0xff;
    for (j = 0; j < 32; j++)
    {
        int hit = 0;
        for (i = 0; i < 8; i++)
        {
            if (d[(yofs[row*8+i]-0x4000)+j] != ref)
            {
                hit = 1;
            }
        }
        if (hit)
        {
            ret = 1;
            if (verbose)
                printf("*");
        }
        else
        {
            if (verbose)
                printf(" ");
        }                
    }
    if (verbose)
        printf(" row %d, live %d\n", row, ret);
    return ret;
}

void process_images()
{
    int i;
    unsigned char t[6912];
    for (i = 0; i < gImages; i++)
    {
        FILE * f = fopen(gImage[i].mName, "rb");
        if (!f)
        {
            printf("Image \"%s\" not found.\n", gImage[i].mName);
            exit(-1);
        }
        fseek(f,0,SEEK_END);
        if (ftell(f) != 6912)
        {
            printf("Image \"%s\" wrong size (has to be 6912 bytes).\n", gImage[i].mName);
            exit(-1);
        }
        fseek(f,0,SEEK_SET);
        fread(t,1,6912,f);
        fclose(f);
        int j = 0;
        int maxlive = 0;
        for (j = 0; j < 24; j++)
        {
            if (scan_row(j, t))
            {
                maxlive = j;
            }
        }
        maxlive++;
        if (maxlive > 14)
        {
            printf("Warning: image \"%s\" has %d live character rows, 14 used.\n", gImage[i].mName, maxlive);
            maxlive = 14;
        }
        datalen = 0;
        putbyte(maxlive*8);
        int k;
        for (j = 0; j < 8*maxlive; j++)
            for (k = 0; k < 32; k++)
                putbyte(t[yofs[j]-0x4000 + k]);
        for (j = 0; j < 32*maxlive; j++)
            putbyte(t[j + 192*32]);

        pack.mMax = 0;
        if (datalen > 4096)
        {
            printf("Image %s data too large; max 4096 bytes, has %d bytes (%d lines)\n", gImage[i].mName, datalen, maxlive);
            exit(-1);
        }
        pack.pack((unsigned char*)&databuf[0], datalen);
        patchword(0x5b00 + outlen, i + image_id_start);        

        if (!quiet)
            printf("%25s (%02d) zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", gImage[i].mName, maxlive, datalen, pack.mMax, (pack.mMax*100.0f)/datalen, 0x5b00+outlen);
        outdata(pack.mPackedData, pack.mMax);            
    }
}

void process_codes(char *path)
{
    int i;
    for (i = 0; i < gCodes; i++)
    {
        if (gCode[i].mHits > 0)
        {
            FILE * f;
            
            f = fopen(gCode[i].mName, "rb");
            if (!f)
            {
                char temp[1024];
                strcpy(temp, path);
                char *d = strrchr(temp, '\\');
                if (d)
                {
                    strcpy(d+1, gCode[i].mName);
                    f = fopen(temp, "rb");
                }
            }
            if (!f)
            {
                printf("Code \"%s\" not found", gCode[i].mName);
                exit(-1);
            }
            fseek(f,0,SEEK_END);
            int len = ftell(f);
            fseek(f,0,SEEK_SET);
            unsigned char *ihx = new unsigned char[len+1];
            fread(ihx, 1, len, f);
            fclose(f);
            
            unsigned char codebuf[65536];
            int start, end;
            int l = decode_ihx(ihx, len, codebuf, start, end, 0);
            if (l == 0)
            {
                if (!quiet)
                {
                    printf("Couldn't decode \"%s\" as .ihx, assuming binary\n", gCode[i].mName);
                }
                start = 0xd000;
                l = len;
                if (len < 4096)
                    memcpy(codebuf+0xd000, ihx, len);
            }
            delete[] ihx;
            
            if (l > 4096 || start != 0xd000)
            {
                if (l > 4096)
                {
                    printf("Code %s data too large; max 4096 bytes, has %d bytes\n", gCode[i].mName, l);
                }
                if (start != 0xd000)
                {
                    printf("Code %s start address not 0xd000; 0x%04x found\n", gCode[i].mName, start);
                }
                exit(-1);
            }
    
    
            datalen = 0;
            int j;
            for (j = 0; j < l; j++)
                putbyte(codebuf[0xd000 + j]);
    
            pack.mMax = 0;
            pack.pack((unsigned char*)&databuf[0], datalen);
            patchword(0x5b00 + outlen, i + code_id_start);        
    
            if (!quiet)
                printf("%30s zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", gCode[i].mName, datalen, pack.mMax, (pack.mMax*100.0f)/datalen, 0x5b00+outlen);
            outdata(pack.mPackedData, pack.mMax);            
        }
    }
}

void output(char *aFilename)
{
    FILE * f = fopen(aFilename, "wb");
    if (!f)
    {
        printf("Can't open \"%s\" for writing.\n", aFilename);
        exit(-1);
    }
    

    fwrite(outbuf, 1, outlen, f);
    fclose(f);
}

int find_data(unsigned char *buf, int start, const unsigned char *data, int bytes)
{
    int i;
    for (i = 0; i < (65535 - start - bytes); i++)
    {
        int found = 1;
        int j;
        for (j = 0; found && j < bytes; j++)
        {
            if (buf[start + i + j] != data[j])
            {
                found = 0;
            }
        }
        if (found)
        {
            return i + start;            
        }
    }
    printf("Can't find data to patch in crt0.ihx!\n");
    exit(-1);
    return 0;
}

void patch_data(unsigned char *buf, int start, const unsigned char *data, int bytes)
{
    memcpy(buf + start, data, bytes);
}

void patch_ihx(char *path)
{
    FILE * f;
    
    f = fopen("crt0.ihx", "rb");
    if (!f)
    {
        char temp[1024];
        strcpy(temp, path);
        char *d = strrchr(temp, '\\');
        if (d)
        {
            strcpy(d+1, "crt0.ihx");
            f = fopen(temp, "rb");
        }
    }
    if (!f)
    {
        printf("crt0.ihx not found");
        exit(-1);
    }
    fseek(f,0,SEEK_END);
    int len = ftell(f);
    fseek(f,0,SEEK_SET);
    unsigned char *ihx = new unsigned char[len+1];
    fread(ihx, 1, len, f);
    fclose(f);
    
    unsigned char codebuf[65536];
    int start, end;
    decode_ihx(ihx, len, codebuf, start, end);
    delete[] ihx;
    
    int ofs = find_data(codebuf, start, builtin_data, 94*8);
    patch_data(codebuf, ofs, propfont_data, 94*8);
    
    ofs = find_data(codebuf, start, builtin_width, 94);
    patch_data(codebuf, ofs, propfont_width, 94);
    
    ofs = find_data(codebuf, start, gDividerPattern, 8);
    patch_data(codebuf, ofs, divider_data, 8);

    ofs = find_data(codebuf, start, gSelectorPattern, 8);
    patch_data(codebuf, ofs, selector_data, 8);
    
    write_ihx("patched.ihx", codebuf, start, end);
}

struct CharData
{
    int xmin, xmax;
    unsigned char pixdata[20];
};

CharData chardata[94];

void shift()
{
    int i, j;
    for (i = 0; i < 94; i++)
    {
        for (j = 0; j < 8; j++)
        {
            chardata[i].pixdata[j] <<= chardata[i].xmin;
        }
    }
}

void scan_glyph(unsigned int *data, CharData &parsed)
{
    parsed.xmin = 8;
    parsed.xmax = 0;
    int i, j;
    for (i = 0; i < 8; i++)
    {        
        parsed.pixdata[i] = 0;
        for (j = 0; j < 8; j++)
        {
            if (data[i*8+j] & 0xffffff)
            {
                if (parsed.xmin > j) parsed.xmin = j;
                if (parsed.xmax < j) parsed.xmax = j;
                parsed.pixdata[i] |= 0x80 >> j;
            }            
        }
    }
    if (parsed.xmin > parsed.xmax)
    {
        parsed.xmin = 0;
        parsed.xmax = 4-1;
    }
}


void scan_font(char *aFilename)
{   
    int x,y,n;
    unsigned int *data = (unsigned int*)stbi_load(aFilename, &x, &y, &n, 4);
    if (!data)
    {
        printf("unable to load \"%s\"\n", aFilename);
        exit(-1);
    }
    if (x != 8 || y < 752 || y % 94)
    {
        printf("Bad image dimensions; should be 8x752 (found %dx%d)\n", x, y);
        exit(-1);
    }
    
    for (n = 0; n < 94; n++)
    {
        scan_glyph(data+8*8*n, chardata[n]);
    }
    
    shift();

    propfont_data = new unsigned char[94*8];
    propfont_width = new unsigned char[94];
    
    int i, j, c;
    for (i = 0, c = 0; i < 94; i++)
    {
        for (j = 0; j < 8; j++, c++)
        {
            propfont_data[c] = chardata[i].pixdata[j];
        }
    }

    for (i = 0; i < 94; i++)
    {
        propfont_width[i] = chardata[i].xmax-chardata[i].xmin+1+1;
    }
}

void scan_div(char *aFilename)
{
    int x,y,n,i,j;
    unsigned int *data = (unsigned int*)stbi_load(aFilename, &x, &y, &n, 4);
    if (!data)
    {
        printf("unable to load \"%s\"\n", aFilename);
        exit(-1);
    }
    if (x != 8 || y != 8)
    {
        printf("Divider pattern image not 8x8 (found %dx%d)\n", x, y);
        exit(-1);
    }
    divider_data = new unsigned char[8];
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            divider_data[i] <<= 1;
            divider_data[i] |= (data[i * 8 + j] & 0xff) ? 1 : 0;
        }
    }              
}

void scan_sel(char *aFilename)
{
    int x,y,n,i,j;
    unsigned int *data = (unsigned int*)stbi_load(aFilename, &x, &y, &n, 4);
    if (!data)
    {
        printf("unable to load \"%s\"\n", aFilename);
        exit(-1);
    }
    if (x != 8 || y != 8)
    {
        printf("Selector pattern image not 8x8 (found %dx%d)\n", x, y);
        exit(-1);
    }
    selector_data = new unsigned char[8];
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            selector_data[i] <<= 1;
            selector_data[i] |= (data[i * 8 + j] & 0xff) ? 1 : 0;
        }
    }              
}

void output_trainer()
{
    int i;
    int freebytes = DATA_AREA_SIZE - gTrainers - outlen;
    for (i = 0; i < freebytes; i++)
        outbyte(0);
    outdata(gTrainer, gTrainers);
}

/*
// 186 bytes free
void process_rooms()
{
    int i, j;
    int minidx = -1;
    do
    {
        int minvalue = 8192;
        minidx = -1;
        for (j = 0; j < rooms; j++)
        {
            if (gRoom[j].mUsed == 0)
            {
                if (gRoom[j].mLen+packbufofs-trainers < 4096)
                {
                    memcpy(packbuf+packbufofs, gRoom[j].mData, gRoom[j].mLen);
                    pack.mMax = 0;
                    pack.pack((unsigned char*)&packbuf[0], packbufofs + gRoom[j].mLen, trainers);
                    if (pack.mMax < minvalue)
                    {
                        minvalue = pack.mMax;
                        minidx = j;
                    }
                }
            }
        }
        
        if (minidx != -1)
        {
            memcpy(packbuf+packbufofs, gRoom[minidx].mData, gRoom[minidx].mLen);
            packbufofs += gRoom[minidx].mLen;
            gRoom[minidx].mUsed = 1;
            patchword(0x5b00 + outlen, minidx);
            printf("%s ", gRoom[minidx].mName);
        }
        else
        {
            if (packbufofs > trainers)
            {
                flush_packbuf();
                minidx = 0;
            }                      
        }
    }
    while (minidx != -1);
}
*/

/*
int biggest_unused_room()
{
    int i;
    int bigsize = 0;
    int bigidx = -1;
    for (i = 0; i < rooms; i++)
    {
        if (gRoom[i].mUsed == 0)
        {
            if (gRoom[i].mLen > bigsize)
            {
                bigsize = gRoom[i].mLen;
                bigidx = i;
            }
        }
    }    
    return bigidx;
}
*/

float *compression_results;

int best_compressible_room()
{
    int i, j;
    int idx = -1;
    float ratio = 1000;
    for (i = 0; i < gRooms; i++)
    {
        if (gRoom[i].mUsed == 0)
        {
            for (j = i+1; j < gRooms; j++)
            {
                if (gRoom[j].mUsed == 0)
                {
                    float r;
                    if (compression_results[i * gRooms + j] == 0)
                    {
                        int total = gRoom[i].mLen + gRoom[j].mLen;
                        char tempbuf[8192];
                        memcpy(tempbuf, gRoom[i].mData, gRoom[i].mLen);
                        memcpy(tempbuf + gRoom[i].mLen, gRoom[j].mData, gRoom[j].mLen);
                        pack.mMax = 0;
                        pack.pack((unsigned char*)&tempbuf[0], total);
                        //printf("ZX7: %4d bytes - %3.3f%%", pack.mMax, ((float)pack.mMax/total)*100);
                        r = ((float)pack.mMax / total);
                        compression_results[i * gRooms + j] = r;
                    }
                    else
                    {
                        r = compression_results[i * gRooms + j];
                    }
                    if (ratio > r)
                    {
                        ratio = r;
                        idx = i;
                    }                
                }
            }
        }
    }
    return idx;
}
   

//270 bytes free (start from page 0)
//314 bytes free (start from longest page)
// - makes sense, most pairing opportunities for the biggest page
//restaring from longest makes things worse
// - not sure why
//using trainers when matching makes things worse
// - not all data is at start of buffer..
//1620 bytes free (restart from page with best compression ratio with some other page)
// - slow, but apparently very effective.
//6558 bytes free (bugfix, wasn't tracking idx correctly, thanks to bad variable naming)
// - this is ridiculous. The savings are so large I'm not sure I'm storing everything anymore.
void process_rooms()
{
    int j, idx;
    if (!quiet)
        printf("Compressing..\n");
    
    compression_results = new float[gRooms * gRooms];
    for (j = 0; j < gRooms * gRooms; j++)
        compression_results[j] = 0;
    
    //int minidx = biggest_unused_room();    
    int minidx = best_compressible_room();
    idx = minidx;
    gRoom[idx].mUsed = 1;
    patchword(0x5b00 + outlen, idx);
    memcpy(packbuf+packbufofs, gRoom[idx].mData, gRoom[idx].mLen);
    packbufofs += gRoom[idx].mLen;
    printf("%s ", gRoom[idx].mName);
    totaldata = gRoom[idx].mLen;
    do 
    {
        minidx = -1;
        float minvalue = 1000;
        for (j = 0; j < gRooms; j++)
        {
            if (idx != j && gRoom[j].mUsed == 0)
            {
                int total = gRoom[idx].mLen+gRoom[j].mLen;
                if (total < 4096)
                {
                    float r;
                    if (compression_results[idx * gRooms + j] == 0)
                    {
                        char tempbuf[8192];
                        memcpy(tempbuf, gRoom[idx].mData, gRoom[idx].mLen);
                        memcpy(tempbuf + gRoom[idx].mLen, gRoom[j].mData, gRoom[j].mLen);
                        pack.mMax = 0;
                        pack.pack((unsigned char*)&tempbuf[0], total);
                        //printf("ZX7: %4d bytes - %3.3f%%", pack.mMax, ((float)pack.mMax/total)*100);
                        r = ((float)pack.mMax / total);
                        compression_results[idx * gRooms + j] = r;
                    }
                    else
                    {
                        r = compression_results[idx * gRooms + j];
                    }
                    if (minvalue > r)
                    {
                        minvalue = r;
                        minidx = j;
                    }
                }
            }
        }    
        
        if (minidx != -1)
        {
            totaldata += gRoom[minidx].mLen;
            if (totaldata > 4096)
            {
                flush_packbuf();
                //minidx = biggest_unused_room();
                minidx = best_compressible_room();
                totaldata = gRoom[minidx].mLen;
            }
            printf("%s ", gRoom[minidx].mName);
            gRoom[minidx].mUsed = 1;
            idx = minidx;
            
            memcpy(packbuf+packbufofs, gRoom[minidx].mData, gRoom[minidx].mLen);
            packbufofs += gRoom[minidx].mLen;
             
            patchword(0x5b00 + outlen, minidx); // N rooms will have same offset
        }
    } while (minidx != -1);
    
    flush_packbuf();
    int panic = 0;
    for (j = 0; j < gRooms; j++)
    {
        if (gRoom[j].mUsed == 0)
        {
            printf("ERROR Room \"%s\" didn't compress with anything\n", gRoom[j].mName);
            panic = 1;
        }
    }
    if (panic)
        exit(-1);

    printf("\n");
    delete[] compression_results;
}

void sanity()
{
    int i, j;
    for (i = 0; i < gSymbols; i++)
    {
        for (j = 0; j < gNumbers; j++)
        {
            if (stricmp(gSymbol[i].mName, gNumber[j].mName) == 0)
            {
                printf("Warning: Symbol \"%s\" used both as a flag and a number. This probably isn't what you wanted.\n", gSymbol[i].mName);
            }
        }
    }

    if (gSymbols > MAX_SYMBOLS)
    {
        printf("Too many symbols in use (%d > %d)\n", gSymbols, MAX_SYMBOLS);
        exit(-1);
    }
    
    if (gNumbers > MAX_NUMBERS)
    {
        printf("Too many numeric variables in use (%d > %d)\n", gNumbers, MAX_NUMBERS);
        exit(-1);
    }

    if (outlen > DATA_AREA_SIZE)
    {
        printf("Total output size too large (%d bytes max, %d found)\n", DATA_AREA_SIZE, outlen);
        exit(-1);
    }
}

int main(int parc, char **pars)
{    
    printf("MuCho compiler, by Jari Komppa http://iki.fi/sol/\n");
    if (parc < 3)
    {
        printf(
            "%s <input> <output> [font image [divider image [selector image]]] [flags]\n"
            "Optional flags:\n"
            "  -v   verbose (useful for debugging)\n"
            "  -q   quiet (minimal output)\n",
            pars[0]);
        return -1;
    }
    int infile = -1;
    int outfile = -1;
    int fontfile = -1;
    int divfile = -1;
    int selfile = -1;
    int i;
    for (i = 1; i < parc; i++)
    {
        if (pars[i][0] == '-')
        {
            switch (pars[i][1])
            {
                case 'v':
                case 'V':
                    verbose = 1;
                    break;
                case 'q':
                case 'Q':
                    quiet = 1;
                    verbose = 0;
                    break;
                default:
                    printf("Unknown parameter \"%s\"\n", pars[i]);
                    exit(-1);
            }
        }
        else
        {
            if (infile == -1) 
            {
                infile = i; 
            }
            else
            {    
                if (outfile == -1) 
                {
                    outfile = i; 
                }
                else
                {
                    if (fontfile == -1) 
                    {
                        fontfile = i; 
                    }
                    else
                    {
                        if (divfile == -1) 
                        {
                            divfile = i; 
                        }
                        else
                        {
                            if (selfile == -1) 
                            {
                                selfile = i; 
                            }
                            else
                            {
                                printf("Invalid parameter \"%s\" (input, output, font, divider and selector image files already defined)\n", pars[i]);
                                exit(-1);
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (outfile == -1)
    {
        printf("Invalid parameters (run without params for help\n");
        exit(-1);
    }

    if (fontfile != -1)
    {
        scan_font(pars[fontfile]);
    }

    if (divfile != -1)
    {
        scan_div(pars[divfile]);
    }

    if (selfile != -1)
    {
        scan_sel(pars[selfile]);
    }

    patch_ihx(pars[0]);

    line = 0;    
    scan_first_pass(pars[infile]);
    maketrainer();    
    memcpy(packbuf, gTrainer, gTrainers);
    packbufofs = gTrainers;
    datalen = 0;
    image_id_start = gSymbols + 1;
    code_id_start = gSymbols + gImages + 1;
    outlen = gSymbols * 2 + gImages * 2 + gCodes * 2 + 2;
    line = 0;    
    scan(pars[infile]);
    process_rooms();
    gPageData = outlen;
    process_images();
    gImageData = outlen - gPageData;
    process_codes(pars[0]);
    gCodeData = outlen - gPageData - gImageData;
    output_trainer();
    if (!quiet)
        report();
    sanity();
    output(pars[outfile]);

    
    return 0;
}

