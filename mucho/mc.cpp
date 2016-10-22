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

#define DATA_AREA_SIZE 29952

const unsigned char divider_pattern[8] = 
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

const unsigned char selector_pattern[8] = 
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
    char * name;
    int hits;
};

struct RoomBuf
{
    char *name;
    unsigned char *data;
    int len;
    int used;
};

int pagedata = 0;
int imgdata = 0;
int trainers = 0;
#define MAX_TRAINER (1024)
unsigned char trainer[MAX_TRAINER];

#define MAX_SYMBOLS (8*256) // 2k symbols should be enough for everybody
int symbols = 0;
Symbol symbol[MAX_SYMBOLS];
int images = 0;
Symbol image[MAX_SYMBOLS];

#define MAX_ROOMS 1024
int rooms = 0;
RoomBuf room[MAX_ROOMS];

unsigned char *propfont_data = (unsigned char *)&builtin_data[0];
unsigned char *propfont_width = (unsigned char *)&builtin_width[0];
unsigned char *divider_data = (unsigned char*)&divider_pattern[0];
unsigned char *selector_data = (unsigned char*)&selector_pattern[0];

int verbose = 0;
int quiet = 0;

unsigned char commandbuffer[1024];
int commandptr = 0;

int line = 0;
int image_id_start = 0;

char outbuf[1024*1024];
int outlen = 0;

char databuf[1024*1024];
int datalen = 0;

char scratch[64 * 1024];
char stringlit[64 * 1024];
int stringptr;
int lits = 0;

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

ZX7Pack pack;

int roomno = 0;
int totaldata = 0;
char packbuf[8192];
int packbufofs = 0;

void flush_packbuf()
{
    if (packbufofs > trainers)
    {
        pack.mMax = 0;
        pack.pack((unsigned char*)&packbuf[0], packbufofs, trainers);
    
        if (!quiet)
            printf("zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", packbufofs-trainers, pack.mMax, ((pack.mMax)*100.0f)/(packbufofs-trainers), 0x5b00+outlen);
    
        outdata(pack.mPackedData, pack.mMax);
        packbufofs = trainers;
    }
}

void flush_room()
{
    if (datalen > 0)
    {
        putbyte(0); // add a zero byte for good measure.
        if (datalen > 4096)
        {
            printf("Room %s data too large; max 4096 bytes, has %d bytes\n", symbol[roomno].name, datalen);
            exit(-1);
        }
        room[rooms].name = symbol[roomno].name;
        room[rooms].data = new unsigned char[datalen];
        room[rooms].len = datalen;
        room[rooms].used = 0;
        memcpy(room[rooms].data, databuf, datalen);
        rooms++;
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
        if (verbose)printf("  Command buffer '%c' with %d bytes payload (about %d ops)\n", 
            commandbuffer[0], 
            commandptr-1, 
            (commandptr-1) / 3);
            
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

int get_symbol_id(char * s)
{
    int i;
    for (i = 0; i < symbols; i++)
    {
        if (stricmp(symbol[i].name, s) == 0)
        {
            symbol[i].hits++;
            return i;
        }
    }
    symbol[symbols].name = strdup(s);
    symbol[symbols].hits = 1;
    symbols++;
    return symbols-1;            
}

int get_image_id(char * s)
{
    int i;
    for (i = 0; i < images; i++)
    {
        if (stricmp(image[i].name, s) == 0)
        {
            image[i].hits++;
            return i + image_id_start;
        }
    }
    image[images].name = strdup(s);
    image[images].hits = 1;
    images++;
    return images - 1 + image_id_start;            
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
    OP_EXT,
    OP_IATTR,
    OP_DATTR
};

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
        case OP_HAS: if (verbose) printf("HAS(%s)", symbol[value].name); break;
        case OP_NOT: if (verbose) printf("NOT(%s)", symbol[value].name); break;
        case OP_SET: if (verbose) printf("SET(%s)", symbol[value].name); break;
        case OP_CLR: if (verbose) printf("CLR(%s)", symbol[value].name); break;
        case OP_XOR: if (verbose) printf("XOR(%s)", symbol[value].name); break;
        case OP_RND: if (verbose) printf("RND(%d)", value); break;
        case OP_ATTR: if (verbose) printf("ATTR(%d)", value); break;
        case OP_EXT: if (verbose) printf("EXT(%d)", value); break;
        case OP_IATTR: if (verbose) printf("IATTR(%d)", value); break;
        case OP_DATTR: if (verbose) printf("DATTR(%d)", value); break;
    }
    if (verbose) printf("\n");
    store_cmd(opcode, value);
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
    // Op may be of form "foo" "!foo" or "foo:bar"
    if (op[0] == 0)
    {
        printf("Syntax error (op=null), line %d\n", line);
        exit(-1);
    }
    if (op[0] == ':')
    {
        printf("Syntax error (op starting with ':') \"%s\", line %d\n", op, line);
        exit(-1);
    }

    int i = 0;
    int colons = 0;
    while (op[i]) 
    { 
        if (op[i] == ':')
            colons++;
        i++; 
    }

    if (colons > 1)
    {
        printf("Syntax error (op with more than one ':') \"%s\", line %d\n", op, line);
        exit(-1);
    }
    
    if (colons == 0)
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
        while (op[i] != ':') 
        {
            cmd[i] = op[i];
            i++;
        }
        cmd[i] = 0;
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
        if (stricmp(cmd, "sound") == 0) set_eop(atoi(sym)+100, 155); else
        if (stricmp(cmd, "beep") == 0) set_eop(atoi(sym)+100, 155); else
        {
            printf("Syntax error: unknown operation \"%s\", line %d\n", cmd, line);
            exit(-1);
        }                
    }
}

int previous_section = 0;

void parse()
{
    // parse statement
    int first_token = 1;
    int i;
    char t[256];
    switch (scratch[1])
    {
    case 'Q':
        flush_room(); // flush previous room
        token(1, scratch, t);
        i = get_symbol_id(t);
        first_token = 2;      
        store_section('Q', i);  
        if (verbose) printf("Room: \"%s\" (%d)\n", t, i);
        previous_section = 'Q';
        break;
    case 'A':
        token(1, scratch, t);
        i = get_symbol_id(t);
        first_token = 2;      
        store_section('A', i);          
        if (verbose) printf("Choice: %s (%d)\n", t, i);
        previous_section = 'Q';
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
        first_token = 2;   
        store_section('I', get_image_id(t)); // TODO: store images & image id:s     
        if (verbose) printf("Image: \"%s\"\n", t);
        previous_section = 'I';
        break;
    default:
        printf("Syntax error: unknown statement \"%s\", line %d\n", scratch, line);
        exit(-1);            
    }
    
    i = first_token;    
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
    flush_cmd();
    putstring(lit);
    if (verbose) printf("  lit %d: \"%s\"\n", lits++, lit);
}

void process()
{
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
            if (width > 240)
            {
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

struct Token
{
    char * s;
    int hits;
    int hash;

    int used;

    int nexts;
    int next[128];
    int nexthit[128];
    
};

#define MAXTOKENS 1024
Token gToken[MAXTOKENS];
int gTokens = 0;
int gPrevToken = -1;

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
        for (i = 0; i < gToken[prev].nexts; i++)
        {
            if (gToken[prev].next[i] == cur)
            {
                gToken[prev].nexthit[i]++;
                return;
            }
        }
        if (gToken[prev].nexts < 128)
        {
            i = gToken[prev].nexts;
            gToken[prev].next[i] = cur;
            gToken[prev].nexthit[i] = 1;
            gToken[prev].nexts++;
        }
    }
}


void addwordcounttoken(char *aToken)
{    
    int h = calchash(aToken);
    int i;	
    for (i = 0; i < gTokens; i++)
    {
        if (gToken[i].hash == h && strcmp(gToken[i].s, aToken) == 0)
        {
            tokenref(i);
            gToken[i].hits++;
            return;
        }
    }
    if (gTokens < MAXTOKENS)
    {
        tokenref(gTokens);
        gToken[gTokens].s = strdup(aToken);
        gToken[gTokens].hash = h;
        gToken[gTokens].hits = 1;
        gToken[gTokens].used = 0;
        gToken[gTokens].nexts = 0;
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
}

int tokencmp (const void * a, const void * b)
{
    int idx1 = *(int*)a;
    int idx2 = *(int*)b;
    return gToken[idx2].hits - gToken[idx1].hits;
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
            printf("%d. \"%s\"\t(%d)\n", i, gToken[idx[i]].s, gToken[idx[i]].hits);
        }
    }
        
    int c = 0;
    int done = 0;
    i = 0;
    
    while (c < MAX_TRAINER && i < gTokens)
    {
        c += strlen(gToken[idx[i]].s) + 1;
        i++;
    }
    int maxtoken = i;
    c = 0;
    i = 0;
    while (c < MAX_TRAINER && !done)
    {   
        if (gToken[idx[i]].used) i = 0;
        while (gToken[idx[i]].used && i < gTokens) i++;
        if (i >= gTokens) { done = 1; i = 0; }
            
        char * s = gToken[idx[i]].s;
        while (*s && c < MAX_TRAINER)
        {
            trainer[c] = *s;
            s++;
            c++;
        }
        
        if (c < MAX_TRAINER)
        {
            trainer[c] = ' ';
            c++;
        }
        
        gToken[idx[i]].used = 1;
        
        // Try to chain the words together
        int min = -1;
        int mini = 0;
        int j;
        for (j = 0; j < gToken[idx[i]].nexts; j++)
        {
            int n = gToken[idx[i]].next[j];
            int h = gToken[idx[i]].nexthit[j];
            int nextidx = findidx(idx, n);
            if (nextidx < maxtoken && 
                h > min && 
                gToken[n].used == 0)
            {
                min = h;
                mini = nextidx;
            }
        }
		
		if (verbose)
	    {    
			printf("%s", gToken[idx[i]].s);
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
    trainers = c;
    
    if (!quiet)
    {
        printf(
            "Trainer data: %d bytes\n" 
            "|---------1---------2---------3---------4---------5---------6----|\n ",          
            trainers);
        for (c = 0; c < trainers; c++)
        {
            printf("%c", trainer[c]);
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
	
    while (!feof(f))
    {
        readline(scratch, f);
        if (scratch[0] == '$')
        {
            char t[256];
            token(1, scratch, t);
            if (scratch[1] == 'Q')
            {
                int i;
                i = get_symbol_id(t);
                if (symbol[i].hits > 1)
                {
                    printf("syntax error: room id \"%s\" used more than once, line %d\n", t, line);
                    exit(-1);
                }
                symbol[i].hits--; // clear the hit, as it'll be scanned again
            }
            if (scratch[1] == 'I')
            {
                int i;
                i = get_image_id(t);
                image[i].hits--; // clear the hit, as it'll be scanned again
            }
        }
        else
        {
			// string literal
			wordcount(scratch);
        }
    }
    fclose(f);
}


void report()
{
    int i;
    printf("\n");
    printf("Token Hits Symbol\n");
    for (i = 0; i < symbols; i++)
    {
        printf("%5d %4d \"%s\"\n", i, symbol[i].hits, symbol[i].name);
    }

    if (images)
    {
        printf("Token Hits Image\n");
        for (i = 0; i < images; i++)
        {
            printf("%5d %4d \"%s\"\n", i, image[i].hits, image[i].name);
        }
    }
    //      123456789012345678901234567890123456789012345678901234567890
    printf("\n");
    printf("Memory map:\n\n");
    printf("         5         10        15        20        25      29\n");
    printf("---------.---------|---------.---------|---------.--------\n");
    int o = 0;
    for (i = 0; i < pagedata / 512; i++)
        o += printf("P");
    for (i = 0; i < imgdata / 512; i++)
        o += printf("I");
    for (i = o; i < 29*2-(trainers/512); i++)
        o += printf(".");
    for (i = o; i < 29*2; i++)
        o += printf("t");
    printf("\n\n");
    printf("Page data : %5d bytes\n", pagedata);
    printf("Image data: %5d bytes\n", imgdata);
    printf("Free      : %5d bytes\n", DATA_AREA_SIZE - trainers - pagedata - imgdata);
    printf("Trainer   : %5d bytes (used to improve compression by \"training\" it)\n", trainers);
        
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
    for (i = 0; i < images; i++)
    {
        FILE * f = fopen(image[i].name, "rb");
        if (!f)
        {
            printf("Image \"%s\" not found.\n", image[i].name);
            exit(-1);
        }
        fseek(f,0,SEEK_END);
        if (ftell(f) != 6912)
        {
            printf("Image \"&s\" wrong size (has to be 6912 bytes).\n", image[i].name);
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
            printf("Warning: image \"%s\" has %d live character rows, 14 used.\n", image[i].name, maxlive);
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
            printf("Image %s data too large; max 4096 bytes, has %d bytes (%d lines)\n", image[i].name, datalen, maxlive);
            exit(-1);
        }
        pack.pack((unsigned char*)&databuf[0], datalen);
        patchword(0x5b00 + outlen, i + image_id_start);        

        if (!quiet)
            printf("%25s (%02d) zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", image[i].name, maxlive, datalen, pack.mMax, (pack.mMax*100.0f)/datalen, 0x5b00+outlen);
        outdata(pack.mPackedData, pack.mMax);            
    }
}

void output(char *aFilename)
{
    if (outlen > DATA_AREA_SIZE)
    {
        printf("Total output size too large (%d bytes max, %d found)\n", DATA_AREA_SIZE, outlen);
        exit(-1);
    }

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
        strcpy(d+1, "crt0.ihx");
        f = fopen(temp, "rb");
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
    
    int ofs = find_data(codebuf, start, builtin_data, 94*8);
    patch_data(codebuf, ofs, propfont_data, 94*8);
    
    ofs = find_data(codebuf, start, builtin_width, 94);
    patch_data(codebuf, ofs, propfont_width, 94);
    
    ofs = find_data(codebuf, start, divider_pattern, 8);
    patch_data(codebuf, ofs, divider_data, 8);

    ofs = find_data(codebuf, start, selector_pattern, 8);
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
    int freebytes = DATA_AREA_SIZE - trainers - outlen;
    for (i = 0; i < freebytes; i++)
        outbyte(0);
    outdata(trainer, trainers);
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
            if (room[j].used == 0)
            {
                if (room[j].len+packbufofs-trainers < 4096)
                {
                    memcpy(packbuf+packbufofs, room[j].data, room[j].len);
                    pack.mMax = 0;
                    pack.pack((unsigned char*)&packbuf[0], packbufofs + room[j].len, trainers);
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
            memcpy(packbuf+packbufofs, room[minidx].data, room[minidx].len);
            packbufofs += room[minidx].len;
            room[minidx].used = 1;
            patchword(0x5b00 + outlen, minidx);
            printf("%s ", room[minidx].name);
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


int biggest_unused_room()
{
    int i;
    int bigsize = 0;
    int bigidx = -1;
    for (i = 0; i < rooms; i++)
    {
        if (room[i].used == 0)
        {
            if (room[i].len > bigsize)
            {
                bigsize = room[i].len;
                bigidx = i;
            }
        }
    }    
    return bigidx;
}

float *compression_results;

int best_compressible_room()
{
    int i, j;
    int idx = -1;
    float ratio = 1;
    for (i = 0; i < rooms; i++)
    {
        if (room[i].used == 0)
        {
            for (j = i+1; j < rooms; j++)
            {
                if (room[j].used == 0)
                {
                    float r;
                    if (compression_results[i*rooms+j] == 0)
                    {
                        int total = room[i].len + room[j].len;
                        char tempbuf[8192];
                        memcpy(tempbuf, room[i].data, room[i].len);
                        memcpy(tempbuf + room[i].len, room[j].data, room[j].len);
                        pack.mMax = 0;
                        pack.pack((unsigned char*)&tempbuf[0], total);
                        //printf("ZX7: %4d bytes - %3.3f%%", pack.mMax, ((float)pack.mMax/total)*100);
                        r = ((float)pack.mMax / total);
                        compression_results[i*rooms+j] = r;
                    }
                    else
                    {
                        r = compression_results[i*rooms+j];
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
    
    compression_results = new float[rooms*rooms];
    for (j = 0; j < rooms*rooms; j++)
        compression_results[j] = 0;
    
    //int minidx = biggest_unused_room();    
    int minidx = best_compressible_room();
    idx = minidx;
    room[minidx].used = 1;
    patchword(0x5b00 + outlen, minidx);
    memcpy(packbuf+packbufofs, room[minidx].data, room[minidx].len);
    packbufofs += room[minidx].len;
    printf("%s ", room[minidx].name);
    totaldata = room[minidx].len;
    do 
    {
        minidx = -1;
        float minvalue = 1;
        for (j = 0; j < rooms; j++)
        {
            if (idx != j && !room[j].used)
            {
                int total = room[idx].len+room[j].len;
                if (total > 4096)
                {
                    //printf("(too big)");
                }
                else
                {
                    float r;
                    if (compression_results[idx*rooms+j] == 0)
                    {
                        char tempbuf[8192];
                        memcpy(tempbuf, room[idx].data, room[idx].len);
                        memcpy(tempbuf + room[idx].len, room[j].data, room[j].len);
                        pack.mMax = 0;
                        pack.pack((unsigned char*)&tempbuf[0], total);
                        //printf("ZX7: %4d bytes - %3.3f%%", pack.mMax, ((float)pack.mMax/total)*100);
                        r = ((float)pack.mMax / total);
                        compression_results[idx*rooms+j] = r;
                    }
                    else
                    {
                        r = compression_results[idx*rooms+j];
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
            printf("%s ", room[minidx].name);
            room[minidx].used = 1;

            totaldata += room[minidx].len;
            if (totaldata > 4096)
            {
                flush_packbuf();
                //minidx = biggest_unused_room();
                minidx = best_compressible_room();
                totaldata = room[minidx].len;
            }
            idx = minidx;
            
            memcpy(packbuf+packbufofs, room[minidx].data, room[minidx].len);
            packbufofs += room[minidx].len;
             
            patchword(0x5b00 + outlen, minidx); // N rooms will have same offset
        }
    } while (minidx != -1);
    flush_packbuf();
    for (j = 0; j < rooms; j++)
    {
        if (room[j].used == 0)
        {
            printf("ERROR Room %s didn't compress with anything\n", room[j].name);
        }
    }

    printf("\n");
    delete[] compression_results;
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
    memcpy(packbuf, trainer, trainers);
    packbufofs = trainers;
    datalen = 0;
    image_id_start = symbols + 1;
    outlen = symbols * 2 + images * 2 + 2;
    line = 0;    
    scan(pars[infile]);
    process_rooms();
    pagedata = outlen;
    process_images();
    imgdata = outlen - pagedata;
    output_trainer();
    if (!quiet)
        report();
    output(pars[outfile]);

    
    return 0;
}

