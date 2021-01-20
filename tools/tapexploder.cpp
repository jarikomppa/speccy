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

struct Header
{
    unsigned char mFlag;
    unsigned char mType;
    unsigned char mName[11];
    unsigned short mLen;
    unsigned short mParam1;
    unsigned short mParam2;
    unsigned char mChecksum;
};

int parse_header(unsigned char *aData, Header *aHeader)
{
    unsigned short l = (int)aData[0] | ((int)aData[1] << 8);
    if (l != 18+1)
    {
        return 3;
    }
    
    aData += 2;
    
    aHeader->mFlag = aData[0];
    aHeader->mType = aData[1];
    int i;
    for (i = 0; i < 10; i++)
        aHeader->mName[i] = aData[2+i];
    aHeader->mName[10] = 0;
    aHeader->mLen = (int)aData[12] | ((int)aData[13] << 8);
    aHeader->mParam1 = (int)aData[14] | ((int)aData[15] << 8);
    aHeader->mParam2 = (int)aData[16] | ((int)aData[17] << 8);
    aHeader->mChecksum = aData[18];

    if (*aData != 0)
        return 1;
    
    unsigned char check = 0;
    for (i = 0; i < 18; i++)
        check ^= aData[i];
    
    if (check != aHeader->mChecksum)
        return 2;
    
    return 0;
}

int parse_data(unsigned char *aData, Header *aHeader, unsigned char *aDest)
{
    unsigned short l = (int)aData[0] | ((int)aData[1] << 8);
    
    aData += 2;
    
    if (*aData != 0xff)
        return 1;

    if (l != aHeader->mLen+2)
    {
        return 3;
    }

    int i;
    for (i = 0; i < aHeader->mLen; i++)
        aDest[i] = aData[i+1];

    unsigned char check = 0;
    for (i = 0; i < aHeader->mLen+1; i++)
        check ^= aData[i];

    if (check != aData[aHeader->mLen+1])
        return 2;
    
    return 0;
}


/*
TAP file format

Repeating blocks of:

Len     Field
2       Size: block size, not including size field
1       Flag: 0 for headers
1       Type: 0 for program, 1 for numeric array, 2 for alphanumeric array, 3 for binary data
10      Filename: 10 chars, padded with spaces
2       Data len: length of data, in bytes
2       Param1: autostart line, variable name, start address
2       Param2: program length, or 32768
1       Checksum - all previous bytes xor:ed together

Len     Field
1       Flag: 0xff for data blocks (such as CODE)
N       Data, length from header
1       Checksum - all previos bytes xor:ed together
*/

int main(int parc, char ** pars)
{
    if (parc < 2)
    {
        printf(
        "TAP file exploder. Extracts all blocks in a tap file into separate files\n"
        "(Largely useless thing to do, but useful for very specific things)\n"
        "\n"
        "Usage: %s inputfile.tap [-writeout]\n"
        "\n"
        "where:\n"
        "inputfile.tap - input .tap file\n"
        "-writeout - optional, perform write out\n"
        "            without this, only analysis is done\n", pars[0]);
        exit(0);
    }
    
    int writeout = 0;
    int i;
    for (i = 2; i < parc; i++)    
    {
        if (stricmp(pars[i], "-writeout") == 0)
        {
            writeout = 1;
        }
        else
        {
            printf("Unknown parameter \"%s\"\n", pars[i]);
            exit(0);
        }                        
    }
    
    FILE * f;
    f = fopen(pars[1], "rb");
    if (f == NULL)
    {
        printf("Can't open \"%s\"\n", pars[1]);
        exit(0);
    }
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *data = new unsigned char[len];
    fread(data, 1, len, f);
    fclose(f);
    
    int ofs = 0;
    Header header;
    unsigned char buf[65536];
    int block = 0;
    while (ofs < len)
    {
        if (data[ofs+2] == 0)
        {        
            switch (parse_header(data + ofs, &header))
            {
            case 0:
                printf("ofs %6d: \"%s\", type:%d len:%5d par1:%5d par2:%5d\n",
                    ofs,
                    header.mName,
                    header.mType,
                    header.mLen,
                    header.mParam1,
                    header.mParam2);
                break;
            case 1:
                printf("ofs %6d: flag type not 0 parsing header\n", ofs);
                exit(0);
            case 2:
                printf("ofs %6d: checksum failure parsing header\n", ofs);
                exit(0);
            case 3:
                printf("ofs %6d: length mismatch parsing header\n", ofs);
                exit(0);
            default:
                printf("ofs %6d: unknown error parsing header\n", ofs);
                exit(0);
            }
        }
        else
        if (data[ofs+2] == 0xff)
        {
            switch (parse_data(data + ofs, &header, buf))
            {
            case 0:
                printf("ofs %6d: data block of %d bytes ok\n",
                    ofs,
                    header.mLen);                
                break;
            case 1:
                printf("ofs %6d: flag type not 0xff parsing data\n", ofs);
                exit(0);
            case 2:
                printf("ofs %6d: checksum failure parsing data\n", ofs);
                exit(0);
            case 3:
                printf("ofs %6d: length mismatch parsing data\n", ofs);
                exit(0);
            default:
                printf("ofs %6d: unknown error parsing data\n", ofs);
                exit(0);
            }
            if (writeout)
            {
                char temp[512];
                sprintf(temp, "%s_block%d_type%d.dat", pars[1], block, header.mType);
                block++;
                FILE * f = fopen(temp, "wb");
                fwrite(buf, 1, header.mLen, f);
                fclose(f);
            }
        }
        else
        {
            int l = (int)data[ofs] | ((int)data[ofs+1] << 8);
            printf("ofs %6d: Unexpected flag byte %d (%x) on data block size %d (%x)\n", ofs, data[ofs+2], data[ofs+2], l, l);
            exit(0);
        }
        int l = (int)data[ofs] | ((int)data[ofs+1] << 8);
        ofs += l + 2;
    }
    
    return 0;
}
