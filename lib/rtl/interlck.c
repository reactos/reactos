/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library
 * PURPOSE:         RTL Interlocked Routines
 * FILE:            lib/rtl/interlck.c
 * PROGRAMERS:      Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

PSLIST_ENTRY
NTAPI
RtlInterlockedPushListSList(IN PSLIST_HEADER ListHead,
                            IN PSLIST_ENTRY List,
                            IN PSLIST_ENTRY ListEnd,
                            IN ULONG Count)
{
    UNIMPLEMENTED;
    return NULL;
}

LONGLONG
NTAPI
RtlInterlockedCompareExchange64(LONGLONG volatile *Destination,
                                LONGLONG Exchange,
                                LONGLONG Comparand)
{
    /* Just call the intrinsic */
    return _InterlockedCompareExchange64(Destination, Exchange, Comparand);
}
