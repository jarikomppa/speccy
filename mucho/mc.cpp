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

int pagedata = 0;
int imgdata = 0;
int trainers = 0;
#define MAX_TRAINER 1024
unsigned char trainer[MAX_TRAINER];

#define MAX_SYMBOLS (8*256) // 2k symbols should be enough for everybody
int symbols = 0;
Symbol symbol[MAX_SYMBOLS];
int images = 0;
Symbol image[MAX_SYMBOLS];

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

void flush_room()
{
    if (datalen > trainers)
    {
        putbyte(0); // add a zero byte for good measure.
        pack.mMax = 0;
        if (datalen > 4096+trainers)
        {
            printf("Room %s data too large; max 4096 bytes, has %d bytes\n", symbol[roomno].name, datalen-trainers);
            exit(-1);
        }
        pack.pack((unsigned char*)&databuf[0], datalen, trainers);
        patchword(0x5b00 + outlen, roomno);
        
        if (!quiet)
            printf("%30s zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", symbol[roomno].name, datalen-trainers, pack.mMax, ((pack.mMax)*100.0f)/(datalen-trainers), 0x5b00+outlen);
        outdata(pack.mPackedData, pack.mMax);
        datalen = trainers;       
        memcpy(databuf, trainer, trainers);
        roomno++;
    }        
}



void flush_sect()
{
    if (datalen > trainers)  // skip end if we're in the beginning
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

void scan_first_pass(char *aFilename)
{
    FILE * f = fopen(aFilename, "rb");
    if (!f)
    {
        printf("File \"%s\" not found.\n", aFilename);
        exit(-1);
    }
	
	int pagestringlen = 0;

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
				pagestringlen = 0;
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
			if (pagestringlen < 2048)
			{
				// string literal
				wordcount(scratch);
				pagestringlen += strlen(scratch);
			}
        }
    }
    fclose(f);
}


void report()
{
    int i;
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
    printf("Memory map:\n");
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
    printf("Free      : %5d bytes\n", 29952 - trainers - pagedata - imgdata);
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
    if (outlen > 29952)
    {
        printf("Total output size too large (29952 bytes max, %d found)\n", outlen);
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
    int freebytes = 29952 - trainers - outlen;
    for (i = 0; i < freebytes; i++)
        outbyte(0);
    outdata(trainer, trainers);
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
    memcpy(databuf, trainer, trainers);
    datalen = trainers;
    image_id_start = symbols + 1;
    outlen = symbols * 2 + images * 2 + 2;
    line = 0;    
    scan(pars[infile]);
    pagedata = outlen;    
    process_images();
    imgdata = outlen - pagedata;
    output_trainer();
    if (!quiet)
        report();
    output(pars[outfile]);

    
    return 0;
}


/*                                                                                                                                                                                                          
>           start:  614 ->  430 (70.033%),  615 ->  381 (61.951%)
>  Foraminifera_9: 1416 ->  921 (65.042%), 1417 ->  789 (55.681%)
>Foraminifera_9_2: 2069 -> 1227 (59.304%), 2070 -> 1087 (52.512%)
>Foraminifera_9_3: 1645 -> 1059 (64.377%), 1646 ->  949 (57.655%)
>Foraminifera_9_4: 1544 ->  992 (64.249%), 1545 ->  844 (54.628%)
>Foraminifera_9_5: 1834 -> 1156 (63.032%), 1835 ->  990 (53.951%)
>Foraminifera_9_6: 1319 ->  864 (65.504%), 1320 ->  743 (56.288%)
>Foraminifera_9_7: 1692 -> 1065 (62.943%), 1693 ->  916 (54.105%)
>Foraminifera_9_8: 2161 -> 1301 (60.204%), 2162 -> 1132 (52.359%)
>     Shield_8805: 1732 -> 1078 (62.240%), 1733 ->  931 (53.722%)
>   Shield_8805_2: 1076 ->  729 (67.751%), 1077 ->  613 (56.917%)
>   Shield_8805_3: 1359 ->  892 (65.636%), 1360 ->  756 (55.588%)
>   Shield_8805_4: 1560 ->  986 (63.205%), 1561 ->  851 (54.516%)
>   Shield_8805_5: 1847 -> 1133 (61.343%), 1848 -> 1005 (54.383%)
>   Shield_8805_6: 1621 -> 1031 (63.603%), 1622 ->  900 (55.487%)
>             Cow: 1228 ->  804 (65.472%), 1229 ->  672 (54.679%)
>           Cow_2: 1983 -> 1152 (58.094%), 1984 -> 1011 (50.958%)
>           Cow_3: 1841 -> 1092 (59.316%), 1842 ->  954 (51.792%)
>  Sandy_Van_Pelt: 1383 ->  913 (66.016%), 1384 ->  791 (57.153%)
>Sandy_Van_Pelt_2: 1635 -> 1013 (61.957%), 1636 ->  881 (53.851%)
>Sandy_Van_Pelt_3: 1122 ->  764 (68.093%), 1123 ->  637 (56.723%)
>Sandy_Van_Pelt_4: 1840 -> 1124 (61.087%), 1841 ->  986 (53.558%)
>Sandy_Van_Pelt_5: 1909 -> 1199 (62.808%), 1910 -> 1041 (54.503%)
>Sandy_Van_Pelt_6: 1178 ->  762 (64.686%), 1179 ->  654 (55.471%)
>Sandy_Van_Pelt_7:  857 ->  553 (64.527%),  858 ->  464 (54.079%)
>     Priam's_Maw: 2002 -> 1283 (64.086%), 2003 -> 1153 (57.564%)
>   Priam's_Maw_2: 2249 -> 1349 (59.982%), 2250 -> 1190 (52.889%)
>   Priam's_Maw_3: 1312 ->  857 (65.320%), 1313 ->  720 (54.836%)
>   Priam's_Maw_4: 1495 ->  937 (62.676%), 1496 ->  807 (53.944%)
*/