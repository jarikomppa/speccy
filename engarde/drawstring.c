#include "main.h"

void drawstringz(unsigned char *aS, unsigned char aX, unsigned char aY)
{
    unsigned char i, *s, *d, sx;
    const unsigned char *datap = (unsigned char*)propfont_data - 32 * 8; // font starts from space (32)
    const unsigned char *widthp = (unsigned char*)propfont_width - 32;
    aY *= 8;
    for (i = 0; i < 8; i++)
    {
		unsigned char ch = *aS;
        s = aS;
        sx = 0;
        d = (unsigned char*)yofs[aY] + aX;
        while (ch)
        {
			unsigned char data = datap[ch * 8];
			unsigned char width = widthp[ch];
			if (data)
			{
				*d |= data >> sx;
			}
			
			sx += width;
			if (sx > 8)
			{
				d++;
				sx -= 8;
				if (data)
				{
					*d = data << (width - sx);
				}
			}

            s++;
			ch = *s;
        }
        aY++;
        datap++;
    }
}


/*
// 282 bytes as of c7f5295
void drawstring(unsigned char *aS, unsigned char aX, unsigned char aY)
{
    unsigned char i, *s, *d, sx, c;
    unsigned char *datap = (unsigned char*)(int*)propfont_data - 32 * 8;
    unsigned char *widthp = (unsigned char*)(int*)propfont_width - 32;
    aY *= 8;
    for (i = 0; i < 8; i++)
    {
        s = aS + 1;
        c = *aS;
        sx = 0;
        d = (unsigned char*)yofs[aY] + aX;
        while (c)
        {
            unsigned char ch = *s;
            if (ch != 128)
            {
                unsigned char data = datap[ch * 8];
                unsigned char width = widthp[ch];
                if (data)
                {
                    *d |= data >> sx;
                }
                
                sx += width;
                if (sx > 8)
                {
                    d++;
                    sx -= 8;
                    if (data)
                    {
                        *d = data << (width - sx);
                    }
                }
            }
            s++;
            c--;
        }
        aY++;
        datap++;
    }
}
*/

/*
// 410 bytes (as of c7f5295)
void drawstringz(unsigned char *aS, unsigned char x, unsigned char y)
{
    unsigned char i;
    unsigned char c;
    char sx = 0;
    unsigned char *yp[8];
    unsigned char *datap = (unsigned char*)(int*)propfont_data - 32 * 8;
    unsigned char *widthp = (unsigned char*)(int*)propfont_width - 32;
    unsigned char *s = aS;
    unsigned char len = 0;
    while (aS[len]) len++;
    
    for (i = 0; i < 8; i++)
    {
        yp[i] = (unsigned char*)yofs[y + i] + x;
    }
    c = len;    
    while (c)
    {
        unsigned char ch = *s;
        unsigned char wd = widthp[ch];
        unsigned char *chp = datap + ch * 8;
        if (*s != 32)
        {
            for (i = 0; i < 8; i++, chp++)
            {
                unsigned char data = *chp;
                if (data)
                {
                    *(yp[i]) |= data >> sx;                        
                }
            }
        }
        else
        {
            chp += 8;
        }
        
        sx += wd;
        if (sx > 8)
        {
            sx -= 8;
            chp -= 8;            
            for (i = 0; i < 8; i++, chp++)
            {
                unsigned char data = *chp;
                yp[i]++;
                if (data)
                {
                    *(yp[i]) = data << (wd - sx);
                }
            }            
        }
        s++;
        c--;
    }
}
*/