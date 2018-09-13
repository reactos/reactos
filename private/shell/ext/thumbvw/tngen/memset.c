/***
*memset.c - set a section of memory to all one byte
*
*       Copyright (c) 1988-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       contains the memset() routine
*
*******************************************************************************/

//#include <cruntime.h>
#include <string.h>

#ifdef _MSC_VER
#pragma function(memset)
#endif  /* _MSC_VER */

/***
*char *memset(dst, val, count) - sets "count" bytes at "dst" to "val"
*
*Purpose:
*       Sets the first "count" bytes of the memory starting
*       at "dst" to the character value "val".
*
*Entry:
*       void *dst - pointer to memory to fill with val
*       int val   - value to put in dst bytes
*       size_t count - number of bytes of dst to fill
*
*Exit:
*       returns dst, with filled bytes
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl memset (
        void *dst,
        int val,
        size_t count
        )
{
        void *start = dst;

#if defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC)
        {
        extern void RtlFillMemory( void *, size_t count, char );

        RtlFillMemory( dst, count, (char)val );
        }
#else  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */
        while (count--) {
                *(char *)dst = (char)val;
                dst = (char *)dst + 1;
        }
#endif  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */

        return(start);
}
