//#define DUMP_BINARY_BLOBS

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

#define DATA_AREA_SIZE 29952
#define MAX_SYMBOLS (8*256) // 2k symbols should be enough for everybody
#define MAX_NUMBERS 16
#define MAXTOKENS 1024
#define MAX_TRAINER 1024
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
    
    OP_GT,
    OP_GTC,
    OP_LT,
    OP_LTC,
    OP_GTE,
    OP_GTEC,
    OP_LTE,
    OP_LTEC,
    OP_EQ,
    OP_EQC,
    OP_IEQ,
    OP_IEQC,
    
    OP_ASSIGN,
    OP_ASSIGNC,
    OP_ADD,
    OP_ADDC,
    OP_SUB,
    OP_SUBC    
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

	void tokenRef(int cur)
	{
		int prev = mPrevToken;
		mPrevToken = cur;
		if (prev != -1)
		{
			int i;
			for (i = 0; i < mNexts[prev]; i++)
			{
				if (mNext[prev][i] == cur)
				{
					mNextHit[prev][i]++;
					return;
				}
			}
			if (mNexts[prev] < 128)
			{
				i = mNexts[prev];
				mNext[prev][i] = cur;
				mNextHit[prev][i] = 1;
				mNexts[prev]++;
			}
		}
	}


	void addWordcountToken(char *aToken)
	{    
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

unsigned char gCommandBuffer[1024];
int gCommandPtr = 0;
int gCommandPtrOpOfs = 0;

int gLine = 0;
int gFirstImageId = 0;
int gFirstCodeId = 0;

char gScratch[64 * 1024];
char gStringLit[64 * 1024];
int gStringIdx;

ZX7Pack gPack;

char gPackBuf[8192];
int gPackBufIdx = 0;

float *gCompressionResults;

struct Symbol
{
    char *mName[MAX_SYMBOLS*2];
    int mHits[MAX_SYMBOLS*2];
	int mHash[MAX_SYMBOLS*2];
	int mCount;

	Symbol()
	{
		mCount = 0;
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
				return i;
			}
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


class Buffer
{
public:
	char mData[1024*1024];
	int mLen;
	Buffer()
	{
		mLen = 0;
	}

	void putByte(unsigned char aData)
	{
		mData[mLen] = aData;
		mLen++;
	}

	void putArray(unsigned char *aData, int aLen)
	{
		putByte(aLen);
		while (aLen)
		{
			putByte(*aData);
			aLen--;
			aData++;
		}
	}

	void putString(char *aData)
	{
		putByte(strlen((const char*)aData));
		while (*aData)
		{
			if (*aData < 32 || *aData > 126)
			{
				printf("Invalid character \"%c\" found near line %d\n", *aData, gLine);
				exit(-1);
			}
			putByte(*aData);
			aData++;
		}
	}

	void patchWord(unsigned short aWord, int aPos)
	{
		mData[aPos * 2] = aWord & 0xff;
		mData[aPos * 2 + 1] = aWord >> 8;
	}
};

Buffer gDataBuffer, gOutBuffer;


int whitespace(char c)
{
    switch (c)
    {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            return 1;
    }
    return 0;
}

int is_numeric(char *s)
{
    if (!*s) return 0;
    while (*s)
    {
        if (*s < '0' || *s > '9')
            return 0;
        s++;
    }
    return 1;
}

void readrawline(char *buf, FILE * f)
{
    int i = 0;
    int c;
    do
    {
        c = fgetc(f);
        if (c == '\r')
            c = fgetc(f);
        if (c == '\t')
            c = ' ';
        if (feof(f))
            c = '\n';
        buf[i] = c;
        if (!feof(f) && c != '\n')
            i++;
    }
    while (!feof(f) && c > 31);
    
    // trim trailing spaces:
    while (i >= 0 && whitespace(buf[i])) i--;
    i++;
    
    // terminate
    buf[i] = 0;
}

void readline(char *buf, FILE * f)
{
    do
    {
        readrawline(buf, f);
        gLine++;
    }
    while (!feof(f) && buf[0] == '#' && buf[0] > 31);
}

void token(int n, char *src, char *dst)
{    
    while (n && *src)
    {
        while (*src && !whitespace(*src)) src++;
        while (*src && whitespace(*src)) src++;
        n--;
    }
    while (*src && !whitespace(*src))
    {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = 0;
}





void flush_packbuf()
{
    if (gPackBufIdx > gTrainers)
    {
        gPack.mMax = 0;
        gPack.pack((unsigned char*)&gPackBuf[0], gPackBufIdx, gTrainers);
#ifdef DUMP_BINARY_BLOBS        
        char temp[64];
        static int blobno = 0;
        sprintf(temp, "blob%02d.bin", blobno);
        FILE * f = fopen(temp, "wb");
        fwrite(gPackBuf+gTrainers,1,gPackBufIdx-gTrainers,f);
        fclose(f);
#endif
    
        if (!gQuiet)
            printf("zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", gPackBufIdx-gTrainers, gPack.mMax, ((gPack.mMax)*100.0f)/(gPackBufIdx-gTrainers), 0x5b00+gOutBuffer.mLen);
    
		gDataBuffer.putArray(gPack.mPackedData, gPack.mMax);
        gPackBufIdx = gTrainers;
    }
}

void flush_room()
{
    if (gDataBuffer.mLen > 0)
    {
        gDataBuffer.putByte(0); // add a zero byte for good measure.
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
        if (!gVerbose) printf("End of section\n");
        gDataBuffer.putByte(0);
    }
    
}

void flush_cmd()
{
    if (gCommandPtr)
    {
        int ops = (gCommandPtr-1-(gCommandPtrOpOfs-1)*2) / 3;
        if (!gVerbose) 
        {
            printf("  Command buffer '%c' (%d bytes) with %d bytes payload (%d ops) %d\n", 
            gCommandBuffer[0], 
            //commandptropofs+1,
            gCommandPtr,
            gCommandPtr-1-(gCommandPtrOpOfs-1)*2, 
            ops);
        }
                
        if (gCommandPtr > 255)
        {
            printf("Syntax error - too many operations on one statement, gLine %d\n", gLine);
            exit(-1);
        }
        gDataBuffer.putArray(gCommandBuffer, gCommandPtr);
    }
    gCommandPtr = 0;
}

void store_cmd(int op, int param)
{
    gCommandBuffer[gCommandPtr] = op; gCommandPtr++;
    gCommandBuffer[gCommandPtr] = param & 0xff; gCommandPtr++;
    gCommandBuffer[gCommandPtr] = param >> 8; gCommandPtr++;
}

void store_number_cmd(int op, int param1, int param2)
{
    gCommandBuffer[gCommandPtr] = op; gCommandPtr++;
    gCommandBuffer[gCommandPtr] = param1 & 0xff; gCommandPtr++;
    gCommandBuffer[gCommandPtr] = param2 & 0xff; gCommandPtr++;
}

void store_section(int section)
{
    flush_cmd();
    flush_sect();
    gCommandBuffer[gCommandPtr] = section; gCommandPtr++;
}

void store_section(int section, int param)
{
    flush_cmd();
    flush_sect();
    store_cmd(section, param);
}

void store_section(int section, int param, int param2)
{
    flush_cmd();
    flush_sect();
    gCommandBuffer[gCommandPtr] = section; gCommandPtr++;
    gCommandBuffer[gCommandPtr] = param & 0xff; gCommandPtr++;
    gCommandBuffer[gCommandPtr] = param >> 8; gCommandPtr++;
    gCommandBuffer[gCommandPtr] = param2 & 0xff; gCommandPtr++;
    gCommandBuffer[gCommandPtr] = param2 >> 8; gCommandPtr++;
}

void set_op(int opcode, int value)
{
    if (value > 255)
    {
        printf("Parameter value out of range, gLine %d\n", gLine);
        exit(-1);
    }
    if (!gVerbose) printf("    Opcode: ");
    switch(opcode)
    {
        case OP_HAS: if (!gVerbose) printf("HAS(%s)", gSymbol.mName[value]); break;
        case OP_NOT: if (!gVerbose) printf("NOT(%s)", gSymbol.mName[value]); break;
        case OP_SET: if (!gVerbose) printf("SET(%s)", gSymbol.mName[value]); break;
        case OP_CLR: if (!gVerbose) printf("CLR(%s)", gSymbol.mName[value]); break;
        case OP_XOR: if (!gVerbose) printf("XOR(%s)", gSymbol.mName[value]); break;
        case OP_RND: if (!gVerbose) printf("RND(%d)", value); break;
        case OP_ATTR: if (!gVerbose) printf("ATTR(%d)", value); break;
        case OP_EXT: if (!gVerbose) printf("EXT(%d)", value); break;
        case OP_IATTR: if (!gVerbose) printf("IATTR(%d)", value); break;
        case OP_DATTR: if (!gVerbose) printf("DATTR(%d)", value); break;
        case OP_GO:  if (!gVerbose) printf("GO(%s)", gSymbol.mName[value]); break;
        case OP_GOSUB:  if (!gVerbose) printf("GOSUB(%s)", gSymbol.mName[value]); break;
    }
    if (!gVerbose) printf("\n");
    store_cmd(opcode, value);
}

void set_number_op(int opcode, int value1, int value2)
{
    if (value1 > 255 || value2 > 255)
    {
        printf("Parameter value out of range, gLine %d\n", gLine);
        exit(-1); 
    }
    if (!gVerbose) printf("    Opcode: ");
    switch(opcode)
    {
        case OP_GT: if (!gVerbose) printf("GT(%s,%s)", gNumber.mName[value1], gNumber.mName[value2]); break;
        case OP_GTC: if (!gVerbose) printf("GTC(%s,%d)", gNumber.mName[value1], value2); break;
        case OP_LT: if (!gVerbose) printf("LT(%s,%s)", gNumber.mName[value1], gNumber.mName[value2]); break;
        case OP_LTC: if (!gVerbose) printf("LTC(%s,%d)", gNumber.mName[value1], value2); break;
        case OP_GTE: if (!gVerbose) printf("GTE(%s,%s)", gNumber.mName[value1], gNumber.mName[value2]); break;
        case OP_GTEC: if (!gVerbose) printf("GTEC(%s,%d)", gNumber.mName[value1], value2); break;
        case OP_LTE: if (!gVerbose) printf("LTE(%s,%s)", gNumber.mName[value1], gNumber.mName[value2]); break;
        case OP_LTEC: if (!gVerbose) printf("LTEC(%s,%d)", gNumber.mName[value1], value2); break;
        case OP_EQ: if (!gVerbose) printf("EQ(%s,%s)", gNumber.mName[value1], gNumber.mName[value2]); break;
        case OP_EQC: if (!gVerbose) printf("EQC(%s,%d)", gNumber.mName[value1], value2); break;
        case OP_IEQ: if (!gVerbose) printf("IEQ(%s,%s)", gNumber.mName[value1], gNumber.mName[value2]); break;
        case OP_IEQC: if (!gVerbose) printf("IEQC(%s,%d)", gNumber.mName[value1], value2); break;
        case OP_ASSIGN: if (!gVerbose) printf("ASSIGN(%s,%s)", gNumber.mName[value1], gNumber.mName[value2]); break;
        case OP_ASSIGNC: if (!gVerbose) printf("ASSIGNC(%s,%d)", gNumber.mName[value1], value2); break;
        case OP_ADD: if (!gVerbose) printf("ADD(%s,%s)", gNumber.mName[value1], gNumber.mName[value2]); break;
        case OP_ADDC: if (!gVerbose) printf("ADDC(%s,%d)", gNumber.mName[value1], value2); break;
        case OP_SUB: if (!gVerbose) printf("SUB(%s,%s)", gNumber.mName[value1], gNumber.mName[value2]); break;
        case OP_SUBC: if (!gVerbose) printf("SUBC(%s,%d)", gNumber.mName[value1], value2); break;
    }
    if (!gVerbose) printf("\n");
    store_number_cmd(opcode, value1, value2);
}

void set_opgo(int op, int value)
{
    if (value >= gMaxRoomSymbol)
    {
        printf("Invalid GO%s parameter: symbol \"%s\" is not a room, gLine %d\n", 
            op==OP_GOSUB?"SUB":"",
            gSymbol.mName[value],
            gLine);
        exit(-1);
    }
    set_op(op, value);
}

void set_eop(int value, int maxvalue)
{
    if (value > maxvalue)
    {
        printf("Parameter value out of range, gLine %d\n", gLine);
        exit(-1);
    }
    set_op(OP_EXT, value);
}

void parse_op(char *op)
{
    // Op may be of form "foo" "!foo" or "foo:bar" or "foo[intop]bar" where intop is <,>,<=,>=,==,!=,=,+,-
    if (op[0] == 0)
    {
        printf("Syntax error (op=null), gLine %d\n", gLine);
        exit(-1);
    }
    if (op[0] == ':' || op[0] == '>' || op[0] == '<' || op[0] == '=' || op[0] == '+' || op[0] == '-' || (op[0] == '!' && op[1] == '='))
    {
        printf("Syntax error (op starting with '%c') \"%s\", gLine %d\n", op[0], op, gLine);
        exit(-1);
    }

    int i = 0;
    int operations = 0;
    while (op[i]) 
    { 
        if (op[i] == ':' || 
            op[i] == '-' || 
            op[i] == '+' ||
            (op[i] == '<' && op[i+1] != '=') ||
            (op[i] == '>' && op[i+1] != '=') ||
            (op[i] == '=' && op[i+1] != '='))
        {
            operations++;
        }
        if ((op[i] == '<' && op[i+1] == '=') ||
            (op[i] == '>' && op[i+1] == '=') ||
            (op[i] == '=' && op[i+1] == '=') ||
            (op[i] == '!' && op[i+1] == '='))
        {
            operations++;
            i++;
        }
        
        i++; 
    }

    if (operations > 1)
    {
        printf("Syntax error (op with more than one instruction) \"%s\", gLine %d\n", op, gLine);
        exit(-1);
    }
    
    if (operations == 0)
    {
        if (op[0] == '!')
        {        
            set_op(OP_NOT, gSymbol.getId(op+1));
        }
        else
        {
            set_op(OP_HAS, gSymbol.getId(op));
        }
    }
    else
    {
        char cmd[256];
        char *sym;
        i = 0;
        while (op[i] != ':' && 
               op[i] != '<' && 
               op[i] != '>' && 
               op[i] != '=' && 
               op[i] != '!' && 
               op[i] != '+' && 
               op[i] != '-') 
        {
            cmd[i] = op[i];
            i++;
        }
        cmd[i] = 0;
        if (op[i] == ':')
        {
            sym = op+i+1;
    
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
            if (stricmp(cmd, "border") == 0) set_eop(atoi(sym), 7); else
            if (stricmp(cmd, "cls") == 0) set_eop(atoi(sym)+8, 10); else
            if (stricmp(cmd, "go") == 0) set_opgo(OP_GO,gSymbol.getId(sym)); else
            if (stricmp(cmd, "goto") == 0) set_opgo(OP_GO,gSymbol.getId(sym)); else
            if (stricmp(cmd, "gosub") == 0) set_opgo(OP_GOSUB,gSymbol.getId(sym)); else
            if (stricmp(cmd, "call") == 0) set_opgo(OP_GOSUB,gSymbol.getId(sym)); else
            {
                printf("Syntax error: unknown operation \"%s\", gLine %d\n", cmd, gLine);
                exit(-1);
            }                
        }
        else
        {
            int first = gNumber.getId(cmd);
            // numeric op <,>,<=,>=,==,!=,=,+,-
            int v = 0;
            if (op[i] == '<' && op[i+1] != '=') v = OP_LT;
            if (op[i] == '<' && op[i+1] == '=') v = OP_LTE;
            if (op[i] == '>' && op[i+1] != '=') v = OP_GT;
            if (op[i] == '>' && op[i+1] == '=') v = OP_GTE;
            if (op[i] == '=' && op[i+1] != '=') v = OP_ASSIGN;
            if (op[i] == '=' && op[i+1] == '=') v = OP_EQ;
            if (op[i] == '!' && op[i+1] == '=') v = OP_IEQ;
            if (op[i] == '+' && op[i+1] != '=') v = OP_ADD;
            if (op[i] == '-' && op[i+1] != '=') v = OP_SUB;
                
            if (v == 0)
            {
                printf("Parse error near \"%s\" (\"%s\"), gLine %d\n", op+i, op, gLine);
                exit(-1);
            }
            
            sym = op + i + 1;
            if (op[i+1] == '=') sym++;
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

int previous_section = 0;
int previous_stringlits = 0;

void parse()
{
    if (previous_section == 'A' && previous_stringlits != 1)
    {
        printf("Statement A must have exactly one line of printable text (%d found)\n"
               "(Multiple lines may be caused by word wrapping; see verbose output\n"
               "to see what's going on), near line %d\n", previous_stringlits, gLine);
               exit(-1);
    }

    previous_stringlits = 0;
    
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
        if (!gVerbose) printf("Room: \"%s\" (%d)\n", t, i);
        previous_section = 'Q';
        break;
    case 'A':
        token(1, gScratch, t);
        i = gSymbol.getId(t);
        gCommandPtrOpOfs = 2; // $A + roomno
        store_section('A', i);          
        if (!gVerbose) printf("Choice: %s (%d)\n", t, i);
        previous_section = 'A';
        break;
    case 'P':
        if (previous_section == 'A')
        {
            printf("Syntax error - statement P may not be included in statement A, gLine %d\n", gLine);
            exit(-1);
        }
        if (!gVerbose) printf("Empty paragraph\n");
        gDataBuffer.putString(" ");
        break;
    case 'O':
        if (previous_section == 'A')
        {
            printf("Syntax error - statement O may not be included in statement A, gLine %d\n", gLine);
            exit(-1);
        }
        gCommandPtrOpOfs = 1; // $O
        store_section('O');
        if (!gVerbose) printf("Predicated section\n");
        previous_section = 'O';
        break;
    case 'I':
        if (previous_section == 'A')
        {
            printf("Syntax error - statement I may not be included in statement A, gLine %d\n", gLine);
            exit(-1);
        }
        token(1, gScratch, t);
        gCommandPtrOpOfs = 2; // $I + imageid
        store_section('I', gImage.getId(t));
        if (!gVerbose) printf("Image: \"%s\"\n", t);
        previous_section = 'I';
        break;
    case 'C':
        if (previous_section == 'A')
        {
            printf("Syntax error - statement C may not be included in statement A, gLine %d\n", gLine);
            exit(-1);
        }
        token(2, gScratch, t);
        i = strtol(t, 0, 0);
        token(1, gScratch, t);
        gCommandPtrOpOfs = 3; // $C + codeblock + HL
        store_section('C', gCode.getId(t), i);
        if (!gVerbose) printf("Code: \"%s\", %d\n", t, i);
        previous_section = 'C';
        break;
    default:
        printf("Syntax error: unknown statement \"%s\", gLine %d\n", gScratch, gLine);
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

void store(char *lit)
{
    gDataBuffer.putString(lit);
    if (!gVerbose) printf("  String literal: \"%s\"\n", lit);
    previous_stringlits++;
}

void process()
{
    flush_cmd();
    if (gStringIdx != 0)
    {
        char temp[256];
//        temp[0] = 0;
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
            width += gPropfontWidth[*s-32];
            if (width > 248)
            {
                c--;
                s--;
                while (temp[c] != ' ')
                {
                    c--;
                    s--;
                }
                s++;
                temp[c] = 0;
                store(temp);
                c = 0;
                width = 0;
            }
            s++;
        }
        temp[c] = 0;
        store(temp);
    
        gStringIdx = 0;
        gStringLit[0] = 0;
    }
}

void capture()
{
    // capture string literal
    char *s = gScratch;
    
    if (*s == 0)
    {
        // Empty line, cut to separate string lit
        process();
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
            gStringLit[gStringIdx] = *s;
            gStringIdx++;
        }
        
        s++;
    }
    gStringLit[gStringIdx] = 0;
}

void scan(char *aFilename)
{
    FILE * f = fopen(aFilename, "rb");
    if (!f)
    {
        printf("File \"%s\" not found.\n", aFilename);
        exit(-1);
    }
    
    
    gStringIdx = 0;
    gStringLit[0] = 0;
    
    while (!feof(f))
    {
        readline(gScratch, f);
        if (gScratch[0] == '$')
        {
            // process last string literal
            process();
            // opcode
            parse();
        }
        else
        {
            // string literal
            capture();
        }
    }
    // process final string literal
    process();
    // flush any pending commands
    flush_cmd();
    flush_sect();
    // end with empty section
    flush_sect();
    flush_room(); 
    fclose(f);
}


int tokencmp (const void * a, const void * b)
{
	int idx1 = *(int*)a;
	int idx2 = *(int*)b;
	return gWordCounter.mHits[idx2] - gWordCounter.mHits[idx1];
}

int findidx(int *idx, int i)
{
	int c = 0;
	while (c < MAXTOKENS && idx[c] != i) c++;
	return c;
}


void maketrainer()
{
    int idx[MAXTOKENS];
    int i;
    for (i = 0; i < MAXTOKENS; i++)
        idx[i] = i;
        
	qsort(idx, gWordCounter.mTokens, sizeof(int), tokencmp);

    if (!gVerbose)
    {    
        printf("Most frequent words in source material:\n");
		int total = gWordCounter.mTokens;
		if (total > 25) total = 25;
        for (i = 0; i < total; i++)
        {
            printf("%d. \"%s\"\t(%d)\n", i, gWordCounter.mWord[idx[i]], gWordCounter.mHits[idx[i]]);
        }
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
		
			if (!gVerbose)
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
    
    // Register beepfx as a possible code block. 
    // Won't be stored if not actually in use.
    int i;
    i = gCode.getId("beepfx.ihx");
    gCode.mHits[i]--;
	
    while (!feof(f))
    {
        readline(gScratch, f);
        if (gScratch[0] == '$')
        {
            char t[256];
            token(1, gScratch, t);
            if (gScratch[1] == 'Q')
            {
                i = gSymbol.getId(t);
                if (gSymbol.mHits[i] > 1)
                {
                    printf("syntax error: room id \"%s\" used more than once, gLine %d\n", t, gLine);
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
            printf("%5d %4d \"%s\"\n", i, gNumber.mHits[i], gNumber.mName[i]);
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

	if (gCode.mCount > 1 || (gCode.mCount == 1 && gCode.mHits[0] > 0))
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

int scan_row(int row, unsigned char*d)
{
    int i, j;
    int ret = 0;
    int ref = 0;
    if (d[yofs[row*8]-0x4000] == 0xff)
        ref = 0xff;
    for (j = 0; j < 32; j++)
    {
        int hit = 0;
        for (i = 0; i < 8; i++)
        {
            if (d[(yofs[row*8+i]-0x4000)+j] != ref)
            {
                hit = 1;
            }
        }
        if (hit)
        {
            ret = 1;
            if (!gVerbose)
                printf("*");
        }
        else
        {
            if (!gVerbose)
                printf(" ");
        }                
    }
    if (!gVerbose)
        printf(" row %d, live %d\n", row, ret);
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
            if (scan_row(j, t))
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
        gDataBuffer.putByte(maxlive*8);
        int k;
        for (j = 0; j < 8*maxlive; j++)
            for (k = 0; k < 32; k++)
                gDataBuffer.putByte(t[yofs[j]-0x4000 + k]);
        for (j = 0; j < 32*maxlive; j++)
            gDataBuffer.putByte(t[j + 192*32]);

        gPack.mMax = 0;
        if (gDataBuffer.mLen > 4096)
        {
            printf("Image %s data too large; max 4096 bytes, has %d bytes (%d lines)\n", gImage.mName[i], gDataBuffer.mLen, maxlive);
            exit(-1);
        }
        gPack.pack((unsigned char*)&gDataBuffer.mData[0], gDataBuffer.mLen);
        gOutBuffer.patchWord(0x5b00 + gOutBuffer.mLen, i + gFirstImageId);        

        if (!gQuiet)
            printf("%25s (%02d) zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", gImage.mName[i], maxlive, gDataBuffer.mLen, gPack.mMax, (gPack.mMax*100.0f)/gDataBuffer.mLen, 0x5b00+gOutBuffer.mLen);
        gOutBuffer.putArray(gPack.mPackedData, gPack.mMax);            
    }
}

void process_codes(char *path)
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
                strcpy(temp, path);
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
            gOutBuffer.patchWord(0x5b00 + gOutBuffer.mLen, i + gFirstCodeId);        
    
            if (!gQuiet)
                printf("%30s zx7: %4d -> %4d (%3.3f%%), 0x%04x\n", gCode.mName[i], gDataBuffer.mLen, gPack.mMax, (gPack.mMax*100.0f)/gDataBuffer.mLen, 0x5b00+gOutBuffer.mLen);
            gOutBuffer.putArray(gPack.mPackedData, gPack.mMax);            
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

int find_data(unsigned char *buf, int start, const unsigned char *data, int bytes)
{
    int i;
    for (i = 0; i < (65535 - start - bytes); i++)
    {
        int found = 1;
        int j;
        for (j = 0; found && j < bytes; j++)
        {
            if (buf[start + i + j] != data[j])
            {
                found = 0;
            }
        }
        if (found)
        {
            return i + start;            
        }
    }
    printf("Can't find data to patch in crt0.ihx!\n");
    exit(-1);
    return 0;
}

void patch_data(unsigned char *buf, int start, const unsigned char *data, int bytes)
{
    memcpy(buf + start, data, bytes);
}

void patch_ihx(char *path)
{
    FILE * f;
    
    f = fopen("crt0.ihx", "rb");
    if (!f)
    {
        char temp[1024];
        strcpy(temp, path);
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

struct CharData
{
    int xmin, xmax;
    unsigned char pixdata[20];
};

CharData chardata[94];

void shift()
{
    int i, j;
    for (i = 0; i < 94; i++)
    {
        for (j = 0; j < 8; j++)
        {
            chardata[i].pixdata[j] <<= chardata[i].xmin;
        }
    }
}

void scan_glyph(unsigned int *data, CharData &parsed)
{
    parsed.xmin = 8;
    parsed.xmax = 0;
    int i, j;
    for (i = 0; i < 8; i++)
    {        
        parsed.pixdata[i] = 0;
        for (j = 0; j < 8; j++)
        {
            if (data[i*8+j] & 0xffffff)
            {
                if (parsed.xmin > j) parsed.xmin = j;
                if (parsed.xmax < j) parsed.xmax = j;
                parsed.pixdata[i] |= 0x80 >> j;
            }            
        }
    }
    if (parsed.xmin > parsed.xmax)
    {
        parsed.xmin = 0;
        parsed.xmax = 4-1;
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
        scan_glyph(data+8*8*n, chardata[n]);
    }
    
    shift();

    gPropfontData = new unsigned char[94*8];
    gPropfontWidth = new unsigned char[94];
    
    int i, j, c;
    for (i = 0, c = 0; i < 94; i++)
    {
        for (j = 0; j < 8; j++, c++)
        {
            gPropfontData[c] = chardata[i].pixdata[j];
        }
    }

    for (i = 0; i < 94; i++)
    {
        gPropfontWidth[i] = chardata[i].xmax-chardata[i].xmin+1+1;
    }
}

void scan_div(char *aFilename)
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

void scan_sel(char *aFilename)
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
    gOutBuffer.putArray(gTrainer, gTrainers);
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
    memcpy(gPackBuf+gPackBufIdx, gRoom[idx].mData, gRoom[idx].mLen);
    gPackBufIdx += gRoom[idx].mLen;
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
                int total = gRoom[idx].mLen+gRoom[j].mLen;
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
            
            memcpy(gPackBuf+gPackBufIdx, gRoom[minidx].mData, gRoom[minidx].mLen);
            gPackBufIdx += gRoom[minidx].mLen;
             
            gOutBuffer.patchWord(0x5b00 + gOutBuffer.mLen, minidx); // N rooms will have same offset
        }
    } while (minidx != -1);
    
    flush_packbuf();
    int panic = 0;
    for (j = 0; j < gRooms; j++)
    {
        if (gRoom[j].mUsed == 0)
        {
            printf("ERROR Room \"%s\" didn't compress with anything\n", gRoom[j].mName);
            panic = 1;
        }
    }
    if (panic)
        exit(-1);

    printf("\n");
    delete[] gCompressionResults;
}

void sanity()
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

int main(int parc, char **pars)
{    
    printf("MuCho compiler, by Jari Komppa http://iki.fi/sol/\n");
    if (parc < 3)
    {
        printf(
            "%s <input> <output> [font image [divider image [selector image]]] [flags]\n"
            "Optional flags:\n"
            "  -v   verbose (useful for debugging)\n"
            "  -q   quiet (minimal output)\n",
            pars[0]);
        return -1;
    }
    int infile = -1;
    int outfile = -1;
    int fontfile = -1;
    int divfile = -1;
    int selfile = -1;
    int i;
    for (i = 1; i < parc; i++)
    {
        if (pars[i][0] == '-')
        {
            switch (pars[i][1])
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
                    printf("Unknown parameter \"%s\"\n", pars[i]);
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
                                printf("Invalid parameter \"%s\" (input, output, font, divider and selector image files already defined)\n", pars[i]);
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
        scan_font(pars[fontfile]);
    }

    if (divfile != -1)
    {
        scan_div(pars[divfile]);
    }

    if (selfile != -1)
    {
        scan_sel(pars[selfile]);
    }

    patch_ihx(pars[0]);

    gLine = 0;    
    scan_first_pass(pars[infile]);
    maketrainer();    
    memcpy(gPackBuf, gTrainer, gTrainers);
    gPackBufIdx = gTrainers;
    gDataBuffer.mLen = 0;
	gFirstImageId = gSymbol.mCount + 1;
	gFirstCodeId = gSymbol.mCount + gImage.mCount + 1;
	gOutBuffer.mLen = gSymbol.mCount * 2 + gImage.mCount * 2 + gCode.mCount * 2 + 2;
	gLine = 0;    

    scan(pars[infile]);
    process_rooms();
    gPageData = gOutBuffer.mLen;
    process_images();
    gImageData = gOutBuffer.mLen - gPageData;
    process_codes(pars[0]);
    gCodeData = gOutBuffer.mLen - gPageData - gImageData;
    output_trainer();
    if (!gQuiet)
        report();
    sanity();
    output(pars[outfile]);

    
    return 0;
}

