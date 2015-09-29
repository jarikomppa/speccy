#include <string.h>

unsigned char fbcopy_idx;
unsigned char sin_idx;

#include "tab.h"
#include "s.h"

const unsigned char *bufp;

void fbcopy(const unsigned char * src)
{
    for (fbcopy_idx = 0; fbcopy_idx < 192; fbcopy_idx++, src+=32)
    {   
        memcpy((void*)yofs[fbcopy_idx], src, 32);
    }
}

void main()
{       
    sin_idx = 0;
    
    while(1)
    {
        sin_idx++;
        bufp = s_png;
        bufp += sinofs[sin_idx];
        fbcopy(bufp);
    }    
}
