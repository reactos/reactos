/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library
 * PURPOSE:         Slist Routines
 * FILE:            lib/rtl/slist.c
 * PROGRAMERS:      Stefan Ginsberg (stefan__100__@hotmail.com)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#ifdef _WIN64
BOOLEAN RtlpUse16ByteSLists = -1;
#endif

/* FUNCTIONS ***************************************************************/

VOID
NTAPI
RtlInitializeSListHead(
    _Out_ PSLIST_HEADER SListHead)
{
#if defined(_WIN64)
    /* Make sure the header is 16 byte aligned */
    if (((ULONG_PTR)SListHead & 0xf) != 0)
    {
        DPRINT1("Unaligned SListHead: 0x%p\n", SListHead);
        RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
    }

    /* Initialize the Region member */
#if defined(_IA64_)
    /* On Itanium we store the region in the list head */
    SListHead->Region = (ULONG_PTR)SListHead & VRN_MASK;
#else
    /* On amd64 we don't need to store anything */
    SListHead->Region = 0;
#endif /* _IA64_ */
#endif /* _WIN64 */

    SListHead->Alignment = 0;
}

PSLIST_ENTRY
NTAPI
RtlFirstEntrySList(
    _In_ const SLIST_HEADER *SListHead)
{
#if defined(_WIN64)
    /* Check if the header is initialized as 16 byte header */
    if (SListHead->Header16.HeaderType)
    {
        return (PVOID)(SListHead->Region & ~0xFLL);
    }
    else
    {
        union {
            ULONG64 Region;
            struct {
                ULONG64 Reserved:4;
                ULONG64 NextEntry:39;
                ULONG64 Reserved2:21;
            } Bits;
        } Pointer;

#if defined(_IA64_)
        /* On Itanium we stored the region in the list head */
        Pointer.Region = SListHead->Region;;
#else
        /* On amd64 we just use the list head itself */
        Pointer.Region = (ULONG64)SListHead;
#endif
        Pointer.Bits.NextEntry = SListHead->Header8.NextEntry;
        return (PVOID)Pointer.Region;
    }
#else
    return SListHead->Next.Next;
#endif
}

WORD
NTAPI
RtlQueryDepthSList(
    _In_ PSLIST_HEADER SListHead)
{
#if defined(_WIN64)
    return (USHORT)(SListHead->Alignment & 0xffff);
#else
    return SListHead->Depth;
#endif
}

PSLIST_ENTRY
FASTCALL
RtlInterlockedPushListSList(
    _Inout_ PSLIST_HEADER SListHead,
    _Inout_ __drv_aliasesMem PSLIST_ENTRY List,
    _Inout_ PSLIST_ENTRY ListEnd,
    _In_ ULONG Count)
{
#ifdef _WIN64
    UNIMPLEMENTED;
    DbgBreakPoint();
    return NULL;
#else
    SLIST_HEADER OldHeader, NewHeader;
    ULONGLONG Compare;

    /* Read the header */
    OldHeader = *SListHead;

    do
    {
        /* Link the last list entry */
        ListEnd->Next = OldHeader.Next.Next;

        /* Create a new header */
        NewHeader = OldHeader;
        NewHeader.Next.Next = List;
        NewHeader.Depth += Count;
        NewHeader.Sequence++;

        /* Try to exchange atomically */
        Compare = OldHeader.Alignment;
        OldHeader.Alignment = InterlockedCompareExchange64((PLONGLONG)&SListHead->Alignment,
                                                           NewHeader.Alignment,
                                                           Compare);
    }
    while (OldHeader.Alignment != Compare);

    /* Return the old first entry */
    return OldHeader.Next.Next;
#endif /* _WIN64 */
}


#if !defined(_M_IX86) && !defined(_M_AMD64)

_WARN("C based S-List functions can bugcheck, if not handled properly in kernel")

#ifdef _WIN64
#error "No generic S-List functions for WIN64!"
#endif

/* This variable must be used in kernel mode to prevent the system from
   bugchecking on non-present kernel memory. If this variable is set to TRUE
   an exception needs to be dispatched. */
BOOLEAN RtlpExpectSListFault;

PSLIST_ENTRY
NTAPI
RtlInterlockedPushEntrySList(
    _Inout_ PSLIST_HEADER SListHead,
    _Inout_ __drv_aliasesMem PSLIST_ENTRY SListEntry)
{
    SLIST_HEADER OldHeader, NewHeader;
    ULONGLONG Compare;

    /* Read the header */
    OldHeader = *SListHead;

    do
    {
        /* Link the list entry */
        SListEntry->Next = OldHeader.Next.Next;

        /* Create a new header */
        NewHeader = OldHeader;
        NewHeader.Next.Next = SListEntry;
        NewHeader.Depth++;
        NewHeader.Sequence++;

        /* Try to exchange atomically */
        Compare = OldHeader.Alignment;
        OldHeader.Alignment = InterlockedCompareExchange64((PLONGLONG)&SListHead->Alignment,
                                                           NewHeader.Alignment,
                                                           Compare);
    }
    while (OldHeader.Alignment != Compare);

    /* Return the old first entry */
    return OldHeader.Next.Next;
}

PSLIST_ENTRY
NTAPI
RtlInterlockedPopEntrySList(
    _Inout_ PSLIST_HEADER SListHead)
{
    SLIST_HEADER OldHeader, NewHeader;
    ULONGLONG Compare;

restart:

    /* Read the header */
    OldHeader = *SListHead;

    do
    {
        /* Check for empty list */
        if (OldHeader.Next.Next == NULL)
        {
            return NULL;
        }

        /* Create a new header */
        NewHeader = OldHeader;

        /* HACK to let the kernel know that we are doing slist-magic */
        RtlpExpectSListFault = TRUE;

        /* Wrapped in SEH, since OldHeader.Next.Next can already be freed */
        _SEH2_TRY
        {
            NewHeader.Next = *OldHeader.Next.Next;
        }
        _SEH2_EXCEPT((SListHead->Next.Next != OldHeader.Next.Next) ?
                     EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            /* We got an exception and the list head changed.
               Restart the whole operation. */
            RtlpExpectSListFault = FALSE;
            goto restart;
        }
        _SEH2_END;

        /* We are done */
        RtlpExpectSListFault = FALSE;

        /* Adjust depth */
        NewHeader.Depth--;

        /* Try to exchange atomically */
        Compare = OldHeader.Alignment;
        OldHeader.Alignment = InterlockedCompareExchange64((PLONGLONG)SListHead->Alignment,
                                                           NewHeader.Alignment,
                                                           Compare);
    }
    while (OldHeader.Alignment != Compare);

    return OldHeader.Next.Next;
}

PSLIST_ENTRY
NTAPI
RtlInterlockedFlushSList(
    _Inout_ PSLIST_HEADER SListHead)
{
    SLIST_HEADER OldHeader, NewHeader;
    ULONGLONG Compare;

    /* Read the header */
    OldHeader = *SListHead;

    do
    {
        /* Check for empty list */
        if (OldHeader.Next.Next == NULL)
        {
            return NULL;
        }

        /* Create a new header (keep the sequence number) */
        NewHeader = OldHeader;
        NewHeader.Next.Next = NULL;
        NewHeader.Depth = 0;

        /* Try to exchange atomically */
        Compare = OldHeader.Alignment;
        OldHeader.Alignment = InterlockedCompareExchange64((PLONGLONG)&SListHead->Alignment,
                                                           NewHeader.Alignment,
                                                           Compare);
    }
    while (OldHeader.Alignment != Compare);

    /* Return the old first entry */
    return OldHeader.Next.Next;

}

#ifdef _MSC_VER
#pragma comment(linker, "/alternatename:ExpInterlockedPopEntrySList=RtlInterlockedPopEntrySList")
#pragma comment(linker, "/alternatename:ExpInterlockedPushEntrySList=RtlInterlockedPushEntrySList")
#pragma comment(linker, "/alternatename:ExpInterlockedFlushSList=RtlInterlockedFlushSList")
#else
#pragma redefine_extname RtlInterlockedPopEntrySList ExpInterlockedPopEntrySList
#pragma redefine_extname RtlInterlockedPushEntrySList ExpInterlockedPushEntrySList
#pragma redefine_extname RtlInterlockedFlushSList ExpInterlockedFlushSList
#endif

#endif

