
/* $Id: mem.c,v 1.1 2004/05/31 19:29:02 gdalsnes Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rtl/mem.c
 * PURPOSE:         Memory functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>

#define NDEBUG
#include <debug.h>




/* FUNCTIONS *****************************************************************/

/******************************************************************************
 *  RtlCompareMemory   [NTDLL.@]
 *
 * Compare one block of memory with another
 *
 * PARAMS
 *  Source1 [I] Source block
 *  Source2 [I] Block to compare to Source1
 *  Length  [I] Number of bytes to fill
 *
 * RETURNS
 *  The length of the first byte at which Source1 and Source2 differ, or Length
 *  if they are the same.
 *
 * @implemented
 */
ULONG
STDCALL
RtlCompareMemory(PVOID Source1,
                 PVOID Source2,
                 ULONG Length)
{
   SIZE_T i;
   for(i=0; (i<Length) && (((LPBYTE)Source1)[i]==((LPBYTE)Source2)[i]); i++)
      ;
   return i;
}


/*
 * @implemented
 */
ULONG
STDCALL
RtlCompareMemoryUlong (
   PVOID Source,
   ULONG Length,
   ULONG Value
)
/*
 * FUNCTION: Compares a block of ULONGs with an ULONG and returns the number of equal bytes
 * ARGUMENTS:
 *      Source = Block to compare
 *      Length = Number of bytes to compare
 *      Value = Value to compare
 * RETURNS: Number of equal bytes
 */
{
   PULONG ptr = (PULONG)Source;
   ULONG  len = Length / sizeof(ULONG);
   ULONG i;

   for (i = 0; i < len; i++)
   {
      if (*ptr != Value)
         break;
      ptr++;
   }

   return (ULONG)((PCHAR)ptr - (PCHAR)Source);
}


/*
 * @implemented
 */
VOID
STDCALL
RtlFillMemory (
   PVOID Destination,
   ULONG Length,
   UCHAR Fill
)
{
   memset(Destination, Fill, Length);
}



/*
 * @implemented
 */
VOID
STDCALL
RtlFillMemoryUlong (
   PVOID Destination,
   ULONG Length,
   ULONG Fill
)
{
   PULONG Dest  = Destination;
   ULONG  Count = Length / sizeof(ULONG);

   while (Count > 0)
   {
      *Dest = Fill;
      Dest++;
      Count--;
   }
}



/*
 * @implemented
 */
VOID
STDCALL
RtlMoveMemory (
   PVOID    Destination,
   CONST VOID  * Source,
   ULONG    Length
)
{
   memmove (
      Destination,
      Source,
      Length
   );
}


/*
 * @implemented
 */
VOID
STDCALL
RtlZeroMemory (
   PVOID Destination,
   ULONG Length
)
{
   RtlFillMemory (
      Destination,
      Length,
      0
   );
}


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
#if defined(__i386__) && defined(__GNUC__)
   ULONG ret;
__asm__("bswap %0" : "=r" (ret) : "0" (Source) );
   return ret;
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
RtlUlonglongByteSwap (IN ULONGLONG Source)
{
   return ((ULONGLONG) RtlUlongByteSwap (Source) << 32) | RtlUlongByteSwap (Source>>32);
}


/* EOF */
