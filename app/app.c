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
        //int i = fbcopy_idx;
        //void *ofs = (void*)(((((i>>0)&7)<< 3) | (((i >> 3)&7) <<0) | ((i >> 6) & 3) << 6) * 32 + 0x4000);
        //memcpy(ofs, src, 32);
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
