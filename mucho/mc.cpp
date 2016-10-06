#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "propfont.h"


struct Symbol
{
    char * name;
    int hits;
};

#define MAX_SYMBOLS (8*256) // 2k symbols should be enough for everybody
int symbols = 0;
Symbol symbol[MAX_SYMBOLS];

unsigned char commandbuffer[1024];
int commandptr = 0;

char databuf[64*1024];
int datalen = 0;

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
    }
    while (!feof(f) && buf[0] == '#' && buf[0] > 31);
}

char scratch[4096];
char stringlit[4096];
int stringptr;
int lits = 0;

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


void flush_sect()
{
    printf("End of section\n");
    if (datalen)  // skip end if we're in the beginning
    {
        putbyte(0);
    }
}

void flush_cmd()
{
    if (commandptr)
    {
        printf("  Command buffer '%c' with %d bytes payload (about %d ops)\n", 
            commandbuffer[0], 
            commandptr-1, 
            (commandptr-1) / 3);
            
        if (commandptr > 255)
        {
            printf("Syntax error - too many operations on one statement\n");
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

void set_op(int opcode, int value)
{
    printf("    Opcode: ");
    switch(opcode)
    {
        case OP_HAS: printf("HAS(%s)", symbol[value].name); break;
        case OP_NOT: printf("NOT(%s)", symbol[value].name); break;
        case OP_SET: printf("SET(%s)", symbol[value].name); break;
        case OP_CLR: printf("CLR(%s)", symbol[value].name); break;
        case OP_XOR: printf("XOR(%s)", symbol[value].name); break;
        case OP_RND: printf("RND(%d)", value); break;
        case OP_ATTR: printf("ATTR(%d)", value); break;
        case OP_EXT: printf("EXT(%d)", value); break;
    }
    printf("\n");
    store_cmd(opcode, value);
}

void parse_op(char *op)
{
    // Op may be of form "foo" "!foo" or "foo:bar"
    if (op[0] == 0)
    {
        printf("Syntax error (op=null)\n");
        exit(-1);
    }
    if (op[0] == ':')
    {
        printf("Syntax error (op starting with ':') \"%s\"\n", op);
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
        printf("Syntax error (op with more than one ':') \"%s\"\n", op);
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
        if (stricmp(cmd, "attrib") == 0) set_op(OP_ATTR, atoi(sym)); else
        if (stricmp(cmd, "color") == 0) set_op(OP_ATTR, atoi(sym)); else
        if (stricmp(cmd, "ext") == 0) set_op(OP_EXT, atoi(sym)); else
        {
            printf("Syntax error: unknown operation \"%s\"\n", cmd);
            exit(-1);
        }                
    }
}

void parse()
{
    // parse statement
    int first_token = 1;
    int i;
    char t[256];
    switch (scratch[1])
    {
    case 'Q':
        token(1, scratch, t);
        i = get_symbol_id(t);
        first_token = 2;      
        store_section('Q', i);  
        printf("Room: \"%s\" (%d)\n", t, i);
        break;
    case 'A':
        token(1, scratch, t);
        i = get_symbol_id(t);
        first_token = 2;      
        store_section('A', i);          
        printf("Choise: %s (%d)\n", t, i);
        break;
    case 'O':
        store_section('O');
        printf("Predicated section\n");
        break;
    case 'I':
        token(1, scratch, t);
        first_token = 2;   
        store_section('I', 0); // TODO: store images & image id:s     
        printf("Image: \"%s\"\n", t);
        break;
    default:
        printf("Syntax error: unknown statement \"%s\"\n", scratch);
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
    printf("  lit %d: \"%s\"\n", lits++, lit);
}

void process()
{
    if (stringptr != 0)
    {
        char temp[256];
        temp[0] = 0;
        int c = 0;
        int width = 0;
        char *s = stringlit;
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


int main(int parc, char **pars)
{    
    if (parc < 3)
    {
        printf("%s <input> <output>\n", pars[0]);
        return -1;
    }
    FILE * f = fopen(pars[1], "rb");
    
    
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
    fclose(f);
    
    int i;
    printf("Token Hits Symbol\n");
    for (i = 0; i < symbols; i++)
    {
        printf("%5d %4d \"%s\"\n", i, symbol[i].hits, symbol[i].name);
    }

    f = fopen(pars[2], "wb");
    fwrite(databuf, 1, datalen, f);
    fclose(f);
    
    return 0;
}
