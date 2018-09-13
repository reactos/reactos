/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    memalloc.cxx

Abstract:

    Debug-only memory allocation routines

    Contents:
        InetInitializeDebugMemoryPackage
        InetTerminateDebugMemoryPackage
        InetAllocateMemory
        InetReallocateMemory
        (InetIsBlockMoveable)
        InetFreeMemory
        (InetCheckBlockConsistency)
        InetLockMemory
        InetUnlockMemory
        InetMemorySize
        InetCheckDebugMemoryFreed
        (x86SleazeCallersAddress)

Author:

     Richard L Firth (rfirth) 02-Feb-1995

Environment:

    Win32 user mode

Revision History:

    02-Feb-1995
        Created

--*/

#include <wininetp.h>
#include "rprintf.h"

#if INET_DEBUG

//
// manifests
//

#define HEADER_SIGNATURE    0x414d454d  // 'MEMA'
#define FOOTER_SIGNATURE    0x434f4c4c  // 'LLOC'
#define DWORD_FILL          0xa9a9a9a9
#define BYTE_FILL           0xa9
#define BYTE_FILL_EXTRA     0xcb
#define GUARD_DWORD_FILL    0xcccd21f4
#define DWORD_FREE_FILL     0xb7b7b7b7
#define BYTE_FREE_FILL      0xb7

//
// private types
//

typedef struct {

    //
    // hMoveable - local handle of moveable memory that this tag links
    //

    HLOCAL hMoveable;

} DEBUG_MOVEABLE_TAG, *LPDEBUG_MOVEABLE_TAG;

typedef struct {

    //
    // List - maintains a list of allocated blocks
    //

    LIST_ENTRY List;

    //
    // BlockLength - the size of this block, *including* all headers, footers
    // and padding
    //

    UINT BlockLength;

    //
    // RealLength - the original caller request
    //

    UINT RealLength;

    //
    // Signature - just used as a sanity check to ensure that what we are
    // dealing with is actually a block we allocated
    //

    DWORD Signature;

    //
    // LockCount - if this is moveable memory, keeps the number of times this
    // block has been locked
    //

    LONG LockCount;

    //
    // Flags - what type of memory this is, etc.
    //

    DWORD Flags;

    //
    // LastAccessOperation - the operation caller at LastAccessReturnAddress
    // performed
    //

    MEMALLOC_ACTION LastAccessOperation;

    //
    // LastAccessReturnAddress - caller of last function to perform memory
    // function operation (alloc, lock, realloc, unlock, etc) on this block
    //

    LPVOID LastAccessReturnAddress[2];

    //
    // CreatorReturnAddress - return EIP (x86-only) of caller of allocator
    // and caller of caller
    //

    LPVOID CreatorReturnAddress[2];

    //
    // Tag - if this is moveable memory, we can't add this block to the allocated
    // block list, we have to allocate a DEBUG_MOVEABLE_TAG, link that and point
    // to it from here
    //

    LPDEBUG_MOVEABLE_TAG Tag;

    //
    // Guard - just a sentinel to find out if the caller is writing before the
    // start of this block
    //

    DWORD Guard[4];

    //
    // sizeof(MEMORY_SIGNATURE) currently 17 DWORDs
    //

} DEBUG_MEMORY_HEADER, *LPDEBUG_MEMORY_HEADER;

typedef struct {

    //
    // Guard - allows us to determine if the end of allocated memory was
    // overwritten
    //

    DWORD Guard[4];

    //
    // Signature - should be the footer signature
    //

    DWORD Signature;

    //
    // BlockLength - should be the same as the header
    //

    DWORD BlockLength;

    //
    // Guard2 - to make sure the end of the block is coherent
    //

    DWORD Guard2[2];

    //
    // sizeof(DEBUG_MEMORY_FOOTER) currently 8 DWORDs
    //

} DEBUG_MEMORY_FOOTER, *LPDEBUG_MEMORY_FOOTER;

//
// data
//

LONG ActualMemoryAllocated = 0;
LONG BlockLengthAllocated = 0;
LONG RealLengthAllocated = 0;
DWORD MemoryAllocations = 0;
DWORD MemoryFrees = 0;
SERIALIZED_LIST AllocatedBlockList;

//
// macros
//

#if defined(i386)

#define GET_CALLERS_ADDRESS(p, pp)  x86SleazeCallersAddress(p, pp)

#else

#define GET_CALLERS_ADDRESS(p, pp)

#endif // defined(i386)

//
// private prototypes
//

PRIVATE
BOOL
InetIsBlockMoveable(
    IN HLOCAL hLocal
    );

PRIVATE
VOID
InetCheckBlockConsistency(
    IN LPVOID lpMemory
    );

PRIVATE
VOID
x86SleazeCallersAddress(
    LPVOID* pCaller,
    LPVOID* pCallersCaller
    );

//
// functions
//


VOID
InetInitializeDebugMemoryPackage(
    VOID
    )

/*++

Routine Description:

    Just initializes data items in this module

Arguments:

    None.

Return Value:

    None.

--*/

{
    static BOOL MemoryPackageInitialized = FALSE;

    if (!MemoryPackageInitialized) {
        InitializeSerializedList(&AllocatedBlockList);
        MemoryPackageInitialized = TRUE;
    } else {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("Memory package already initialized\n"
                    ));

        DEBUG_BREAK(MEMALLOC);

    }
}


VOID
InetTerminateDebugMemoryPackage(
    VOID
    )

/*++

Routine Description:

    Undoes any resource allocation in InetInitializeDebugMemoryPackage, after
    checking that all memory is freed

Arguments:

    None.

Return Value:

    None.

--*/

{
    InetCheckDebugMemoryFreed();
    TerminateSerializedList(&AllocatedBlockList);
}


HLOCAL
InetAllocateMemory(
    IN UINT LocalAllocFlags,
    IN UINT NumberOfBytes
    )

/*++

Routine Description:

    Debug memory allocator: allocates memory with head & tail. Fills memory
    with signature unless otherwise requested. If this is moveable memory
    then the caller must lock the memory with InetLockMemory(), else a pointer
    will be returned to the head of the heap's real start-of-block, and the
    caller will probably nuke the signature contents (but we should discover
    this when the block is freed)

Arguments:

    LocalAllocFlags - flags to be passed on to LocalAlloc
    NumberOfBytes   - to allocate for caller

Return Value:

    LPVOID
        Success - pointer to memory after DEBUG_MEMORY_HEADER
        Failure - NULL

--*/

{
    HLOCAL hLocal;
    UINT blockLength;
    BOOL isMoveable;

    isMoveable = (LocalAllocFlags & LMEM_MOVEABLE) ? TRUE : FALSE;
    blockLength = ROUND_UP_DWORD(NumberOfBytes)
                + sizeof(DEBUG_MEMORY_HEADER)
                + sizeof(DEBUG_MEMORY_FOOTER)
                ;

    //
    // possible problem: if NumberOfBytes + signatures would overflow UINT.
    // Only really problematic on 16-bit platforms
    //

    if (blockLength < NumberOfBytes) {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("can't allocate %lu bytes: would overflow\n",
                    (DWORD)NumberOfBytes
                    ));

        DEBUG_BREAK(MEMALLOC);

        return (HLOCAL)NULL;
    }

    hLocal = LocalAlloc(LocalAllocFlags, blockLength);
    if (hLocal != NULL) {

        LPVOID lpMem;
        LPDEBUG_MEMORY_HEADER lpHeader;
        DWORD dwFiller;
        BYTE bFiller;
        UINT dwFillLength;
        UINT bFillLength1;
        UINT bFillLength2;
        UINT i;
        LPVOID userPointer;

        ActualMemoryAllocated += LocalSize(hLocal);
        BlockLengthAllocated += blockLength;
        RealLengthAllocated += NumberOfBytes;
        ++MemoryAllocations;

        if (isMoveable) {
            lpMem = (LPVOID)LocalLock(hLocal);
            if (lpMem == NULL) {

                DEBUG_PRINT(MEMALLOC,
                            ERROR,
                            ("LocalLock(%x) failed: %d\n",
                            hLocal,
                            GetLastError()
                            ));

                DEBUG_BREAK(MEMALLOC);

            }
        } else {
            lpMem = (LPVOID)hLocal;
        }

        lpHeader = (LPDEBUG_MEMORY_HEADER)lpMem;
        InitializeListHead(&lpHeader->List);
        lpHeader->BlockLength = blockLength;
        lpHeader->RealLength = NumberOfBytes;
        lpHeader->Signature = HEADER_SIGNATURE;
        lpHeader->LockCount = 0;
        lpHeader->Flags = LocalAllocFlags;

        GET_CALLERS_ADDRESS(&lpHeader->CreatorReturnAddress[0],
                            &lpHeader->CreatorReturnAddress[1]
                            );

        lpHeader->LastAccessOperation = MemAllocate;

        for (i = 0; i < ARRAY_ELEMENTS(lpHeader->Guard); ++i) {
            lpHeader->Guard[i] = GUARD_DWORD_FILL;
        }

        if (LocalAllocFlags & LMEM_ZEROINIT) {
            dwFiller = 0;
            bFiller = 0;
        } else {
            dwFiller = DWORD_FILL;
            bFiller = BYTE_FILL;
        }
        dwFillLength = NumberOfBytes / sizeof(DWORD);
        bFillLength1 = NumberOfBytes % sizeof(DWORD);
        bFillLength2 = bFillLength1 ? (sizeof(DWORD) - bFillLength1) : 0;
        userPointer = (LPVOID)(lpHeader + 1);

        LPDWORD lpdwUserPointer = (LPDWORD)userPointer;

        for (i = 0; i < dwFillLength; ++i) {
            *lpdwUserPointer++ = dwFiller;
        }

        LPBYTE lpbUserPointer = (LPBYTE)lpdwUserPointer;

        for (i = 0; i < bFillLength1; ++i) {
            *lpbUserPointer++ = bFiller;
        }
        for (i = 0; i < bFillLength2; ++i) {
            *lpbUserPointer++ = BYTE_FILL_EXTRA;
        }

        userPointer = (LPVOID)lpbUserPointer;

        for (i = 0; i < ARRAY_ELEMENTS(((LPDEBUG_MEMORY_FOOTER)userPointer)->Guard); ++i) {
            ((LPDEBUG_MEMORY_FOOTER)userPointer)->Guard[i] = GUARD_DWORD_FILL;
        }
        ((LPDEBUG_MEMORY_FOOTER)userPointer)->BlockLength = blockLength;
        ((LPDEBUG_MEMORY_FOOTER)userPointer)->Signature = FOOTER_SIGNATURE;
        for (i = 0; i < ARRAY_ELEMENTS(((LPDEBUG_MEMORY_FOOTER)userPointer)->Guard2); ++i) {
            ((LPDEBUG_MEMORY_FOOTER)userPointer)->Guard2[i] = GUARD_DWORD_FILL;
        }

        //
        // if this is moveable memory, then we can't link it into the allocated
        // block list because if it moves, the list gets nuked. So we have to
        // allocate a DEBUG_MOVEABLE_TAG, link that and point to it from here
        //

        if (isMoveable) {

            LPDEBUG_MOVEABLE_TAG lpTag;

            lpTag = (LPDEBUG_MOVEABLE_TAG)InetAllocateMemory(LMEM_FIXED, sizeof(DEBUG_MOVEABLE_TAG));

            INET_ASSERT(lpTag != NULL);

            lpTag->hMoveable = hLocal;
            lpHeader->Tag = lpTag;
        } else {
            InsertAtHeadOfSerializedList(&AllocatedBlockList, &lpHeader->List);
        }

        if (isMoveable) {
            if (LocalUnlock(hLocal)) {

                DEBUG_PRINT(MEMALLOC,
                            ERROR,
                            ("LocalUnlock(%x): memory still locked\n",
                            hLocal
                            ));

                DEBUG_BREAK(MEMALLOC);

            } else {

                DWORD err;

                err = GetLastError();
                if (err != NO_ERROR) {

                    DEBUG_PRINT(MEMALLOC,
                                ERROR,
                                ("LocalUnlock(%x) returns %d\n",
                                hLocal,
                                err
                                ));

                    DEBUG_BREAK(MEMALLOC);

                }
            }
        } else {
            hLocal = (HLOCAL)(lpHeader + 1);
        }
    } else {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("failed to allocate %u bytes memory\n",
                    blockLength
                    ));

        DEBUG_BREAK(MEMALLOC);

    }
    return hLocal;
}


HLOCAL
InetReallocateMemory(
    IN HLOCAL hLocal,
    IN UINT Size,
    IN UINT Flags,
    IN BOOL IsMoveable
    )

/*++

Routine Description:

    Reallocates previously allocated block

    BUGBUG - this doesn't handle the more exotic LocalReAlloc stuff, like
             DISCARDABLE memory, allocating/freeing through realloc etc

Arguments:

    hLocal      - block to reallocate
    Size        - new size
    Flags       - new flags
    IsMoveable  - TRUE if this is moveable memory. We need this help because
                  there is no good way to find out from hLocal whether this
                  memory is moveable or fixed

Return Value:

    HLOCAL

--*/

{
    LPDEBUG_MEMORY_HEADER lpHeader;
    UINT realLength;
    UINT heapLength;

    //
    // can't handle reallocating down to zero
    //

    INET_ASSERT(Size != 0);

    if (IsMoveable) {
        lpHeader = (LPDEBUG_MEMORY_HEADER)LocalLock(hLocal);
        heapLength = LocalSize(hLocal);
    } else {
        lpHeader = (LPDEBUG_MEMORY_HEADER)hLocal - 1;
        heapLength = LocalSize((HLOCAL)lpHeader);
    }

    InetCheckBlockConsistency((LPVOID)lpHeader);

    if (IsMoveable) {
        LocalUnlock(hLocal);
    }

    realLength = Size;

    Size = ROUND_UP_DWORD(Size)
         + sizeof(DEBUG_MEMORY_HEADER)
         + sizeof(DEBUG_MEMORY_FOOTER)
         ;

    ActualMemoryAllocated -= heapLength;
    BlockLengthAllocated -= lpHeader->BlockLength;
    RealLengthAllocated -= lpHeader->RealLength;

    hLocal = LocalReAlloc(hLocal, Size, Flags);
    if (hLocal != NULL) {

        LPBYTE extraPointer;
        UINT extraLength;
        UINT i;
        LPDEBUG_MEMORY_FOOTER lpFooter;

        if (IsMoveable) {
            lpHeader = (LPDEBUG_MEMORY_HEADER)LocalLock(hLocal);
        } else {
            lpHeader = (LPDEBUG_MEMORY_HEADER)hLocal;
        }

        lpHeader->BlockLength = Size;
        lpHeader->RealLength = realLength;
        lpHeader->Flags = Flags;

        GET_CALLERS_ADDRESS(&lpHeader->LastAccessReturnAddress[0],
                            &lpHeader->LastAccessReturnAddress[1]
                            );

        lpHeader->LastAccessOperation = MemReallocate;

        extraPointer = (LPBYTE)(lpHeader + 1) + realLength;
        extraLength = (sizeof(DWORD) - (realLength % sizeof(DWORD)))
                    & (sizeof(DWORD) - 1)
                    ;
        for (i = 0; i < extraLength; ++i) {
            *extraPointer++ = BYTE_FILL_EXTRA;
        }
        lpFooter = (LPDEBUG_MEMORY_FOOTER)((LPBYTE)(lpHeader + 1)
                 + ROUND_UP_DWORD(realLength)
                 );
        for (i = 0; i < ARRAY_ELEMENTS(lpFooter->Guard); ++i) {
            lpFooter->Guard[i] = GUARD_DWORD_FILL;
        }
        lpFooter->Signature = FOOTER_SIGNATURE;
        lpFooter->BlockLength = Size;
        for (i = 0; i < ARRAY_ELEMENTS(lpFooter->Guard2); ++i) {
            lpFooter->Guard2[i] = GUARD_DWORD_FILL;
        }
        ActualMemoryAllocated += LocalSize(hLocal);
        BlockLengthAllocated += Size;
        RealLengthAllocated += lpHeader->RealLength;
        if (IsMoveable) {
            LocalUnlock(hLocal);
        } else {
            hLocal = (HLOCAL)(lpHeader + 1);
        }
    } else {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("failed to reallocate %u bytes memory. Last error = %d\n",
                    Size,
                    GetLastError()
                    ));

        DEBUG_BREAK(MEMALLOC);

    }
    return hLocal;
}


PRIVATE
BOOL
InetIsBlockMoveable(
    IN HLOCAL hLocal
    )

/*++

Routine Description:

    Determines if hLocal is moveable or fixed memory

Arguments:

    hLocal  -

Return Value:

    BOOL

--*/

{
    LPDEBUG_MEMORY_HEADER lpHeader;
    BOOL isMoveable;

    //
    // BUGBUG - this method won't work for Win32s unless it supports SEH. But
    //          there is another method...
    //

    lpHeader = (LPDEBUG_MEMORY_HEADER)hLocal - 1;
    __try {
        if (lpHeader->Signature == HEADER_SIGNATURE) {
            isMoveable = FALSE;
        } else {

            lpHeader = (LPDEBUG_MEMORY_HEADER)LocalLock(hLocal);

            INET_ASSERT(lpHeader != NULL);

            isMoveable = (BOOL)(lpHeader->Signature == HEADER_SIGNATURE);
            LocalUnlock(hLocal);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // yeowww! hLocal must be a handle to moveable memory. Either that, or
        // it is completely bogus
        //

        lpHeader = (LPDEBUG_MEMORY_HEADER)LocalLock(hLocal);

        INET_ASSERT(lpHeader != NULL);

        isMoveable = (BOOL)(lpHeader->Signature == HEADER_SIGNATURE);
        LocalUnlock(hLocal);
    }
    return isMoveable;
}


HLOCAL
InetFreeMemory(
    IN HLOCAL hLocal,
    IN BOOL IsMoveable
    )

/*++

Routine Description:

    Debug memory deallocator: checks memory is already allocated and that the
    head and tail structures are still ok. Fills freed memory with signature

Arguments:

    hLocal  - address/handle of memory to free
    IsMoveable  - TRUE if this is moveable memory. We need this help because
                  there is no good way to determine if the memory is moveable
                  or fixed

Return Value:

    HLOCAL
        Success - NULL
        Failure - hLocal

--*/

{
    UINT memFlags;
    LPDEBUG_MEMORY_HEADER lpHeader;
    BOOL isMoveable;
    UINT memSize;
    UINT blockLength;
    UINT realLength;

    if (!IsMoveable) {
        hLocal = (HLOCAL)((LPDEBUG_MEMORY_HEADER)hLocal - 1);
    }

    memFlags = LocalFlags(hLocal);

    INET_ASSERT(memFlags != LMEM_INVALID_HANDLE);
    INET_ASSERT((memFlags & LMEM_LOCKCOUNT) == 0);

    if (IsMoveable) {
        lpHeader = (LPDEBUG_MEMORY_HEADER)LocalLock(hLocal);

        INET_ASSERT(lpHeader != NULL);

    } else {
        lpHeader = (LPDEBUG_MEMORY_HEADER)hLocal;
    }

    memSize = LocalSize(hLocal);

    INET_ASSERT((lpHeader->BlockLength <= memSize)
                && !(lpHeader->BlockLength & (sizeof(DWORD) - 1))
                && (lpHeader->RealLength < lpHeader->BlockLength)
                );

    InetCheckBlockConsistency((LPVOID)lpHeader);

    //
    // if this is moveable memory then we didn't link it to the allocated
    // block list, but allocated a DEBUG_MOVEABLE_TAG to do the job. We
    // must remove it
    //

    if (IsMoveable) {

        LPDEBUG_MOVEABLE_TAG lpTag;

        lpTag = lpHeader->Tag;

        INET_ASSERT(lpTag->hMoveable == hLocal);

        InetFreeMemory(lpTag, FALSE);
    } else {
        RemoveFromSerializedList(&AllocatedBlockList, &lpHeader->List);
    }

    if (IsMoveable) {

        BOOL stillLocked;

        stillLocked = LocalUnlock(hLocal);

        INET_ASSERT(!stillLocked);
        INET_ASSERT(GetLastError() == NO_ERROR);

    }

    blockLength = lpHeader->BlockLength;
    realLength = lpHeader->RealLength;
    hLocal = LocalFree(hLocal);

    INET_ASSERT(hLocal == NULL);

    ActualMemoryAllocated -= memSize;
    BlockLengthAllocated -= blockLength;
    RealLengthAllocated -= realLength;
    ++MemoryFrees;

    return hLocal;
}


PRIVATE
VOID
InetCheckBlockConsistency(
    IN LPVOID lpMemory
    )

/*++

Routine Description:

    Checks that what we think is a valid allocated block (allocated by
    InetAllocateMemory), really is

Arguments:

    lpMemory    - pointer to what we think is DEBUG_MEMORY_HEADER

Return Value:

    None.

--*/

{
    LPDEBUG_MEMORY_HEADER lpHeader;
    LPDEBUG_MEMORY_FOOTER lpFooter;
    UINT i;
    BOOL headerGuardOverrun;
    BOOL footerGuardOverrun;
    BOOL footerGuard2Overrun;
    BOOL extraMemoryOverrun;
    LPBYTE lpExtraMemory;
    UINT byteLength;

    __try {
        lpHeader = (LPDEBUG_MEMORY_HEADER)lpMemory;
        lpFooter = (LPDEBUG_MEMORY_FOOTER)((LPBYTE)lpMemory
                 + (lpHeader->BlockLength - sizeof(DEBUG_MEMORY_FOOTER)))
                 ;

        headerGuardOverrun = FALSE;
        for (i = 0; i < ARRAY_ELEMENTS(lpHeader->Guard); ++i) {
            if (lpHeader->Guard[i] != GUARD_DWORD_FILL) {
                headerGuardOverrun = TRUE;
                break;
            }
        }

        footerGuardOverrun = FALSE;
        for (i = 0; i < ARRAY_ELEMENTS(lpFooter->Guard); ++i) {
            if (lpFooter->Guard[i] != GUARD_DWORD_FILL) {
                footerGuardOverrun = TRUE;
                break;
            }
        }

        footerGuard2Overrun = FALSE;
        for (i = 0; i < ARRAY_ELEMENTS(lpFooter->Guard2); ++i) {
            if (lpFooter->Guard2[i] != GUARD_DWORD_FILL) {
                footerGuard2Overrun = TRUE;
                break;
            }
        }

        lpExtraMemory = (LPBYTE)(lpHeader + 1) + lpHeader->RealLength;
        extraMemoryOverrun = FALSE;
        byteLength = ROUND_UP_DWORD(lpHeader->RealLength) - lpHeader->RealLength;
        for (i = 0; i < byteLength; ++i) {
            if (lpExtraMemory[i] != BYTE_FILL_EXTRA) {
                extraMemoryOverrun = TRUE;
                break;
            }
        }

        if (headerGuardOverrun
        || footerGuardOverrun
        || footerGuard2Overrun
        || extraMemoryOverrun
        || (lpHeader->Signature != HEADER_SIGNATURE)
        || (lpFooter->Signature != FOOTER_SIGNATURE)
        || (lpFooter->BlockLength != lpHeader->BlockLength)) {

            DEBUG_PRINT(MEMALLOC,
                        ERROR,
                        ("Bad block: %x\n",
                        lpMemory
                        ));

            DEBUG_BREAK(MEMALLOC);

        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        DEBUG_PRINT(MEMALLOC,
                    FATAL,
                    ("Invalid block %x - exception occurred\n",
                    lpMemory
                    ));

        DEBUG_BREAK(MEMALLOC);

    }
}


LPVOID
InetLockMemory(
    IN HLOCAL hLocal
    )

/*++

Routine Description:

    Locks a moveable memory block and increments the lock count. Checks block
    consistency

Arguments:

    hLocal  - handle of moveable memory to lock

Return Value:

    LPVOID
        pointer to locked memory

--*/

{
    LPDEBUG_MEMORY_HEADER lpHeader = NULL;
    UINT memFlags;

    memFlags = LocalFlags(hLocal);

    INET_ASSERT(memFlags != LMEM_INVALID_HANDLE);

    lpHeader = (LPDEBUG_MEMORY_HEADER)LocalLock(hLocal);

    INET_ASSERT(lpHeader != NULL);

    InetCheckBlockConsistency((LPVOID)lpHeader);
    ++lpHeader->LockCount;

    GET_CALLERS_ADDRESS(&lpHeader->LastAccessReturnAddress[0],
                        &lpHeader->LastAccessReturnAddress[1]
                        );

    lpHeader->LastAccessOperation = MemLock;

    memFlags = LocalFlags(hLocal);

    INET_ASSERT((memFlags != LMEM_INVALID_HANDLE)
                && (lpHeader->LockCount == (LONG)(memFlags & LMEM_LOCKCOUNT))
                );

    return ++lpHeader;
}


BOOL
InetUnlockMemory(
    IN HLOCAL hLocal
    )

/*++

Routine Description:

    Unlocks a (locked!) moveable memory block

Arguments:

    hLocal  - handle (pointer) of block to unlock

Return Value:

    None.

--*/

{
    UINT memFlags;
    BOOL stillLocked;
    LPDEBUG_MEMORY_HEADER lpHeader;
    DWORD lockCount;

    memFlags = LocalFlags(hLocal);

    INET_ASSERT(memFlags != LMEM_INVALID_HANDLE);
    INET_ASSERT((memFlags & LMEM_LOCKCOUNT) >= 1);

    //
    // memory must be locked or LocalFlags would have returned error.
    // Lock memory again to get pointer to block, then unlock it.
    // There should still be at least one lock on the block
    //

    lpHeader = (LPDEBUG_MEMORY_HEADER)LocalLock(hLocal);
    LocalUnlock(hLocal);

    InetCheckBlockConsistency((LPVOID)lpHeader);

    GET_CALLERS_ADDRESS(&lpHeader->LastAccessReturnAddress[0],
                        &lpHeader->LastAccessReturnAddress[1]
                        );

    lpHeader->LastAccessOperation = MemUnlock;

    lockCount = --lpHeader->LockCount;
    stillLocked = LocalUnlock(hLocal);

    INET_ASSERT(stillLocked ? (lockCount > 0) : GetLastError() == NO_ERROR);

    return stillLocked;
}


UINT
InetMemorySize(
    IN HLOCAL hLocal,
    IN BOOL IsMoveable
    )

/*++

Routine Description:

    Returns allocated block size

Arguments:

    hLocal      - memory handle
    IsMoveable  - TRUE if hLocal is a handle to moveable memory >>> THAT IS NOT
                  LOCKED <<<

Return Value:

    UINT

--*/

{
    UINT size;
    UINT sizeInHeader;
    LPDEBUG_MEMORY_HEADER lpHeader;

    if (IsMoveable) {
        lpHeader = (LPDEBUG_MEMORY_HEADER)LocalLock(hLocal);

        INET_ASSERT(lpHeader != NULL);

        sizeInHeader = lpHeader->RealLength;
        size = LocalSize(hLocal);
        LocalUnlock(hLocal);
    } else {
        lpHeader = (LPDEBUG_MEMORY_HEADER)hLocal - 1;

        INET_ASSERT(lpHeader->Signature == HEADER_SIGNATURE);

        sizeInHeader = lpHeader->RealLength;
        size = LocalSize((HLOCAL)lpHeader);
    }

    INET_ASSERT((sizeInHeader <= size)
                && (size >= sizeof(DEBUG_MEMORY_HEADER) + sizeof(DEBUG_MEMORY_FOOTER))
                );

    return sizeInHeader;
}


BOOL
InetCheckDebugMemoryFreed(
    VOID
    )

/*++

Routine Description:

    Check that we don't have any memory allocated

Arguments:

    None.

Return Value:

    BOOL

--*/

{
    if (ActualMemoryAllocated || (MemoryFrees != MemoryAllocations)) {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("MemoryAllocated = %ld, MemoryAllocations = %lu, MemoryFrees = %lu\n",
                    ActualMemoryAllocated,
                    MemoryAllocations,
                    MemoryFrees
                    ));

        DEBUG_BREAK(MEMALLOC);

        return FALSE;
    }
    return TRUE;
}

#if defined(i386)


VOID
x86SleazeCallersAddress(
    LPVOID* pCaller,
    LPVOID* pCallersCaller
    )

/*++

Routine Description:

    This is a sleazy function that reads return addresses out of the stack/
    frame pointer (ebp). We pluck out the return address of the function
    that called THE FUNCTION THAT CALLED THIS FUNCTION, and the caller of
    that function. Returning the return address of the function that called
    this function is not interesting to that caller (almost worthy of Sir
    Humphrey Appleby isn't it?)

    Assumes:

        my ebp  =>  | caller's ebp |
                    | caller's eip |
                    | arg #1       | (pCaller)
                    | arg #2       | (pCallersCaller

Arguments:

    pCaller         - place where we return addres of function that called
                      the function that called this function
    pCallersCaller  - place where we return caller of above

Return Value:

    None.

--*/

{

    //
    // this only works on x86 and only if not fpo functions!
    //

    LPVOID* ebp;

    ebp = (PVOID*)&pCaller - 2; // told you it was sleazy
    ebp = (PVOID*)*(PVOID*)ebp;
    *pCaller = *(ebp + 1);
    ebp = (PVOID*)*(PVOID*)ebp;
    *pCallersCaller = *(ebp + 1);
}

#endif // defined(i386)

#endif // INET_DEBUG
