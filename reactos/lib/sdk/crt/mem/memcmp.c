
#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable: 4164)
#pragma function(memcmp)
#endif

int __cdecl memcmp(const void *s1, const void *s2, size_t n)
{
    if (n != 0) {
        const unsigned char *p1 = s1, *p2 = s2;
        do {
            if (*p1++ != *p2++)
                return (*--p1 - *--p2);
        } while (--n != 0);
    }
    return 0;
}
