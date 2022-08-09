/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

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

    void putdataintlit_min(int n)
    {
        //0x33, 0x32, 0x37, 0x36, 0x37, 0x0e, 0x00, 0x00, 0xff, 0x7f, 0x00,
        putdata('0');
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