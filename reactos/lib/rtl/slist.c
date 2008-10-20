/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library
 * PURPOSE:         Slist Routines
 * FILE:            lib/rtl/slist.c
 * PROGRAMERS:      Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

VOID
NTAPI
RtlInitializeSListHead(IN PSLIST_HEADER ListHead)
{
    UNIMPLEMENTED;
}

PSLIST_ENTRY
NTAPI
RtlFirstEntrySList(IN const SLIST_HEADER *ListHead)
{
    UNIMPLEMENTED;
    return NULL;
}

WORD  
NTAPI
RtlQueryDepthSList(IN PSLIST_HEADER ListHead)
{
    UNIMPLEMENTED;
    return 0;
}
