
// Presumes aDst to point at a 64k buffer
int decode_ihx(unsigned char *aSrc, int aLen, unsigned char *aDst, int &aStartAddr, int &aEndAddr)
{
    int start = 0x10000;
    int end = 0;
    int line = 0;
    int idx = 0;
    int done = 0;
    while (aSrc[idx] && !done)
    {
        line++;
        int sum = 0;
        char tmp[128];
        if (aSrc[idx] != ':')
        {
            printf("decode_ihx: Parse error near line %d? (previous chars:\"%c%c%c%c%c%c\")\n", line,
                (idx<5)?'?':(aSrc[idx-5]<32)?'?':aSrc[idx-5],
                (idx<4)?'?':(aSrc[idx-4]<32)?'?':aSrc[idx-4],
                (idx<3)?'?':(aSrc[idx-3]<32)?'?':aSrc[idx-3],
                (idx<2)?'?':(aSrc[idx-2]<32)?'?':aSrc[idx-2],
                (idx<1)?'?':(aSrc[idx-1]<32)?'?':aSrc[idx-1],
                (idx<0)?'?':(aSrc[idx-0]<32)?'?':aSrc[idx-0]);
            exit(-1);
        }
        idx++;
        tmp[0] = '0';
        tmp[1] = 'x';
        tmp[2] = aSrc[idx]; idx++;
        tmp[3] = aSrc[idx]; idx++;
        tmp[4] = 0;
        int bytecount = strtol(tmp, 0, 16);
        sum += bytecount;
        tmp[2] = aSrc[idx]; idx++;
        tmp[3] = aSrc[idx]; idx++;
        tmp[4] = aSrc[idx]; idx++;
        tmp[5] = aSrc[idx]; idx++;
        tmp[6] = 0;
        int address = strtol(tmp, 0, 16);
        sum += (address >> 8) & 0xff;
        sum += (address & 0xff);
        tmp[2] = aSrc[idx]; idx++;
        tmp[3] = aSrc[idx]; idx++;
        tmp[4] = 0;
        int recordtype = strtol(tmp, 0, 16);
        sum += recordtype;
        switch (recordtype)
        {
            case 0:
                break;               
            case 1:
                done = 1;
                break;
            default:
                printf("decode_ihx: Unsupported record type %d\n", recordtype);
                exit(-1);
                break;
        }
        //printf("%d bytes from %d, record %d\n", bytecount, address, recordtype);
        while (bytecount)
        {
            tmp[2] = aSrc[idx]; idx++;
            tmp[3] = aSrc[idx]; idx++;
            tmp[4] = 0;
            int byte = strtol(tmp,0,16);
            sum += byte;
            aDst[address] = byte;
            if (start > address)
                start = address;
            if (end < address)
                end = address;
            address++;
            bytecount--;
        }
        tmp[2] = aSrc[idx]; idx++;
        tmp[3] = aSrc[idx]; idx++;
        tmp[4] = 0;
        int checksum = strtol(tmp,0,16);
        sum = (sum ^ 0xff) + 1;
        if (checksum != (sum & 0xff))
        {
            printf("decode_ihx: Checksum failure %02x, expected %02x\n", sum & 0xff, checksum);
            exit(-1);
        }

        while (aSrc[idx] == '\n' || aSrc[idx] == '\r') idx++;         
    }    
    aStartAddr = start;
    aEndAddr = end;
    return end - start + 1;
}

void write_ihx(char *aFilename, unsigned char *aSrc, int aStartAddr, int aEndAddr)
{
    FILE * f = fopen(aFilename, "w");
    if (!f)
    {
        printf("write_ihx: can't open \"%s\" for writing\n", aFilename);
        exit(-1);
    }
    int ofs = aStartAddr;
    while (ofs < aEndAddr)
    {
        // :aabbbbCC[dd..]ee
        // aa = byte count in data field
        // bbbb = 16bit start address
        // CC = record type, 0 for data, 1 for end of record
        // dd = data bytes
        // ee = checksum = -(sum&0xff) of all previous bytes
        int bytes = 0x20;
        if (ofs + bytes > aEndAddr) bytes = aEndAddr - ofs + 1;
        //            aa  bb CC
        fprintf(f, ":%02x%04x00",
            bytes,
            ofs);
        int checksum = bytes + (ofs >> 8) + (ofs & 0xff);
        int i;
        for (i = 0; i < bytes; i++)
        {
            //           dd
            fprintf(f, "%02x", aSrc[ofs]);
            checksum += aSrc[ofs];
            ofs++;
        }
        //           ee
        fprintf(f, "%02x\n", (-(checksum&0xff))&0xff);
    }
    //           aabbbbCCee
    fprintf(f, ":00000001FF\n");
    fclose(f);
}
