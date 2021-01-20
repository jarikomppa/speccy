/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

/* Make Visual Studio not consider fopen() deprecated */
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DEBUGPRINT

/*

    000LLLLL <L+1>             - literal reference
    LLLddddd dddddddd          - copy L+2 from d bytes before most recently decoded byte
    111ddddd dddddddd LLLLLLLL - copy L+2+7 from d bytes before most recently decoded byte
*/
int main(int parc, char ** pars)
{
    if (parc < 3)
    {
        printf("Usage: %s infile outfile\n", pars[0]);
        exit(0);
    }
    FILE * f = fopen(pars[1], "rb");
    if (!f)
    {
        printf("\"%s\" not found\n", pars[1]);
        exit(0);
    }
    fseek(f,0,SEEK_END);
    int filelen = ftell(f);
    fseek(f,0,SEEK_SET);
    unsigned short len;
    fread(&len, 1, 2, f);    
    if (len != filelen - 2)
    {
        printf("Warning: File length differs from header (file: %d, header: %d)\n", filelen, len);
    }
    printf("%s - %d bytes\n", pars[1], len);
    unsigned char * data = new unsigned char[len];
    fread(data, len, 1, f);
    fclose(f);

    f = fopen(pars[2], "wb");
    if (!f)
    {
        printf("Can't open \"%s\"\n", pars[2]);
        exit(0);
    }
    char *outbuf = new char[1024*1024];
    int outidx = 0;
    int idx = 0;
    while (idx < len)
    {
        unsigned char op = data[idx];
        int len = op >> 5;
#ifdef DEBUGPRINT
        printf("(%d) ", len);
#endif        
        idx++;
        if (len == 0)
        {
            // literals
            len = (op & 31) + 1;
#ifdef DEBUGPRINT
            printf("Literal %d chars, ofs %d \"", len, outidx);
#endif            
            while (len)
            {
                outbuf[outidx] = data[idx];
                fputc(outbuf[outidx], f);
#ifdef DEBUGPRINT
                fputc(outbuf[outidx], stdout);
#endif                
                idx++;
                outidx++;
                len--;
            }
        }
        else
        {
            // run
            if (len == 7)
            {
                // long run
                len = data[idx] + 7;
                idx++;
            }
            len += 2;
            
            int ofs = ((op & 31) << 8) | data[idx];
            idx++;
#ifdef DEBUGPRINT
            int k;
            printf("[");
            for (k = 0; k < outidx; k++)
            printf("%c", outbuf[k]);
            printf("]\n");
            
            printf("Rep %d chars, ofs -%d \"", len, ofs);
#endif
            ofs = outidx - ofs - 1;
            while (len)
            {
                outbuf[outidx] = outbuf[ofs];
                fputc(outbuf[outidx], f);               
#ifdef DEBUGPRINT
                fputc(outbuf[outidx], stdout);
#endif                
                outidx++;
                ofs++;
                len--;
            }
        }            
#ifdef DEBUGPRINT
        printf("\"\n");            
#endif        
    }
    printf("%s - %d bytes\n", pars[2], outidx);   
    fclose(f);
    return 0;    
}
