#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct Question
{
    char * a[5];
    int l[5];
    char * c[5];
};

int maxquestion = 0;
Question question[10000];
int maxsubstring = 0;
#define MAXSUBSTRING 1000000
char *substring[MAXSUBSTRING];
int substringhit[MAXSUBSTRING];
int substringhash[MAXSUBSTRING];
int substringorder[MAXSUBSTRING];
int substringlen[MAXSUBSTRING];
int substringvalue[MAXSUBSTRING];

#define ROR(x, v) ((unsigned)(x) >> (v)) | ((unsigned)(x) << (32-(v)))

void parsefield(char *in, char *out, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        if (i != 0 && *in == ' ' && *(out-1) == ' ')
        {
            // skip
            in++;
        }
        else
        {
            *out = *in;
            out++;
            in++;
        }
    }
    *out = 0;
    while (*(out-1) == ' ')
    {
        out--;
        *out = 0;
    }
}

int parse(char * aFilename)
{
    char buf[256];
    char out[256];
    FILE * f = fopen(aFilename, "rb");
    if (!f)
        return 0;
    while (!feof(f))
    {
        memset(buf, 0, 256);
        fread(buf, 155, 1, f);
        if (buf[0])
        {
            parsefield(buf, out, 72);
            question[maxquestion].a[0] = strdup(out);
            question[maxquestion].l[0] = strlen(out);
            parsefield(buf+72+20*0, out, 20);
            question[maxquestion].a[1] = strdup(out);
            question[maxquestion].l[1] = strlen(out);
            parsefield(buf+72+20*1, out, 20);
            question[maxquestion].a[2] = strdup(out);
            question[maxquestion].l[2] = strlen(out);
            parsefield(buf+72+20*2, out, 20);
            question[maxquestion].a[3] = strdup(out);
            question[maxquestion].l[3] = strlen(out);
            parsefield(buf+72+20*3, out, 20);
            question[maxquestion].a[4] = strdup(out);
            question[maxquestion].l[4] = strlen(out);
            int correct = buf[152] - '0';
            if (correct != 1)
            {
                char *temp = question[maxquestion].a[1];
                question[maxquestion].a[1] = question[maxquestion].a[correct];
                question[maxquestion].a[correct] = temp;

                int templ = question[maxquestion].l[1];
                question[maxquestion].l[1] = question[maxquestion].l[correct];
                question[maxquestion].l[correct] = templ;

            }
            maxquestion++;            
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
    
    int i, j;
    for (i = 0; i < 120; i++)
        writepascalstring(f, substring[substringorder[i]]);
    
    for (i = 0; i < maxquestion; i++)
    {
        for (j = 0; j < 5; j++)
        {
    //        writepascalstring(f, question[i].a[j]);
            writepascalstring(f,  question[i].c[j]);
        }
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

void sort(int* base, size_t num, size_t size, int (*compar)(const void*,const void*))
{
    int i, j;
    for (i = 0; i < num; i++)
    {
        for (j = i+1; j < num; j++)
        {
            if (compar((void*)(base+i), (void*)(base+j)) > 0)
            {
                int x = *(base+i);
                *(base+i) = *(base+j);
                *(base+j) = x;
            }
        }
    }        
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
    int i, j, k, n, p;
    for (i = 0; i < maxquestion; i++)
    {
        for (j = 0; j < 5; j++)
        {
            char * str = question[i].a[j];
            int    len = question[i].l[j];
            for (k = 0; k < len - 1; k++)
            {
                int max = len + 1 - k;
                if (max > KEYWORD_MAX) max = KEYWORD_MAX;
                for (n = KEYWORD_MIN; n < max; n++)
                {
                    add_substring(str + k, n);
                }
            }
        }
        printf("\r%3d questions, %6d substrings", i+1, maxsubstring);
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
        for (i = 0; i < maxquestion; i++)
        {
            for (j = 0; j < 5; j++)
            {
                int len = question[i].l[j];
                for (k = 0; k < len - 1;)
                {
                    int max = len + 1 - k - 1;
                    if (max > KEYWORD_MAX) max = KEYWORD_MAX;
                    char *str = question[i].a[j] + k; 
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
        
        printf("Actual order, top 10: (savings in bytes) - total projected savings %d\n", total);
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
    for (i = 0; i < maxquestion; i++)
    {
        for (j = 0; j < 5; j++)
        {
            char temp[128];
            int outidx = 0;
            int len = question[i].l[j];
            for (k = 0; k < len;)
            {
                int max = len + 1 - k - 1;
                if (max > KEYWORD_MAX) max = KEYWORD_MAX;
                char *str = question[i].a[j] + k; 
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
            question[i].c[j] = strdup(temp);            
        }
    }
    printf("\n");            
}


void wrapstring(char *str, int len)
{
    //if (*str >= 'A' && *str <= 'Z') *str += 'a' - 'A';
    
    int x = 0;
    while (*str)
    {
        if (*str == ' ')
        {
            int w = 1;
            while (str[w] > ' ') w++;
            if (x+w > len)
            {
                *str = '\n';
                x = 0;
            }
        }
        str++;
        x++;
    }
}


void wordwrap()
{
    int i, j;
    for (i = 0; i < maxquestion; i++)
    {
        for (j = 0; j < 5; j++)
        {            
            wrapstring(question[i].a[j], (j==0)? 16 : 12);
        }
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
    
    return 0;
}