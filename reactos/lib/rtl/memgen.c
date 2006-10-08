/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/mem.c
 * PURPOSE:         Memory functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#undef RtlUlonglongByteSwap
#undef RtlUlongByteSwap
#undef RtlUshortByteSwap

/*************************************************************************
 * RtlUshortByteSwap
 *
 * Swap the bytes of an unsigned short value.
 *
 * NOTES
 * Based on the inline versions in Wine winternl.h
 *
 * @implemented
 */
USHORT FASTCALL
RtlUshortByteSwap (IN USHORT Source)
{
   return (Source >> 8) | (Source << 8);
}



/*************************************************************************
 * RtlUlongByteSwap    [NTDLL.@]
 *
 * Swap the bytes of an unsigned int value.
 *
 * NOTES
 * Based on the inline versions in Wine winternl.h
 *
 * @implemented
 */
ULONG
FASTCALL
RtlUlongByteSwap(
   IN ULONG Source
)
{
   return ((ULONG)RtlUshortByteSwap((USHORT)Source) << 16) | RtlUshortByteSwap((USHORT)(Source >> 16));
}


/*************************************************************************
 * RtlUlonglongByteSwap
 *
 * Swap the bytes of an unsigned long long value.
 *
 * PARAMS
 *  i [I] Value to swap bytes of
 *
 * RETURNS
 *  The value with its bytes swapped.
 *
 * @implemented
 */
ULONGLONG FASTCALL
RtlUlonglongByteSwap (IN ULONGLONG Source)
{
   return ((ULONGLONG) RtlUlongByteSwap (Source) << 32) | RtlUlongByteSwap (Source>>32);
}


/* EOF */
