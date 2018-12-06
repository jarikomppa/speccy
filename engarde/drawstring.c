#include "main.h"

void drawstringz(unsigned char *aS, unsigned char aX, unsigned char aY)
{
    const unsigned char *datap = (unsigned char*)(propfont + 94 - 32); // font starts from space (32)
    const unsigned char *widthp = (unsigned char*)(propfont - 32);
    const unsigned char *shiftp = (unsigned char*)(propfont + 846);
    unsigned char *bd = (unsigned char*)yofs[aY * 8] + aX;
    unsigned char i;
    for (i = 0; i < 8; i++)
    {
        unsigned char *d = bd;
        unsigned char *s = aS;        
        unsigned char pixofs = 0;

		// first char special case
		
		{
            unsigned char ch = *s;
            unsigned char w = widthp[ch];
            unsigned char g = datap[ch];
			if (g)
			{
				unsigned short si = (unsigned short)g * 2 + pixofs * 2;
				*d |= shiftp[si];
				pixofs += w;
			}
			else
			{
				pixofs += w;
			}
            s++;                    
		}
		
        while (*s)
        {
            unsigned char ch = *s;
            unsigned char w = widthp[ch];
            unsigned char g = datap[ch];
			if (g)
			{
				unsigned short si = (unsigned short)g * 2 + pixofs * 2;
				*d |= shiftp[si];
				pixofs += w;
				if (pixofs > 7)
				{
					d++; 
					*d |= shiftp[si+1];
					pixofs &= 7;
				}
			}
			else
			{
				pixofs += w;
				if (pixofs > 7)
				{
					pixofs &= 7;
					d++;
				}
			}
            s++;                    
        }
        datap += 94;
        bd += 0x0100;
    }
}
