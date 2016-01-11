/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/pack.h"
#include "../common/lzfpack.h"
#include "../common/zx7pack.h"
#include "../common/rcspack.h"
#include "../common/tapper.h"
#include "fona.h"

#include "screen_unpacker_lzf.h"
#include "screen_unpacker_zx7.h"
#include "screen_unpacker_rcs.h"
#include "boot_lzf.h"
#include "boot_zx7.h"

#define VERSION "2.0"

#define SPEC_Y(y)  (((y) & 0xff00) | ((((y) >> 0) & 7) << 3) | ((((y) >> 3) & 7) << 0) | (((y) >> 6) & 3) << 6)
#define SCR_LENGTH (32*192+24*32)

#define BASIC_SIZE 51
#define CODE_OFFSET (23759+BASIC_SIZE) // legal code offset, 23759+basic size

Tapper gLoaderHeader, gLoaderPayload;
Tapper gAppHeader, gAppPayload;
Pack *gScreen, *gApp, *gLowBlock;

int gImageSize = 0;
int gExecAddr = 0;
int gBootExecAddr = 0;
int gMaxAddr = 0xff58;
char gProgName[11];

int gOptWeirdScr = 0;
int gOptNoClear = 0;
int gOptNoEI = 0;
int gOptNoSPRestore = 0;
int gOptVerbose = 1;
int gOptBinary = 0;
int gOptScreenCodec = 2;
int gOptAppCodec = 1;
int gOptLowBlock = 0;
int gOptLowBlockAddr = 0x5b00;
int gOptLowBlockCompress = 1;
int gOptLowBlockLen = 0;
char *gOptLowBlockFilename = 0;
int gOptHiBlock = 0;
int gOptHiBlockAddr = 0xffff;
char *gOptHiBlockFilename = 0;
int gOptHiBlockLen = 0;
unsigned char *gOptHiBlockData = 0;

unsigned char *screen_unpacker_bin;
int screen_unpacker_bin_len;
unsigned char *boot_bin;
int boot_bin_len;

char * gScreenPackerString[] = { "LZF", "ZX7", "RCS" };
char * gAppPackerString[] = { "LZF", "ZX7" };

void drawtext(unsigned char *aBuf, int aX, int aY, char *aText)
{
    int j=0;
    while (*aText)
    {
        int charofs = (*aText / 16) * 16 * 32 + (*aText % 16)*2;
        int i;
        for (i = 0; i < 8; i++)
        {
            aBuf[SPEC_Y(aY+i)*32+aX] = fona[charofs + i * 32 * 2] ^ 0xff;
            aBuf[SPEC_Y(aY+i)*32+aX+1] = fona[charofs+1 + i * 32 * 2] ^ 0xff;
        }
        aText++;
        aX+=2;
    }
}

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
    gLoaderPayload.putdata(0x00);
    gLoaderPayload.putdata(0x0a); // line number 10
    gLoaderPayload.putdata(BASIC_SIZE); // bytes on line (51)
    gLoaderPayload.putdata(0x00); // 0?
    gLoaderPayload.putdata(0xfd); // CLEAR
    gLoaderPayload.putdataintlit_min(gBootExecAddr-1);    
    gLoaderPayload.putdata(':');
    gLoaderPayload.putdata(0xf9); // RANDOMIZE
    gLoaderPayload.putdata(0xc0); // USR
    gLoaderPayload.putdataintlit_min(CODE_OFFSET); // 23759+basic size
    gLoaderPayload.putdata(':');
    gLoaderPayload.putdata(0xf4); // POKE
    gLoaderPayload.putdataintlit_min(23739);
    gLoaderPayload.putdata(',');
    gLoaderPayload.putdataintlit_min(111);
    gLoaderPayload.putdata(':');
    gLoaderPayload.putdata(0xef); // LOAD
    gLoaderPayload.putdata('"');
    gLoaderPayload.putdata('"');
    gLoaderPayload.putdata(0xaf); // CODE
    gLoaderPayload.putdata(':');
    gLoaderPayload.putdata(0xf9); // RANDOMIZE
    gLoaderPayload.putdata(0xc0); // USR
    gLoaderPayload.putdataintlit_min(gBootExecAddr);
    gLoaderPayload.putdata(0x0d); // enter
    if (gOptVerbose) printf("BASIC part       : %d bytes\n", BASIC_SIZE);
}

void append_screen_unpacker()
{
    int i;
    for (i = 0; i < screen_unpacker_bin_len; i++)
        gLoaderPayload.putdata(screen_unpacker_bin[i]);
    if (gOptVerbose) printf("Screen unpacker  : %d bytes\n", screen_unpacker_bin_len);
}

void append_pic()
{ 
    int i;
    for (i = 0; i < gScreen->mMax; i++)
        gLoaderPayload.putdata(gScreen->mPackedData[i]);   
}

int checkpatch16(unsigned char *data, int ofs, int expected)
{
    if (data[ofs] != (expected & 0xff)) return 0;
    if (data[ofs+1] != ((expected >> 8) & 0xff)) return 0;
    return 1;
}

void patch16(unsigned char *data, int ofs, int patch)
{
    data[ofs] = patch & 0xff;
    data[ofs+1] = (patch >> 8) & 0xff;
}

void patch8(unsigned char *data, int ofs, int patch)
{
    data[ofs] = patch & 0xff;
}

unsigned char gBootasm[512];
int gBootasmidx = 0;
void p(unsigned char c)
{
    gBootasm[gBootasmidx] = c;
    gBootasmidx++;
}

void gen_bootasm()
{
    p(0xf3); // DI

//    p(0x76); // HALT (for debugging)

    // Put stack into video memory
    p(0x31); p(0x00); p(0x44);            // ld sp, #0x4400 ; put stack into video memory

    if (!gOptNoClear)
    {
        p(0x3E); p(0x00);          //  ld a, #0
        p(0x11); p(0x01); p(0x58); //  ld de, #0x5801
        p(0x21); p(0x00); p(0x58); //  ld hl, #0x5800
        p(0x01); p(0xFF); p(0x02); //  ld bc, #767
        p(0x77);                   //  ld (hl), a        
        p(0xED); p(0xB0);          //  ldir       
    }
   
    // Copy bootloader to video memory 
    int vidmemposidx = gBootasmidx + 1;
    p(0x11); p(0x00); p(0x40);   //  ld de, #0x4000 ; dst (needs patching)
    p(0x01); p(0x00); p(0x02);   //  ld bc, #0x200  ; len, currently ~140 bytes, but a little extra doesn't hurt

    int bootidx = gBootasmidx + 1;

    p(0x21); p(0x07); p(0xB0);   //  ld hl, #0xb007 ; src (needs patching)
    p(0xED); p(0xB0);            //  ldir
    
    // Jump to video memory
    int vidmemposidx2 = gBootasmidx + 1;
    p(0x21); p(0x00); p(0x40);   //  ld hl, #0x4000                        
    p(0xE9);                     //  jp (hl)    

    int bootloaderidx = gBootasmidx;

    int lowdestposidx;
    int lowsrcposidx;
    int lowlenidx;
    if (gOptLowBlock)
    {
        lowdestposidx = gBootasmidx + 1;
        p(0x11); p(0x57); p(0xDE);      // ld de, #0xde57 ; destination position (needs patching)
        lowsrcposidx = gBootasmidx + 1;
        p(0x21); p(0x52); p(0x50);      // ld hl, #0x5052 ; source position (needs patching)   
        if (gOptLowBlockCompress)
        {
            p(0xCD); p(0x00); p(0x41);  // call unpack at 0x4100
        }
        else
        {
            lowlenidx = gBootasmidx + 1;
            p(0x01); p(0x00); p(0x02);   //  ld bc, #0x200  ; len (needs patching)
            p(0xED); p(0xB0);            //  ldir       
        }
    }

    int destposidx = gBootasmidx + 1;
    // Bootloader:
    p(0x11); p(0x57); p(0xDE);      // ld de, #0xde57 ; destination position (needs patching)
    int srcposidx = gBootasmidx + 1;
    p(0x21); p(0x52); p(0x50);      // ld hl, #0x5052 ; source position (needs patching)   
    p(0xD5);                        // push de ; put dest into stack
    p(0xCD); p(0x00); p(0x41);      // call unpack at 0x4100
    p(0xE1);                        // pop hl ; dest address
    
    int sprestoreidx = 0;
    
    if (!gOptNoSPRestore)
    {
        sprestoreidx = gBootasmidx + 1;
        p(0x31); p(0xf0); p(0xea);      // restore BASIC stack
    }


    if (!gOptNoEI)
    {
        p(0xFB);                    // ei
    }


    p(0xE9);                        // jp (hl) ; jumps to dest position          

    gBootExecAddr = gMaxAddr - (gApp->mMax + boot_bin_len + gBootasmidx + gOptLowBlock * gLowBlock->mMax);

    int boot_len = boot_bin_len + gBootasmidx;
    int image_ofs = gMaxAddr - (boot_len + gApp->mMax + gOptLowBlock * gLowBlock->mMax);
    int vidmem_ofs = 0x4100 - (gBootasmidx - bootloaderidx);
    int source_ofs = image_ofs + gBootasmidx + boot_bin_len + gOptLowBlock * gLowBlock->mMax;
    int boot_ofs = image_ofs + bootloaderidx;
    int lowblock_ofs = image_ofs + boot_len;
    
    // Sanity check
    if (!checkpatch16(gBootasm, vidmemposidx, 0x4000)) { printf("SANITY FAIL patch position vidmemposidx mismatch, aborting %02x%02x\n", gBootasm[vidmemposidx], gBootasm[vidmemposidx+1]); exit(0);}
    if (!checkpatch16(gBootasm, vidmemposidx2, 0x4000)) { printf("SANITY FAIL patch position vidmemposidx2 mismatch, aborting %02x%02x\n", gBootasm[vidmemposidx2], gBootasm[vidmemposidx2+1]); exit(0);}
    if (!checkpatch16(gBootasm, bootidx, 0xb007)) { printf("SANITY FAIL patch position bootidx mismatch, aborting %02x%02x\n", gBootasm[bootidx], gBootasm[bootidx+1]); exit(0);}
    if (!checkpatch16(gBootasm, destposidx, 0xDE57)) { printf("SANITY FAIL patch position destposidx mismatch, aborting %02x%02x\n", gBootasm[destposidx], gBootasm[destposidx+1]); exit(0);}
    if (!checkpatch16(gBootasm, srcposidx, 0x5052)) { printf("SANITY FAIL patch position srcposidx mismatch, aborting %02x%02x\n", gBootasm[srcposidx], gBootasm[srcposidx+1]); exit(0);}

    if (!gOptNoSPRestore)
    {
        if (!checkpatch16(gBootasm, sprestoreidx, 0xeaf0)) { printf("SANITY FAIL patch position sprestoreidx mismatch, aborting %02x%02x\n", gBootasm[sprestoreidx], gBootasm[sprestoreidx+1]); exit(0);}
        patch16(gBootasm, sprestoreidx, gBootExecAddr-24);
    }

    if (gOptLowBlock)
    {
        if (!gOptLowBlockCompress)
            if (!checkpatch16(gBootasm, lowlenidx, 0x0200)) { printf("SANITY FAIL patch position lowlenidx mismatch, aborting %02x%02x\n", gBootasm[lowlenidx], gBootasm[lowlenidx+1]); exit(0);}
        if (!checkpatch16(gBootasm, lowdestposidx, 0xDE57)) { printf("SANITY FAIL patch position lowdestposidx mismatch, aborting %02x%02x\n", gBootasm[lowdestposidx], gBootasm[lowdestposidx+1]); exit(0);}
        if (!checkpatch16(gBootasm, lowsrcposidx, 0x5052)) { printf("SANITY FAIL patch position lowsrcposidx mismatch, aborting %02x%02x\n", gBootasm[lowsrcposidx], gBootasm[lowsrcposidx+1]); exit(0);}

        if (!gOptLowBlockCompress)
            patch16(gBootasm, lowlenidx, gLowBlock->mMax);
        patch16(gBootasm, lowsrcposidx, lowblock_ofs);
        patch16(gBootasm, lowdestposidx, gOptLowBlockAddr);
        
    }
            
    patch16(gBootasm, vidmemposidx, vidmem_ofs);
    patch16(gBootasm, bootidx, boot_ofs);
    patch16(gBootasm, vidmemposidx2, vidmem_ofs);
    patch16(gBootasm, destposidx, gExecAddr);
    patch16(gBootasm, srcposidx, source_ofs);

/*
    if (gOptVerbose) 
    {
        printf("Video memory code offset : 0x%04x %5d\n", vidmem_ofs, vidmem_ofs);
        printf("Low block image offset   : 0x%04x %5d\n", lowblock_ofs, lowblock_ofs);
        printf("Compressed image offset  : 0x%04x %5d\n", source_ofs, source_ofs);
        printf("Compressed image len     : 0x%04x %5d\n", gApp->mMax, gApp->mMax);
        printf("Destination image offset : 0x%04x %5d\n", gExecAddr, gExecAddr);
        printf("Bootloader code offset   : 0x%04x %5d\n", boot_ofs, boot_ofs);
    }
*/    
}

void append_bootbin()
{   
    int i;
    
    for (i = 0; i < gBootasmidx; i++)
        gAppPayload.putdata(gBootasm[i]);
    
    for (i = 0; i < boot_bin_len; i++)
        gAppPayload.putdata(boot_bin[i]);
        
    if (gOptVerbose) printf("App bootstrap    : %d bytes (%d codec, %d rest)\n", boot_bin_len + gBootasmidx, boot_bin_len, gBootasmidx);
}

void append_hiblock()
{ 
    if (gOptHiBlock)
    {        
        int i;
        for (i = 0; i < gOptHiBlockLen; i++)
            gAppPayload.putdata(gOptHiBlockData[i]);   
    }
}

void append_lowblock()
{ 
    if (gOptLowBlock)
    {
        int i;
        for (i = 0; i < gLowBlock->mMax; i++)
            gAppPayload.putdata(gLowBlock->mPackedData[i]);   
    }
}

void append_app()
{ 
    int i;
    for (i = 0; i < gApp->mMax; i++)
        gAppPayload.putdata(gApp->mPackedData[i]);   
}

void save_tap(char *aFilename)
{
    FILE * f = fopen(aFilename, "wb");
    if (!f)
    {
        printf("Can't open \"%s\" for writing.\n", aFilename);
        exit(0);
    }
    gLoaderHeader.putdata((unsigned char)0); // 0 = program
    gLoaderHeader.putdatastr(gProgName); // 10 chars exact
    gLoaderHeader.putdataint(gLoaderPayload.ofs-1);
    gLoaderHeader.putdataint(10); // autorun row
    gLoaderHeader.putdataint(gLoaderPayload.ofs-1);
    
    gLoaderHeader.write(f);
    gLoaderPayload.write(f);
    
    gAppHeader.putdata((unsigned char)3); // 3 = code
    gAppHeader.putdatastr("iki.fi/sol"); // 10 chars exact (pretty much nobody will see this)
    gAppHeader.putdataint(gAppPayload.ofs-1);
    gAppHeader.putdataint(gBootExecAddr); // "Start of the code block when saved"
    gAppHeader.putdataint(32768);
    
    gAppHeader.write(f);
    gAppPayload.write(f);
    
    int len = ftell(f);
    fclose(f);
    if (gOptVerbose) 
        printf("\"%s\" written: %d bytes\n"
        "Estimated load time: %d seconds (%d secs to loading screen).\n", 
        aFilename, 
        len, 
        len * 8 / 1200 + 10, 
        (gScreen->mMax + BASIC_SIZE) * 8 / 1200 + 6);
}

int decode_ihx(unsigned char *src, int len, unsigned char *data)
{
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
    gExecAddr = start;
    return end-start+1;
}

void load_code(char *aFilename)
{   
    unsigned char *data = new unsigned char[0x10000];
    memset(data, 0, 0x10000);
    FILE * f = fopen(aFilename, "rb");
    if (!f)
    {
        printf("\"%s\" not found\n", aFilename);
        exit(0);
    }
    fseek(f,0,SEEK_END);
    int len = ftell(f);
    fseek(f,0,SEEK_SET);
    
    unsigned char * src = new unsigned char[len+1];
    
    fread(src, len, 1, f);
    fclose(f);
    src[len] = 0;
    int image_size;

    if (gOptBinary)
    {
        image_size = len;
        memcpy(data + gExecAddr, src, len);
    }
    else
    {            
        image_size = decode_ihx(src, len, data);
    }
    gImageSize = image_size;
    delete[] src;
    gApp->pack(data + gExecAddr, gImageSize);
    if (gOptVerbose)
    printf(
        "\n\"%s\":\n"
        "\tExec address : %d (0x%x)\n"
        "\tImage size   : %d bytes\n"
        "\tCompressed to: %d bytes (%3.3f%%) by %s\n", 
        aFilename, 
        gExecAddr, gExecAddr,
        image_size, 
        gApp->mMax, gApp->mMax*100.0f/image_size, gAppPackerString[gOptAppCodec]);
    
    delete[] data;
}

void load_hiblock()
{   
    if (!gOptHiBlock)
        return;
    FILE * f = fopen(gOptHiBlockFilename, "rb");
    if (!f)
    {
        printf("\"%s\" not found\n", gOptHiBlockFilename);
        exit(0);
    }
    fseek(f,0,SEEK_END);
    int len = ftell(f);
    fseek(f,0,SEEK_SET);
    
    unsigned char * src = new unsigned char[len];
    
    fread(src, len, 1, f);
    fclose(f);

    if (gOptVerbose)
    printf(
        "\n\"%s\":\n"
        "\tHiBlock addr : %d (0x%x)\n"
        "\tHiBlock size : %d bytes\n"
        "\tCompressed to: (no compression on hi blocks)\n", 
        gOptHiBlockFilename, 
        gOptHiBlockAddr, gOptHiBlockAddr,
        len);

    gOptHiBlockData = src;
    gOptHiBlockLen = len;
    
    gMaxAddr = gOptHiBlockAddr - gOptHiBlockLen;
}

void load_lowblock()
{   
    if (!gOptLowBlock)
        return;
    FILE * f = fopen(gOptLowBlockFilename, "rb");
    if (!f)
    {
        printf("\"%s\" not found\n", gOptLowBlockFilename);
        exit(0);
    }
    fseek(f,0,SEEK_END);
    int len = ftell(f);
    fseek(f,0,SEEK_SET);
    
    unsigned char * src = new unsigned char[len];
    
    fread(src, len, 1, f);
    fclose(f);

    if (gOptLowBlockCompress)
        gLowBlock->pack(src, len);
        
    if (gLowBlock->mMax >= len || gOptLowBlockCompress == 0)
    {
        gOptLowBlockCompress = 0;
        gLowBlock->mMax = len;
        int i;
        for (i = 0; i < len; i++)
            gLowBlock->mPackedData[i] = src[i];
    }

    if (gOptVerbose)
    printf(
        "\n\"%s\":\n"
        "\tLowBlock addr: %d (0x%x)\n"
        "\tLowBlock size: %d bytes\n"
        "\tCompressed to: %d bytes (%3.3f%%) by %s\n", 
        gOptLowBlockFilename, 
        gOptLowBlockAddr, gOptLowBlockAddr,
        len, 
        gLowBlock->mMax, gLowBlock->mMax*100.0f/len, gOptLowBlockCompress?gAppPackerString[gOptAppCodec]:"(uncompressed)");

    gOptLowBlockLen = len;

    delete[] src;        
}


void set_progname(char *aName)
{
    int i;
    for (i = 0; i < 10; i++)
    {
        gProgName[i] = ' ';
    }
    
    gProgName[10] = 0;
    
    for (i = 0; i < 10 && aName[i]; i++)
    {
        gProgName[i] = aName[i];
    }
    
    if (gOptVerbose) printf("Progname set to \"%s\"\n", gProgName);
}


void load_loadingscreen(int aHaveit, char * aFilename)
{
    if (aHaveit)
    {
        FILE * f = fopen(aFilename, "rb");
        if (!f)
        {
            printf("\"%s\" not found\n", aFilename);
            exit(0);
        }
        fseek(f,0,SEEK_END);
        int len = ftell(f);
        if (gOptWeirdScr == 0 && len != SCR_LENGTH)
        {
            printf("\"%s\" size not %d - invalid .scr file?\n", aFilename, SCR_LENGTH);
            exit(0);
        }
        fseek(f,0,SEEK_SET);
        unsigned char *temp = new unsigned char[len];
        fread(temp, 1, len, f);
        fclose(f);
        gScreen->pack(temp, len);
        if (gOptVerbose)
        printf(
            "\n\"%s\"\n"
            "\tImage size   : %d bytes\n"
            "\tCompressed to: %d bytes (%3.3f%%) by %s\n", 
            aFilename, len, gScreen->mMax, gScreen->mMax*100.0f/len, gScreenPackerString[gOptScreenCodec]);
        delete[] temp;
    }
    else
    {
        unsigned char *temp = new unsigned char[SCR_LENGTH];
        memset(temp, 0, 32*192);
        memset(temp + 32*192, 7 | (1 << 6), 11*32);       
        memset(temp + 32*192 + 11*32, 1 << 3, 32);       
        memset(temp + 32*192 + 12*32, 7, 32*12);
        drawtext(temp, 2, 8*8+8, "Loading");
        int l = 0;
        while (gProgName[l] != ' ' && l < 10) l++;
        drawtext(temp, 30-(l*2), 13*8, gProgName);        
        gScreen->pack(temp, SCR_LENGTH);
        if (gOptVerbose)
        printf(
            "\nGenerated loading screen\n"
            "\tImage size   : %d bytes\n"
            "\tCompressed to: %d bytes (%3.3f%%) by %s\n", 
            SCR_LENGTH, gScreen->mMax, gScreen->mMax*100.0f/SCR_LENGTH, gScreenPackerString[gOptScreenCodec]);
        delete[] temp;        
    }
}

void print_usage(int aDo, char *aFilename)
{
    if (gOptVerbose || aDo) printf("Mackarel " VERSION " by Jari Komppa, http://iki.fi/sol\n");
    
    if (aDo)
    {
        printf(
            "\n"
            "Generate zx spectrum tape files from intel hex files\n"
            "(with compressed loading screens and compressed data images)\n"
            "\n"
            "Usage:\n"
            "\n"
            "%s IHXFILE OUTFILENAME [APPNAME] [LOADINGSCREEN] [OPTIONS]\n"
            "\n"
            "Where:\n"
            "IHXFILE          - .ihx file generated by compiler/linker\n"
            "OUTFILENAME      - name of .tap file to generate\n"
            "APPNAME          - application name, max 10 chars, optional\n"
            "LOADINGSCREEN    - .scr file to use as loading screen, optional\n"
            "Options:\n"
            "-binimage addr   - Input is not ihx but binary file. Needs exec addr.\n"
            "-maxaddress addr - Maximum address to overwrite, default 0x%04x\n"            
            "-noclear         - Don't clear attributes to 0 before unpacking\n"
            "-noei            - Don't enable interrupts before calling image\n"
            "-nosprestore     - Don't bother restoring SP before calling image\n"
            "-weirdscr        - Ignore .scr file size, use whatever it is.\n"
            "-screencodec x   - How to compress loading screen;\n"
            "                   values lzf, zx7, rcs (default rcs)\n"
            "-appcodec x      - How to compress app; values lzf, zx7 (default zx7)\n"
            "-lowblock fn addr- Low-memory data block, filename and mem offset\n"
            "-nolowblockpack  - Don't try to compress low block\n"
            "-hiblock fn addr - High-memory data block, filename and mem offset\n"
            "-q               - Quiet mode, only print errors.\n"
            "\n",
            aFilename,
            gMaxAddr);
        exit(0);
    }
}

void parse_commandline(int parc, char ** pars)
{
    int i;
    for (i = 0; i < parc; i++)
    {
        if (pars[i][0] == '-')
        {
            if (stricmp(pars[i], "-nosprestore") == 0)
            {
                gOptNoSPRestore = 1;
            }
            else
            if (stricmp(pars[i], "-nolowblockpack") == 0)
            {
                gOptLowBlockCompress = 0;
            }
            else
            if (stricmp(pars[i], "-lowblock") == 0)
            {
                gOptLowBlock = 1;
                i++;
                gOptLowBlockFilename = strdup(pars[i]);
                i++;
                gOptLowBlockAddr = strtol(pars[i],0,0);                
            }
            else
            if (stricmp(pars[i], "-hiblock") == 0)
            {
                gOptHiBlock = 1;
                i++;
                gOptHiBlockFilename = strdup(pars[i]);
                i++;
                gOptHiBlockAddr = strtol(pars[i],0,0);                
            }
            else
            if (stricmp(pars[i], "-appcodec") == 0)
            {
                i++;
                if (stricmp(pars[i], "lzf") == 0)
                {
                    gOptAppCodec = 0;
                }
                else
                if (stricmp(pars[i], "zx7") == 0)
                {
                    gOptAppCodec = 1;
                }
                else
                {
                    printf("Unknown app codec \"%s\"\n", pars[i]);
                    exit(0);
                }
            }
            else
            if (stricmp(pars[i], "-screencodec") == 0)
            {
                i++;
                if (stricmp(pars[i], "lzf") == 0)
                {
                    gOptScreenCodec = 0;
                }
                else
                if (stricmp(pars[i], "zx7") == 0)
                {
                    gOptScreenCodec = 1;
                }
                else
                if (stricmp(pars[i], "rcs") == 0)
                {
                    gOptScreenCodec = 2;
                }
                else
                {
                    printf("Unknown screen codec \"%s\"\n", pars[i]);
                    exit(0);
                }
            }
            else
            if (stricmp(pars[i], "-weirdscr") == 0)
            {
                gOptWeirdScr = 1;
            }
            else
            if (stricmp(pars[i], "-noclear") == 0)
            {
                gOptNoClear = 1;
            }
            else
            if (stricmp(pars[i], "-noei") == 0)
            {
                gOptNoEI = 1;
            }
            else
            if (stricmp(pars[i], "-q") == 0)
            {
                gOptVerbose = 0;
            }
            else
            if (stricmp(pars[i], "-maxaddress") == 0)
            {
                i++;
                gMaxAddr = strtol(pars[i],0,0);
            }
            else
            if (stricmp(pars[i], "-binimage") == 0)
            {
                i++;
                gExecAddr = strtol(pars[i],0,0);
                gOptBinary = 1;
            }
            else
            {
                printf("Unknown option parameter %s", pars[i]);
                exit(0);
            }
        }
    }
        
}

void memory_map()
{
    int i;
    printf("\n");
    printf("Memory : 0       2       4       6       8       10      12      14      16\n");
    printf("         |-------|-------|-------|-------|-------|-------|-------|-------|\n");
    printf("On load: ");

#define MEMBLOCK(max, ch) \
    while (ofs < (max)) \
    {\
        o += printf("%c", (ch));\
        ofs+= 256;\
        if (o == 64)\
        {\
            printf("\n         ");\
            o = 0;\
        }\
    }

    int o = 0;
    int ofs = 0;
    MEMBLOCK(16*1024, 'r');
    MEMBLOCK(16*1024+(192+24)*32, 's');
    MEMBLOCK(CODE_OFFSET - BASIC_SIZE, '.');
    MEMBLOCK(CODE_OFFSET, 'b');
    MEMBLOCK(gBootExecAddr, '.');
    MEMBLOCK(gBootExecAddr + gBootasmidx + boot_bin_len, 'B');
    MEMBLOCK(gBootExecAddr + gBootasmidx + boot_bin_len + gOptLowBlock * gLowBlock->mMax, 'L');
    MEMBLOCK(gBootExecAddr + gBootasmidx + boot_bin_len + gOptLowBlock * gLowBlock->mMax + gApp->mMax, 'C');
    MEMBLOCK(gBootExecAddr + gBootasmidx + boot_bin_len + gOptLowBlock * gLowBlock->mMax + gApp->mMax + gOptHiBlockLen, 'H');
    MEMBLOCK(gMaxAddr, '.');
    MEMBLOCK(0xffff, '-');

    printf("\n");
    printf("On boot: ");
    ofs = 0;
    MEMBLOCK(16*1024, 'r');
    MEMBLOCK(16*1024+(192+24)*32, 's');
    MEMBLOCK(gOptLowBlockAddr, '.');
    MEMBLOCK(gOptLowBlockAddr + gOptLowBlockLen, 'L');
    MEMBLOCK(gExecAddr, '.');
    MEMBLOCK(gExecAddr+gImageSize, 'C');
    MEMBLOCK(gBootExecAddr + gBootasmidx + boot_bin_len + gOptLowBlock * gLowBlock->mMax + gApp->mMax, '.');
    MEMBLOCK(gBootExecAddr + gBootasmidx + boot_bin_len + gOptLowBlock * gLowBlock->mMax + gApp->mMax + gOptHiBlockLen, 'H');
    MEMBLOCK(0xffff, '.');

    printf("\n");
    printf("Key    : r)om s)creen b)asic L)ow block C)ode block\n"
           "         H)igh block .)unused -)reserved\n");
    printf("\n");
}

void sanity_checks()
{
    int ok = 1;
    if (gExecAddr < 0x5b00) 
        { ok = 0; printf("\nError: Binary exec address (0x%04x) must not be below 0x5b00 (end of vidram)\n", gExecAddr); }
    if (gOptLowBlock && gOptLowBlockAddr < 0x5b00) 
        { ok = 0; printf("\nError: Low block address (0x%04x) must not be below 0x5b00 (end of vidram)\n", gOptLowBlockAddr); }
    if (gOptHiBlockAddr < gExecAddr + gImageSize) 
        { ok = 0; printf("\nError: Hi block start (0x%04x) not above code block (0x%04x-0x%04x)\n",
            gOptHiBlockAddr, gExecAddr, gExecAddr + gImageSize); }
    if (gOptHiBlockAddr + gOptHiBlockLen > 0xffff)
        { ok = 0; printf("\nError: Hi block doesn't fit in memory (goes past 0xffff) - min working address would be 0x%04x\n", 0xffff - gOptHiBlockLen); }
    if (gOptLowBlockAddr + gOptLowBlockLen > gExecAddr)
        { ok = 0; printf("\nError: Low block (0x%04x-0x%04x) overlaps with code block (0x%04x-) (low block too big?)\n", 
            gOptLowBlockAddr, gOptLowBlockAddr + gOptLowBlockLen, gExecAddr); }
    if (gBootExecAddr < gExecAddr)
        { ok = 0; printf("\nError: Compressed image (0x%04x-) must start later in memory than uncompressed (0x%04x-)\n", gBootExecAddr, gExecAddr); }

    if (gExecAddr + gImageSize == gMaxAddr)
        { printf("\nWarning: compressed and decompressed images end at same address, success of decompression uncertain\n\n"); }
    else
    if (gExecAddr + gImageSize >= gMaxAddr - 16)
        { printf("\nWarning: compressed and decompressed images end at nearly same address, success of decompression uncertain\n\n"); }

    // stack is at (gBootExecAddr-1) - (gBootExecAddr-24)
    if (!gOptNoSPRestore &&
        (gOptLowBlock &&
         gOptLowBlockAddr <= gBootExecAddr-1 &&
         gOptLowBlockAddr + gOptLowBlockLen >= gBootExecAddr-24))
        { printf("\nWarning: BASIC stack (0x%04x-0x%04x) gets destroyed. Either move stuff around it, or use -nosprestore if you don't plan to return to BASIC\n\n", 
            gBootExecAddr-24, gBootExecAddr-1); }

    if (!gOptNoSPRestore &&
        gOptLowBlock &&
        gOptLowBlockAddr < CODE_OFFSET)
        { printf("\nWarning: BASIC code gets destroyed by low block. Either use move it up to 0x%04x or higher, or use -nosprestore if you don't plan to return to BASIC\n\n", 
            CODE_OFFSET); }
        
    if (!ok)
    {
        printf("Error(s) found, exiting.\n");
        exit(1);
    }
}

int main(int parc, char ** pars)
{
    int nonflagparc = 0;
    int i;
    for (i = 0; i < parc && pars[i][0] != '-'; i++)
        nonflagparc++;
    
    print_usage(nonflagparc < 3, pars[0]);
    parse_commandline(parc, pars);

    switch (gOptScreenCodec)
    {
    case 0:
        screen_unpacker_bin = screen_unpacker_lzf_bin;
        screen_unpacker_bin_len = screen_unpacker_lzf_bin_len;
        gScreen = new LZFPack;
        break;
    case 1:
        screen_unpacker_bin = screen_unpacker_zx7_bin;
        screen_unpacker_bin_len = screen_unpacker_zx7_bin_len;
        gScreen = new ZX7Pack;
        break;
    case 2:
        screen_unpacker_bin = screen_unpacker_rcs_bin;
        screen_unpacker_bin_len = screen_unpacker_rcs_bin_len;
        gScreen = new RCSPack;
        break;
    }
    
    switch (gOptAppCodec)
    {
    case 0:
        boot_bin = boot_lzf_bin;
        boot_bin_len = boot_lzf_bin_len;
        gApp = new LZFPack;
        gLowBlock = new LZFPack;
        break;
    case 1:
        boot_bin = boot_zx7_bin;
        boot_bin_len = boot_zx7_bin_len;
        gApp = new ZX7Pack;
        gLowBlock = new ZX7Pack;
        break;
    }


    set_progname(nonflagparc > 3 ? pars[3] : pars[1]);
    load_code(pars[1]);    
    load_loadingscreen(nonflagparc > 4, pars[4]);

    load_lowblock();
    load_hiblock();

    // Generate bootup assembly
    gen_bootasm();
    
    if (gOptVerbose)
    printf("\nBoot exec address: %d (0x%x)\n", gBootExecAddr, gBootExecAddr);
    
    // Block flag bytes
    gLoaderHeader.putdata((unsigned char)0x00);
    gLoaderPayload.putdata((unsigned char)0xff);
    gAppHeader.putdata((unsigned char)0x00);
    gAppPayload.putdata((unsigned char)0xff);
    
    // Generate basic bootstrapper
    gen_basic();

    // Loader
    append_screen_unpacker();
    append_pic();
    
    // App
    append_bootbin();
    append_lowblock();
    append_app();
    append_hiblock();
    
    // Is this going to work?
    sanity_checks();
    
    save_tap(pars[2]);    

    if (gOptVerbose) memory_map();            
}    
