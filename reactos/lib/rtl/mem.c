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
SIZE_T
NTAPI
RtlCompareMemory(IN const VOID *Source1,
                 IN const VOID *Source2,
                 IN SIZE_T Length)
{
    SIZE_T i;
    for (i = 0; (i < Length) && (((PUCHAR)Source1)[i] == ((PUCHAR)Source2)[i]); i++)
        ;

    return i;
}


/*
 * FUNCTION: Compares a block of ULONGs with an ULONG and returns the number of equal bytes
 * ARGUMENTS:
 *      Source = Block to compare
 *      Length = Number of bytes to compare
 *      Value = Value to compare
 * RETURNS: Number of equal bytes
 *
 * @implemented
 */
SIZE_T
NTAPI
RtlCompareMemoryUlong(IN PVOID Source,
                      IN SIZE_T Length,
                      IN ULONG Value)
{
    PULONG ptr = (PULONG)Source;
    ULONG_PTR len = Length / sizeof(ULONG);
    ULONG_PTR i;

    for (i = 0; i < len; i++)
    {
        if (*ptr != Value)
            break;

        ptr++;
    }

    return (SIZE_T)((PCHAR)ptr - (PCHAR)Source);
}


#undef RtlFillMemory
/*
 * @implemented
 */
VOID
NTAPI
RtlFillMemory(PVOID Destination,
              SIZE_T Length,
              UCHAR Fill)
{
    memset(Destination, Fill, Length);
}



/*
 * @implemented
 */
VOID
NTAPI
RtlFillMemoryUlong(PVOID Destination,
                   SIZE_T Length,
                   ULONG Fill)
{
    PULONG Dest  = Destination;
    SIZE_T Count = Length / sizeof(ULONG);

    while (Count > 0)
    {
        *Dest = Fill;
        Dest++;
        Count--;
    }
}

#ifdef _WIN64
VOID
NTAPI
RtlFillMemoryUlonglong(
    PVOID Destination,
    SIZE_T Length,
    ULONGLONG Fill)
{
    PULONGLONG Dest  = Destination;
    SIZE_T Count = Length / sizeof(ULONGLONG);

    while (Count > 0)
    {
        *Dest = Fill;
        Dest++;
        Count--;
    }
}
#endif

#undef RtlMoveMemory
/*
 * @implemented
 */
VOID
NTAPI
RtlMoveMemory(PVOID Destination,
              CONST VOID *Source,
              SIZE_T Length)
{
    memmove(Destination, Source, Length);
}


/*
* @implemented
*/
VOID
FASTCALL
RtlPrefetchMemoryNonTemporal(IN PVOID Source,
                             IN SIZE_T Length)
{
    /* By nature of prefetch, this is non-portable. */
    (void)Source;
    (void)Length;
}


#undef RtlZeroMemory
/*
 * @implemented
 */
VOID
NTAPI
RtlZeroMemory(PVOID Destination,
              SIZE_T Length)
{
    RtlFillMemory(Destination, Length, 0);
}

/* EOF */
