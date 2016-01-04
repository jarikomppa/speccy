/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

/*

    000LLLLL <L+1>             - literal reference
    LLLddddd dddddddd          - copy L+2 from d bytes before most recently decoded byte
    111ddddd dddddddd LLLLLLLL - copy L+2+7 from d bytes before most recently decoded byte
*/

class LZFPack : public Pack
{
public:

    void dump_literals(unsigned char * data, int &literals, int &literalsofs, int ofs)
    {
        while (literals) 
        {
            int cc = literals;
            if (cc > 32) cc = 32;
            putc(cc-1);
            int j;
            for (j = 0; j < cc; j++)
            {
                putc(data[literalsofs]);
                literalsofs++;                        
            }
            literals -= cc;
        }
    }    
    
    virtual void pack(unsigned char *aData, int aLen)
    {
        int i;
        int literals = 0;
        int literalsofs = 0;
        
        putc(0); // placeholder for size
        putc(0); 
        
        for (i = 0; i < aLen; i++)
        {
            int bestofs = 0;
            int bestchars = 0;
            if (i)
            {
                int ofs = i - 8192;
                if (ofs < 0) ofs = 0;
                int j;
                for (j = ofs; j < i; j++)
                {
                    if (aData[j] == aData[i])
                    {
                        int k = 1;
                        while (aData[j+k] == aData[i+k] && k < (255 + 7 + 2)) k++;
                        if (k > bestchars)
                        {
                            bestchars = k;
                            bestofs = j;
                        }
                    }
                } 
            }
            
            if (bestchars < 3)
            {
                literals++;
            }
            else
            {
                if (literals)
                {
                    // every 32 literals takes one extra byte to encode
                    dump_literals(aData, literals, literalsofs, i);
                }
    
                int ofs = -(bestofs - i) - 1;
                if (bestchars >= 7+2)
                {
                    putc((7 << 5) | ((ofs >> 8) & 31));
                    putc(bestchars - (7 + 2));
                    putc(ofs & 0xff);
                }
                else
                {
                    putc(((bestchars-2) << 5) | ((ofs >> 8) & 31));
                    putc(ofs & 0xff);
                }
                
                i += bestchars - 1;
                
                literalsofs = i + 1;
            }
        }
        
        if (literals)
        {
             dump_literals(aData, literals, literalsofs, i);
        }   
        
        mPackedData[0] = (((unsigned int)mMax-2) >> 0) & 0xff;
        mPackedData[1] = (((unsigned int)mMax-2) >> 8) & 0xff;
    }
};
