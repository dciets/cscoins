#include <utilities.h>

// Found here: https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920

#define P01     10
#define P02     100
#define P03     1000
#define P04     10000
#define P05     100000
#define P06     1000000
#define P07     10000000
#define P08     100000000
#define P09     1000000000
#define P10     10000000000
#define P11     100000000000
#define P12     1000000000000

int digits10(uint64_t v)
{
    if (v < P01) return 1;
    if (v < P02) return 2;
    if (v < P03) return 3;
    if (v < P12) {
        if (v < P08) {
            if (v < P06) {
                if (v < P04) return 4;
                return 5 + (v >= P05);
            }
            return 7 + (v >= P07);
        }
        if (v < P10) {
            return 9 + (v >= P09);
        }
        return 11 + (v >= P11);
    }
    return 12 + digits10(v / P12);
}

// Modified from here: https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920
unsigned int uint64_to_ascii(uint64_t value, unsigned char* dst)
{
    static const char digits[201] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";
    uint32_t const length = digits10(value);
    uint32_t next = length - 1;

    while (value >= 100) {
        int const i = (value % 100) * 2;
        value /= 100;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
        next -= 2;
    }

    // Handle last 1-2 digits
    if (value < 10) {
        dst[next] = '0' + (uint32_t)value;
    } else {
        int i = (uint32_t)value * 2;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
    }

    return length;
}

// Taken from here:
// http://stackoverflow.com/questions/32188370/fastest-sort-algorithm-for-millions-of-uint64-rgbz-graphics-pixels
// I have no idea why it works...
typedef union {
    struct {
        uint32_t c8[256];
        uint32_t c7[256];
        uint32_t c6[256];
        uint32_t c5[256];
        uint32_t c4[256];
        uint32_t c3[256];
        uint32_t c2[256];
        uint32_t c1[256];
    };
    uint32_t counts[256 * 8];
} rscounts_t;

void radixsort_uint64_array(uint64_t* array, int size) {
    rscounts_t counts;
    memset(&counts, 0, 256 * 8 * sizeof(uint32_t));
    uint64_t* cpy = malloc(size * sizeof(uint64_t));
    uint32_t o8=0, o7=0, o6=0, o5=0, o4=0, o3=0, o2=0, o1=0;
    uint32_t t8, t7, t6, t5, t4, t3, t2, t1;
    uint32_t x;

    // calculate counts
    for(x = 0; x < size; x++) {
        t8 = array[x] & 0xff;
        t7 = (array[x] >> 8) & 0xff;
        t6 = (array[x] >> 16) & 0xff;
        t5 = (array[x] >> 24) & 0xff;
        t4 = (array[x] >> 32) & 0xff;
        t3 = (array[x] >> 40) & 0xff;
        t2 = (array[x] >> 48) & 0xff;
        t1 = (array[x] >> 56) & 0xff;
        counts.c8[t8]++;
        counts.c7[t7]++;
        counts.c6[t6]++;
        counts.c5[t5]++;
        counts.c4[t4]++;
        counts.c3[t3]++;
        counts.c2[t2]++;
        counts.c1[t1]++;
    }

    // convert counts to offsets
    for(x = 0; x < 256; x++) {
        t8 = o8 + counts.c8[x];
        t7 = o7 + counts.c7[x];
        t6 = o6 + counts.c6[x];
        t5 = o5 + counts.c5[x];
        t4 = o4 + counts.c4[x];
        t3 = o3 + counts.c3[x];
        t2 = o2 + counts.c2[x];
        t1 = o1 + counts.c1[x];
        counts.c8[x] = o8;
        counts.c7[x] = o7;
        counts.c6[x] = o6;
        counts.c5[x] = o5;
        counts.c4[x] = o4;
        counts.c3[x] = o3;
        counts.c2[x] = o2;
        counts.c1[x] = o1;
        o8 = t8; 
        o7 = t7; 
        o6 = t6; 
        o5 = t5; 
        o4 = t4; 
        o3 = t3; 
        o2 = t2; 
        o1 = t1;
    }

    // radix
    for(x = 0; x < size; x++) {
        t8 = array[x] & 0xff;
        cpy[counts.c8[t8]] = array[x];
        counts.c8[t8]++;
    }

    for(x = 0; x < size; x++) {
        t7 = (cpy[x] >> 8) & 0xff;
        array[counts.c7[t7]] = cpy[x];
        counts.c7[t7]++;
    }

    for(x = 0; x < size; x++) {
        t6 = (array[x] >> 16) & 0xff;
        cpy[counts.c6[t6]] = array[x];
        counts.c6[t6]++;
    }

    for(x = 0; x < size; x++) {
        t5 = (cpy[x] >> 24) & 0xff;
        array[counts.c5[t5]] = cpy[x];
        counts.c5[t5]++;
    }

    for(x = 0; x < size; x++) {
        t4 = (array[x] >> 32) & 0xff;
        cpy[counts.c4[t4]] = array[x];
        counts.c4[t4]++;
    }

    for(x = 0; x < size; x++) {
        t3 = (cpy[x] >> 40) & 0xff;
        array[counts.c3[t3]] = cpy[x];
        counts.c3[t3]++;
    }

    for(x = 0; x < size; x++) {
        t2 = (array[x] >> 48) & 0xff;
        cpy[counts.c2[t2]] = array[x];
        counts.c2[t2]++;
    }

    for(x = 0; x < size; x++) {
        t1 = (cpy[x] >> 56) & 0xff;
        array[counts.c1[t1]] = cpy[x];
        counts.c1[t1]++;
    }

    free(cpy);
}

typedef uint64_t type;                                     /* array type */
#define MAX 64            /* stack size for max 2^(64/2) array elements  */

void quicksort_uint64_array(type* array, int len) {
   unsigned left = 0, stack[MAX], pos = 0, seed = rand();
   for ( ; ; ) {                                           /* outer loop */
      for (; left+1 < len; len++) {                /* sort left to len-1 */
         if (pos == MAX) len = stack[pos = 0];  /* stack overflow, reset */
         type pivot = array[left+seed%(len-left)];  /* pick random pivot */
         seed = seed*69069+1;                /* next pseudorandom number */
         stack[pos++] = len;                    /* sort right part later */
         for (unsigned right = left-1; ; ) { /* inner loop: partitioning */
            while (array[++right] < pivot);  /* look for greater element */
            while (pivot < array[--len]);    /* look for smaller element */
            if (right >= len) break;           /* partition point found? */
            type temp = array[right];
            array[right] = array[len];                  /* the only swap */
            array[len] = temp;
         }                            /* partitioned, continue left part */
      }
      if (pos == 0) break;                               /* stack empty? */
      left = len;                             /* left to right is sorted */
      len = stack[--pos];                      /* get next range to sort */
   } 
}

void bin2hex(unsigned char* bytes, unsigned char* str, int nBytes)
{
    const char * hex = "0123456789abcdef";
    unsigned char* in = bytes;

    int i = 0;
    for(; in < bytes+nBytes; in++)
    {
        str[i++] = hex[(*in>>4) & 0xF];
        str[i++] = hex[*in & 0xF];
    }
}

// seed generator for parallel prng (probably not the best, but will probably
// work just fine.
long seedgen() 
{
    long s, seed, pid;

    pid = getpid();
    s = time (NULL);

    seed = abs(((s*181)*((pid-83)*359))%104729); 
    return seed;
}


