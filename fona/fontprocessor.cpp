#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define LETTER_SPACING 1
#define SPACE_WIDTH 4

int pixheight = 8;

struct CharData
{
    int xmin, xmax;
    unsigned char pixdata[20];
};

CharData chardata[94];

void scan(unsigned int *data, CharData &parsed)
{
    parsed.xmin = 8;
    parsed.xmax = 0;
    int i, j;
    for (i = 0; i < pixheight; i++)
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
        parsed.xmax = SPACE_WIDTH-1;
    }    
}

void output(char *filename, char *varname)
{
    FILE * f = fopen(filename, "w");
    fprintf(f, "const unsigned char %s_data[94*%d] = {\n", varname, pixheight);
    int i, j;
    for (i = 0; i < 94; i++)
    {
        for (j = 0; j < pixheight; j++)
        {
            fprintf(f, "0x%02x", chardata[i].pixdata[j]);
            if (i != 93 || j != (pixheight-1)) fprintf(f, ","); else fprintf(f, " ");
            if (j != (pixheight-1)) fprintf(f, " "); else fprintf(f, " // '%c'\n", i+32);
        }
    }
    fprintf(f,"};\n\n");
    
    fprintf(f, "const unsigned char %s_width[94] = {\n", varname);
    for (i = 0; i < 94; i++)
    {
        fprintf(f, "%d", chardata[i].xmax-chardata[i].xmin+1+LETTER_SPACING);
        if (i != 93) fprintf(f, ",");
        if ((i & 15) != 15) fprintf(f, " "); else fprintf(f, "\n");        
    }
    fprintf(f,"};\n\n");
    fclose(f);    
    
}

void shift()
{
    int i, j;
    for (i = 0; i < 94; i++)
    {
        for (j = 0; j < pixheight; j++)
        {
            chardata[i].pixdata[j] <<= chardata[i].xmin;
        }
    }
}

int main(int parc, char ** pars)
{
    if (parc < 4)
    {
        printf("%s <inputfile> <outputfile> <varname>\n", pars[0]);
        return -1;
    }
    
    int x,y,n;
    unsigned int *data = (unsigned int*)stbi_load(pars[1], &x, &y, &n, 4);
    if (!data)
    {
        printf("unable to load \"%s\"\n", pars[1]);
        return -1;
    }
    printf("%s image dimensions: %dx%d\n", pars[1], x, y);
    if (x != 8 || y < 752 || y % 94)
    {
        printf("Bad image dimensions; width should be 8, height should be at least 752 (94xX), divisible by 94\n");
        return -1;
    }
    
    pixheight = y / 94;
    
    for (n = 0; n < 94; n++)
    {
        scan(data+8*pixheight*n, chardata[n]);
    }
    
    shift();
    
    output(pars[2], pars[3]);
    
    return 0;
}