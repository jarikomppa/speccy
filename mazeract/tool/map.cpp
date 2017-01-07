#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "well512.h"

WELL512 gRand;


int map[8*8*8];
int floodmap[8*8*8];
int mapdifficulty = 0;

void printmap(int seed, int keys)
{
	int i, j, k;

	printf("// Map difficulty %d, seed %d, %d keys\n", mapdifficulty, seed, keys);
	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
		    int byte = 0;
			for (k = 0; k < 8; k++)
			{
			    byte <<= 1;
			    if (map[i*8*8+j*8+k] == 1) byte |= 1;
			}
			printf("0x%02x,", byte);
		}
	}
	for (i = 0; i < 8*8*8; i++)
	    if (map[i] == 100)
	        printf("0x%02x,0x%02x,", i&0xff, (i>>8)&0xff);
	for (i = 0; i < 8*8*8; i++)
	    if (map[i] == 3)
	        printf("0x%02x,0x%02x,", i&0xff, (i>>8)&0xff);
	for (i = 0; i < 8*8*8; i++)
	    if (map[i] == 4)
	        printf("0x%02x,0x%02x,", i&0xff, (i>>8)&0xff);
    for (i = keys; i < 5; i++)
        printf("0xff,0xff,");
	printf("\n/*\n");

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			for (k = 0; k < 8; k++)
			{
				char c;
				switch(map[j*8*8+i*8+k])
				{
				case 0: c = '_'; break;
				case 1: c = '#'; break;
				case 100: c = 's'; break;
				case 3: c = 'e'; break;
				case 4: c = 'k'; break;
				default: c = '?';
				}
				printf("%c", c);
			}
			if (j != 7)
			    printf("  ");
		    else
			    printf("\n");
		}
	}
	printf("*/\n");
}

int isValid(int placeToTry, int keys)
{
	if (map[placeToTry] != 0) return 0;
	memcpy(floodmap,map,sizeof(int)*8*8*8);
	floodmap[placeToTry] = 1;
	int changed = 1;
	int valid = keys+1; // keys and end
	int validist = 0;
	while (changed && valid>0)
	{
		changed = 0;
		int i, j, k, p;
		for (i = 0; i < 8; i++)
		for (j = 0; j < 8; j++)
		for (k = 0; k < 8; k++)
		{
			int v = floodmap[i*8*8+j*8+k];
			if (v >= 100)
			{
				v+=6; // make z transitions more expensive
#define PAYLOAD { if (floodmap[p] == 3 || floodmap[p] == 4) { valid--; validist=v-100; } floodmap[p] = v; changed=1; }
				p = (i-1)*8*8+j*8+k; if (i>0 && floodmap[p] != 1 && floodmap[p] < 100) PAYLOAD
				p = (i+1)*8*8+j*8+k; if (i<7 && floodmap[p] != 1 && floodmap[p] < 100) PAYLOAD
			    v-=5;
				p = i*8*8+(j-1)*8+k; if (j>0 && floodmap[p] != 1 && floodmap[p] < 100) PAYLOAD
				p = i*8*8+(j+1)*8+k; if (j<7 && floodmap[p] != 1 && floodmap[p] < 100) PAYLOAD
				p = i*8*8+j*8+(k-1); if (k>0 && floodmap[p] != 1 && floodmap[p] < 100) PAYLOAD
				p = i*8*8+j*8+(k+1); if (k<7 && floodmap[p] != 1 && floodmap[p] < 100) PAYLOAD
			}
		}
	}
	if (valid == 0)
	{
		mapdifficulty = validist + keys; // make keys count
		return 1;
	}
	return 0;
}

void genmap(int keys, int levels, int sparse)
{
    if (levels < 1 || levels > 8)
    {
        printf("Bad levels %d\n", levels);
        exit(0);
    }
	int a;
	mapdifficulty = 0;
	memset(map, 0, sizeof(int)*8*8*8);
	if (levels < 8)
    {
        for (a = 0; a < 8*8*8; a++)
            map[a] = 1;
    	memset(map, 0, sizeof(int)*8*8*levels);
    }	

	a = gRand.genrand_int32()%(8*8*levels);
	while (map[a] != 0) a = gRand.genrand_int32()%(8*8*levels);
	map[a] = 100;
	while (map[a] != 0) a = gRand.genrand_int32()%(8*8*levels);
	map[a] = 3;
	int b = keys;
	while (b)
	{
    	while (map[a] != 0) a = gRand.genrand_int32()%(8*8*levels);
    	map[a] = 4;
    	b--;
	}

	int i;
	for (i = 0; i < 1024 - sparse; i++)
	{
		int p = gRand.genrand_int32()%(8*8*levels);
		if (isValid(p, keys)) map[p] = 1;
	}
}

int does_map_suck()
{
    int x, y, z;
   
    // Check for single tiles
    for (z = 0; z < 8; z++)
    {
        for (y = 0; y < 8; y++)
        {
            for (x = 0; x < 8; x++)
            {
                int p = z*8*8+y*8+x;
                int c = map[p];

                // Note: single empty tiles are okay.
                if (c != 1 && c != 0)
                {
                    c = 0;
                    if (x == 0 || map[p-1] == 1) c++;
                    if (x == 7 || map[p+1] == 1) c++;
                    if (y == 0 || map[p-8] == 1) c++;
                    if (y == 7 || map[p+8] == 1) c++;
                    
                    // Single tile found, discard map.
                    if (c == 4)
                        return 1;
                }
            }
        }
    }
    
    // no obvious problems found
    return 0;
}


int main(int parc, char **pars)
{

	int wanteddiff = 0;
	mapdifficulty = 0;
	int seed = 0;
	int keys = 0;
	int levels = 0;
	int levelcount = 0;	
	while(levelcount <= 50)
	{
		while (mapdifficulty != (10+wanteddiff))
		{
    		seed++;
            gRand.init_genrand(seed);	    
            keys = gRand.genrand_int32()%6;
			genmap(keys, levels/6+1, levelcount * 12);
//			printf(".. %4d ..\r", mapdifficulty);
            if (does_map_suck())
                mapdifficulty = -1;
		}		
		printmap(seed, keys);
		wanteddiff++;
//		seed = 0;
		levels++;
		if (levels > 7*6) levels = 7*6;
		levelcount++;
		mapdifficulty = 0;
	}
	return 0;
}
