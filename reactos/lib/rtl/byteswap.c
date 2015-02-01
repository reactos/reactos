/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/mem.c
 * PURPOSE:         Memory functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#if defined(_M_IX86)
/* RtlUlonglongByteSwap is broken and cannot be done in C on x86 */
#error "Use rtlswap.S!"
#endif

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
USHORT
FASTCALL
RtlUshortByteSwap(
    IN USHORT Source)
{
#if defined(_M_AMD64)
    return _byteswap_ushort(Source);
#else
    return (Source >> 8) | (Source << 8);
#endif
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
   IN ULONG Source)
{
#if defined(_M_AMD64)
    return _byteswap_ulong(Source);
#else
    return ((ULONG)RtlUshortByteSwap((USHORT)Source) << 16) | RtlUshortByteSwap((USHORT)(Source >> 16));
#endif
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
RtlUlonglongByteSwap(
    IN ULONGLONG Source)
{
#if defined(_M_AMD64)
    return _byteswap_uint64(Source);
#else
    return ((ULONGLONG) RtlUlongByteSwap (Source) << 32) | RtlUlongByteSwap (Source>>32);
#endif
}


/* EOF */
