/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library
 * PURPOSE:         RTL Interlocked Routines
 * FILE:            lib/rtl/interlck.c
 * PROGRAMERS:      Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

LONGLONG
NTAPI
RtlInterlockedCompareExchange64(LONGLONG volatile *Destination,
                                LONGLONG Exchange,
                                LONGLONG Comparand)
{
    /* Just call the intrinsic */
    return _InterlockedCompareExchange64(Destination, Exchange, Comparand);
}
