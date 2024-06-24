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

int gOptNoConst = 0;

int main(int parc, char ** pars)
{
    if (parc < 3)
    {
        printf("Usage: %s infile outfile\n", pars[0]);
        exit(0);
    }
    
    int i;
    for (i = 3; i < parc; i++)
    {
        if (stricmp(pars[i], "-noconst") == 0)
        {
            gOptNoConst = 1;
        }
        else
        {
            printf("Unknown option %s\n", pars[i]);
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

    char *name = strdup(pars[1]);
    i = 0;
    while (name[i])
    {
        if (!((name[i] >= 'a' && name[i] <= 'z') ||
              (name[i] >= 'A' && name[i] <= 'Z') ||
              (name[i] >= '0' && name[i] <= '9' && i > 0)))
            name[i] = '_';
        i++;
    }

    f = fopen(pars[2], "w");
    
    fprintf(f, "#define %s_len %d\n", name, len);
    fprintf(f, "%sunsigned char %s[%s_len] = {\n", gOptNoConst?"":"const ",name, name);
    for (i = 0; i < len; i++)
    {
        fprintf(f, "0x%02x%s%s", data[i], (i == len-1) ? "" : ", ", (i & 7) == 7 ? "\n":"");
    }
    if (len & 7)
        fprintf(f, "\n");
    fprintf(f, "};\n");
    fclose(f);
}    
