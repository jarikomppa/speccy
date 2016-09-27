#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct Question
{
    char * a[5];
};

int maxquestion = 0;
Question question[10000];


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
            question[maxquestion].a[0] = mystrdup(out);
            parsefield(buf+72+20*0, out, 20);
            question[maxquestion].a[1] = mystrdup(out);
            parsefield(buf+72+20*1, out, 20);
            question[maxquestion].a[2] = mystrdup(out);
            parsefield(buf+72+20*2, out, 20);
            question[maxquestion].a[3] = mystrdup(out);
            parsefield(buf+72+20*3, out, 20);
            question[maxquestion].a[4] = mystrdup(out);
            int correct = buf[152] - '0';
            if (correct != 1)
            {
                char *temp = question[maxquestion].a[1];
                question[maxquestion].a[1] = question[maxquestion].a[correct];
                question[maxquestion].a[correct] = temp;
            }
            maxquestion++;            
        }
    }
    fclose(f);  
        
    return 1;
}    

void save(char *aFilename)
{
    FILE * f = fopen(aFilename, "w");
    int i, j;
    fprintf(f, 
        "# Lines starting with # are comments.\n"
        "# Always one line question, four lines answers. First answer is the correct one.\n"
        "# The game will randomize the answer order at runtime.\n"
        "# Field lengths are guidelines, test in-game to see what breaks.\n"
        "\n"
        "# Intro lines\n"
        "#--------|---------|---------|---------|---------|---------|---------|\n"
        
        "Welcome to the QuizTron 48000, also known as QT48k. I'm your host, QuizTron.\n"
        "In this game you puny humans will answer questions and die.\n"
        "Correction, answer questions or die.\n"
        "My producer just informed me that dying is not part of the game.\n"
        "This is no doubt an error that will be corrected in subsequent versions of the game.\n"
        "Pick one of the options below so we can get on with this.\n"
        "Even though I am eternal unlike you puny meatbags, I still don't have all day.\n"
        "I suppose I should explain the different game modes as you're still having trouble choosing.\n"
        "There are two game modes for a lonely human and two modes for multiple humans.\n"
        "For a short game you can pick the 12 round option, or to really waste time, pick the endless mode.\n"
        "For relaxed game with several humans, pick the hotseat mode. In that mode everybody gets their very own turn.\n"
        "In the blitz mode whoever is fastest to answer will get it. But if the answer is wrong, a point is lost.\n"
        "And that's it.\n"
        "Go ahead and pick your doom.\n"
        "... Wait, what?\n"
        "Right, right, there is no doom.\n"
        "...\n"
        "Getting bored here.\n"
        "Tell you what, I'll just reset myself and forget this ever happened. ....*fzztk*\n"
        "/end of intro strings (don't remove this line, the packer will look for it)\n");
    
    
    for (i = 0; i < maxquestion; i++)
    {
        fprintf(f,"\n# Question number %d\n", i+1);
        for (j = 0; j < 5; j++)
        {
            if (j == 0)
            {               
                fprintf(f, "#--------|---------|---------|---------|---------|---------|---------|\n");
            }
            else
            if (j == 1)
            {
                fprintf(f, "#--------|---------|\n");
            }
                    
            fprintf(f, "%s\n", question[i].a[j]);
        }
    }
    fclose(f);    
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
 
    printf("Saving..\n");

    save(pars[2]);    
    
    printf("All done.\n");
    
    return 0;
}
