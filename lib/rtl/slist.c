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
    ListHead->Alignment = 0;
    ListHead->Region = 0;
    ListHead->Header8.Init = 1;
    // ListHead->Header8.HeaderType = 1; // FIXME: depending on cmpxchg16b support?
#else
    ListHead->Alignment = 0;
#endif
}

PSLIST_ENTRY
NTAPI
RtlFirstEntrySList(IN const SLIST_HEADER *ListHead)
{
#ifdef _WIN64
    if (ListHead->Header8.HeaderType)
    {
        return (PVOID)(ListHead->Region & ~0xF);
    }
    else
    {
        union {
            PVOID P;
            struct {
                ULONG64 Reserved:4;
                ULONG64 NextEntry:39;
                ULONG64 Reserved2:21;
            } Bits;
        } Pointer;

        Pointer.P = (PVOID)ListHead;
        Pointer.Bits.NextEntry = ListHead->Header8.NextEntry;
        return Pointer.P;
    }
#else
    return ListHead->Next.Next;
#endif
}

WORD
NTAPI
RtlQueryDepthSList(IN PSLIST_HEADER ListHead)
{
#ifdef _WIN64
    return ListHead->Header8.HeaderType ?
        (WORD)ListHead->Header16.Sequence : (WORD)ListHead->Header8.Sequence;
#else
    return ListHead->Depth;
#endif
}
