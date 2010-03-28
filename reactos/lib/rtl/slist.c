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
RtlInitializeSListHead(
  OUT PSLIST_HEADER SListHead)
{
#if defined(_IA64_)
    ULONG64 FeatureBits;
#endif

#if defined(_WIN64)
    /* Make sure the alignment is ok */
    if (((ULONG_PTR)SListHead & 0xf) != 0)
    {
        RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
    }

    SListHead->Region = 0;
#endif

    /* Zero it */
    SListHead->Alignment = 0;

#if defined(_IA64_)
    FeatureBits = __getReg(CV_IA64_CPUID4);
    if (FeatureBits & KF_16BYTE_INSTR)
    {
        SListHead->Header16.HeaderType = 1;
        SListHead->Header16.Init = 1;
    }
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
        ListHead->Header16.Depth : ListHead->Header8.Depth;
#else
    return ListHead->Depth;
#endif
}
