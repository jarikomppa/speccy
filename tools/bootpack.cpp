/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOOTLEN 303
#define BOOTLOADER_OFS 0x22

#define PATCH_BOOTLOADER_POS 0x1a
#define PATCH_DESTPOS 0x23
#define PATCH_COMPRESSEDLEN 0x27
#define PATCH_SOURCEPOS 0x2b
#define PATCH_DESTPOS2 0x12C

#define PATCH_BORDER 0x61
#define PATCH_CLEAR 0xc

int max_addr = 0xffff;
//int bootloader_addr = 0x4000;
int image_addr = 0x6000;
int no_border = 0;
int no_clear = 0;
int debugprint = 0;

int checkpatch16(unsigned char *data, int ofs, int expected)
{
    if (data[ofs] != (expected & 0xff)) return 0;
    if (data[ofs+1] != ((expected >> 8) & 0xff)) return 0;
    return 1;
}

void patch16(unsigned char *data, int ofs, int patch, char * name)
{
    if (debugprint) printf("patch [%s 0x%02x] from 0x%02x to 0x%02x\n", name, ofs, data[ofs], patch & 0xff);
    data[ofs] = patch & 0xff;
    if (debugprint) printf("patch [%s 0x%02x] from 0x%02x to 0x%02x\n", name, ofs+1, data[ofs+1], (patch >> 8) & 0xff);
    data[ofs+1] = (patch >> 8) & 0xff;
}

void patch8(unsigned char *data, int ofs, int patch)
{
    data[ofs] = patch & 0xff;
}

int main(int parc, char ** pars)
{
    if (parc < 2 || pars[1][0] == '-' || pars[1][0] == '/')
    {
        printf("Usage:\n"
               "\n"
               "%s inputfile [-nb] [-nc] [-v] [-maxaddress addr] [-imgaddress addr]\n"
               "\n"
               "where:\n"
               "\n"
               "inputfile        - combined boot.bin+yourimage.bin to patch\n"
               "-nb              - don't blink borders\n"
               "-nc              - don't clear attribute ram\n"
               "-v               - verbose mode (debug prints)\n"
               "-maxaddress addr - maximum address to overwrite, default 0x%04x\n"
               "-imgaddress addr - start address of decompressed image, default 0x%04x\n", 
               pars[0], 
               max_addr,
               image_addr);
        exit(0);
    }
    
    int i;
    for (i = 2; i < parc; i++)
    {
        if (stricmp(pars[i], "-nb") == 0)
        {
            no_border = 1;
        }
        else
        if (stricmp(pars[i], "-nc") == 0)
        {
            no_clear = 1;
        }
        else
        if (stricmp(pars[i], "-v") == 0)
        {
            debugprint = 1;
        }
        else
        if (stricmp(pars[i], "-maxaddress") == 0)
        {
            i++;
            max_addr = strtol(pars[i],0,0);
        }
        else
        if (stricmp(pars[i], "-imgaddress") == 0)
        {
            i++;
            image_addr = strtol(pars[i],0,0);
        }
        else
        {
            printf("Unknown parameter %s", pars[i]);
            exit(0);
        }        
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
    
    unsigned char * data = new unsigned char[len];
    
    fread(data, len, 1, f);
    fclose(f);
    f = fopen("boot.bin", "rb");
    if (!f)
    {
        printf("Can't find boot.bin\n");
        exit(0);
    }
    fseek(f,0,SEEK_END);
    int bootlen = ftell(f);
    fseek(f,0,SEEK_SET);
    fclose(f);
    
    if (bootlen != BOOTLEN)
    {
        printf("boot.bin not expected length, aborting\n");
        exit(0);
    }
    
    if (max_addr - len < 0x5b00)
    {
        printf("Image doesn't fit in physical memory: 0x%04x - 0x%04x < 0x5b00\n", max_addr, len);
        exit(0);
    }
    if (max_addr - len < image_addr)
    {
        printf("Compressed image overlaps uncompressed: 0x%04x - 0x%04x < %d\n", max_addr, len, image_addr);
        exit(0);
    }
    
    if (!checkpatch16(data, PATCH_DESTPOS, 0x6000)) { printf("patch position mismatch, aborting\n"); exit(0);}
    if (!checkpatch16(data, PATCH_DESTPOS2, 0x6000)) { printf("patch position mismatch, aborting\n"); exit(0);}
    if (!checkpatch16(data, PATCH_COMPRESSEDLEN, 0x2727)) { printf("patch position mismach, aborting\n"); exit(0);}
    if (!checkpatch16(data, PATCH_SOURCEPOS, 0xd000)) { printf("patch position mismatch, aborting\n"); exit(0);}
    
    int image_ofs = max_addr - len;
    int compressed_len = len - bootlen;
    int dest_pos = image_addr;
    int source_pos = image_ofs + bootlen;
    int bootloader_pos = image_ofs + BOOTLOADER_OFS;

    if (debugprint) 
    {
        printf("Image offset   : 0x%04x (%d)\n", image_ofs, image_ofs);
        printf("Compressed len : 0x%04x (%d)\n", compressed_len, compressed_len);
        printf("Destination pos: 0x%04x (%d)\n", dest_pos, dest_pos);
        printf("Source pos     : 0x%04x (%d)\n", source_pos, source_pos);
        printf("Bootloader pos : 0x%04x (%d)\n", bootloader_pos, bootloader_pos);
    }
    
    patch16(data, PATCH_BOOTLOADER_POS, bootloader_pos, "PATCH_BOOTLOADER_POS");
    patch16(data, PATCH_DESTPOS, dest_pos, "PATCH_DESTPOS");
    patch16(data, PATCH_DESTPOS2, dest_pos, "PATCH_DESTPOS2");
    patch16(data, PATCH_COMPRESSEDLEN, compressed_len, "PATCH_COMPRESSEDLEN");
    patch16(data, PATCH_SOURCEPOS, source_pos, "PATCH_SOURCEPOS");

    if (no_border)
    {
        patch8(data, PATCH_BORDER, 0);
        patch8(data, PATCH_BORDER+1, 0);
    }
    
    if (no_clear)
    {
        patch8(data, PATCH_CLEAR, 0);
    }    
   
    f = fopen(pars[1], "wb");
    if (!f)
    {
        printf("Can't open %s for writing\n", pars[1]);
        exit(0);
    }
    fwrite(data, 1, len, f);
    fclose(f);
    
    f = fopen("call_appmake.bat", "w");
    if (!f)
    {
        printf("Can't open call_appmake.bat for writing\n");
        exit(0);
    }
    fprintf(f, "appmake +zx --binfile %s --org %d\n", pars[1], image_ofs);
    fclose(f);
}    
