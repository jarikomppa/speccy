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
    
    unsigned char * src = new unsigned char[len+1];
    
    fread(src, len, 1, f);
    fclose(f);
    src[len] = 0;
    
    unsigned char *data = new unsigned char[0x10000];
    memset(data, 0, 0x10000);
    int start = 0x10000;
    int end = 0;
    int line = 0;
    int idx = 0;
    while (src[idx])
    {
        line++;
        int sum = 0;
        char tmp[8];
        if (src[idx] != ':')
        {
            printf("Parse error near line %d? (previous chars:\"%c%c%c%c%c%c\")\n", line,
                (idx<5)?'?':(src[idx-5]<32)?'?':src[idx-5],
                (idx<4)?'?':(src[idx-4]<32)?'?':src[idx-4],
                (idx<3)?'?':(src[idx-3]<32)?'?':src[idx-3],
                (idx<2)?'?':(src[idx-2]<32)?'?':src[idx-2],
                (idx<1)?'?':(src[idx-1]<32)?'?':src[idx-1],
                (idx<0)?'?':(src[idx-0]<32)?'?':src[idx-0]);
            exit(0);
        }
        idx++;
        tmp[0] = '0';
        tmp[1] = 'x';
        tmp[2] = src[idx]; idx++;
        tmp[3] = src[idx]; idx++;
        tmp[4] = 0;
        int bytecount = strtol(tmp,0,16);
        sum += bytecount;
        tmp[2] = src[idx]; idx++;
        tmp[3] = src[idx]; idx++;
        tmp[4] = src[idx]; idx++;
        tmp[5] = src[idx]; idx++;
        tmp[6] = 0;
        int address = strtol(tmp,0,16);
        sum += (address >> 8) & 0xff;
        sum += (address & 0xff);
        tmp[2] = src[idx]; idx++;
        tmp[3] = src[idx]; idx++;
        tmp[4] = 0;
        int recordtype = strtol(tmp,0,16);
        sum += recordtype;
        switch (recordtype)
        {
            case 0:
            case 1:
                break;
            default:
                printf("Unsupported record type %d\n", recordtype);
                exit(0);
                break;
        }
        //printf("%d bytes from %d, record %d\n", bytecount, address, recordtype);
        while (bytecount)
        {
            tmp[2] = src[idx]; idx++;
            tmp[3] = src[idx]; idx++;
            tmp[4] = 0;
            int byte = strtol(tmp,0,16);
            sum += byte;
            data[address] = byte;
            if (start > address)
                start = address;
            if (end < address)
                end = address;
            address++;
            bytecount--;
        }
        tmp[2] = src[idx]; idx++;
        tmp[3] = src[idx]; idx++;
        tmp[4] = 0;
        int checksum = strtol(tmp,0,16);
        sum = (sum ^ 0xff) + 1;
        if (checksum != (sum & 0xff))
        {
            printf("Checksum failure %02x, expected %02x\n", sum & 0xff, checksum);
            exit(0);
        }

        while (src[idx] == '\n' || src[idx] == '\r') idx++;         
    }

    f = fopen(pars[2], "wb");
    if (!f)
    {
        printf("Can't open %s\n", pars[2]);
        exit(0);
    }
    fwrite(data + start, 1, end-start+1, f);
    fclose(f);
    
}    
