#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "propfont.h"
#include "../common/pack.h"
#include "../common/zx7pack.h"
#include "yofstab.h"

struct Symbol
{
    char * name;
    int hits;
};

#define MAX_SYMBOLS (8*256) // 2k symbols should be enough for everybody
int symbols = 0;
Symbol symbol[MAX_SYMBOLS];
int images = 0;
Symbol image[MAX_SYMBOLS];

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

void outdata(unsigned char *d, int len)
{
    while (len)
    {
        outbyte(*d);
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
    if (datalen)
    {
        putbyte(0); // add a zero byte for good measure.
        pack.mMax = 0;
        if (datalen > 4096)
        {
            printf("Room %s data too large; max 4096 bytes, has %d bytes\n", symbol[roomno].name, datalen);
            exit(-1);
        }
        pack.pack((unsigned char*)&databuf[0], datalen);
        patchword(0x5b00 + outlen, roomno);
        

        if (!quiet)
            printf("%30s zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", symbol[roomno].name, datalen, pack.mMax, (pack.mMax*100.0f)/datalen, 0x5b00+outlen);
        outdata(pack.mPackedData, pack.mMax);
        datalen = 0;       
        roomno++;
    }        
}

void flush_sect()
{
    if (verbose) printf("End of section\n");
    if (datalen)  // skip end if we're in the beginning
    {
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
        if (verbose) printf("Choise: %s (%d)\n", t, i);
        previous_section = 'Q';
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
        temp[2] = 0;
        c = 2;
        width = propfont_width[0] * 2;
        while (*s)
        {
            temp[c] = *s;
            c++;
            width += propfont_width[*s-32];
            if (width > 256)
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

void scan_rooms(char *aFilename)
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

    printf("Token Hits Image\n");
    for (i = 0; i < images; i++)
    {
        printf("%5d %4d \"%s\"\n", i, image[i].hits, image[i].name);
    }
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
        if (maxlive > 14)
        {
            printf("Warning: image \"%s\" has %d live character rows, 14 used.\n", image[i].name, maxlive);
            maxlive = 14;
        }
        maxlive++;
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
    FILE * f = fopen(aFilename, "wb");
    if (!f)
    {
        printf("Can't open \"%s\" for writing.\n", aFilename);
        exit(-1);
    }

    fwrite(outbuf, 1, outlen, f);
    fclose(f);
}

int main(int parc, char **pars)
{    
    printf("MuCho compiler, by Jari Komppa http://iki.fi/sol/\n");
    if (parc < 3)
    {
        printf(
            "%s <input> <output> [flags]\n"
            "Optional flags:\n"
            "  -v   verbose (useful for debugging)\n"
            "  -q   quiet (minimal output)\n",
            pars[0]);
        return -1;
    }
    int infile = -1;
    int outfile = -1;
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
                    printf("Invalid parameter \"%s\" (input and output files already defined)\n", pars[i]);
                    exit(-1);
                }
            }
        }
    }
    
    if (outfile == -1)
    {
        printf("Invalid parameters (run without params for help\n");
        exit(-1);
    }

    line = 0;    
    scan_rooms(pars[infile]);
    image_id_start = symbols + 1;
    outlen = symbols * 2 + images * 2 + 2;
    line = 0;    
    scan(pars[infile]);
    process_images();
    if (!quiet)
        report();
    output(pars[outfile]);

    
    return 0;
}
