
// copy N scanlines from linear memory to video memory
void fbcopy(const unsigned char * src, unsigned char start, unsigned char end)
{
    for (fbcopy_idx = start; fbcopy_idx < end; fbcopy_idx++, src+=32)
    {   
        memcpy((void*)yofs[fbcopy_idx], src, 32);
    }
}

unsigned char fbcopy_i_idx;
unsigned char fbcopy_i_idxtab[256];
void fbcopy_i(const unsigned char *src, unsigned char lines)
{
    while (lines)
    {
        fbcopy_idx = fbcopy_i_idxtab[fbcopy_i_idx];
        memcpy((void*)yofs[fbcopy_idx], src + fbcopy_idx*32, 32);
        lines--;
        fbcopy_i_idx++;
    }    
}
