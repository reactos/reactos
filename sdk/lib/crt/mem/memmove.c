#include <string.h>

#if defined(_MSC_VER)
#pragma function(memmove)
#endif /* _MSC_VER */

/* NOTE: This code is duplicated in memcpy function */
void * __cdecl memmove(void *dest,const void *src,size_t count)
{
    char *char_dest = (char *)dest;
    char *char_src = (char *)src;

    if ((char_dest <= char_src) || (char_dest >= (char_src+count)))
    {
        /*  non-overlapping buffers */
        while(count > 0)
	{
            *char_dest = *char_src;
            char_dest++;
            char_src++;
            count--;
	}
    }
    else
    {
        /* overlaping buffers */
        char_dest = (char *)dest + count - 1;
        char_src = (char *)src + count - 1;

        while(count > 0)
	{
           *char_dest = *char_src;
           char_dest--;
           char_src--;
           count--;
	}
    }

    return dest;
}
