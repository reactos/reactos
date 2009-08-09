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
#ifdef _WIN64
    UNIMPLEMENTED;
#else
    ListHead->Alignment = 0;
#endif
}

PSLIST_ENTRY
NTAPI
RtlFirstEntrySList(IN const SLIST_HEADER *ListHead)
{
#ifdef _WIN64
    UNIMPLEMENTED;
    return NULL;
#else
    return ListHead->Next.Next;
#endif
}

WORD  
NTAPI
RtlQueryDepthSList(IN PSLIST_HEADER ListHead)
{
#ifdef _WIN64
    UNIMPLEMENTED;
    return 0;
#else
    return ListHead->Depth;
#endif
}
