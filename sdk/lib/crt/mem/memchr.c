
#include <string.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1910 || !defined(_WIN64))
#pragma function(memchr)
#endif /* _MSC_VER */

void* __cdecl memchr(const void *s, int c, size_t n)
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
