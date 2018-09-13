/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    memory.c

Abstract:

    Memory handling routines for Windows NT Setup API dll.

Author:

    Ted Miller (tedm) 11-Jan-1995

Revision History:

    Jamie Hunter (jamiehun) 13-Feb-1998

        Improved this further for debugging
        added linked list,
        alloc tracing,
        memory fills
        and memory leak detection

    jamiehun 30-April-1998

        Added some more consistancy checks
        Put try/except around access

    jimschm 27-Oct-1998

        Wrote fast allocation routines to speed up setupapi.dll on Win9x

--*/


#include "precomp.h"
#pragma hdrstop


HANDLE g_hHeap;

#ifdef MyMalloc
#undef MyMalloc
#endif



#define REAL_ALLOC(x)        HeapAlloc(g_hHeap,0,x)
#define REAL_FREE(x)         HeapFree(g_hHeap,0,x)
#define REAL_REALLOC(x,y)    HeapReAlloc(g_hHeap,0,x,y)
#define REAL_MEMSIZE(x)      HeapSize(g_hHeap,0,x)


#if ANSI_SETUPAPI

//
// These allocation routines are designed to speed up setupapi.dll
// on Win9x.  It implements a reuse pool to reduce the number of
// HeapAlloc calls.  On Win9x, the memory allocation routines are
// extremely slow.
//

VOID
pInitFastAlloc (
    VOID
    );

VOID
pTerminateFastAlloc (
    VOID
    );

PVOID
pFastAlloc (
    DWORD Size
    );

VOID
pFastDeAlloc (
    PVOID Alloc
    );

DWORD
pFastSize (
    PVOID Alloc
    );

PVOID
pFastReAlloc (
    PVOID Alloc,
    DWORD NewSize
    );

#define ALLOC(x)        pFastAlloc(x)
#define FREE(x)         pFastDeAlloc(x)
#define REALLOC(x,y)    pFastReAlloc(x,y)
#define MEMSIZE(x)      pFastSize(x)

#else

#define ALLOC           REAL_ALLOC
#define FREE            REAL_FREE
#define REALLOC         REAL_REALLOC
#define MEMSIZE         REAL_MEMSIZE

#endif


//
// String to be used when displaying insufficient memory msg box.
// We load it at process attach time so we can be guaranteed of
// being able to display it.
//
PCTSTR OutOfMemoryString;


#if MEM_DBG

DWORD g_Track = 0;
PCSTR g_TrackFile = NULL;
UINT g_TrackLine = 0;

DWORD g_MemoryFlags = 0; // set this to 1 in the debugger to catch some extra dbg assertions.

DWORD g_DbgAllocNum = -1; // set g_MemoryFlags to 1 and this to the allocation number you want
                          // to catch if the same number allocation leaks every time.


//
// Checked builds have a block head/tail check
// and extra statistics
//
#define QUERYENABLE 0           // 1 = ask to enable stats, 0 = always enable

#define HEAD_MEMSIG 0x4d444554  // = MDET (MSB to LSB) or TEDM (LSB to MSB)
#define TAIL_MEMSIG 0x5445444d  // = TEDM (MSB to LSB) or MDET (LSB to MSB)
#define MEM_ALLOCCHAR 0xdd      // makes sure we fill with non-null
#define MEM_FREECHAR 0xee       // if we see this, memory has been de-allocated
#define MEM_DEADSIG 0xdeaddead
#define MEM_TOOBIG 0x80000000   // use this to pick up big allocs

//
// Put in a struct so you can just say ?MemStats in the debugger
// to get a complete report
//

struct _MemHeader {
    struct _MemHeader * PrevAlloc;  // previous on chain
    struct _MemHeader * NextAlloc;  // next on chain
    DWORD BlockSize;                // bytes of "real" data
    DWORD AllocNum;                 // number of this allocation, ie AllocCount at the time this was allocated
    PCSTR AllocFile;                // name of file that did allocation, if set
    DWORD AllocLine;                // line of this allocation
    DWORD HeadMemSig;               // head-check, stop writing before actual data
    BYTE Data[sizeof(DWORD)];       // size allows for tail-check at end of actual data
};

//
// (jamiehun) enhanced this structure
//

struct _MemStats {
    struct _MemHeader * FirstAlloc; // will be NULL if no allocations, else earliest malloc/realloc in chain
    struct _MemHeader * LastAlloc;  // last alloc/realloc goes to end of chain
    DWORD MemoryAllocated;          // bytes, excluding headers
    DWORD AllocCount;               // incremented for every alloc
    DWORD ReallocCount;             // incremented for every realloc
    DWORD FreeCount;                // incremented for every free
    CRITICAL_SECTION DebugMutex;    // We need a mutex to manage memstats, setupapi is MT
} MemStats = {
    NULL, NULL, 0,0,0,0
};


#define MemMutexLock            EnterCriticalSection(&MemStats.DebugMutex)
#define MemMutexUnlock          LeaveCriticalSection(&MemStats.DebugMutex)

VOID
MemBlockError(
    VOID
    )

/*++

Routine Description:

    Inform the user that a memory block error has been detected and
    offer to break into the debugger.

Arguments:

    None.

Return Value:

    None.

--*/

{
    int i;
    TCHAR Name[MAX_PATH];
    PTCHAR p;

    //
    // Don't popup a dialog if we're not running interactively...
    //
    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        OutputDebugString(TEXT("SETUPAPI: Internal heap error. Someone is behaving badly!\r\n"));
        i = IDYES;
    } else {
        //
        // Use dll name as caption
        //
        GetModuleFileName(MyDllModuleHandle,Name,MAX_PATH);
        if(p = _tcsrchr(Name,TEXT('\\'))) {
            p++;
        } else {
            p = Name;
        }

        i = MessageBox(
                NULL,
                TEXT("SETUPAPI: Internal heap error. Someone is behaving badly! Call DebugBreak()?"),
                p,
                MB_YESNO | MB_TASKMODAL | MB_ICONSTOP | MB_SETFOREGROUND
                );
    }

    if(i == IDYES) {
        DebugBreak();
    }
}

BOOL MemBlockCheck(
    struct _MemHeader * Mem
    )
/*++

Routine Description:

    Verify a block header is valid

Arguments:
    Mem = Header to verify

Returns:
    TRUE if valid
    FALSE if not valid

++*/
{
    if (Mem == NULL) {
        return TRUE;
    }
    if (Mem->HeadMemSig != HEAD_MEMSIG) {
        MemBlockError();
        return FALSE;
    }
    if (Mem->BlockSize >= MEM_TOOBIG) {
        MemBlockError();
        return FALSE;
    }
    if((Mem->PrevAlloc == Mem) || (Mem->NextAlloc == Mem)) {
        //
        // we should have failed the MEMSIG, but it's ok as an extra check
        MemBlockError();
        return FALSE;
    }
    if ((*(DWORD UNALIGNED *)(Mem->Data+Mem->BlockSize)) != TAIL_MEMSIG) {
        MemBlockError();
        return FALSE;
    }
    return TRUE;
}

struct _MemHeader *MemBlockGet(
    IN PVOID Block
    )
/*++

Routine Description:

    Verify a block is valid, and return real memory pointer

Arguments:
    Block - address the application uses

++*/
{
    struct _MemHeader * Mem;

    Mem = (struct _MemHeader *)(((PBYTE)Block) - offsetof(struct _MemHeader,Data[0]));

    if (MemBlockCheck(Mem)==FALSE) {
        //
        // block fails test
        //
        MemBlockError();
        return NULL;
    }

    if(Mem->PrevAlloc != NULL && MemBlockCheck(Mem->PrevAlloc)==FALSE) {
        //
        // back link is invalid
        //
        MemBlockError();
        return NULL;
    }
    if(Mem->NextAlloc != NULL && MemBlockCheck(Mem->NextAlloc)==FALSE) {
        //
        // forward link is invalid
        //
        MemBlockError();
        return NULL;
    }

    //
    // seems pretty good
    //

    return Mem;
}

PVOID MemBlockLink(
    struct _MemHeader * Mem
    )

{
    if (Mem == NULL) {
        return NULL;
    }

    Mem->PrevAlloc = MemStats.LastAlloc;
    Mem->NextAlloc = NULL;
    MemStats.LastAlloc = Mem;
    if (Mem->PrevAlloc == NULL) {
        MemStats.FirstAlloc = Mem;
    } else {
        Mem->PrevAlloc->NextAlloc = Mem;
    }

    Mem->HeadMemSig = HEAD_MEMSIG;
    *(DWORD UNALIGNED *)(Mem->Data+Mem->BlockSize) = TAIL_MEMSIG;

    return (PVOID)(Mem->Data);
}

PVOID MemBlockUnLink(
    struct _MemHeader * Mem
    )

{
    if (Mem == NULL) {
        return NULL;
    }

    if (Mem->PrevAlloc == NULL) {
        MemStats.FirstAlloc = Mem->NextAlloc;
    } else {
        Mem->PrevAlloc->NextAlloc = Mem->NextAlloc;
    }
    if (Mem->NextAlloc == NULL) {
        MemStats.LastAlloc = Mem->PrevAlloc;
    } else {
        Mem->NextAlloc->PrevAlloc = Mem->PrevAlloc;
    }
    Mem->PrevAlloc = Mem;  // make pointers harmless and also adds as an exta debug check
    Mem->NextAlloc = Mem;  // make pointers harmless and also adds as an exta debug check
    Mem->HeadMemSig = MEM_DEADSIG;
    *(DWORD UNALIGNED *)(Mem->Data+Mem->BlockSize) = MEM_DEADSIG;

    return Mem->Data;
}


BOOL
MemDebugInit(
    IN BOOL Attach
    )
{
    struct _MemHeader *Mem;
    TCHAR Msg[1024];
    TCHAR Process[MAX_PATH];

    if (Attach) {
        InitializeCriticalSection(&MemStats.DebugMutex);
    } else {
        //
        // Dump the leaks
        //

        Mem = MemStats.FirstAlloc;

        GetModuleFileName( GetModuleHandle(NULL),Process, sizeof(Process)/sizeof(TCHAR));


        while (Mem) {
            wsprintf (Msg, TEXT("SETUPAPI.DLL: Leak (%d bytes) at %hs line %u (allocation #%d) in process %s \r\n"), Mem->BlockSize, Mem->AllocFile, Mem->AllocLine, Mem->AllocNum, Process );
            OutputDebugString (Msg);
            if (g_MemoryFlags != 0) {
                if ( Mem->AllocLine == 219) {
                    OutputDebugString( TEXT("Leak of DEVICE_INFO_SET, someone forgot to call SetupDiDestroyDeviceInfoList(). Calling DebugBreak()\n"));
                    DebugBreak();
                }
                
                if (Mem->BlockSize > 1024) {
                    OutputDebugString( TEXT("Leak of > 1K. Calling DebugBreak.\n"));
                    DebugBreak();
                }
            }

            Mem = Mem->NextAlloc;
        }

        //
        // Clean up
        //

        DeleteCriticalSection(&MemStats.DebugMutex);

        //
        // any last minute checks
        //

    }

    return TRUE;
}


VOID
SetTrackFileAndLine (
    PCSTR File,
    UINT Line
    )
{
    if (!g_Track) {
        g_TrackFile = File;
        g_TrackLine = Line;
    }

    g_Track++;
}


VOID
ClrTrackFileAndLine (
    VOID
    )
{
    if (g_Track) {
        g_Track--;
        if (!g_Track) {
            g_TrackFile = NULL;
            g_TrackLine = 0;
        }
    }
}



PVOID MyDebugMalloc(
    IN DWORD Size,
    IN PCSTR Filename,
    IN DWORD Line
    )
{
    struct _MemHeader *Mem;
    PVOID Ptr = NULL;

    MemMutexLock;

    try {
        MemStats.AllocCount++;

        if (Size >= MEM_TOOBIG) {
            MemBlockError();
            leave;
        }

        if((Mem = (struct _MemHeader*) ALLOC(Size+sizeof(struct _MemHeader))) == NULL) {
            leave;  // it failed ALLOC, but prob not due to a bug
        }

        Mem->BlockSize = Size;
        Mem->AllocNum = MemStats.AllocCount;
        Mem->AllocFile = g_TrackFile ? g_TrackFile : Filename;
        Mem->AllocLine = g_TrackFile ? g_TrackLine : Line;

        // init memory we have allocated (to make sure we don't accidently get zero's)
        FillMemory(Mem->Data,Size,MEM_ALLOCCHAR);

        MemStats.MemoryAllocated += Size;

        Ptr = MemBlockLink(Mem);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        MemBlockError();
        Ptr = NULL;
    }

    if (g_MemoryFlags && (g_DbgAllocNum == Mem->AllocNum)) {
        MYASSERT( 0 && TEXT("g_DbgAllocNum hit"));
    }

    MemMutexUnlock;

    return Ptr;
}

PVOID
MyDebugRealloc(
    IN PVOID Block,
    IN DWORD NewSize
    )

/*++

Routine Description:

    Debug version of MyRealloc

Arguments:

    Block - pointer to block to be reallocated.

    NewSize - new size in bytes of block. If the size is 0, this function
        works like MyFree, and the return value is NULL.

Return Value:

    Pointer to block of memory, or NULL if a block could not be allocated.
    In that case the original block remains unchanged.

--*/

{
    PVOID p;
    DWORD OldSize;
    struct _MemHeader *Mem;
    PVOID Ptr = NULL;

    MemMutexLock;

    try {
        MemStats.ReallocCount++;

        if (Block == NULL) {
            leave;
        }

        if (NewSize >= MEM_TOOBIG) {
            MemBlockError();
            leave;
        }

        Mem = MemBlockGet(Block);
        if (Mem == NULL) {
            leave;
        }

        OldSize = Mem->BlockSize;
        MemBlockUnLink(Mem);

        if (NewSize < OldSize) {
            // trash memory we're about to free
            FillMemory(Mem->Data+NewSize,OldSize-NewSize+sizeof(DWORD),MEM_FREECHAR);
        }

        if((p = REALLOC(Mem, NewSize+sizeof(struct _MemHeader))) == NULL) {
            //
            // failed to re-alloc
            //
            MemBlockLink(Mem);
            leave;
        }
        Mem = (struct _MemHeader*)p;
        Mem->BlockSize = NewSize;

        if (NewSize > OldSize) {
            // init extra memory we have allocated
            FillMemory(Mem->Data+OldSize,NewSize-OldSize,MEM_ALLOCCHAR);
        }
        MemStats.MemoryAllocated -= OldSize;
        MemStats.MemoryAllocated += NewSize;

        Ptr = MemBlockLink(Mem);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        MemBlockError();
        Ptr = NULL;
    }

    MemMutexUnlock;

    return Ptr;
}


VOID
MyDebugFree(
    IN CONST VOID *Block
    )

/*++

Routine Description:

    Debug version of MyFree

Arguments:

    Buffer - pointer to block to be freed.

Return Value:

    None.

--*/

{
    DWORD OldSize;
    struct _MemHeader *Mem;

    MemMutexLock;

    try {
        MemStats.FreeCount++;

        if (Block == NULL) {
            leave;
        }

        Mem = MemBlockGet((PVOID)Block);
        if (Mem == NULL) {
            leave;
        }
        OldSize = Mem->BlockSize;
        MemBlockUnLink(Mem);

        //
        // trash memory we're about to free, so we can immediately see it has been free'd!!!!
        // we keep head/tail stuff to have more info available when debugging
        //
        FillMemory((PVOID)Block,OldSize,MEM_FREECHAR);
        FREE(Mem);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        MemBlockError();
    }

    MemMutexUnlock;
}

#endif //MEM_DBG


PVOID
MyMalloc(
    IN DWORD Size
    )

/*++

Routine Description:

    Allocate a chunk of memory. The memory is not zero-initialized.

Arguments:

    Size - size in bytes of block to be allocated. The size may be 0.

Return Value:

    Pointer to block of memory, or NULL if a block could not be allocated.

--*/

{
#if MEM_DBG
    return MyDebugMalloc(Size, __FILE__ , __LINE__);
#else
    return ALLOC(Size);

#endif

}

PVOID
MyRealloc(
    IN PVOID Block,
    IN DWORD NewSize
    )

/*++

Routine Description:

    Reallocate a block of memory allocated by MyAlloc().

Arguments:

    Block - pointer to block to be reallocated.

    NewSize - new size in bytes of block. If the size is 0, this function
        works like MyFree, and the return value is NULL.

Return Value:

    Pointer to block of memory, or NULL if a block could not be allocated.
    In that case the original block remains unchanged.

--*/

{
#if MEM_DBG
    return MyDebugRealloc(Block,NewSize);
#else
    return REALLOC(Block, NewSize);
#endif
}


VOID
MyFree(
    IN CONST VOID *Block
    )

/*++

Routine Description:

    Free memory block previously allocated with MyMalloc or MyRealloc.

Arguments:

    Buffer - pointer to block to be freed.

Return Value:

    None.

--*/

{
#if MEM_DBG
    MyDebugFree(Block);
#else
    FREE ((void *)Block);
#endif
}


DWORD
MySize (
    IN CONST VOID *Block
    )
{
    return (DWORD)MEMSIZE((PVOID) Block);
}


BOOL
MemoryInitialize(
    IN BOOL Attach
    )
{
    if(Attach) {
        g_hHeap = GetProcessHeap();
        
#ifdef ANSI_SETUPAPI
        pInitFastAlloc();
#endif

#if MEM_DBG
        MemDebugInit(Attach);
#endif
        OutOfMemoryString = MyLoadString(IDS_OUTOFMEMORY);
        return(OutOfMemoryString != NULL);
    } else {
        MyFree(OutOfMemoryString);
#if MEM_DBG
        MemDebugInit(Attach);
#endif

#ifdef ANSI_SETUPAPI
        pTerminateFastAlloc();
#endif

        return(TRUE);
    }
}

VOID
OutOfMemory(
    IN HWND Owner OPTIONAL
    )
{
    //
    // Don't popup a dialog if we're not running interactively...
    //
    if(!(GlobalSetupFlags & PSPGF_NONINTERACTIVE)) {

        MYASSERT(OutOfMemoryString);

        //
        // Use special combination of flags that guarantee
        // display of the message box regardless of available memory.
        //
        MessageBox(
            Owner,
            OutOfMemoryString,
            NULL,
            MB_ICONHAND | MB_SYSTEMMODAL | MB_OK | MB_SETFOREGROUND
            );
    }
}


#ifdef ANSI_SETUPAPI

//
// Memory allocation routines to pool small memory allocations
//

#define FASTPOOL_MAX        8192
#define FASTPOOL_ALIGN      64          // must be a multiple of 16
#define FASTPOOL_POOL_SIZE  (32768 - sizeof(FASTPOOL))

//
// Our pool structure.  Each pool has n FASTBLOCK structs.
//

typedef struct _tagFASTPOOL {
    WORD UseCount;
    WORD Filled;
    DWORD AllocatedBytes;
    struct _tagFASTPOOL *Prev, *Next;

    // Important: This must be aligned on 16
    BYTE Memory[];

} FASTPOOL, *PFASTPOOL;

//
// Our block structure.  A block is either allocated or deallocated.
//

typedef struct {

    // Important: This struct must be aligned on a DWORD

    PFASTPOOL Pool;             // NULL = not pooled

    union {
        DWORD NonPoolSize;      // for non-pool allocations only
        struct {
            WORD SizeInBlocks;      // for pool allocations only
            WORD ActualSize;
        };
    };

#if DBG

    DWORD Signature;

#endif

} FASTBLOCK_ALLOC, *PFASTBLOCK_ALLOC;

typedef struct _tagFASTBLOCK_DEALLOC {
    // Important: This struct must be aligned on a DWORD

    PFASTPOOL Pool;             // never NULL

    union {
        DWORD Reserved;         // to match FASTBLOCK_ALLOC
        struct {
            WORD SizeInBlocks;
            WORD ActualSize;
        };
    };

    struct _tagFASTBLOCK_DEALLOC *NextDeleted, *PrevDeleted;

#if DBG

    DWORD Signature;

#endif

} FASTBLOCK_DEALLOC, *PFASTBLOCK_DEALLOC;


#if DBG

#define MARK_SIGNATURE(x)       (x) = 0x102898aa
#define VERIFY_SIGNATURE(x)     MYASSERT((x) == 0x102898aa)
#define RECORD_ALLOC(x)         pRecordAlloc(x)
#define RECORD_DEALLOC(x)       pRecordDeAlloc(x)

DWORD g_Allocs;
DWORD g_DeAllocs;
DWORD g_ReAllocs;
DWORD g_PoolAllocs;
DWORD g_BytesAllocated;
DWORD g_MaxBytesAllocated;
DWORD g_FreesThatFailed;

#define INC_STAT(x)     x++

VOID
pRecordAlloc (
    DWORD Size
    )
{
    g_Allocs++;
    g_BytesAllocated += Size;
    g_MaxBytesAllocated = max (g_MaxBytesAllocated, g_BytesAllocated);
}

VOID
pRecordDeAlloc (
    DWORD Size
    )
{
    g_DeAllocs++;
    g_BytesAllocated -= Size;
}

#else

#define MARK_SIGNATURE(x)
#define VERIFY_SIGNATURE(x)
#define RECORD_ALLOC(x)
#define RECORD_DEALLOC(x)

#define INC_STAT(x)

#endif


PFASTBLOCK_DEALLOC g_ReUseList[FASTPOOL_MAX / FASTPOOL_ALIGN];
PFASTPOOL g_HeadPool;

VOID
pInitFastAlloc (
    VOID
    )
{
    ZeroMemory (&g_ReUseList, sizeof (g_ReUseList));
    g_HeadPool = NULL;
}

#define ALIGN(Size,AllocSize)           \
    Size = max (Size, sizeof (FASTBLOCK_DEALLOC) - sizeof (FASTBLOCK_ALLOC));   \
    AllocSize = Size + sizeof (FASTBLOCK_ALLOC);                                \
    if (AllocSize & (FASTPOOL_ALIGN-1))                                         \
        AllocSize = (AllocSize & ~(FASTPOOL_ALIGN-1)) + FASTPOOL_ALIGN


PVOID
pFastAlloc (
    DWORD Size
    )
{
    PFASTPOOL Pool;
    PFASTBLOCK_ALLOC Block;
    DWORD AllocSize;
    UINT u;
    PFASTBLOCK_DEALLOC DelBlock;

    //
    // Size must be big enough to hold a FASTBLOCK_DEL struct
    //

    ALIGN(Size,AllocSize);

    if (AllocSize >= FASTPOOL_MAX) {
        //
        // For big blocks, use the normal APIs.  On Win9x, they're slow!!
        //

        Block = (PFASTBLOCK_ALLOC) REAL_ALLOC (AllocSize);

        if (Block) {
            Block->Pool = NULL;
            Block->NonPoolSize = Size;
            MARK_SIGNATURE(Block->Signature);
        }

    } else {
        u = AllocSize / FASTPOOL_ALIGN;
        MYASSERT (AllocSize == u * FASTPOOL_ALIGN);

        if (g_ReUseList[u]) {
            //
            // Here is where we get the speed.  We have a list of
            // deleted blocks that are just ready to go.
            //

            DelBlock = g_ReUseList[u];

            VERIFY_SIGNATURE (DelBlock->Signature);

            g_ReUseList[u] = DelBlock->NextDeleted;

            if (DelBlock->NextDeleted) {
                DelBlock->NextDeleted->PrevDeleted = NULL;
            }

            Block = (PFASTBLOCK_ALLOC) DelBlock;
            Block->ActualSize = (WORD) Size;
            Block->Pool->UseCount++;

            MARK_SIGNATURE(Block->Signature);

        } else {
            //
            // Check to see if we need to allocate a new block in the pool.
            // If so, put it at the head of the list, so g_HeadPool always
            // has enough space.
            //

            Block = NULL;

            if (!g_HeadPool || (g_HeadPool->AllocatedBytes + AllocSize > FASTPOOL_POOL_SIZE)) {

                if (g_HeadPool) {
                    g_HeadPool->Filled = 1;
                }

                Pool = (PFASTPOOL) REAL_ALLOC (FASTPOOL_POOL_SIZE + sizeof (FASTPOOL));
                if (Pool) {
                    INC_STAT(g_PoolAllocs);

                    Pool->UseCount = 0;
                    Pool->AllocatedBytes = 0;

                    Pool->Next = g_HeadPool;
                    if (g_HeadPool) {
                        g_HeadPool->Prev = Pool;
                    }
                    g_HeadPool = Pool;

                    Pool->Prev = NULL;
                    Pool->Filled = 0;
                }
            }

            if (g_HeadPool && (g_HeadPool->AllocatedBytes + AllocSize <= FASTPOOL_POOL_SIZE)) {
                //
                // Now we have space to alloc a FASTBLOCK struct
                //

                Block = (PFASTBLOCK_ALLOC) (g_HeadPool->Memory + g_HeadPool->AllocatedBytes);
                g_HeadPool->AllocatedBytes += AllocSize;

                Block->Pool = g_HeadPool;
                Block->SizeInBlocks = u;
                Block->ActualSize = (WORD) Size;

                MARK_SIGNATURE(Block->Signature);

                g_HeadPool->UseCount++;
            }
        }
    }

    if (!Block) {
        return NULL;
    }

    RECORD_ALLOC (AllocSize);

    return (PVOID) ((PBYTE) Block + sizeof (FASTBLOCK_ALLOC));
}


VOID
pFastDeAlloc (
    PVOID Alloc
    )
{
    PFASTBLOCK_ALLOC Block;
    UINT u, v;
    PFASTBLOCK_DEALLOC DelBlock;
    PFASTPOOL Pool;

    try {
        Block = (PFASTBLOCK_ALLOC) ((PBYTE) Alloc - sizeof (FASTBLOCK_ALLOC));

        VERIFY_SIGNATURE(Block->Signature);

        if (!Block->Pool) {
            //
            // Large block
            //

            u = Block->NonPoolSize;
            ALIGN(u,v);
            RECORD_DEALLOC (v);
            REAL_FREE (Block);

        } else if (Block->ActualSize) {
            //
            // Small block
            //
            // Convert the allocated block into a deleted block
            //

            u = Block->SizeInBlocks;
            RECORD_DEALLOC (u * FASTPOOL_ALIGN);

            DelBlock = (PFASTBLOCK_DEALLOC) Block;

            MARK_SIGNATURE(DelBlock->Signature);

            DelBlock->NextDeleted = g_ReUseList[u];
            DelBlock->PrevDeleted = NULL;

            if (DelBlock->NextDeleted) {
                DelBlock->NextDeleted->PrevDeleted = DelBlock;
            }

            DelBlock->ActualSize = 0;

            g_ReUseList[u] = DelBlock;

            Pool = DelBlock->Pool;
            Pool->UseCount--;

            //
            // If the pool use count goes to zero, and it was used to its
            // capacity, remove the pool.
            //

            if (Pool->Filled && !Pool->UseCount) {
                //
                // Remove all of the entries in the reuse array
                //

                u = 0;
                v = Pool->AllocatedBytes;

                while (u < v) {
                    DelBlock = (PFASTBLOCK_DEALLOC) (Pool->Memory + u);

                    VERIFY_SIGNATURE(DelBlock->Signature);

                    if (DelBlock->PrevDeleted) {
                        DelBlock->PrevDeleted->NextDeleted = DelBlock->NextDeleted;
                    } else {
                        g_ReUseList[DelBlock->SizeInBlocks] = DelBlock->NextDeleted;
                    }

                    if (DelBlock->NextDeleted) {
                        DelBlock->NextDeleted->PrevDeleted = DelBlock->PrevDeleted;
                    }

                    u += DelBlock->SizeInBlocks * FASTPOOL_ALIGN;
                }

                //
                // Deallocate the pool
                //

                if (Pool->Prev) {
                    Pool->Prev->Next = Pool->Next;
                } else {
                    g_HeadPool = Pool->Next;
                }

                if (Pool->Next) {
                    Pool->Next->Prev = Pool->Prev;
                }

                REAL_FREE (Pool);
            }
        } else {
            // Block already free!
            MYASSERT (FALSE);

#if DBG
            g_FreesThatFailed++;
#endif

        }
    } except (TRUE) {
    }
}


DWORD
pFastSize (
    PVOID Alloc
    )
{
    PFASTBLOCK_ALLOC Block;
    DWORD Size;

    try {
        Block = (PFASTBLOCK_ALLOC) ((PBYTE) Alloc - sizeof (FASTBLOCK_ALLOC));

        if (Block->Pool) {
            Size = (DWORD) Block->ActualSize;
            MYASSERT (Size);
        } else {
            Size = Block->NonPoolSize;
            MYASSERT (Size);
        }

    } except (TRUE) {
        Size = 0;
    }

    return Size;
}


PVOID
pFastReAlloc (
    PVOID Alloc,
    DWORD NewSize
    )
{
    PFASTBLOCK_ALLOC Block;
    PBYTE Data = NULL;
    DWORD Size;
    DWORD OrgAllocSize;
    DWORD NewAllocSize;

    if (!Alloc) {
        return pFastAlloc (NewSize);
    }

    try {
        Size = pFastSize (Alloc);

        ALIGN(Size,OrgAllocSize);
        ALIGN(NewSize,NewAllocSize);

        if (OrgAllocSize != NewAllocSize) {

            Data = pFastAlloc (NewSize);
            if (Data) {
                INC_STAT(g_ReAllocs);
                CopyMemory (Data, Alloc, min (NewSize, Size));
                pFastDeAlloc (Alloc);
            }
        } else {
            Data = (PBYTE) Alloc;
            Block = (PFASTBLOCK_ALLOC) ((PBYTE) Data - sizeof (FASTBLOCK_ALLOC));
            if (Block->Pool) {
                //
                // Fast allocators are used for small allocations only (less
                // than FASTPOOL_MAX), so the following cast is safe.
                //
                Block->ActualSize = (WORD)NewSize;
            } else {
                Block->NonPoolSize = NewSize;
            }
        }        
        
    } except (TRUE) {
    }

    return Data;
}


VOID
pTerminateFastAlloc (
    VOID
    )
{
#if DBG

    CHAR Msg[1024];

    wsprintfA (
        Msg,
        "SETUPAPI.DLL STATISTICS\r\n\r\n"
            "Allocs: %u\r\n"
            "DeAllocs: %u\r\n"
            "ReAllocs: %u\r\n"
            "Pool Allocs: %u\r\n"
            "Bytes Allocated Now: %u\r\n"
            "Max Bytes Allocated: %u\r\n"
            "Frees That Failed: %u",
        g_Allocs,
        g_DeAllocs,
        g_ReAllocs,
        g_PoolAllocs,
        g_BytesAllocated,
        g_MaxBytesAllocated,
        g_FreesThatFailed
        );

    //MessageBox (NULL, Msg, NULL, MB_OK);
    OutputDebugString (Msg);

#endif
}



#endif



