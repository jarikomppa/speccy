/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

class Pack
{
public:
    int mMax;
    unsigned char mPackedData[65536];
    
    Pack()
    {
        mMax = 0;
    }
    
    void putc(unsigned char d)
    {
        mPackedData[mMax] = d;
        mMax++;
    }

    virtual void pack(unsigned char *aData, int aLen) = 0;
};
