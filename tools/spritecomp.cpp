/*
 * Part of Jari Komppa's zx spectrum suite
 * https://github.com/jarikomppa/speccy
 * released under the unlicense, see http://unlicense.org 
 * (practically public domain)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
        printf("Usage: %s pngfile outfile\n", pars[0]);
        return 0;
    }
    stbi_uc * raw = stbi_load(pars[1], &x, &y, &comp, 4);
    printf("%s - %d %d %d\n", pars[1], x, y, comp);
    unsigned int *p = (unsigned int *)raw;
    
    int i, j, c;
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

    FILE * f = fopen(pars[2],"w");

/*
void drawsprite(short aX, short aY)
{    
    aData; aX; aY;
     
    __asm
        push ix
    	ld	ix,#0
    	add	ix,sp
    
        push iy
        push hl
        push bc
        push de
       
        ld d, 7 (ix) ; aY
        ld e, 6 (ix) 
        ld hl, #(_yofs)
        add hl, de      
        add hl, de      
        push hl
        pop iy
                
        ld h, 5 (ix) ; aX
        ld l, 4 (ix)
        ; divide aX by 8
        srl h
        rr l
        srl h
        rr l
        srl h
        rr l
        push hl
        pop bc
              
#define HPASS  \
        ld a, (hl)  \
        and XX    \
        or XX     \
        ld (hl),a   \
        inc hl      \
                    \
        ld a, (hl)  \
        and XX    \
        or XX     \
        ld (hl),a   \
        inc hl      \
                    \
        ld a, (hl)  \
        and XX    \
        or XX     \
        ld (hl),a   \
        inc hl

        ld l, 0 (iy)
        ld h, 1 (iy)
        add hl, bc
        HPASS
        ld l, 2 (iy)
        ld h, 3 (iy)
        add hl, bc
        HPASS
...
        ld l, 30 (iy)
        ld h, 31 (iy)
        add hl, bc
        HPASS
#undef HPASS        
        pop de
        pop bc
        pop hl
        pop iy
        pop ix            
    __endasm;   
}

*/
   
    int shift;
    for (shift = 0; shift < 8; shift++)
    {
        fprintf(f, 
        "void drawsprite_bubble_%d(short aX, short aY)\n"
        "{\n"
        "  aX; aY;\n", shift);
        
        fprintf(f,        
    "__asm\n"
    "    push ix\n"
    " 	ld	ix,#0\n"
    "	add	ix,sp\n"
    "\n"
    "   push iy\n"
    "   push hl\n"
    "   push bc\n"
    "   push de\n"
    "\n"
    "    ld d, 7 (ix) ; aY\n"
    "    ld e, 6 (ix) \n"
    "    ld hl, #(_yofs)\n"
    "    add hl, de\n"
    "    add hl, de\n"      
    "    push hl\n"
    "    pop iy\n"
    "\n"
    "    ld h, 5 (ix) ; aX\n"
    "    ld l, 4 (ix)\n"
    "    ; divide aX by 8\n"
    "    srl h\n"
    "    rr l\n"
    "    srl h\n"
    "    rr l\n"
    "    srl h\n"
    "    rr l\n"
    "    push hl\n"
    "    pop bc\n"
    "\n");
             
        for (i = 0; i < y; i++)
        {
            fprintf(f,
            "\n"
            "    ld l, %d (iy)\n"
            "    ld h, %d (iy)\n"
            "    add hl, bc\n", i*2, i*2 + 1);
            for (j = 0; j < (x/8)+1; j++)
            {
                int andval = (~((spritemask[i]) >> shift) >> (8 * (3 - j))) & 0xff;
                int orval = ((spritedata[i] >> shift) >> (8 * (3 - j))) & 0xff;
                if (andval == 0)
                {
                    fprintf(f,"    ld (hl), #%d ; 100%% mask shortcut\n", orval);
                }
                else
                {
                    if (andval!=255) fprintf(f,"    ld a, (hl)\n"); //else fprintf(f,"; skip - andval=255\n");
                    if (andval!=255) fprintf(f,"    and #%d\n", andval); //else fprintf(f,"; skip - andval=255\n");
                    if (orval)       fprintf(f,"    or #%d\n", orval); //else fprintf(f,"; skip - orval=0\n");
                    if (andval!=255) fprintf(f,"    ld (hl), a\n");  //else fprintf(f,"; skip - andval=255\n");
                }
                // find out if this span has more live masks
                // (this could be done in a less heavy-handed way, but, meh)
                int k, n = 0;
                for (k = j+1; k < (x/8)+1; k++)
                {
                    int andval = (~((spritemask[k]) >> shift) >> (8 * (3 - k))) & 0xff;
                    if (andval != 255)
                        {
                        n = 1;
                        //fprintf(f,"; found future span j=%d k=%d andval=%d spritemask=%0x\n", j, k, andval, spritemask[k]);
                    }
                }
                if (n)               fprintf(f,"    inc hl\n");
            }
        }
        fprintf(f,
        "\n"
        "    pop de\n"
        "    pop bc\n"
        "    pop hl\n"
        "    pop iy\n"
        "    pop ix\n"
        "__endasm;\n"
        "}\n"
        "\n");                
    }

    fprintf(f, 
    "void drawsprite_bubble(short aX, short aY)\n"
    "{\n"
    "  switch (aX & 7)\n"
    "  {\n"
    "  case 0: drawsprite_bubble_0(aX, aY); break; \n"
    "  case 1: drawsprite_bubble_1(aX, aY); break; \n"
    "  case 2: drawsprite_bubble_2(aX, aY); break; \n"
    "  case 3: drawsprite_bubble_3(aX, aY); break; \n"
    "  case 4: drawsprite_bubble_4(aX, aY); break; \n"
    "  case 5: drawsprite_bubble_5(aX, aY); break; \n"
    "  case 6: drawsprite_bubble_6(aX, aY); break; \n"
    "  case 7: drawsprite_bubble_7(aX, aY); break; \n"
    "  }\n"
    "}\n"
    "\n");
   fclose(f);
    return 0;
}