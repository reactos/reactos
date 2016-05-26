
#include <string.h>

#ifdef _MSC_VER
#pragma function(memset)
#endif /* _MSC_VER */

void* __cdecl memset(void* src, int val, size_t count)
{
    char *char_src = (char *)src;

    while(count>0) {
        *char_src = val;
        char_src++;
        count--;
    }
    return src;
}
