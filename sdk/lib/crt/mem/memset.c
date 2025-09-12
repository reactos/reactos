/*
 * Simple, portable memset fallback
 * Correct: returns dst; uses only low 8 bits of val
 * Reasonably fast: aligns, fills by word, then tails
 */

#include <stddef.h>
#include <stdint.h>

#ifdef _MSC_VER
  #pragma function(memset)           /* prevent builtin expansion */
  #define CDECL __cdecl
#else
  #define CDECL
#endif

void * CDECL memset(void *dst, int c, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    unsigned char  uc = (unsigned char)c;

    if (n == 0) return dst;

    /* Align to machine word */
    while (((uintptr_t)d & (sizeof(size_t) - 1)) && n) {
        *d++ = uc;
        --n;
    }

    /* Fill by word */
    if (n >= sizeof(size_t)) {
        size_t pat = uc;
        pat |= (pat << 8);
        pat |= (pat << 16);
#if SIZE_MAX > 0xFFFFFFFFu
        pat |= (pat << 32);
#endif
        size_t *w = (size_t *)d;

        /* (optional tiny unroll for fewer branches) */
        while (n >= 4 * sizeof(size_t)) {
            w[0] = pat; w[1] = pat; w[2] = pat; w[3] = pat;
            w += 4;
            n -= 4 * sizeof(size_t);
        }
        while (n >= sizeof(size_t)) {
            *w++ = pat;
            n -= sizeof(size_t);
        }
        d = (unsigned char *)w;
    }

    /* Tail bytes */
    while (n--) {
        *d++ = uc;
    }

    return dst;
}
