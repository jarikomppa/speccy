/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class Tapper
{
    public:
    unsigned char data[65536];
    int ofs;
    Tapper()
    {
        ofs = 0;
    }

    void putdata(unsigned char c)
    {
        data[ofs] = c;
        ofs++;
    }
    
    void putdatastr(const char *s)
    {
        while (*s)
        {
            putdata(*s);
            s++;
        }
    }
    
    void putdataint(int n)
    {
        putdata((unsigned char)(n));
        putdata((unsigned char)(n >> 8));
    }
    
    void putdataintlit(int n)
    {
        //0x33, 0x32, 0x37, 0x36, 0x37, 0x0e, 0x00, 0x00, 0xff, 0x7f, 0x00,
        char temp[6];
        sprintf(temp, "%d", n & 0xffff);
        putdatastr(temp);
        putdata(0x0e);
        putdata(0x00);
        putdata(0x00);
        putdataint(n);
        putdata(0x00);
    }
    
    void write(FILE * f)
    {
        int checksum = 0;
        int i;
        for (i = 0; i < ofs; i++)
            checksum^=data[i];
        int len = ofs + 1;
        putc((unsigned char)(len),f);
        putc((unsigned char)(len >> 8),f);
        for (i = 0; i < ofs; i++)
            putc(data[i], f);
        putc((unsigned char)checksum, f);
    }
};

Tapper header, payload;

void gen_basic()
{
    /*
    10 CLEAR 32767 : RANDOMIZE USR 23759 : POKE 23739,111 : LOAD ""CODE : RANDOMIZE USR 32768    
    */
    payload.putdata(0x00);
    payload.putdata(0x0a); // line number 10
    payload.putdata(69); // bytes on line (69)
    payload.putdata(0x00); // 0?
    payload.putdata(0xfd); // CLEAR
    payload.putdataintlit(32767);
    payload.putdata(':');
    payload.putdata(0xf9); // RANDOMIZE
    payload.putdata(0xc0); // USR
    payload.putdataintlit(23759+69); // TODO: put correct value - 23769 based on 10 chars of basic, so 23759+basic size
    payload.putdata(':');
    payload.putdata(0xf4); // POKE
    payload.putdataintlit(23739);
    payload.putdata(',');
    payload.putdataintlit(111);
    payload.putdata(':');
    payload.putdata(0xef); // LOAD
    payload.putdata('"');
    payload.putdata('"');
    payload.putdata(0xaf); // CODE
    payload.putdata(':');
    payload.putdata(0xf9); // RANDOMIZE
    payload.putdata(0xc0); // USR
    payload.putdataintlit(32768);
    payload.putdata(0x0d); // enter

/*
CLEAR 32767    
0xfd, 0x33, 0x32, 0x37, 0x36, 0x37, 0x0e, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x0d, 
RANDOMIZE USR 23759
0xf9, 0xc0, 0x32, 0x33, 0x37, 0x35, 0x39, 0x0e, 0x00, 0x00, 0xcf, 0x5c, 0x00, 0x0d, 
POKE 23739,111
0xf4, 0x32, 0x33, 0x37, 0x33, 0x39, 0x0e, 0x00, 0x00, 0xbb, 0x5c, 0x00, 0x2c, 0x31, 0x31, 0x31, 0x0e, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x0d, 
LOAD""CODE
0xef, 0x22, 0x22, 0xaf, 0x0d, 
RANDOMIZE USR 32768
0xf9, 0xc0, 0x33, 0x32, 0x37, 0x36, 0x38, 0x0e, 0x00, 0x00, 0x00, 0x80, 0x00, 0x0d, 
*/
    
    
/*    
    payload.putdata(0x00);
    payload.putdata(0x0a); // line number 10
    payload.putdata(0x0b); // bytes on line (11)
    payload.putdata(0x00); // 0?
    payload.putdata(0xf9); // RANDOMIZE
    payload.putdata(0xc0); // USR
    payload.putdata(0xb0); // VAL
    payload.putdatastr("\"23806\""); // TODO: put correct value - 23769 based on 10 chars of basic, so 23759+basic size
    payload.putdata(0x0d);
    
    
    payload.putdata(0x00);
    payload.putdata(0x14); // line number 20
    payload.putdata(0x1a); // bytes on line
    payload.putdata(0x00); // 0?
    payload.putdata(0xfd); // CLEAR
    payload.putdata(0xb0); // VAL
    payload.putdatastr("\"32767\":"); // TODO: put correct value (note: -1);
    payload.putdata(0xef); // LOAD
    payload.putdata('"');
    payload.putdata('"');
    payload.putdata(0xaf); // CODE
    payload.putdata(':');
    payload.putdata(0xf9); // RANDOMIZE
    payload.putdata(0xc0); // USR
    payload.putdata(0xb0); // VAL
    payload.putdatastr("\"32768\""); // TODO: put correct value
    payload.putdata(0x0d);
    

    payload.putdata(0x00);
    payload.putdata(0x1e); // line number 30
    payload.putdata(0x02); // bytes on line (2)
    payload.putdata(0x00); // 0?
    payload.putdata(0xea); // REM //
    payload.putdata(0x0d);
*/
}

void append_boot()
{
    FILE * f = fopen("boot.bin", "rb");
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    fseek(f, 0, SEEK_SET);
    while (len)
    {
        payload.putdata(getc(f));
        len--;
    }
    fclose(f);
}

void append_pic()
{
    FILE * f = fopen("loading.scr.lzf", "rb");
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    fseek(f, 0, SEEK_SET);
    while (len)
    {
        payload.putdata(getc(f));
        len--;
    }
    fclose(f);
}


void save_tap()
{
    FILE * f = fopen("out.tap", "wb");
    header.putdata((unsigned char)0);
    header.putdatastr("test      "); // 10 chars exact
    header.putdataint(payload.ofs-1);
    header.putdataint(10); // autorun row
    header.putdataint(payload.ofs-1);
    
    header.write(f);
    payload.write(f);
    fclose(f);
}

int main(int parc, char **pars)
{   
    header.putdata((unsigned char)0x00);
    payload.putdata((unsigned char)0xff);
    gen_basic();
    append_boot();
    append_pic();
    save_tap();
    return 0;
}