/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32K
 * FILE:            subsystems/win32/win32k/include/heap.h
 * PURPOSE:         Global user heap
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 */

#ifndef _INCLUDE_GLOBAL_HEAP_H
#define _INCLUDE_GLOBAL_HEAP_H

extern HANDLE GlobalUserHeap;

PWIN32HEAP
UserCreateHeap(OUT PSECTION_OBJECT *SectionObject,
               IN OUT PVOID *SystemBase,
               IN SIZE_T HeapSize);

static __inline PVOID
UserHeapAlloc(SIZE_T Bytes)
{
    return RtlAllocateHeap(GlobalUserHeap,
                           HEAP_NO_SERIALIZE,
                           Bytes);
}

static __inline BOOL
UserHeapFree(PVOID lpMem)
{
    return RtlFreeHeap(GlobalUserHeap,
                       HEAP_NO_SERIALIZE,
                       lpMem);
}

static __inline PVOID
UserHeapReAlloc(PVOID lpMem,
                SIZE_T Bytes)
{
#if 0
    /* NOTE: ntoskrnl doesn't export RtlReAllocateHeap... */
    return RtlReAllocateHeap(GlobalUserHeap,
                             HEAP_NO_SERIALIZE,
                             lpMem,
                             Bytes);
#else
    SIZE_T PrevSize;
    PVOID pNew;

    PrevSize = RtlSizeHeap(GlobalUserHeap,
                           HEAP_NO_SERIALIZE,
                           lpMem);

    if (PrevSize == Bytes)
        return lpMem;

    pNew = RtlAllocateHeap(GlobalUserHeap,
                           HEAP_NO_SERIALIZE,
                           Bytes);
    if (pNew != NULL)
    {
        if (PrevSize < Bytes)
            Bytes = PrevSize;

        RtlCopyMemory(pNew,
                      lpMem,
                      Bytes);

        RtlFreeHeap(GlobalUserHeap,
                    HEAP_NO_SERIALIZE,
                    lpMem);
    }

    return pNew;
#endif
}

static __inline PVOID
UserHeapAddressToUser(PVOID lpMem)
{
    PW32PROCESS W32Process = PsGetCurrentProcessWin32Process();
    return (PVOID)(((ULONG_PTR)lpMem - (ULONG_PTR)GlobalUserHeap) +
                   (ULONG_PTR)W32Process->HeapMappings.UserMapping);
}

#endif
