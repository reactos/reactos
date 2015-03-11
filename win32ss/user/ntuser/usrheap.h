#pragma once

typedef struct _WIN32HEAP WIN32HEAP, *PWIN32HEAP;

/*
typedef struct _W32HEAP_USER_MAPPING
{
    struct _W32HEAP_USER_MAPPING* Next;
    PVOID KernelMapping;
    PVOID UserMapping;
    ULONG_PTR Limit;
    ULONG Count;
} W32HEAP_USER_MAPPING, *PW32HEAP_USER_MAPPING;
*/

/* User heap */
extern HANDLE GlobalUserHeap;
extern PVOID GlobalUserHeapSection;

PWIN32HEAP
UserCreateHeap(OUT PVOID *SectionObject,
               IN OUT PVOID *SystemBase,
               IN SIZE_T HeapSize);

NTSTATUS
UnmapGlobalUserHeap(IN PEPROCESS Process);

NTSTATUS
MapGlobalUserHeap(IN  PEPROCESS Process,
                  OUT PVOID* KernelMapping,
                  OUT PVOID* UserMapping);

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
    PPROCESSINFO W32Process = PsGetCurrentProcessWin32Process();

    /* The first mapping entry is the global user heap mapping */
    return (PVOID)(((ULONG_PTR)lpMem - (ULONG_PTR)GlobalUserHeap) +
                   (ULONG_PTR)W32Process->HeapMappings.UserMapping);
}

/* EOF */
