#pragma once

#define DECLARE_RETURN(type) type _ret_
#define RETURN(value) { _ret_ = value; goto _cleanup_; }
#define CLEANUP /*unreachable*/ ASSERT(FALSE); _cleanup_
#define END_CLEANUP return _ret_;


#define UserEnterCo UserEnterExclusive
#define UserLeaveCo UserLeave

extern PSERVERINFO gpsi;
extern PTHREADINFO gptiCurrent;
extern PPROCESSINFO gppiList;
extern PPROCESSINFO ppiScrnSaver;
extern PPROCESSINFO gppiInputProvider;
extern ATOM gaGuiConsoleWndClass;

INIT_FUNCTION NTSTATUS NTAPI InitUserImpl(VOID);
VOID FASTCALL CleanupUserImpl(VOID);
VOID FASTCALL UserEnterShared(VOID);
VOID FASTCALL UserEnterExclusive(VOID);
VOID FASTCALL UserLeave(VOID);
BOOL FASTCALL UserIsEntered(VOID);
BOOL FASTCALL UserIsEnteredExclusive(VOID);

/* User heap */
extern HANDLE GlobalUserHeap;

PWIN32HEAP
UserCreateHeap(OUT PVOID *SectionObject,
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
    PPROCESSINFO W32Process = PsGetCurrentProcessWin32Process();
    return (PVOID)(((ULONG_PTR)lpMem - (ULONG_PTR)GlobalUserHeap) +
                   (ULONG_PTR)W32Process->HeapMappings.UserMapping);
}

/* EOF */
