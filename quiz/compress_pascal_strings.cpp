#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct String
{
    char * a;
    char * c;
};

int introstrings = 0;
int maxstring = 0;
String string[10000];
int maxsubstring = 0;
#define MAXSUBSTRING 1000000
char *substring[MAXSUBSTRING];
int substringhit[MAXSUBSTRING];
int substringhash[MAXSUBSTRING];
int substringorder[MAXSUBSTRING];
int substringlen[MAXSUBSTRING];
int substringvalue[MAXSUBSTRING];

#define ROR(x, v) ((unsigned)(x) >> (v)) | ((unsigned)(x) << (32-(v)))

char * mystrdup(char * a)
{
    if (!a) return 0;
    int len = strlen(a);
    char *str = (char*)malloc(len + 256); // allocate enough extra for our word wrap
    if (!str)
        return 0;
    memcpy(str, a, len+1);
    return str;
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
        buf[i] = c;
        if (!feof(f) && c != '\n')
            i++;
    }
    while (!feof(f) && c > 31);
    buf[i] = 0;
}

void readline(char *buf, FILE * f)
{
    do
    {
        readrawline(buf, f);
    }
    while (!feof(f) && buf[0] == '#' && buf[0] > 31);
}

int parse(char * aFilename)
{
    char buf[256];
    FILE * f = fopen(aFilename, "rb");
    if (!f)
        return 0;
    
    while (!feof(f))
    {
        memset(buf, 0, 256);
        readline(buf,f);
        if (buf[0])
        {            
            if (buf[0] == '/')
            {
                introstrings = maxstring;
            }
            else
            {                
                string[maxstring].a = mystrdup(buf);
                maxstring++;            
            }
        }
    }
    fclose(f);  
        
    return 1;
}    

// "entertainment.txt"
// raw text         77500
// pascal strings   51643
// compressed       32831
// unpacked         54145 - diff of 2502 bytes (5 strings, crlf instead of 1 byte len, 2 bytes last crlf)

void writepascalstring(FILE *f, char *aAsciiZString)
{
    unsigned char l = strlen(aAsciiZString);    
    fwrite(&l,1,1,f);
    fwrite(aAsciiZString,1,l,f);    
}

void save(char *aFilename)
{
    FILE * f = fopen(aFilename, "wb");
    unsigned char d;
    d = introstrings;
    fwrite(&d, 1, 1, f);
    d = (maxstring - introstrings) / 5;
    fwrite(&d, 1, 1, f);
    
    int i, j;
    for (i = 0; i < 120; i++)
        writepascalstring(f, substring[substringorder[i]]);
    
    for (i = 0; i < maxstring; i++)
    {
        writepascalstring(f,  string[i].c);
    }
    fclose(f);
}


void hashrun(char *src, int *dst, int max)
{
    unsigned int x = 0;
    while (max--)
    {
        x = ROR(x, 7);
        x ^= *src;        
        *dst = x;
        dst++;
        src++;
    }
}

int hash(char * src, int len)
{    
    int h[64];
    hashrun(src, h, len);
    return h[len-1];
}

void add_substring(char * ofs, int len)
{
    int i;
    char buf[200];
    memcpy(buf, ofs, len);
    buf[len] = 0;
    int h = hash(buf, len);
    
    for (i = 0; i < maxsubstring; i++)
    {
        if (substringhash[i] == h && 
            substringlen[i] == len && 
            strcmp(buf, substring[i]) == 0)
        {
            substringhit[i]++;
            return;
        }
    }
    
    if (maxsubstring < MAXSUBSTRING)
    {
        substring[maxsubstring] = strdup(buf);
        substringhit[maxsubstring] = 1;
        substringhash[maxsubstring] = h;
        substringlen[maxsubstring] = len;
        maxsubstring++;
    }
}

int sorter(const void * a, const void * b)
{
    return substringvalue[*(int*)b] - substringvalue[*(int*)a];
}


void calcvalues()
{
    int i;
    for (i = 0; i < maxsubstring; i++)
    {
        substringvalue[i] = (substringhit[i] - 1) * (substringlen[i] - 1) - 1;
    }
}

#define KEYWORD_MAX 12
#define KEYWORD_MIN 2

void compress()
{
    printf("Generating substrings..\n");
    int i, k, n, p;
    for (i = 0; i < maxstring; i++)
    {
        char * str = string[i].a;
        int    len = strlen(str);
        for (k = 0; k < len - 1; k++)
        {
            int max = len + 1 - k;
            if (max > KEYWORD_MAX) max = KEYWORD_MAX;
            for (n = KEYWORD_MIN; n < max; n++)
            {
                add_substring(str + k, n);
            }
        }
        printf("\r%3d strings, %6d substrings", i+1, maxsubstring);
    }
    printf("\n\n");
    
    calcvalues();
    
    for (i = 0; i < maxsubstring; i++)
    {
        substringorder[i] = i;  
    }
    
    qsort(substringorder, maxsubstring, sizeof(int), sorter);
    printf("Potential order, top 10: (potential savings in bytes)\n");
    for (i = 0; i < 10; i++)
    {
        printf("%2d:%6d\"%s\" \t(%d) x%d\n", 
            i+1,
            substringorder[i], 
            substring[substringorder[i]], 
            substringvalue[substringorder[i]],
            substringhit[substringorder[i]]);
    }


    int lastmax = 0;
    int total = 1;
    while (total > lastmax)
    {
        for (i = 0; i < maxsubstring; i++)
        {
            substringhit[i] = 0;  
        }

        lastmax = total;
        int collisions = 0;
        for (i = 0; i < maxstring; i++)
        {
            int len = strlen(string[i].a);
            for (k = 0; k < len - 1;)
            {
                int max = len + 1 - k - 1;
                if (max > KEYWORD_MAX) max = KEYWORD_MAX;
                char *str = string[i].a + k; 
                int h[64];
                hashrun(str, h, max);
                
                p = 0;
                int found = 0;
                do
                {
                    char *pstr =     substring[substringorder[p]];
                    int plen   =  substringlen[substringorder[p]];
                    int phash  = substringhash[substringorder[p]];
                    
                    if (plen <= max && plen-1 > 0 && phash == h[plen-1])
                    {
                        found = 1;
                        int x;
                        for (x = 0; found && x < plen; x++)
                        {
                            if (pstr[x] != str[x])
                            {
                                found = 0;
                                collisions++;
                            }
                        }
                    }
                    
                    if (found)
                    {
                        substringhit[substringorder[p]]++;
                        k += substringlen[substringorder[p]];
                    }
                    
                    p++;
                }
                while (!found && p < maxsubstring);
                
                if (!found)
                    k++;
            }
        }
        printf("\n");
        printf("Hash collisions %d\n", collisions);
    
        calcvalues();
        
        qsort(substringorder, maxsubstring, sizeof(int), sorter);
        
        total = 0;
        for (i = 0; i < 120; i++)
        {
            total += substringvalue[substringorder[i]];
        }
        
        printf("Final order, top 10: (savings in bytes) - total projected savings %d\n", total);
        for (i = 0; i < 10; i++)
        {
            printf("%2d:%6d:\"%s\" \t(%d) x%d\n", 
                i+1, 
                substringorder[i], 
                substring[substringorder[i]],  
                substringvalue[substringorder[i]],
                substringhit[substringorder[i]]);
        }
    }
    
    printf("Final compression pass..\n");
    for (i = 0; i < maxstring; i++)
    {
        char temp[128];
        int outidx = 0;
        int len = strlen(string[i].a);
        for (k = 0; k < len;)
        {
            int max = len + 1 - k - 1;
            if (max > KEYWORD_MAX) max = KEYWORD_MAX;
            char *str = string[i].a + k; 
            int h[64];
            hashrun(str, h, max);
            
            p = 0;
            int found = 0;
            do
            {
                char *pstr =     substring[substringorder[p]];
                int plen   =  substringlen[substringorder[p]];
                int phash  = substringhash[substringorder[p]];
                
                if (plen <= max && phash == h[plen-1])
                {
                    found = 1;
                    int x;
                    for (x = 0; found && x < plen; x++)
                    {
                        if (pstr[x] != str[x])
                        {
                            found = 0;
                        }
                    }
                }
                
                if (found)
                {
                    temp[outidx++] = p+128;
                    substringhit[substringorder[p]]++;
                    k += substringlen[substringorder[p]];
                }
                
                p++;
            }
            while (!found && p < 120);
            
            if (!found)
            {   
                temp[outidx++] = *str;
                k++;
            }
        }
        temp[outidx] = 0;
        string[i].c = strdup(temp);            
    }
    printf("\n");            
}


int try_wrapstring(char *str, int maxlen, int maxrows)
{
    char *str0 = str;
    int x = 0;
    int y = 0;
    while (*str)
    {
        if (x >= maxlen)
        {
            while (str != str0 && *str != ' ' && *str != '\t') 
                str--;
            if (str == str0)
                return 0;
            *str = '\n';
            x = 0;
            y++;            
        }
        str++;
        if (*str != '\t')
        x++;
    }
    if (y < maxrows)
        return 1;
    return 0;
}

void wrap_direct(char *src, char *dst)
{
    while (*src)
    {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = 0;
}

int myisalpha(char c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) 
        return 1;
    return 0;
}

void wrap_nice(char *src, char *dst)
{
    while (*src)
    {
        *dst = *src;
        if (*src == '-' || 
            *src == '.' ||
            *src == ',' ||
            *src == '+' ||
            *src == '/' ||
            *src == '&' ||
            *src == '@' ||
            *src == '?' ||
            *src == '!' ||
            *src == '<' ||
            *src == '>' ||
            *src == ';' ||
            *src == ':' ||
            *src == '%' ||
            *src == '*' ||
            !myisalpha(*src) && myisalpha(*(src+1)) ||
            myisalpha(*src) && !myisalpha(*(src+1))
            )
        {
            // Insert potential word break position
            dst++;
            *dst = '\t';
        }
        dst++;
        src++;
    }
    *dst = 0;
}

void wrap_brute(char *src, char *dst)
{
    while (*src)
    {
        *dst = *src;
        if (*src != ' ')
        {
            // Break anywhere (except after space)
            dst++;
            *dst = '\t';
        }
        dst++;
        src++;
    }
    *dst = 0;
}

void wrapstring(char *str, int len, int maxrows)
{
    //if (*str >= 'A' && *str <= 'Z') *str += 'a' - 'A';
    char workstring[1024];
    int q = 0;
    wrap_direct(str, workstring);
    if (try_wrapstring(workstring, len, maxrows) == 0)
    {
        q = 1;
        printf("? \"%s\"\n", str);
        printf("? Direct wrap to %d/%d didn't work, trying nice.. \n", len, maxrows);
        wrap_nice(str, workstring);
        if (try_wrapstring(workstring, len, maxrows) == 0)
        {
            printf("? Nice wrap to %d/%d didn't work, trying brute..\n", len, maxrows);
            wrap_brute(str, workstring);
            if (try_wrapstring(workstring, len, maxrows) == 0)
            {
                printf("!!!! unable to wrap string even with brute force\n");
            }
        }
    }

    char *s = workstring;
    char *d = str;
    
    while (*s)
    {
        if (*s == '\t')
        {
            // skip
            s++;
        }
        else
        {
            *d = *s;
            s++;
            d++;
        }
    }    
    *d = 0;    
    if (q)
    {
        printf("\"%s\"\n", str);
    }
}

void wordwrap()
{
    int i, j;
    for (i = 0; i < maxstring; i++)
    {
        int q = (((i - introstrings) % 5) == 0) || (i < introstrings);
        wrapstring(
            string[i].a, 
            q ? 16 : 12, 
            q ? 9 : 2);
    }
}


int main(int parc, char **pars)
{
    if (parc < 3)
    {
        printf("Usage: %s infile outfile\n", pars[0]);
        return -1;
    }
    
    printf("Parsing..\n");

    if (!parse(pars[1]))
        return -1;   
 
    printf("Word wrap..\n");
 
    wordwrap();
   
    printf("Compressing..\n");
   
    compress(); 

    printf("Saving..\n");

    save(pars[2]);    
    
    printf("All done.\n");
 
    if (introstrings == 0 || introstrings > 255)
    {
        printf("Warning: Invalid number of intro strings (%d)\n", introstrings);
    }
    
    if ((maxstring - introstrings) / 5 == 0 || (maxstring - introstrings) / 5 > 255)
    {
        printf("Warning: Invalid number of questions (%d)\n", (maxstring - introstrings) / 5);
    }
    
    printf("Stats:\n"
           "Intro strings: %d\n"
           "Questions    : %d\n"
           "Sanity       : %s\n",
           introstrings,
           (maxstring - introstrings) / 5,
           (((maxstring - introstrings) / 5) * 5 + introstrings == maxstring) ? "Yep" : "Nope, expect trouble");
    
    return 0;
}