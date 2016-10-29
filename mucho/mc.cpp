//#define DEBUG_DECODING

#define _CRT_SECURE_NO_WARNINGS
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "propfont.h"
#include "../common/pack.h"
#include "../common/zx7pack.h"
#include "yofstab.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"
#include "../common/ihxtools.h"

#define strdup _strdup
#define stricmp _stricmp

// the space we have on the device
#define DATA_AREA_SIZE 29952 
// 1k symbols should be enough for everybody
#define MAX_SYMBOLS (8*128) 
// max number of numeric variables
#define MAX_NUMBERS 32 
// max word count tokens
#define MAXTOKENS 2048 
// max bytes for the trainer data (1024 is a good value, +512 may help or harm)
#define MAX_TRAINER 1024 
// max number of rooms
#define MAX_ROOMS 1024 

enum opcodeval
{
    OP_HAS,
    OP_NOT,
    
    OP_SET,
    OP_CLR,
    OP_XOR,

    OP_RND,

    OP_ATTR,
    OP_EXT,
    OP_IATTR,
    OP_DATTR,

    OP_GO,
    OP_GOSUB,
    
    // variable-constant pairs. The code below depends on the pairing.
	OP_GT,  // a>b
    OP_GTC, // a>n
    OP_LT,  // a<b
    OP_LTC, // a<n
    OP_GTE, // a>=b
    OP_GTEC,// a>=n
    OP_LTE, // a<=b
    OP_LTEC,// a<=n
    OP_EQ,  // a==b
    OP_EQC, // a==n
    OP_IEQ, // a!=b
    OP_IEQC,// a!=n
    
    OP_ASSIGN,  // a=b
    OP_ASSIGNC, // a=n
    OP_ADD,     // a+b
    OP_ADDC,    // a+n
    OP_SUB,     // a-b
    OP_SUBC     // a-n
};


const unsigned char gDividerPattern[8] = 
{ 
    0x00, 
    0xC1, 
    0x32, 
    0x18, 
    0x0C, 
    0x26, 
    0xC1, 
    0x00  
};

const unsigned char gSelectorPattern[8] = 
{ 
    0x00, 
    0x88, 
    0xcc, 
    0xee, 
    0xcc, 
    0x88, 
    0x00, 
    0x00  
};

struct RoomBuf
{
    char *mName;
    unsigned char *mData;
    int mLen;
    int mUsed;
};

struct GlyphData
{
    int mXMin, mXMax;
    unsigned char mPixelData[20];
};

GlyphData gGlyphData[94];

int gPageData = 0;
int gImageData = 0;
int gCodeData = 0;
int gTrainers = 0;
unsigned char gTrainer[MAX_TRAINER];

int gRooms = 0;
RoomBuf gRoom[MAX_ROOMS];

int gRoomNo = 0;

int gMaxRoomSymbol = 0;

unsigned char *gPropfontData = (unsigned char *)&builtin_data[0];
unsigned char *gPropfontWidth = (unsigned char *)&builtin_width[0];
unsigned char *gDividerData = (unsigned char*)&gDividerPattern[0];
unsigned char *gSelectorData = (unsigned char*)&gSelectorPattern[0];

int gVerbose = 0;
int gQuiet = 0;

int gCommandPtrOpOfs = 0;

int gLine = 0;

char gScratch[64 * 1024];
char gStringLit[64 * 1024];
int gStringIdx;

ZX7Pack gPack;

float *gCompressionResults;

int gPreviousSection = 0;
int gPreviousStringLiterals = 0;


class WordCounter
{
public:
	char *mWord[MAXTOKENS];
	int mHits[MAXTOKENS];
	int mHash[MAXTOKENS];
	int mUsed[MAXTOKENS];
	int mNexts[MAXTOKENS];
	int mNext[MAXTOKENS][128];
	int mNextHit[MAXTOKENS][128];
	int mPrevToken;
	int mTokens;

	int calcHash(char *aString)
	{
		unsigned int i = 0;
		while (*aString)
		{
			i = (i << 11) | (i >> 21);
			i ^= *aString;
	        aString++;
		}    
    
		return 0;
	}    

	WordCounter()
	{
		mPrevToken = -1;
		mTokens = 0;
	}

	void tokenRef(int aCurrent)
	{
		int prev = mPrevToken;
		mPrevToken = aCurrent;
		if (prev != -1)
		{
			int i;
			for (i = 0; i < mNexts[prev]; i++)
			{
				if (mNext[prev][i] == aCurrent)
				{
					mNextHit[prev][i]++;
					return;
				}
			}
			if (mNexts[prev] < 128)
			{
				i = mNexts[prev];
				mNext[prev][i] = aCurrent;
				mNextHit[prev][i] = 1;
				mNexts[prev]++;
			}
		}
	}


	void addWordcountToken(char *aToken)
	{    
		if (strstr(aToken, "<<") != NULL) return; // discard tokens with numeric output
		int h = calcHash(aToken);
		int i;	
		if (aToken == NULL || *aToken == 0)
			return;

		for (i = 0; i < mTokens; i++)
		{
			if (mHash[i] == h && strcmp(mWord[i], aToken) == 0)
			{
				tokenRef(i);
				mHits[i]++;
				return;
			}
		}
		// We don't actually care if we run out of tokens,
		// as we're looking for the most frequent N tokens
		// anyway; if a token hasn't appeared during the 
		// MAXTOKENS, it's probably not all that frequent
		// anyway.
		if (mTokens < MAXTOKENS)
		{
			tokenRef(mTokens);
			mWord[mTokens] = strdup(aToken);
			mHash[mTokens] = h;
			mHits[mTokens] = 1;
			mUsed[mTokens] = 0;
			mNexts[mTokens] = 0;
			mTokens++;        
		}
		else
		{
			mPrevToken = -1;
		}
	}

	void wordCount(char *aString)
	{
		char temp[256];
		int p = 0;
		while (*aString)
		{
			temp[p] = *aString;
			if (*aString == ' ' || *aString == 0 || *aString == '\t' || *aString == '\r' || *aString == '\n')
			{
				temp[p] = 0;
				if (p > 0)
				{
					addWordcountToken(temp);
				}
				p = 0;
			}
			else
			{
				p++;
			}
			aString++;
		}
		temp[p] = 0;
		addWordcountToken(temp);
	}
};

WordCounter gWordCounter;

class Symbol
{
public:
    char *mName[MAX_SYMBOLS*2];
    int mHits[MAX_SYMBOLS*2];
	int mHash[MAX_SYMBOLS*2];
	int mCount;
	int mFirstIndex;

	Symbol()
	{
		mCount = 0;
		mFirstIndex = 0;
	}

	int calcHash(char *aString)
	{
		unsigned int i = 0;
		while (*aString)
		{
			char c = toupper(*aString);
			
			i = (i << 11) | (i >> 21);
			i ^= *aString;
	        aString++;
		}    
    
		return 0;
	}    

	int getId(char * aString)
	{
		int i;
		int hash = calcHash(aString);
		for (i = 0; i < mCount; i++)
		{
	        if (mHash[i] == hash && stricmp(mName[i], aString) == 0)
			{
	            mHits[i]++;
				return i + mFirstIndex;
			}
		}
		if (mCount >= MAX_SYMBOLS)
		{
			printf("Too many symbols, line %d\n", MAX_SYMBOLS);
			exit(-1);
		}
		mName[mCount] = strdup(aString);
		mHits[mCount] = 1;
		mHash[mCount] = hash;
		mCount++;
		return mCount-1;            
	}
};


Symbol gSymbol;
Symbol gImage;
Symbol gCode;
Symbol gNumber;

#define MAX_BUFFER_SIZE (1024*256)
class Buffer
{
public:
	unsigned char mData[MAX_BUFFER_SIZE];
	int mLen;
	Buffer()
	{
		mLen = 0;
	}

	void putByte(unsigned char aData)
	{
		if (aData == 0)
		{
			aData = aData;
		}
		if (mLen >= MAX_BUFFER_SIZE)
		{
			printf("Max buffer size overrun. This should never happen, line %d\n", gLine);
			exit(-1);
		}
		mData[mLen] = aData;
		mLen++;
	}


	void putRawArray(unsigned char *aData, int aLen)
	{
		while (aLen)
		{
			putByte(*aData);
			aLen--;
			aData++;
		}
	}

	void putArray(unsigned char *aData, int aLen)
	{
		putByte(aLen);
		putRawArray(aData, aLen);
	}

	void putString(char *aData)
	{
		int pass = 0;
		int len = 0;
		unsigned char *lenbyte = mData + mLen;
		putByte(0);
		while (*aData || pass)
		{
			if (pass == 0 && *aData == 1) pass = 3;
			if (pass == 0 && (*aData < 32 || *aData > 126))
			{
				printf("Invalid character \"%c\" found near line %d\n", *aData, gLine);
				exit(-1);
			}
			putByte(*aData);
			aData++;
			if (pass) pass--;
			len++;
		}
		*lenbyte = len;
	}

	void patchWord(unsigned short aWord, int aPos)
	{
		mData[aPos * 2] = aWord & 0xff;
		mData[aPos * 2 + 1] = aWord >> 8;
	}
};

Buffer gDataBuffer; // Gathered data
Buffer gOutBuffer;  // Data to be stored to disk
Buffer gPackBuffer; // Data to be sent to compressor
Buffer gCommandBuffer; 

int whitespace(char aCharacter)
{
    switch (aCharacter)
    {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            return 1;
    }
    return 0;
}

int is_numeric(char *aString)
{
    if (!*aString) return 0;
    while (*aString)
    {
        if (*aString < '0' || *aString > '9')
            return 0;
        aString++;
    }
    return 1;
}

void read_raw_line(char *aBuffer, FILE * aFile)
{
    int i = 0;
    int c;
    do
    {
        c = fgetc(aFile);
        if (c == '\r')
            c = fgetc(aFile);
        if (c == '\t')
            c = ' ';
        if (feof(aFile))
            c = '\n';
        aBuffer[i] = c;
        if (!feof(aFile) && c != '\n')
            i++;
    }
    while (!feof(aFile) && c > 31);
    
    // trim trailing spaces:
    while (i >= 0 && whitespace(aBuffer[i])) i--;
    i++;
    
    // terminate
    aBuffer[i] = 0;
}

// Skips empty and commented lines
void read_line(char *aBuffer, FILE * aFile)
{
    do
    {
        read_raw_line(aBuffer, aFile);
        gLine++;
    }
    while (!feof(aFile) && aBuffer[0] == '#' && aBuffer[0] > 31);
}

void token(int aTokenIndex, char *aSource, char *aDestination)
{    
    while (aTokenIndex && *aSource)
    {
        while (*aSource && !whitespace(*aSource)) aSource++;
        while (*aSource && whitespace(*aSource)) aSource++;
        aTokenIndex--;
    }
    while (*aSource && !whitespace(*aSource))
    {
        *aDestination = *aSource;
        aDestination++;
        aSource++;
    }
    *aDestination = 0;
}

#ifdef DEBUG_DECODING
void debug_decode(unsigned char *aData, int aLen)
{
        char temp[64];
        static int blobno = 0;
        sprintf(temp, "blob%02d.bin", blobno);
        blobno++;
        FILE * f = fopen(temp, "wb");
        fwrite(aData, 1, aLen, f);
        fclose(f);

		printf("\n");
		printf("Decoding block:\n");
		int ofs = 0;
		while (ofs < aLen)
		{
			printf("Segment '%c', %d bytes\n", aData[ofs+1], aData[ofs]);
			ofs += aData[ofs]+1;
			while (aData[ofs])
			{
				int len = aData[ofs];
				int i;
				
				printf("  Stringlit \"");
				for (i = 0; i < len; i++)
					printf("%c", aData[ofs + 1 + i]);
				printf("\"\n");

				ofs += aData[ofs] + 1;
			}
			printf("End of segment\n");
			ofs++;
		}
}
#endif

void flush_packbuf()
{
	if (gPackBuffer.mLen > gTrainers)
    {
        gPack.mMax = 0;
		gPack.pack((unsigned char*)&gPackBuffer.mData[0], gPackBuffer.mLen, gTrainers);

#ifdef DEBUG_DECODING
		debug_decode(gPackBuffer.mData + gTrainers, gPackBuffer.mLen - gTrainers);
#endif

        if (!gQuiet)
            printf("zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", 
				gPackBuffer.mLen - gTrainers, 
				gPack.mMax, 
				(gPack.mMax * 100.0f) / (gPackBuffer.mLen - gTrainers), 
				0x5b00 + gOutBuffer.mLen);
    
		gOutBuffer.putRawArray(gPack.mPackedData, gPack.mMax);
		gPackBuffer.mLen = gTrainers;
    }
}

void flush_room()
{
    if (gDataBuffer.mLen > 0)
    {
        if (gDataBuffer.mLen > 4096)
        {
            printf("Room %s data too large; max 4096 bytes, has %d bytes\n", gSymbol.mName[gRoomNo], gDataBuffer.mLen);
            exit(-1);
        }
        gRoom[gRooms].mName = gSymbol.mName[gRoomNo];
        gRoom[gRooms].mData = new unsigned char[gDataBuffer.mLen];
        gRoom[gRooms].mLen = gDataBuffer.mLen;
        gRoom[gRooms].mUsed = 0;
        memcpy(gRoom[gRooms].mData, gDataBuffer.mData, gDataBuffer.mLen);
        gRooms++;
        gDataBuffer.mLen = 0;
                
        gRoomNo++;
    }        
}


void flush_sect()
{
    if (gDataBuffer.mLen > 0)  // skip end if we're in the beginning
    {
        if (gVerbose) printf("End of section\n");
        gDataBuffer.putByte(0);
    }
    
}

void flush_cmd()
{
	if (gCommandBuffer.mLen)
    {
        int ops = (gCommandBuffer.mLen - 1 - (gCommandPtrOpOfs - 1) * 2) / 3;
        if (gVerbose) 
        {
            printf("  Command buffer '%c' (%d bytes) with %d bytes payload (%d ops) %d\n", 
				gCommandBuffer.mData[0], 
				//commandptropofs + 1,
				gCommandBuffer.mLen,
				gCommandBuffer.mLen - 1 - (gCommandPtrOpOfs - 1) * 2, 
				ops);
        }
                
        if (gCommandBuffer.mLen > 255)
        {
            printf("Syntax error - too many operations on one statement, line %d\n", gLine);
            exit(-1);
        }
		gDataBuffer.putArray(gCommandBuffer.mData, gCommandBuffer.mLen);
    }
    gCommandBuffer.mLen = 0;
}

void store_cmd(int aOperation, int aParameter)
{
	gCommandBuffer.putByte(aOperation);
    gCommandBuffer.putByte(aParameter & 0xff);
    gCommandBuffer.putByte(aParameter >> 8);
}

void store_number_cmd(int aOperation, int aParameter1, int aParameter2)
{
    gCommandBuffer.putByte(aOperation);
    gCommandBuffer.putByte(aParameter1 & 0xff);
    gCommandBuffer.putByte(aParameter2 & 0xff);
}

void store_section(int aSection)
{
    gCommandBuffer.putByte(aSection);
}

void store_section(int aSection, int aParameter)
{
    store_cmd(aSection, aParameter);
}

void store_section(int aSection, int aParameter1, int aParameter2)
{
    gCommandBuffer.putByte(aSection);
    gCommandBuffer.putByte(aParameter1 & 0xff);
    gCommandBuffer.putByte(aParameter1 >> 8);
    gCommandBuffer.putByte(aParameter2 & 0xff);
    gCommandBuffer.putByte(aParameter2 >> 8);
}

void set_op(int aOpCode, int aValue)
{
    if (aValue > 255)
    {
        printf("Parameter value out of range, line %d\n", gLine);
        exit(-1);
    }
    if (gVerbose) printf("    Opcode: ");
    switch(aOpCode)
    {
        case OP_HAS: if (gVerbose) printf("HAS(%s)", gSymbol.mName[aValue]); break;
        case OP_NOT: if (gVerbose) printf("NOT(%s)", gSymbol.mName[aValue]); break;
        case OP_SET: if (gVerbose) printf("SET(%s)", gSymbol.mName[aValue]); break;
        case OP_CLR: if (gVerbose) printf("CLR(%s)", gSymbol.mName[aValue]); break;
        case OP_XOR: if (gVerbose) printf("XOR(%s)", gSymbol.mName[aValue]); break;
        case OP_RND: if (gVerbose) printf("RND(%d)", aValue); break;
        case OP_ATTR: if (gVerbose) printf("ATTR(%d)", aValue); break;
        case OP_EXT: if (gVerbose) printf("EXT(%d)", aValue); break;
        case OP_IATTR: if (gVerbose) printf("IATTR(%d)", aValue); break;
        case OP_DATTR: if (gVerbose) printf("DATTR(%d)", aValue); break;
        case OP_GO:  if (gVerbose) printf("GO(%s)", gSymbol.mName[aValue]); break;
        case OP_GOSUB:  if (gVerbose) printf("GOSUB(%s)", gSymbol.mName[aValue]); break;
    }
    if (gVerbose) printf("\n");
    store_cmd(aOpCode, aValue);
}

void set_number_op(int aOpCode, int aValue1, int aValue2)
{
    if (aValue1 > 255 || aValue2 > 255)
    {
        printf("Parameter value out of range, line %d\n", gLine);
        exit(-1); 
    }
    if (gVerbose) printf("    Opcode: ");
    switch(aOpCode)
    {
        case OP_GT: if (gVerbose) printf("GT(%s,%s)", gNumber.mName[aValue1], gNumber.mName[aValue2]); break;
        case OP_GTC: if (gVerbose) printf("GTC(%s,%d)", gNumber.mName[aValue1], aValue2); break;
        case OP_LT: if (gVerbose) printf("LT(%s,%s)", gNumber.mName[aValue1], gNumber.mName[aValue2]); break;
        case OP_LTC: if (gVerbose) printf("LTC(%s,%d)", gNumber.mName[aValue1], aValue2); break;
        case OP_GTE: if (gVerbose) printf("GTE(%s,%s)", gNumber.mName[aValue1], gNumber.mName[aValue2]); break;
        case OP_GTEC: if (gVerbose) printf("GTEC(%s,%d)", gNumber.mName[aValue1], aValue2); break;
        case OP_LTE: if (gVerbose) printf("LTE(%s,%s)", gNumber.mName[aValue1], gNumber.mName[aValue2]); break;
        case OP_LTEC: if (gVerbose) printf("LTEC(%s,%d)", gNumber.mName[aValue1], aValue2); break;
        case OP_EQ: if (gVerbose) printf("EQ(%s,%s)", gNumber.mName[aValue1], gNumber.mName[aValue2]); break;
        case OP_EQC: if (gVerbose) printf("EQC(%s,%d)", gNumber.mName[aValue1], aValue2); break;
        case OP_IEQ: if (gVerbose) printf("IEQ(%s,%s)", gNumber.mName[aValue1], gNumber.mName[aValue2]); break;
        case OP_IEQC: if (gVerbose) printf("IEQC(%s,%d)", gNumber.mName[aValue1], aValue2); break;
        case OP_ASSIGN: if (gVerbose) printf("ASSIGN(%s,%s)", gNumber.mName[aValue1], gNumber.mName[aValue2]); break;
        case OP_ASSIGNC: if (gVerbose) printf("ASSIGNC(%s,%d)", gNumber.mName[aValue1], aValue2); break;
        case OP_ADD: if (gVerbose) printf("ADD(%s,%s)", gNumber.mName[aValue1], gNumber.mName[aValue2]); break;
        case OP_ADDC: if (gVerbose) printf("ADDC(%s,%d)", gNumber.mName[aValue1], aValue2); break;
        case OP_SUB: if (gVerbose) printf("SUB(%s,%s)", gNumber.mName[aValue1], gNumber.mName[aValue2]); break;
        case OP_SUBC: if (gVerbose) printf("SUBC(%s,%d)", gNumber.mName[aValue1], aValue2); break;
    }
    if (gVerbose) printf("\n");
    store_number_cmd(aOpCode, aValue1, aValue2);
}

void set_go_op(int aOperation, int aValue)
{
    if (aValue >= gMaxRoomSymbol)
    {
        printf("Invalid GO%s parameter: symbol \"%s\" is not a room, line %d\n", 
            aOperation==OP_GOSUB?"SUB":"",
            gSymbol.mName[aValue],
            gLine);
        exit(-1);
    }
    set_op(aOperation, aValue);
}

void set_ext_op(int aValue, int aMaxValue)
{
    if (aValue > aMaxValue)
    {
        printf("Parameter value out of range, line %d\n", gLine);
        exit(-1);
    }
    set_op(OP_EXT, aValue);
}

void parse_op(char *aOperation)
{
    // Op may be of form "foo" "!foo" or "foo:bar" or "foo[intop]bar" where intop is <,>,<=,>=,==,!=,=,+,-
    if (aOperation[0] == 0)
    {
        printf("Syntax error (op=null), line %d\n", gLine);
        exit(-1);
    }
    if (aOperation[0] == ':' || 
		aOperation[0] == '>' || 
		aOperation[0] == '<' || 
		aOperation[0] == '=' || 
		aOperation[0] == '+' || 
		aOperation[0] == '-' || 
		(aOperation[0] == '!' && 
		aOperation[1] == '='))
    {
        printf("Syntax error (op starting with '%c') \"%s\", line %d\n", aOperation[0], aOperation, gLine);
        exit(-1);
    }

    int i = 0;
    int operations = 0;
    while (aOperation[i]) 
    { 
        if (aOperation[i] == ':' || 
            aOperation[i] == '-' || 
            aOperation[i] == '+' ||
            (aOperation[i] == '<' && aOperation[i+1] != '=') ||
            (aOperation[i] == '>' && aOperation[i+1] != '=') ||
            (aOperation[i] == '=' && aOperation[i+1] != '='))
        {
            operations++;
        }
        if ((aOperation[i] == '<' && aOperation[i+1] == '=') ||
            (aOperation[i] == '>' && aOperation[i+1] == '=') ||
            (aOperation[i] == '=' && aOperation[i+1] == '=') ||
            (aOperation[i] == '!' && aOperation[i+1] == '='))
        {
            operations++;
            i++;
        }
        
        i++; 
    }

    if (operations > 1)
    {
        printf("Syntax error (op with more than one instruction) \"%s\", line %d\n", aOperation, gLine);
        exit(-1);
    }
    
    if (operations == 0)
    {
        if (aOperation[0] == '!')
        {        
            set_op(OP_NOT, gSymbol.getId(aOperation + 1));
        }
        else
        {
            set_op(OP_HAS, gSymbol.getId(aOperation));
        }
    }
    else
    {
        char cmd[256];
        char *sym;
        i = 0;
        while (aOperation[i] != ':' && 
               aOperation[i] != '<' && 
               aOperation[i] != '>' && 
               aOperation[i] != '=' && 
               aOperation[i] != '!' && 
               aOperation[i] != '+' && 
               aOperation[i] != '-') 
        {
            cmd[i] = aOperation[i];
            i++;
        }
        cmd[i] = 0;
        if (aOperation[i] == ':')
        {
            sym = aOperation + i + 1;
    
            if (stricmp(cmd, "has") == 0) set_op(OP_HAS, gSymbol.getId(sym)); else
            if (stricmp(cmd, "need") == 0) set_op(OP_HAS, gSymbol.getId(sym)); else
            if (stricmp(cmd, "not") == 0) set_op(OP_NOT, gSymbol.getId(sym)); else
            if (stricmp(cmd, "set") == 0) set_op(OP_SET, gSymbol.getId(sym)); else
            if (stricmp(cmd, "clear") == 0) set_op(OP_CLR, gSymbol.getId(sym)); else
            if (stricmp(cmd, "clr") == 0) set_op(OP_CLR, gSymbol.getId(sym)); else
            if (stricmp(cmd, "toggle") == 0) set_op(OP_XOR, gSymbol.getId(sym)); else
            if (stricmp(cmd, "xor") == 0) set_op(OP_XOR, gSymbol.getId(sym)); else
            if (stricmp(cmd, "flip") == 0) set_op(OP_XOR, gSymbol.getId(sym)); else
            if (stricmp(cmd, "random") == 0) set_op(OP_RND, atoi(sym)); else
            if (stricmp(cmd, "rand") == 0) set_op(OP_RND, atoi(sym)); else
            if (stricmp(cmd, "rnd") == 0) set_op(OP_RND, atoi(sym)); else
            if (stricmp(cmd, "attr") == 0) set_op(OP_ATTR, atoi(sym)); else
            if (stricmp(cmd, "iattr") == 0) set_op(OP_IATTR, atoi(sym)); else
            if (stricmp(cmd, "dattr") == 0) set_op(OP_DATTR, atoi(sym)); else
            if (stricmp(cmd, "attrib") == 0) set_op(OP_ATTR, atoi(sym)); else
            if (stricmp(cmd, "iattrib") == 0) set_op(OP_IATTR, atoi(sym)); else
            if (stricmp(cmd, "dattrib") == 0) set_op(OP_DATTR, atoi(sym)); else
            if (stricmp(cmd, "color") == 0) set_op(OP_ATTR, atoi(sym)); else
            if (stricmp(cmd, "ext") == 0) set_op(OP_EXT, atoi(sym)); else
            if (stricmp(cmd, "border") == 0) set_ext_op(atoi(sym), 7); else
            if (stricmp(cmd, "cls") == 0) set_ext_op(atoi(sym)+8, 10); else
            if (stricmp(cmd, "go") == 0) set_go_op(OP_GO, gSymbol.getId(sym)); else
            if (stricmp(cmd, "goto") == 0) set_go_op(OP_GO, gSymbol.getId(sym)); else
            if (stricmp(cmd, "gosub") == 0) set_go_op(OP_GOSUB, gSymbol.getId(sym)); else
            if (stricmp(cmd, "call") == 0) set_go_op(OP_GOSUB, gSymbol.getId(sym)); else
            {
                printf("Syntax error: unknown operation \"%s\", line %d\n", cmd, gLine);
                exit(-1);
            }                
        }
        else
        {
            int first = gNumber.getId(cmd);
            // numeric op <,>,<=,>=,==,!=,=,+,-
            int v = 0;
            if (aOperation[i] == '<' && aOperation[i+1] != '=') v = OP_LT;
            if (aOperation[i] == '<' && aOperation[i+1] == '=') v = OP_LTE;
            if (aOperation[i] == '>' && aOperation[i+1] != '=') v = OP_GT;
            if (aOperation[i] == '>' && aOperation[i+1] == '=') v = OP_GTE;
            if (aOperation[i] == '=' && aOperation[i+1] != '=') v = OP_ASSIGN;
            if (aOperation[i] == '=' && aOperation[i+1] == '=') v = OP_EQ;
            if (aOperation[i] == '!' && aOperation[i+1] == '=') v = OP_IEQ;
            if (aOperation[i] == '+' && aOperation[i+1] != '=') v = OP_ADD; // allow a+b
            if (aOperation[i] == '-' && aOperation[i+1] != '=') v = OP_SUB;
            if (aOperation[i] == '+' && aOperation[i+1] == '=') v = OP_ADD; // allow a+=b
            if (aOperation[i] == '-' && aOperation[i+1] == '=') v = OP_SUB;
                
            if (v == 0)
            {
                printf("Parse error near \"%s\" (\"%s\"), line %d\n", aOperation + i, aOperation, gLine);
                exit(-1);
            }
            
            sym = aOperation + i + 1;
            if (aOperation[i+1] == '=') sym++;
            int second = 0;
            if (is_numeric(sym)) 
            {   
                v++;
                second = atoi(sym);
            }
            else
            {
                second = gNumber.getId(sym);
            }
            set_number_op(v, first, second);
        }
    }
}


void parse_statement()
{
    if (gPreviousSection == 'A' && gPreviousStringLiterals != 1)
    {
        printf("Statement A must have exactly one line of printable text (%d found)\n"
               "(Multiple lines may be caused by word wrapping; see verbose output\n"
               "to see what's going on), near line %d\n", gPreviousStringLiterals, gLine);
               exit(-1);
    }

    gPreviousStringLiterals = 0;
    
    // parse statement
    gCommandPtrOpOfs = 0;
    int i;
    char t[256];
    switch (gScratch[1])
    {
    case 'Q':
        flush_room(); // flush previous room
        token(1, gScratch, t);
        i = gSymbol.getId(t);
        gCommandPtrOpOfs = 2; // $Q + roomno
        store_section('Q', i);  
        if (gVerbose) printf("Room: \"%s\" (%d)\n", t, i);
        gPreviousSection = 'Q';
        break;
    case 'A':
        token(1, gScratch, t);
        i = gSymbol.getId(t);
        gCommandPtrOpOfs = 2; // $A + roomno
        store_section('A', i);          
        if (gVerbose) printf("Choice: %s (%d)\n", t, i);
        gPreviousSection = 'A';
        break;
    case 'P':
        if (gPreviousSection == 'A')
        {
            printf("Syntax error - statement P may not be included in statement A, line %d\n", gLine);
            exit(-1);
        }
        if (gVerbose) printf("Empty paragraph\n");        
		gDataBuffer.mLen--; // overwrite end of section
		gDataBuffer.putString(" ");
		gCommandPtrOpOfs = 10000;
		token(1, gScratch, t);
		if (t[0])
		{
			printf("Syntax error - statement P may not include any operations, line %d\n", gLine);
		}
        break;
    case 'O':
        if (gPreviousSection == 'A')
        {
            printf("Syntax error - statement O may not be included in statement A, line %d\n", gLine);
            exit(-1);
        }
        gCommandPtrOpOfs = 1; // $O
        store_section('O');
        if (gVerbose) printf("Predicated section\n");
        gPreviousSection = 'O';
        break;
    case 'I':
        if (gPreviousSection == 'A')
        {
            printf("Syntax error - statement I may not be included in statement A, line %d\n", gLine);
            exit(-1);
        }
        token(1, gScratch, t);
        gCommandPtrOpOfs = 2; // $I + imageid
        store_section('I', gImage.getId(t));
        if (gVerbose) printf("Image: \"%s\"\n", t);
        gPreviousSection = 'I';
        break;
    case 'C':
        if (gPreviousSection == 'A')
        {
            printf("Syntax error - statement C may not be included in statement A, line %d\n", gLine);
            exit(-1);
        }
        token(2, gScratch, t);
        i = strtol(t, 0, 0);
        token(1, gScratch, t);
        gCommandPtrOpOfs = 3; // $C + codeblock + HL
        store_section('C', gCode.getId(t), i);
        if (gVerbose) printf("Code: \"%s\", %d\n", t, i);
        gPreviousSection = 'C';
        break;
    default:
        printf("Syntax error: unknown statement \"%s\", line %d\n", gScratch, gLine);
        exit(-1);            
    }
    
    i = gCommandPtrOpOfs;    
    do
    {
        token(i, gScratch, t);
        if (t[0]) parse_op(t);
        i++;
    }
    while (t[0]);
}

void print_stringliteral(char *aStringLiteral)
{
	while (*aStringLiteral)
	{
		if (*aStringLiteral == 1)
		{
			aStringLiteral++; // skip 1
			printf("<<%s>>", gNumber.mName[*aStringLiteral]);
			aStringLiteral++; // skip 2, and skip 3 down there.
		}
		else
		{
			printf("%c", *aStringLiteral);
		}
		aStringLiteral++;
	}
}

void store_stringlit(char *aStringLiteral)
{
    gDataBuffer.putString(aStringLiteral);
    if (gVerbose) 
	{
		printf("  String literal: \"");
		print_stringliteral(aStringLiteral);
		printf("\"\n");
	}
	gPreviousStringLiterals++;
}

void process_wordwrap()
{
    if (gStringIdx != 0)
    {
        char temp[256];
        int c = 0;
        int width = 0;
		char *s = gStringLit;
        temp[0] = ' ';
        temp[1] = ' ';
        c = 2;
        width = gPropfontWidth[' '-32] * 2;
        while (*s)
        {
            temp[c] = *s;
            c++;
			if (*s == 1)
			{
				s++;
                temp[c] = *s;
				c++;
				s++;
                temp[c] = *s;
				c++;
				width += gPropfontWidth['8'-32] * 3; // consider number output as wide as three 8's
			}
			else
			{
				width += gPropfontWidth[*s-32];
			}
			
            if (width > 248)
            {
                c--;
                s--;
                while (temp[c] != ' ' && temp[c-1] != 1)
                {
                    c--;
                    s--;
                }
                s++;
                temp[c] = 0;
                store_stringlit(temp);
                c = 0;
                width = 0;
            }
            s++;
        }
        temp[c] = 0;
        store_stringlit(temp);
    
        gStringIdx = 0;
        gStringLit[0] = 0;
    }
}

void capture_stringlit()
{
    // capture string literal
    char *s = gScratch;
    
    if (*s == 0)
    {
        // Empty line, cut to separate string lit
        process_wordwrap();
        return;
    }
    
    int was_whitespace = 1;
    
    if (gStringIdx && *s)
    {
        gStringLit[gStringIdx] = ' ';
        gStringIdx++;
    }
    
    while (*s)
    {
        int ws = whitespace(*s);
        if (ws)
        {
            if (!was_whitespace)
            {
                ws = 0;
                *s = ' ';
            }
            was_whitespace = 1;
        }
        else
        {
            was_whitespace = 0;
        }
                
        if (!ws)
        {
			if (s[0] == '<' && s[1] == '<')
			{
				s += 2;
				char temp[256];
				// number output
				int i = 0;
				while (!whitespace(s[0]) && s[0] != 0 && !(s[0] == '>' && s[1] == '>'))
				{
					temp[i] = *s;
					s++;
					i++;
				}
				temp[i] = 0;
				if (!(s[0] == '>' && s[1] == '>'))
				{
					printf("Error parsing numeric output on line %s\n", gLine);
					exit(-1);
				}
				s++;
				gStringLit[gStringIdx] = 1;
				gStringIdx++;
				gStringLit[gStringIdx] = gNumber.getId(temp);
				gStringIdx++;
				gStringLit[gStringIdx] = '.'; // make sure there's 3 bytes for the number.
				gStringIdx++;
			}
			else
			{
				gStringLit[gStringIdx] = *s;
				gStringIdx++;
			}
        }
        
        s++;
    }
    gStringLit[gStringIdx] = 0;
}

void scan_second_pass(char *aFilename)
{
    FILE * f = fopen(aFilename, "rb");
    if (!f)
    {
        printf("File \"%s\" not found.\n", aFilename);
        exit(-1);
    }
    
	if (gVerbose)
	{
		printf("\n");
		printf("Parsing %s:\n", aFilename);
	}
    
    gStringIdx = 0;
    gStringLit[0] = 0;
    
    while (!feof(f))
    {
        read_line(gScratch, f);
        if (gScratch[0] == '$')
        {
            // process last string literal
			process_wordwrap();
			// end previous section
		    flush_sect();
            // opcode
            parse_statement();
			// Flush the command
			flush_cmd();
        }
        else
        {
            // string literal
            capture_stringlit();
        }
    }
    // process final string literal
    process_wordwrap();
    flush_sect();
    flush_room(); 
    fclose(f);

	if (gVerbose)
	{
		printf("\n");
	}

}


int tokencmp_for_qsort(const void * a, const void * b)
{
	int idx1 = *(int*)a;
	int idx2 = *(int*)b;
	return gWordCounter.mHits[idx2] - gWordCounter.mHits[idx1];
}

int findidx(int *aIndexTable, int aIndex)
{
	int c = 0;
	while (c < MAXTOKENS && aIndexTable[c] != aIndex) c++;
	return c;
}


void maketrainer()
{
    int idx[MAXTOKENS];
    int i;
    for (i = 0; i < MAXTOKENS; i++)
        idx[i] = i;
        
	qsort(idx, gWordCounter.mTokens, sizeof(int), tokencmp_for_qsort);

    if (gVerbose)
    {    
        printf("\n");
		printf("Most frequent words in source material:\n");
		int total = gWordCounter.mTokens;
		if (total > 25) total = 25;
        for (i = 0; i < total; i++)
        {
            printf("%d. \"%s\"\t(%d)\n", i, gWordCounter.mWord[idx[i]], gWordCounter.mHits[idx[i]]);
        }
		printf("\n");
		printf("Making word chains:\n");
    }
        
    int c = 0;
    int done = 0;
    i = 0;
    
    while (c < MAX_TRAINER && i < gWordCounter.mTokens)
    {
        c += strlen(gWordCounter.mWord[idx[i]]) + 1;
        i++;
    }
    int maxtoken = i;
    c = 0;
    i = 0;
    while (c < MAX_TRAINER && !done)
    {   
        if (gWordCounter.mUsed[idx[i]]) i = 0;
        while (gWordCounter.mUsed[idx[i]] && i < gWordCounter.mTokens) i++;
        if (i >= gWordCounter.mTokens) 
		{ 
			done = 1; 
			i = 0; 
		}
		else
		{
            
			char * s = gWordCounter.mWord[idx[i]];
			while (*s && c < MAX_TRAINER)
			{
				gTrainer[c] = *s;
				s++;
				c++;
			}
        
			if (c < MAX_TRAINER)
			{
				gTrainer[c] = ' ';
				c++;
			}
        
			gWordCounter.mUsed[idx[i]] = 1;
        
			// Try to chain the words together
			int min = -1;
			int mini = 0;
			int j;
			for (j = 0; j < gWordCounter.mNexts[idx[i]]; j++)
			{
				int n = gWordCounter.mNext[idx[i]][j];
				int h = gWordCounter.mNextHit[idx[i]][j];
				int nextidx = findidx(idx, n);
				if (nextidx < maxtoken && 
					h > min && 
					gWordCounter.mUsed[n] == 0)
				{
					min = h;
					mini = nextidx;
				}
			}
		
			if (gVerbose)
			{    
				printf("%s", gWordCounter.mWord[idx[i]]);
				if (min == -1)
				{
					printf(" # ");
				}
				else
				{
					printf("[%d]->", min);
				}
			}
			i = mini;
		}
    }
    gTrainers = c;

	if (gVerbose)
		printf("\n\n");
    
    if (!gQuiet)
    {
        printf(
            "Trainer data: %d bytes\n" 
            "|---------1---------2---------3---------4---------5---------6----|\n ",          
            gTrainers);
        for (c = 0; c < gTrainers; c++)
        {
            printf("%c", gTrainer[c]);
            if (c % 64 == 63) printf("\n ");
        }
        printf("\n");
    }
}

void scan_first_pass(char *aFilename)
{
    FILE * f = fopen(aFilename, "rb");
    if (!f)
    {
        printf("File \"%s\" not found.\n", aFilename);
        exit(-1);
    }
	
	int i = 0;
    while (!feof(f))
    {
        read_line(gScratch, f);
        if (gScratch[0] == '$')
        {
            char t[256];
            token(1, gScratch, t);
            if (gScratch[1] == 'Q')
            {
                i = gSymbol.getId(t);
                if (gSymbol.mHits[i] > 1)
                {
                    printf("syntax error: room id \"%s\" used more than once, line %d\n", t, gLine);
                    exit(-1);
                }
                gSymbol.mHits[i]--; // clear the hit, as it'll be scanned again
            }
            if (gScratch[1] == 'I')
            {
                i = gImage.getId(t);
                gImage.mHits[i]--; // clear the hit, as it'll be scanned again
            }
            if (gScratch[1] == 'C')
            {
                i = gCode.getId(t);
                gCode.mHits[i]--; // clear the hit, as it'll be scanned again
            }
        }
        else
        {
			// string literal
			gWordCounter.wordCount(gScratch);
        }
    }
	gMaxRoomSymbol = gSymbol.mCount;
    fclose(f);
}


void report()
{
    int i;
    printf("\n");
    printf("Token Hits Symbol\n");
	for (i = 0; i < gSymbol.mCount; i++)
    {
        printf("%5d %4d \"%s\"%s\n", i, gSymbol.mHits[i], gSymbol.mName[i], gSymbol.mHits[i] < 2 ? " <- Warning":"");
    }

	if (gNumber.mCount)
    {
	    printf("\n");
        printf("Token Hits Number\n");
		for (i = 0; i < gNumber.mCount; i++)
        {
            printf("%5d %4d \"%s\"%s\n", i, gNumber.mHits[i], gNumber.mName[i], gNumber.mHits[i] < 2 ? " <- Warning":"");
        }
    }

	if (gImage.mCount)
    {
	    printf("\n");
        printf("Token Hits Image\n");
		for (i = 0; i < gImage.mCount; i++)
        {
            printf("%5d %4d \"%s\"\n", i, gImage.mHits[i], gImage.mName[i]);
        }
    }

	if (gCode.mCount)
    {
	    printf("\n");
        printf("Token Hits Code\n");
		for (i = 0; i < gCode.mCount; i++)
        {
            printf("%5d %4d \"%s\"\n", i, gCode.mHits[i], gCode.mName[i]);
        }
    }

    //      123456789012345678901234567890123456789012345678901234567890
    printf("\n");
    printf("Memory map:\n\n");
    printf("         5         10        15        20        25      29\n");
    printf("---------.---------|---------.---------|---------.--------\n");
    int o = 0;
    for (i = 0; i < gPageData / 512; i++)
        o += printf("P");
    for (i = 0; i < gImageData / 512; i++)
        o += printf("I");
    for (i = 0; i < gCodeData / 512; i++)
        o += printf("C");
    for (i = o; i < 29*2-(gTrainers/512); i++)
        o += printf(".");
    for (i = o; i < 29*2; i++)
        o += printf("t");
    printf("\n\n");
    printf("Page data : %5d bytes\n", gPageData);
    printf("Image data: %5d bytes\n", gImageData);
    printf("Code data : %5d bytes\n", gCodeData);
    printf("Free      : %5d bytes\n", DATA_AREA_SIZE - gTrainers - gPageData - gImageData);
    printf("Trainer   : %5d bytes (used to improve compression by \"training\" it)\n", gTrainers);
        
}

int scan_image_row(int aRow, unsigned char *aData)
{
    int i, j;
    int ret = 0;
    int ref = 0;
    if (aData[yofs[aRow * 8] - 0x4000] == 0xff)
        ref = 0xff;
    for (j = 0; j < 32; j++)
    {
        int hit = 0;
        for (i = 0; i < 8; i++)
        {
            if (aData[(yofs[aRow * 8 + i] - 0x4000) + j] != ref)
            {
                hit = 1;
            }
        }
        if (hit)
        {
            ret = 1;
            if (gVerbose)
                printf("*");
        }
        else
        {
            if (gVerbose)
                printf(" ");
        }                
    }
    if (gVerbose)
        printf(" row %d, live %d\n", aRow, ret);
    return ret;
}

void process_images()
{
    int i;
    unsigned char t[6912];
	for (i = 0; i < gImage.mCount; i++)
    {
        FILE * f = fopen(gImage.mName[i], "rb");
        if (!f)
        {
            printf("Image \"%s\" not found.\n", gImage.mName[i]);
            exit(-1);
        }
        fseek(f,0,SEEK_END);
        if (ftell(f) != 6912)
        {
            printf("Image \"%s\" wrong size (has to be 6912 bytes).\n", gImage.mName[i]);
            exit(-1);
        }
        fseek(f,0,SEEK_SET);
        fread(t,1,6912,f);
        fclose(f);
        int j = 0;
        int maxlive = 0;
        for (j = 0; j < 24; j++)
        {
            if (scan_image_row(j, t))
            {
                maxlive = j;
            }
        }
        maxlive++;
        if (maxlive > 14)
        {
            printf("Warning: image \"%s\" has %d live character rows, 14 used.\n", gImage.mName[i], maxlive);
            maxlive = 14;
        }
        gDataBuffer.mLen = 0;
        gDataBuffer.putByte(maxlive * 8);
        int k;
        for (j = 0; j < 8*maxlive; j++)
            for (k = 0; k < 32; k++)
                gDataBuffer.putByte(t[yofs[j]-0x4000 + k]);
        for (j = 0; j < 32*maxlive; j++)
            gDataBuffer.putByte(t[j + 192*32]);

        if (gDataBuffer.mLen > 4096)
        {
            printf("Image %s data too large; max 4096 bytes, has %d bytes (%d lines)\n", 
				gImage.mName[i], 
				gDataBuffer.mLen, 
				maxlive);
            exit(-1);
        }
        gPack.mMax = 0;
        gPack.pack((unsigned char*)&gDataBuffer.mData[0], gDataBuffer.mLen);

        if (!gQuiet)
            printf("%25s (%02d) zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", 
				gImage.mName[i], 
				maxlive, 
				gDataBuffer.mLen, 
				gPack.mMax, 
				(gPack.mMax * 100.0f) / gDataBuffer.mLen, 
				0x5b00 + gOutBuffer.mLen);

		gOutBuffer.patchWord(0x5b00 + gOutBuffer.mLen, i + gImage.mFirstIndex);        
        gOutBuffer.putRawArray(gPack.mPackedData, gPack.mMax);            
    }
}

void process_code_blocks(char *aPath)
{
    int i;
	for (i = 0; i < gCode.mCount; i++)
    {
        if (gCode.mHits[i] > 0)
        {
            FILE * f;
            
            f = fopen(gCode.mName[i], "rb");
            if (!f)
            {
                char temp[1024];
                strcpy(temp, aPath);
                char *d = strrchr(temp, '\\');
                if (d)
                {
                    strcpy(d+1, gCode.mName[i]);
                    f = fopen(temp, "rb");
                }
            }
            if (!f)
            {
                printf("Code \"%s\" not found", gCode.mName[i]);
                exit(-1);
            }
            fseek(f,0,SEEK_END);
            int len = ftell(f);
            fseek(f,0,SEEK_SET);
            unsigned char *ihx = new unsigned char[len+1];
            fread(ihx, 1, len, f);
            fclose(f);
            
            unsigned char codebuf[65536];
            int start, end;
            int l = decode_ihx(ihx, len, codebuf, start, end, 0);
            if (l == 0)
            {
                if (!gQuiet)
                {
                    printf("Couldn't decode \"%s\" as .ihx, assuming binary\n", gCode.mName[i]);
                }
                start = 0xd000;
                l = len;
                if (len < 4096)
                    memcpy(codebuf+0xd000, ihx, len);
            }
            delete[] ihx;
            
            if (l > 4096 || start != 0xd000)
            {
                if (l > 4096)
                {
                    printf("Code %s data too large; max 4096 bytes, has %d bytes\n", gCode.mName[i], l);
                }
                if (start != 0xd000)
                {
                    printf("Code %s start address not 0xd000; 0x%04x found\n", gCode.mName[i], start);
                }
                exit(-1);
            }
    
    
            gDataBuffer.mLen = 0;
            int j;
            for (j = 0; j < l; j++)
                gDataBuffer.putByte(codebuf[0xd000 + j]);
    
            gPack.mMax = 0;
            gPack.pack((unsigned char*)&gDataBuffer.mData[0], gDataBuffer.mLen);
            gOutBuffer.patchWord(0x5b00 + gOutBuffer.mLen, i + gCode.mFirstIndex);        
    
            if (!gQuiet)
                printf("%30s zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", 
					gCode.mName[i], 
					gDataBuffer.mLen, 
					gPack.mMax, 
					(gPack.mMax * 100.0f) / gDataBuffer.mLen, 
					0x5b00 + gOutBuffer.mLen);
            gOutBuffer.putRawArray(gPack.mPackedData, gPack.mMax);            
        }
    }
}

void output(char *aFilename)
{
    FILE * f = fopen(aFilename, "wb");
    if (!f)
    {
        printf("Can't open \"%s\" for writing.\n", aFilename);
        exit(-1);
    }
    
    fwrite(gOutBuffer.mData, 1, gOutBuffer.mLen, f);
    fclose(f);
}

int find_data(unsigned char *aBuffer, int aStart, const unsigned char *aData, int aLength)
{
    int i;
    for (i = 0; i < (65535 - aStart - aLength); i++)
    {
        int found = 1;
        int j;
        for (j = 0; found && j < aLength; j++)
        {
            if (aBuffer[aStart + i + j] != aData[j])
            {
                found = 0;
            }
        }
        if (found)
        {
            return i + aStart;            
        }
    }
    printf("Can't find data to patch in crt0.ihx!\n");
    exit(-1);
    return 0;
}

void patch_data(unsigned char *aBuffer, int aStart, const unsigned char *aData, int aLength)
{
    memcpy(aBuffer + aStart, aData, aLength);
}

void patch_ihx(char *aPath)
{
    FILE * f;
    
    f = fopen("crt0.ihx", "rb");
    if (!f)
    {
        char temp[1024];
        strcpy(temp, aPath);
        char *d = strrchr(temp, '\\');
        if (d)
        {
            strcpy(d+1, "crt0.ihx");
            f = fopen(temp, "rb");
        }
    }
    if (!f)
    {
        printf("crt0.ihx not found");
        exit(-1);
    }
    fseek(f,0,SEEK_END);
    int len = ftell(f);
    fseek(f,0,SEEK_SET);
    unsigned char *ihx = new unsigned char[len+1];
    fread(ihx, 1, len, f);
    fclose(f);
    
    unsigned char codebuf[65536];
    int start, end;
    decode_ihx(ihx, len, codebuf, start, end);
    delete[] ihx;
    
    int ofs = find_data(codebuf, start, builtin_data, 94*8);
    patch_data(codebuf, ofs, gPropfontData, 94*8);
    
    ofs = find_data(codebuf, start, builtin_width, 94);
    patch_data(codebuf, ofs, gPropfontWidth, 94);
    
    ofs = find_data(codebuf, start, gDividerPattern, 8);
    patch_data(codebuf, ofs, gDividerData, 8);

    ofs = find_data(codebuf, start, gSelectorPattern, 8);
    patch_data(codebuf, ofs, gSelectorData, 8);
    
    write_ihx("patched.ihx", codebuf, start, end);
}


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


void scan_font(char *aFilename)
{   
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
    
    for (n = 0; n < 94; n++)
    {
        scan_glyph(data + 8 * 8 * n, gGlyphData[n]);
    }
    
    shift();

    gPropfontData = new unsigned char[94*8];
    gPropfontWidth = new unsigned char[94];
    
    int i, j, c;
    for (i = 0, c = 0; i < 94; i++)
    {
        for (j = 0; j < 8; j++, c++)
        {
            gPropfontData[c] = gGlyphData[i].mPixelData[j];
        }
    }

    for (i = 0; i < 94; i++)
    {
        gPropfontWidth[i] = gGlyphData[i].mXMax - gGlyphData[i].mXMin + 1 + 1;
    }
}

void scan_divider(char *aFilename)
{
    int x,y,n,i,j;
    unsigned int *data = (unsigned int*)stbi_load(aFilename, &x, &y, &n, 4);
    if (!data)
    {
        printf("unable to load \"%s\"\n", aFilename);
        exit(-1);
    }
    if (x != 8 || y != 8)
    {
        printf("Divider pattern image not 8x8 (found %dx%d)\n", x, y);
        exit(-1);
    }
    gDividerData = new unsigned char[8];
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            gDividerData[i] <<= 1;
            gDividerData[i] |= (data[i * 8 + j] & 0xff) ? 1 : 0;
        }
    }              
}

void scan_selector(char *aFilename)
{
    int x,y,n,i,j;
    unsigned int *data = (unsigned int*)stbi_load(aFilename, &x, &y, &n, 4);
    if (!data)
    {
        printf("unable to load \"%s\"\n", aFilename);
        exit(-1);
    }
    if (x != 8 || y != 8)
    {
        printf("Selector pattern image not 8x8 (found %dx%d)\n", x, y);
        exit(-1);
    }
    gSelectorData = new unsigned char[8];
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            gSelectorData[i] <<= 1;
            gSelectorData[i] |= (data[i * 8 + j] & 0xff) ? 1 : 0;
        }
    }              
}

void output_trainer()
{
    int i;
    int freebytes = DATA_AREA_SIZE - gTrainers - gOutBuffer.mLen;
    for (i = 0; i < freebytes; i++)
        gOutBuffer.putByte(0);
    gOutBuffer.putRawArray(gTrainer, gTrainers);
}

int best_compressible_room()
{
    int i, j;
    int idx = -1;
    float ratio = 1000;
    for (i = 0; i < gRooms; i++)
    {
        if (gRoom[i].mUsed == 0)
        {
            for (j = i+1; j < gRooms; j++)
            {
                if (gRoom[j].mUsed == 0)
                {
                    float r;
                    if (gCompressionResults[i * gRooms + j] == 0)
                    {
                        int total = gRoom[i].mLen + gRoom[j].mLen;
                        char tempbuf[8192];
                        memcpy(tempbuf, gRoom[i].mData, gRoom[i].mLen);
                        memcpy(tempbuf + gRoom[i].mLen, gRoom[j].mData, gRoom[j].mLen);
                        gPack.mMax = 0;
                        gPack.pack((unsigned char*)&tempbuf[0], total);
                        //printf("ZX7: %4d bytes - %3.3f%%", gPack.mMax, ((float)gPack.mMax/total)*100);
                        r = ((float)gPack.mMax / total);
                        gCompressionResults[i * gRooms + j] = r;
                    }
                    else
                    {
                        r = gCompressionResults[i * gRooms + j];
                    }
                    if (ratio > r)
                    {
                        ratio = r;
                        idx = i;
                    }                
                }
            }
        }
    }
	if (ratio == 1000)
	{
		// No compressible pairs found. Find unused one.
		for (i = 0; i < gRooms; i++)
		{
			if (gRoom[i].mUsed == 0)
			{
				idx = i;
			}
		}
	}
    return idx;
}
   

void process_rooms()
{
    int j, idx, totaldata;
    if (!gQuiet)
        printf("Compressing..\n");
    
    gCompressionResults = new float[gRooms * gRooms];
    for (j = 0; j < gRooms * gRooms; j++)
        gCompressionResults[j] = 0;
    
    //int minidx = biggest_unused_room();    
    int minidx = best_compressible_room();
    idx = minidx;
    gRoom[idx].mUsed = 1;
    gOutBuffer.patchWord(0x5b00 + gOutBuffer.mLen, idx);
	gPackBuffer.putRawArray(gRoom[idx].mData, gRoom[idx].mLen);
    printf("%s ", gRoom[idx].mName);
    totaldata = gRoom[idx].mLen;
    do 
    {
        minidx = -1;
        float minvalue = 1000;
        for (j = 0; j < gRooms; j++)
        {
            if (idx != j && gRoom[j].mUsed == 0)
            {
                int total = gRoom[idx].mLen + gRoom[j].mLen;
                if (total < 4096)
                {
                    float r;
                    if (gCompressionResults[idx * gRooms + j] == 0)
                    {
                        char tempbuf[8192];
                        memcpy(tempbuf, gRoom[idx].mData, gRoom[idx].mLen);
                        memcpy(tempbuf + gRoom[idx].mLen, gRoom[j].mData, gRoom[j].mLen);
                        gPack.mMax = 0;
                        gPack.pack((unsigned char*)&tempbuf[0], total);
                        //printf("ZX7: %4d bytes - %3.3f%%", gPack.mMax, ((float)gPack.mMax/total)*100);
                        r = ((float)gPack.mMax / total);
                        gCompressionResults[idx * gRooms + j] = r;
                    }
                    else
                    {
                        r = gCompressionResults[idx * gRooms + j];
                    }
                    if (minvalue > r)
                    {
                        minvalue = r;
                        minidx = j;
                    }
                }
            }
        }    
        
        if (minidx != -1)
        {
            totaldata += gRoom[minidx].mLen;
            if (totaldata > 4096)
            {
                flush_packbuf();
                //minidx = biggest_unused_room();
                minidx = best_compressible_room();
                totaldata = gRoom[minidx].mLen;
            }
            printf("%s ", gRoom[minidx].mName);
            gRoom[minidx].mUsed = 1;
            idx = minidx;
            
			gPackBuffer.putRawArray(gRoom[minidx].mData, gRoom[minidx].mLen);
             
            gOutBuffer.patchWord(0x5b00 + gOutBuffer.mLen, minidx); // N rooms will have same offset
        }
    } while (minidx != -1);
    
    flush_packbuf();
    int panic = 0;
    for (j = 0; j < gRooms; j++)
    {
        if (gRoom[j].mUsed == 0)
        {
            printf("ERROR Room \"%s\" didn't get included\n", gRoom[j].mName);
            panic = 1;
        }
    }
    if (panic)
        exit(-1);

    printf("\n");
    delete[] gCompressionResults;
}

void sanity_check()
{
    int i, j;
	for (i = 0; i < gSymbol.mCount; i++)
    {
		for (j = 0; j < gNumber.mCount; j++)
        {
            if (stricmp(gSymbol.mName[i], gNumber.mName[j]) == 0)
            {
                printf("Warning: Symbol \"%s\" used both as a flag and a number. This probably isn't what you wanted.\n", gSymbol.mName[i]);
            }
        }
    }

	if (gSymbol.mCount > MAX_SYMBOLS)
    {
		printf("Too many symbols in use (%d > %d)\n", gSymbol.mCount, MAX_SYMBOLS);
        exit(-1);
    }
    
	if (gNumber.mCount > MAX_NUMBERS)
    {
		printf("Too many numeric variables in use (%d > %d)\n", gNumber.mCount, MAX_NUMBERS);
        exit(-1);
    }

    if (gOutBuffer.mLen > DATA_AREA_SIZE)
    {
        printf("Total output size too large (%d bytes max, %d found)\n", DATA_AREA_SIZE, gOutBuffer.mLen);
        exit(-1);
    }
}

int main(int aParc, char **aPars)
{    
    printf("MuCho compiler, by Jari Komppa http://iki.fi/sol/\n");
    if (aParc < 3)
    {
        printf(
            "%s <input> <output> [font image [divider image [selector image]]] [flags]\n"
            "Optional flags:\n"
            "  -v   verbose (useful for debugging)\n"
            "  -q   quiet (minimal output)\n",
            aPars[0]);
        return -1;
    }
    int infile = -1;
    int outfile = -1;
    int fontfile = -1;
    int divfile = -1;
    int selfile = -1;
    int i;
    for (i = 1; i < aParc; i++)
    {
        if (aPars[i][0] == '-')
        {
            switch (aPars[i][1])
            {
                case 'v':
                case 'V':
					gQuiet = 0;
                    gVerbose = 1;
                    break;
                case 'q':
                case 'Q':
                    gQuiet = 1;
                    gVerbose = 0;
                    break;
                default:
                    printf("Unknown parameter \"%s\"\n", aPars[i]);
                    exit(-1);
            }
        }
        else
        {
            if (infile == -1) 
            {
                infile = i; 
            }
            else
            {    
                if (outfile == -1) 
                {
                    outfile = i; 
                }
                else
                {
                    if (fontfile == -1) 
                    {
                        fontfile = i; 
                    }
                    else
                    {
                        if (divfile == -1) 
                        {
                            divfile = i; 
                        }
                        else
                        {
                            if (selfile == -1) 
                            {
                                selfile = i; 
                            }
                            else
                            {
                                printf("Invalid parameter \"%s\" (input, output, font, divider and selector image files already defined)\n", aPars[i]);
                                exit(-1);
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (outfile == -1)
    {
        printf("Invalid parameters (run without params for help\n");
        exit(-1);
    }

    if (fontfile != -1)
    {
        scan_font(aPars[fontfile]);
    }

    if (divfile != -1)
    {
        scan_divider(aPars[divfile]);
    }

    if (selfile != -1)
    {
        scan_selector(aPars[selfile]);
    }

    patch_ihx(aPars[0]);

    gLine = 0;    
    scan_first_pass(aPars[infile]);

	maketrainer();    
	gPackBuffer.putRawArray(gTrainer, gTrainers);
    
	gDataBuffer.mLen = 0;
	gImage.mFirstIndex = gSymbol.mCount + 1;
	gCode.mFirstIndex = gSymbol.mCount + gImage.mCount + 1;
	// reserve space for resource pointers, 2 bytes per pointer
	gOutBuffer.mLen = gSymbol.mCount * 2 + gImage.mCount * 2 + gCode.mCount * 2 + 2;
	
	gLine = 0;    
    scan_second_pass(aPars[infile]);

	process_rooms();
    gPageData = gOutBuffer.mLen;
    
	process_images();
    gImageData = gOutBuffer.mLen - gPageData;
    
	process_code_blocks(aPars[0]);
    gCodeData = gOutBuffer.mLen - gPageData - gImageData;
    
	output_trainer();
    
	if (!gQuiet) report();

	sanity_check();
    
	output(aPars[outfile]);	

    return 0;
}

