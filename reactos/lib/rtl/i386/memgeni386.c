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

USHORT UshortByteSwap (IN USHORT Source);
ULONG  UlongByteSwap (IN ULONG Source);
ULONGLONG UlonglongByteSwap (IN ULONGLONG Source);

/*************************************************************************
 * RtlUshortByteSwap
 *
 * Swap the bytes of an unsigned short value.
 *
 *
 * @implemented
 */
USHORT FASTCALL
RtlUshortByteSwap (IN USHORT Source)
{
  return UshortByteSwap (Source);
}



/*************************************************************************
 * RtlUlongByteSwap    [NTDLL.@]
 *
 * Swap the bytes of an unsigned int value.
 *
 *
 * @implemented
 */
ULONG
FASTCALL
RtlUlongByteSwap(
   IN ULONG Source
)
{
   return UlongByteSwap(Source);
}


/*************************************************************************
 * RtlUlonglongByteSwap
 *
 * Swap the bytes of an unsigned long long value.
 *
 * PARAMS
 *  i [I] Value to swap bytes of
 *
 *
 * @implemented
 */
ULONGLONG FASTCALL
RtlUlonglongByteSwap (IN ULONGLONG Source)
{
   return UlonglongByteSwap(Source);
}


/* EOF */
