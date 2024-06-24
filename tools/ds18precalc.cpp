/* Make Visual Studio not consider fopen() deprecated */
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"

struct GlyphData
{
    int mXMin, mXMax;
    unsigned char mPixelData[8];
};

GlyphData gGlyphData[94];

void shift()
{
    int i, j;
    for (i = 0; i < 94; i++)
    {
        for (j = 0; j < 8; j++)
        {
            gGlyphData[i].mPixelData[j] <<= gGlyphData[i].mXMin;
        }
    }
}

void scan_glyph(unsigned int *aData, GlyphData &aParsed)
{
    aParsed.mXMin = 8;
    aParsed.mXMax = 0;
    int i, j;
    for (i = 0; i < 8; i++)
    {        
        aParsed.mPixelData[i] = 0;
        for (j = 0; j < 8; j++)
        {
            if (aData[i * 8 + j] & 0xffffff)
            {
                if (aParsed.mXMin > j) aParsed.mXMin = j;
                if (aParsed.mXMax < j) aParsed.mXMax = j;
                aParsed.mPixelData[i] |= 0x80 >> j;
            }            
        }
    }
    if (aParsed.mXMin > aParsed.mXMax)
    {
        aParsed.mXMin = 0;
        aParsed.mXMax = 4 - 1;
    }
}

FILE * binfile;
FILE * hfile;
FILE * sfile;
int offset = 0;
int cycle = 0;
int first = 1;
unsigned char outdata[4096];

void output_annotation(char *aStr)
{
    cycle = 0;
    fprintf(hfile,"%s/* offset %d: %s */\n", first?"":",\n\n",offset, aStr);
    fprintf(sfile,"%s; offset %d: %s\n", first?"":"\n\n", offset, aStr);
    first = 1;
    
}

void output(unsigned char data)
{
	outdata[offset] = data;
    fwrite(&data, 1,1,binfile);
    
    fprintf(hfile, "%s%s0x%02x", first?"":", ", ((cycle%16) == 0)?"\n":"", data);
    fprintf(sfile, "%s 0x%02x", ((cycle%16)==0)?"\n.db ":",",data);
    first = 0;
    offset++;
    cycle++;
}

void open_output(char *aFilename)
{
    char temp1[256];
    temp1[0] = 0;
    strcpy(temp1, aFilename);
    strrchr(temp1, '.')[0] = 0;
    char temp2[256];
    sprintf(temp2, "%s.bin", temp1);
    binfile = fopen(temp2, "wb");
    sprintf(temp2, "%s.h", temp1);
    hfile = fopen(temp2, "wb");
    sprintf(temp2, "%s.s", temp1);
    sfile = fopen(temp2, "wb");    
}

void close_output()
{
    fclose(binfile);
    fprintf(hfile, "\n");
    fclose(hfile);
    fclose(sfile);
}

int pattern[256];
int patterns;

void scan_font(char *aFilename)
{   
    int i, j;
    int x,y,n;
    unsigned int *data = (unsigned int*)stbi_load(aFilename, &x, &y, &n, 4);
    if (!data)
    {
        printf("unable to load \"%s\"\n", aFilename);
        exit(-1);
    }

    if (x != 8 || y < 752 || y % 94)
    {
        printf("Bad image dimensions; should be 8x752 (found %dx%d)\n", x, y);
        exit(-1);
    }

    open_output(aFilename);
    
    for (n = 0; n < 94; n++)
    {
        scan_glyph(data + 8 * 8 * n, gGlyphData[n]);
    }
    
    shift();

    for (i = 0; i < 256; i++)
    {
        pattern[i] = -1;
    }
    
    for (i = 0; i < 94; i++)
    {
        for (j = 0; j < 8; j++)
        {
            pattern[gGlyphData[i].mPixelData[j]] = 1;
        }
    }

    patterns = 0;
    for (i = 0; i < 256; i++)
    {
        if (pattern[i] != -1) 
        {
            pattern[i] = patterns * 8;
            patterns++;
        }
    }
	
	if (patterns > 32)
	{
		printf("Too many patterns (max 32, found %d)\n", patterns);
		exit(-1);
	}

    output_annotation("Glyph widths (94 bytes)");
    // width
    for (i = 0; i < 94; i++)
    {
        output(gGlyphData[i].mXMax - gGlyphData[i].mXMin + 1 + 1);
    }
    

    output_annotation("Glyph data encoded as pattern indices (8x94 bytes)");
    // glyph data
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 94; i++)
        {
            output(pattern[gGlyphData[i].mPixelData[j]]);
        }
    }
    
    char temp[256];
    sprintf(temp, "Glyph pattern shift tables (8 shifts, 2 bytes per index, %d patterns)", patterns);
    output_annotation(temp);
    // pre-shifted patterns
    for (i = 0; i < 256; i++)
    {
        if (pattern[i] != -1)
        {
            for (j = 0; j < 8; j++)
            {
                unsigned short v = i << 8;
                v >>= j;
                output((v >> 8) & 0xff);
                output((v >> 0) & 0xff);
            }
        }
    }
    close_output();
}

void verify()
{
	int i, j, glyph;
	for (glyph = 0; glyph < 94; glyph++)
	{
		int errored = 0;
		for (i = 0; i < 8; i++)
		{
			unsigned char rowdata1 = gGlyphData[glyph].mPixelData[i];
			unsigned char rowdata2 = outdata[846+pattern[gGlyphData[glyph].mPixelData[i]]*2];
			if (rowdata1 != rowdata2)
				errored = 1;
		}
		if (errored)
		{
			printf("Glyph %d ('%c')\n", glyph, glyph+32);
			for (i = 0; i < 8; i++)
			{
				unsigned char rowdata1 = gGlyphData[glyph].mPixelData[i];
				unsigned char rowdata2 = outdata[846+pattern[gGlyphData[glyph].mPixelData[i]]*2];
				for (j = 0; j < 8; j++)
				{
					printf("%s", (rowdata1 & (1 << (7-j))) ? "*":".");
				}
				printf(" pattern 0x%02x idx %2d:0x%02x", 
					gGlyphData[glyph].mPixelData[i], 
					pattern[gGlyphData[glyph].mPixelData[i]],
					outdata[846+pattern[gGlyphData[glyph].mPixelData[i]]*2]);
				for (j = 0; j < 8; j++)
				{
					printf("%s", (rowdata2 & (1 << (7-j))) ? "*":".");
				}
				printf("\n");
			}
		}
	}
	
	for (i = 0; i < patterns; i++)
	{
		for (j = 0; j < 8; j++)
		{
			int ref = outdata[846 + i*16];
			int a = outdata[846 + i*16 + j*2];
			int b = outdata[846 + i*16 + j*2 + 1];
			int sha = a << j;
			int shb = b >> (8-j);
			if (ref != (sha|shb))
				printf("%d >> %d: ref:0x%02x a:0x%02x b:0x%02x sha:0x%02x shb:0x%02x\n", i, j, ref, a, b, sha, shb);
		}
	}
}

int main(int parc, char **pars)
{
    printf("DrawString'18 data precalculator (c)2018 Jari Komppa\n");    
    if (parc < 2)
    {
        printf("Usage: %s fontfilename\n", pars[0]);
        return 0;
    }
    scan_font(pars[1]);
	verify();
    return 0;
}
