
#include <string.h>

#if defined(_MSC_VER) && defined(_M_ARM)
#pragma function(memchr)
#endif /* _MSC_VER */

void* memchr(const void *s, int c, size_t n)
{
    if (n)
    {
        const char *p = s;
        do {
            if (*p++ == c)
                return (void *)(p-1);
        } while (--n != 0);
    }
    return 0;
}
