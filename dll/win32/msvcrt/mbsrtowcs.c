#include <precomp.h>
#include <stdlib.h>
#include <string.h>

typedef int mbstate_t;

size_t __cdecl mbsrtowcs(wchar_t *dst, const char **src, size_t len, mbstate_t *ps)
{
    size_t count = 0;
    const char *p = *src;
    
    if (!p) return 0;
    
    if (dst == NULL) {
        /* Count the number of wide characters */
        while (*p) {
            count++;
            p++;
        }
        return count;
    }
    
    /* Convert multibyte string to wide string */
    while (len > 0 && *p) {
        *dst++ = (wchar_t)*p++;
        len--;
        count++;
    }
    
    if (len > 0) {
        *dst = L'\0';
        *src = NULL;
    } else {
        *src = p;
    }
    
    return count;
}