/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUGPRINT

/*

    000LLLLL <L+1>             - literal reference
    LLLddddd dddddddd          - copy L+2 from d bytes before most recently decoded byte
    111ddddd dddddddd LLLLLLLL - copy L+2+7 from d bytes before most recently decoded byte
*/
#ifdef DEBUGPRINT
void dump_rep(char *data, int bestofs, int bestchars, int ofs)
{
    printf("Rep %d chars, ofs %d (%d) \"", bestchars, bestofs, ofs);
    while (bestchars)
    {
        fputc(data[bestofs], stdout);
        bestofs++;
        bestchars--;
    }
    printf("\"\n");
}
#endif

void dump_literals(FILE * f, char * data, int &literals, int &literalsofs, int ofs)
{
#ifdef DEBUGPRINT
    printf("Literal %d chars, ofs %d (%d)\"",literals, literalsofs, ofs);
#endif
    while (literals) 
    {
        int cc = literals;
        if (cc > 32) cc = 32;
        fputc(cc-1, f);
        int j;
        for (j = 0; j < cc; j++)
        {
            fputc(data[literalsofs], f);
#ifdef DEBUGPRINT
            fputc(data[literalsofs], stdout);
#endif
            literalsofs++;                        
        }
        literals -= cc;
    }
#ifdef DEBUGPRINT
    printf("\"\n");
#endif    
}

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
    int len = ftell(f);
    fseek(f,0,SEEK_SET);
    printf("%s - %d bytes\n", pars[1], len);
    char * data = new char[len];
    fread(data, len, 1, f);
    fclose(f);

    f = fopen(pars[2], "wb");
    if (!f)
    {
        printf("Can't open \"%s\"\n", pars[2]);
        exit(0);
    }
   
    int dataout = 0;
    int i;
    int literals = 0;
    int literalsofs = 0;
    for (i = 0; i < len; i++)
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
                if (data[j] == data[i])
                {
                    int k = 1;
                    while (data[j+k] == data[i+k] && k < (255 + 7 + 2)) k++;
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
                dataout += literals + (literals / 32) + 1;
                dump_literals(f, data, literals, literalsofs, i);
            }
#ifdef DEBUGPRINT
            dump_rep(data, bestofs, bestchars, i);
#endif            

            int ofs = -(bestofs - i) - 1;
#ifdef DEBUGPRINT
            printf("bestofs: %d i:%d ofs:%d\n", bestofs, i, ofs);
#endif            
            if (bestchars >= 7+2)
            {
                dataout += 3; // 3 bytes to encode
                fputc((7 << 5) | ((ofs >> 8) & 31), f);
                fputc(ofs & 0xff, f);
                fputc(bestchars - (7 + 2), f);
            }
            else
            {
                dataout += 2; // 2 bytes to encode                
                fputc(((bestchars-2) << 5) | ((ofs >> 8) & 31), f);
                fputc(ofs & 0xff, f);
            }
            
            i += bestchars - 1;
            
            literalsofs = i + 1;
        }
    }
    
    if (literals)
    {
         dataout += literals + (literals / 32) + 1;
         dump_literals(f, data, literals, literalsofs, i);
    }

    printf("%s - %d bytes\n", pars[2], dataout);   
    fclose(f);
    return 0;
}