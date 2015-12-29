/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../loadscrn/boot.bin.h"

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
int picfile_len = 0;
char *picfile_name = 0;
int execaddr = 0;
char progname[11];

void gen_basic()
{
/*
10 CLEAR 32767 : RANDOMIZE USR 23759 : POKE 23739,111 : LOAD ""CODE : RANDOMIZE USR 32768    

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
    payload.putdata(0x00);
    payload.putdata(0x0a); // line number 10
    payload.putdata(69); // bytes on line (69)
    payload.putdata(0x00); // 0?
    payload.putdata(0xfd); // CLEAR
    payload.putdataintlit(execaddr-1);
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
    payload.putdataintlit(execaddr);
    payload.putdata(0x0d); // enter
}

void append_boot()
{
    boot_bin[6] = (picfile_len >> 0) & 0xff;
    boot_bin[7] = (picfile_len >> 8) & 0xff;
    int i;
    for (i = 0; i < boot_bin_len; i++)
        payload.putdata(boot_bin[i]);
}

void append_pic()
{    
    FILE * f = fopen(picfile_name, "rb");
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


void save_tap(char *aFilename)
{
    FILE * f = fopen(aFilename, "wb");
    if (!f)
    {
        printf("Can't open \"%s\" for writing.\n", aFilename);
        return;
    }
    header.putdata((unsigned char)0);
    header.putdatastr(progname); // 10 chars exact
    header.putdataint(payload.ofs-1);
    header.putdataint(10); // autorun row
    header.putdataint(payload.ofs-1);
    
    header.write(f);
    payload.write(f);
    fclose(f);
    printf("\"%s\" written.\n", aFilename);
}

int main(int parc, char **pars)
{   
    if (parc < 5)
    {
        printf(
        "Usage:\n"
        "\n"
        "%s PROGNAME EXECADDR PACKEDPICNAME OUTFILENAME\n"
        "\n"
        "where:\n"
        "PROGNAME must be up to 10 char long name (\"Loading: 1234567890\")\n"
        "EXECADDR must be the address to RANDOMIZE USR after load\n"
        "PACKEDPICNAME must be filename for a .scr that's been lzf-compressed\n"
        "OUTFILENAME must be the name of the generated .tap file, overwritten without asking.\n"
        "\n", pars[0]);
        return 0;
    }
    int i;
    for (i = 0; i < 10; i++)
        progname[i] = ' ';
    progname[10] = 0;
    for (i = 0; i < 10 && pars[1][i]; i++)
        progname[i] = pars[1][i];
    printf("Progname set to \"%s\"\n", progname);
    execaddr = strtol(pars[2], 0, 10);
    printf("Exec address set to %d (0x%x)\n", execaddr, execaddr);
    picfile_name = pars[3];
    FILE * f = fopen(picfile_name, "rb");
    if (!f)
    {
        printf("Compressed picture file \"%s\" not found\n", picfile_name);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    picfile_len = ftell(f);
    fclose(f);
    printf("Compressed picture file \"%s\" of length %d found.\n", picfile_name, picfile_len);
    if (picfile_len == 6912)
    {
        printf("* WARNING * Picture file suspiciously *exactly* as big as\n"
               "            uncompressed ones, did you LZF compress it?\n");
    }
    header.putdata((unsigned char)0x00);
    payload.putdata((unsigned char)0xff);
    gen_basic();
    append_boot();
    append_pic();
    save_tap(pars[4]);
    return 0;
}