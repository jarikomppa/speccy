/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
 *
*/

/* Make Visual Studio not consider fopen() deprecated */
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"

void fprintbin(FILE *f, int val)
{
    fprintf(f, "0b%d%d%d%d%d%d%d%d",
        (val & (1 << 7)) != 0,
        (val & (1 << 6)) != 0,
        (val & (1 << 5)) != 0,
        (val & (1 << 4)) != 0,
        (val & (1 << 3)) != 0,
        (val & (1 << 2)) != 0,
        (val & (1 << 1)) != 0,
        (val & (1 << 0)) != 0);
}

unsigned int spritedata[64];
unsigned int spritemask[64];

int main(int parc, char ** pars)
{    
    int x, y, comp;
    memset(spritedata,0,sizeof(int)*64);
    memset(spritemask,0,sizeof(int)*64);
    if (parc < 3)
    {
        printf("Usage: %s pngfile outfile [-1 -2 -4 -8 -n]\n"
			"Where\n"
			" -0 - Only one shifted version, no shift padding\n"
			" -1 - Only one shifted version\n"
			" -2 - Two shifted versions\n"
			" -4 - Four shifted versions\n"
			" -8 - Eight shifted versions (default)\n"
			" -n - No mask, just the sprite\n"
			, pars[0]);
        return 0;
    }
	char *infile = 0;
	char *outfile = 0;
	int shifts = 8;
	int mask = 1;
	int padding = 1;
	int i, j, c;
	for (i = 1; i < parc; i++)
	{
		if (pars[i][0] == '-')
		{
			if (pars[i][2] != 0)
			{
				printf("Invalid parameter %s\n", pars[i]);
				return -1;
			}
			switch (pars[i][1])
			{
			case '0':
				shifts = 1;
				padding = 0;
				break;
			case '1':
				shifts = 1;
				break;
			case '2':
				shifts = 2;
				break;
			case '4':
				shifts = 4;
				break;
			case '8':
				shifts = 8;
				break;
			case 'n':
				mask = 0;
				break;
			default:
				printf("Unexpected flag %s\n", pars[i]);
				return -1;
			}
		}
		else
		{
			if (infile == 0)
			{
				infile = pars[i];
			}
			else
			{
				if (outfile == 0)
				{
					outfile = pars[i];
				}
				else
				{
					printf("Unexpected non-flag parameter %s\n", pars[i]);
					return -1;
				}
			}
		}
	}

	if (infile == 0 || outfile == 0)
	{
		printf("Invalid parameters. Need at least input and output filenames.\n");
		return -1;
	}

    stbi_uc * raw = stbi_load(infile, &x, &y, &comp, 4);
    printf("%s - %d %d %d\n", infile, x, y, comp);
	if (raw == 0)
	{
		printf("File %s load failed\n", infile);
		return -1;
	}
    unsigned int *p = (unsigned int *)raw;
    
    for (i = 0, c = 0; i < y; i++)
    {
        int d = 0;
        int m = 0;
        int xout = 0;
        for (j = 0; j < x; j++, c++)
        {
            d <<= 1;
            d |= (p[c] == 0xffffffff) != 0;
            m <<= 1;
            m |= (p[c] == 0xffffffff || p[c] == 0xff000000) == 1;
            if (j % 8 == 7)
            {
                spritedata[i] |= d << ((3 - xout) * 8);
                spritemask[i] |= m << ((3 - xout) * 8);
                m = 0;
                d = 0;
                xout++;
            }        
        }
    }    

    FILE * f = fopen(outfile,"w");
	if (f == NULL)
	{
		printf("Can't open %s for writing\n", outfile);
		return -1;
	}

    char name[1024];
    i = 0;
    while (outfile[i] != '.')
    {
        name[i] = outfile[i];
		if (outfile[i] == '/' || outfile[i] == '\\')
			name[i] = '_';
        i++;
    }
    name[i] = 0;

	fprintf(f, "const unsigned char %s[(%d%s) * %d%s * %d] = {\n", name, x / 8, padding ?" + 1":"",y, mask ? " * 2" : "", shifts);
    int shift;
	int shiftadd = 8 / shifts;
    for (shift = 0; shift < 8; shift += shiftadd)
    {
        fprintf(f,"/* shift %d*/\n", shift);
        for (i = 0; i < y; i++)
        {
            for (j = 0; j < (x/8)+padding; j++)
            {
				if (mask)
				{
					fprintbin(f, ~((spritemask[i]) >> shift) >> (8 * (3 - j)));
					fprintf(f, ", ");
				}
                fprintbin(f, (spritedata[i] >> shift) >> (8 * (3 - j)));
                fprintf(f, ", ");            
            }
            fprintf(f,"\n");
        }
    }
    fprintf(f," };\n");
    fclose(f);
    return 0;
}
