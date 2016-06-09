#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
char * dict[120];
 
char * loadpascalstring(FILE * f)
{
    int len = fgetc(f);
    char * str = new char[len+1];
    fread(str, 1, len, f);
    str[len] = 0;
    return str;
}
 
int main(int parc, char ** pars)
{
    if (parc < 2)
    {
        printf("Usage: %s infile\nOutputs in stdout\n", pars[0]);
        return -1;
    }
    FILE * f = fopen(pars[1], "rb");
    
    // First, let's load the dictionary..
    int i;
    for (i = 0; i < 120; i++)
        dict[i] = loadpascalstring(f);
    
    // Next, we can start unpacking the strings
    
    while (!feof(f))
    {
        int len = fgetc(f);
        int i;
        for (i = 0; i < len; i++)
        {
            int c = fgetc(f);
            if (c & 128)
            {
                printf("%s", dict[c^128]);
            }
            else
            {
                printf("%c", c);
            }
        }
        printf("\n");
    }
    fclose(f);
    return 0;   
}