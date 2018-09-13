/****************************** Module Header ******************************\
* Module Name: rtlinit.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the init code for the USERRTL.DLL.  When the DLL is
* dynlinked its initialization procedure (UserRtlDllInitialize) is called by
* the loader.
*
* History:
* 14-Jan-1991 mikeke
\***************************************************************************/

#define MOVE_TO_RTL

#include "precomp.h"
#pragma hdrstop
#include "ntimage.h"


/**************************************************************************\
* RtlCaptureAnsiString
*
* Converts a NULL-terminated ANSI string into a counted
* unicode string.
*
* 03-22-95 JimA         Created.
\**************************************************************************/

BOOL RtlCaptureAnsiString(
    PIN_STRING pstr,
    LPCSTR psz,
    BOOL fForceAlloc)
{
    int cbSrc;
    int cbDst;

    pstr->fAllocated = FALSE;
    if (psz) {
        cbSrc = strlen(psz) + 1;
        if (cbSrc > MAXUSHORT) {
            RIPMSG0(RIP_WARNING, "String too long for standard string");
            return FALSE;
        }

        /*
         * If the allocation is forced or if the string is
         * too long to fit in the TEB, allocate a buffer.
         * Otherwise, store the result in the TEB.
         */
        if (fForceAlloc ||
                cbSrc > (STATIC_UNICODE_BUFFER_LENGTH / sizeof(WCHAR))) {
            pstr->strCapture.Buffer = RtlAllocateHeap(pUserHeap,
                    0, cbSrc * sizeof(WCHAR));
            if (pstr->strCapture.Buffer == NULL)
                return FALSE;
            pstr->fAllocated = TRUE;
            pstr->pstr = &pstr->strCapture;
            pstr->strCapture.MaximumLength = (USHORT)(cbSrc * sizeof(WCHAR));
        } else {
            pstr->pstr = &NtCurrentTeb()->StaticUnicodeString;
        }

        /*
         * Convert the string to Unicode
         */
        if (RtlMultiByteToUnicodeN(pstr->pstr->Buffer,
                (ULONG)pstr->pstr->MaximumLength, &cbDst,
                (LPSTR)psz, cbSrc)) {
            RIPMSG0(RIP_WARNING, "Unicode conversion failed");
            if (pstr->fAllocated) {
                RtlFreeHeap(pUserHeap, 0, pstr->strCapture.Buffer);
                pstr->fAllocated = FALSE;
            }
            return FALSE;
        }
        pstr->pstr->Length = (USHORT)cbDst - sizeof(WCHAR);
    } else {
        pstr->pstr = &pstr->strCapture;
        pstr->strCapture.Length = pstr->strCapture.MaximumLength = 0;
        pstr->strCapture.Buffer = NULL;
    }
    return TRUE;
}

/**************************************************************************\
* RtlCaptureLargeAnsiString
*
* Captures a large ANSI string in the same manner as
* RtlCaptureAnsiString.
*
* 03-22-95 JimA         Created.
\**************************************************************************/

BOOL RtlCaptureLargeAnsiString(
    PLARGE_IN_STRING plstr,
    LPCSTR psz,
    BOOL fForceAlloc)
{
    int cchSrc;
    UINT uLength;

    plstr->fAllocated = FALSE;
    plstr->pstr = &plstr->strCapture;

    if (psz) {
        cchSrc = strlen(psz) + 1;

        /*
         * If the allocation is forced or if the string is
         * too long to fit in the TEB, allocate a buffer.
         * Otherwise, store the result in the TEB.
         */
        if (fForceAlloc || cchSrc > STATIC_UNICODE_BUFFER_LENGTH) {
            plstr->strCapture.Buffer = RtlAllocateHeap(pUserHeap,
                    0, cchSrc * sizeof(WCHAR));
            if (plstr->strCapture.Buffer == NULL)
                return FALSE;
            plstr->fAllocated = TRUE;
            plstr->strCapture.MaximumLength = cchSrc * sizeof(WCHAR);
        } else {
            plstr->strCapture.Buffer = NtCurrentTeb()->StaticUnicodeBuffer;
            plstr->strCapture.MaximumLength =
                    (UINT)(STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR));
        }

        /*
         * Convert the string to Unicode
         */
        if (RtlMultiByteToUnicodeN(plstr->pstr->Buffer,
                plstr->pstr->MaximumLength, &uLength,
                (LPSTR)psz, cchSrc)) {
            RIPMSG0(RIP_WARNING, "Unicode conversion failed");
            if (plstr->fAllocated) {
                RtlFreeHeap(pUserHeap, 0, plstr->strCapture.Buffer);
                plstr->fAllocated = FALSE;
            }
            return FALSE;
        }
        plstr->pstr->Length = uLength - sizeof(WCHAR);
    } else {
        plstr->strCapture.Length = plstr->strCapture.MaximumLength = 0;
        plstr->strCapture.Buffer = NULL;
    }
    return TRUE;
}

//++
//
// PVOID
// AllocateFromZone(
//     IN PZONE_HEADER Zone
//     )
//
// Routine Description:
//
//     This routine removes an entry from the zone and returns a pointer to it.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage from which the
//         entry is to be allocated.
//
// Return Value:
//
//     The function value is a pointer to the storage allocated from the zone.
//
/***************************************************************************\
\***************************************************************************/

#define AllocateFromZone(Zone) \
    (PVOID)((Zone)->FreeList.Next); \
    if ( (Zone)->FreeList.Next ) (Zone)->FreeList.Next = (Zone)->FreeList.Next->Next


//++
//
// PVOID
// FreeToZone(
//     IN PZONE_HEADER Zone,
//     IN PVOID Block
//     )
//
// Routine Description:
//
//     This routine places the specified block of storage back onto the free
//     list in the specified zone.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         entry is to be inserted.
//
//     Block - Pointer to the block of storage to be freed back to the zone.
//
// Return Value:
//
//     Pointer to previous block of storage that was at the head of the free
//         list.  NULL implies the zone went from no available free blocks to
//         at least one free block.
//
/***************************************************************************\
\***************************************************************************/

#define FreeToZone(Zone,Block)                                    \
    ( ((PSINGLE_LIST_ENTRY)(Block))->Next = (Zone)->FreeList.Next,  \
      (Zone)->FreeList.Next = ((PSINGLE_LIST_ENTRY)(Block)),        \
      ((PSINGLE_LIST_ENTRY)(Block))->Next                           \
    )

/***************************************************************************\
* InitLookaside
*
* Initializes the lookaside list. This improves control locality
* by keeping control entries in a single page
*
* 05-04-95 JimA         Created.
\***************************************************************************/

NTSTATUS
InitLookaside(
    PLOOKASIDE pla,
    DWORD cbEntry,
    DWORD cEntries)
{
    ULONG i;
    PCH p;
    ULONG BlockSize;
    PZONE_HEADER Zone;
    PVOID InitialSegment;
    ULONG InitialSegmentSize;

    InitialSegmentSize = (cEntries * cbEntry) + sizeof(ZONE_SEGMENT_HEADER);

    p = (PCH)UserLocalAlloc(0, InitialSegmentSize);

    if ( !p ) {
        return STATUS_NO_MEMORY;
        }

    RtlEnterCriticalSection(&gcsLookaside);

    //
    // If the lookaside list has already been initialized, we're done.
    //

    if (pla->LookasideBase != NULL && pla->EntrySize == cbEntry) {
        RtlLeaveCriticalSection(&gcsLookaside);
        UserLocalFree(p);
        return STATUS_SUCCESS;
    }

    pla->LookasideBase = (PVOID)p;
    pla->LookasideBounds = (PVOID)(p + InitialSegmentSize);
    pla->EntrySize = cbEntry;

    //
    // Using the ExZone-like code, slice up the page into QMSG's
    //

    Zone = &pla->LookasideZone;
    BlockSize = cbEntry;
    InitialSegment = pla->LookasideBase;

    Zone->BlockSize = BlockSize;

    Zone->SegmentList.Next = &((PZONE_SEGMENT_HEADER) InitialSegment)->SegmentList;
    ((PZONE_SEGMENT_HEADER) InitialSegment)->SegmentList.Next = NULL;
    ((PZONE_SEGMENT_HEADER) InitialSegment)->Reserved = NULL;

    Zone->FreeList.Next = NULL;

    p = (PCH)InitialSegment + sizeof(ZONE_SEGMENT_HEADER);

    for (i = sizeof(ZONE_SEGMENT_HEADER);
         i <= InitialSegmentSize - BlockSize;
         i += BlockSize
        ) {
        ((PSINGLE_LIST_ENTRY)p)->Next = Zone->FreeList.Next;
        Zone->FreeList.Next = (PSINGLE_LIST_ENTRY)p;
        p += BlockSize;
    }
    Zone->TotalSegmentSize = i;

    RtlLeaveCriticalSection(&gcsLookaside);

    return STATUS_SUCCESS;

}

/***************************************************************************\
* AllocLookasideEntry
*
* Allocates an entry from the lookaside list.
*
* 05-04-95 JimA         Created.
\***************************************************************************/

PVOID AllocLookasideEntry(
    PLOOKASIDE pla)
{
    PVOID pEntry;

    //
    // Attempt to get an entry from the zone. If this fails, then
    // LocalAlloc the entry
    //

    RtlEnterCriticalSection(&gcsLookaside);
    pEntry = AllocateFromZone(&pla->LookasideZone);
    RtlLeaveCriticalSection(&gcsLookaside);

    if ( !pEntry ) {

        /*
         * Allocate a local structure.
         */
#if DBG
        pla->AllocSlowCalls++;
#endif // DBG
        if ((pEntry = UserLocalAlloc(0, pla->EntrySize)) == NULL)
            return NULL;
        }
    RtlZeroMemory(pEntry, pla->EntrySize);
#if DBG
    pla->AllocCalls++;

    if (pla->AllocCalls - pla->DelCalls > pla->AllocHiWater ) {
        pla->AllocHiWater = pla->AllocCalls - pla->DelCalls;
        }
#endif // DBG

    return pEntry;
}

/***************************************************************************\
* FreeLookasideEntry
*
* Returns a qmsg to the lookaside buffer or free the memory.
*
* 05-04-95 JimA         Created.
\***************************************************************************/

void FreeLookasideEntry(
    PLOOKASIDE pla,
    PVOID pEntry)
{
#if DBG
    pla->DelCalls++;
#endif // DBG

    //
    // If the pEntry was from zone, then free to zone
    //
    if ( (PVOID)pEntry >= pla->LookasideBase && (PVOID)pEntry < pla->LookasideBounds ) {
        RtlEnterCriticalSection(&gcsLookaside);
        FreeToZone(&pla->LookasideZone,pEntry);
        RtlLeaveCriticalSection(&gcsLookaside);
    } else {
#if DBG
        pla->DelSlowCalls++;
#endif // DBG
        UserLocalFree(pEntry);
    }
}


