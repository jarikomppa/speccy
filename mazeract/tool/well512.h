#ifndef WELL512_H_INCLUDED
#define WELL512_H_INCLUDED

class WELL512
{
public:

    // uses init_genrand to initialize the rng
    WELL512(unsigned long s = 5489UL) :
        index(0)
    {
        init_genrand(s);
    }
        
    ~WELL512()
    {
    }

    /* initializes state[N] with a seed */
    void init_genrand(unsigned long s)
    {
        int j;
        state[0]= s & 0xffffffffUL;
        for (j=1; j<16; j++) {
            state[j] = (1812433253UL * (state[j-1] ^ (state[j-1] >> 30)) + j); 
            /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
            /* In the previous versions, MSBs of the seed affect   */
            /* only MSBs of the array state[].                        */
            /* 2002/01/09 modified by Makoto Matsumoto             */
            state[j] &= 0xffffffffUL;  /* for >32 bit machines */
        }
    }
    

    /* generates a random number on [0,0xffffffff]-interval */
    unsigned long genrand_int32()
    {
      unsigned long a, b, c, d;
      a = state[index];
      c = state[(index+13)&15];
      b = a^c^(a<<16)^(c<<15);
      c = state[(index+9)&15];
      c ^= (c>>11);
      a = state[index] = b^c;
      d = a^((a<<5)&0xDA442D20UL);
      index = (index + 15)&15;
      a = state[index];
      state[index] = a^b^d^(a<<2)^(b<<18)^(c<<28);
      return state[index];
    }

private:

    unsigned long state[16]; /* the array for the state vector */
    int index;
};


#endif // !WELL512_H_INCLUDED
