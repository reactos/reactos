/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    debugmem.cxx

Abstract:

    Debug memory allocator

    Contents:
        InternetDebugMemInitialize
        InternetDebugMemTerminate
        InternetDebugAllocMem
        InternetDebugFreeMem
        InternetDebugReAllocMem
        InternetDebugSizeMem
        InternetDebugCheckMemFreed
        InternetDebugMemReport
        (InternetDebugCheckMemBlock)
        (DebugFillMem)
        (InternetAlloc)
        (InternetFree)
        (InternetReAlloc)
        (InternetSize)
        (InternetHeapAlloc)
        (InternetHeapReAlloc)
        (InternetHeapFree)
        (InternetHeapSize)
        (InternetDebugMemTest)
        (ReportMemoryUsage)
        (ReportMemoryBlocks)
        (DumpDeferredFreeList)
        (DumpMemoryList)
        (FindAndDumpDeferredBlock)
        (DumpBlock)
        (DumpDebugMemoryHeader)
        (DumpDebugMemoryFooter)
        (DumpUserData)
        (MapLastAccessOperation)
        (MapMemoryFlags)
        (DbgMemGetDebugSymbol)

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

#if defined(USE_DEBUG_MEMORY)

//
// manifests
//

#define DEFAULT_INITIAL_HEAP_SIZE   (64 K)
#define DEFAULT_MAXIMUM_HEAP_SIZE   (1 M)
#define DEFAULT_HEADER_GUARD_SIZE   32
#define DEFAULT_FOOTER_GUARD_SIZE   32
#define DEFAULT_ALLOC_ALIGNMENT     4
#define HEADER_SIGNATURE            0x414d454d  // "MEMA"
#define FOOTER_SIGNATURE            0x434f4c4c  // "LLOC"
#define DWORD_ALLOC_FILL            0xc5c5c5c5
#define BYTE_ALLOC_FILL             0xc5
#define BYTE_ALLOC_FILL_EXTRA       0x88
#define GUARD_DWORD_FILL            0x44524147  // "GARD"
#define DWORD_FREE_FILL             0xb7b7b7b7
#define BYTE_FREE_FILL              0xb7
#define DEFAULT_MAX_BLOCKS_DUMPED   1024
#define DEFAULT_MAX_DATA_DUMPED     65536
#define DEFAULT_BACKTRACE_DEPTH     2

//
// only perform stack dump for x86 (or other stack-based processors)
//

#if defined(i386)
#define DUMP_STACK  1
#else
#define DUMP_STACK  0
#endif

//
// just using one stack these days
//

#define ONE_STACK   1

//
// private types
//

typedef enum {
    MemAllocate = 0x6f6c6c41,   // "Allo"
    MemReallocate = 0x6c416552, // "ReAl"
    MemLock = 0x6b636f4c,       // "Lock"
    MemUnlock = 0x6f6c6e55,     // "Unlo"
    MemFree = 0x65657246,       // "Free"
    MemSize = 0x657a6953        // "Size"
} MEMORY_ACTION;

typedef enum {
    HEAP_COMPACT_NEVER = 0,
    HEAP_COMPACT_ON_ALLOC_FAIL,
    HEAP_COMPACT_ON_FREE
} HEAP_COMPACT_TYPE;

typedef enum {
    HEAP_VALIDATE_NEVER = 0,
    HEAP_VALIDATE_ON_ALLOC,
    HEAP_VALIDATE_ON_FREE
} HEAP_VALIDATE_TYPE;

//
// DEBUG_MEMORY_HEADER - keeps debug memory on list
//

typedef struct {
    LIST_ENTRY List;
    DWORD ThreadId;
    LPSTR CreatedFile;
    DWORD CreatedLine;
    LPSTR AccessedFile;
    DWORD AccessedLine;
    SIZE_T RequestedLength;
    SIZE_T BlockLength;
    SIZE_T ActualLength;
    DWORD Signature;
    DWORD Flags;
    DWORD TimeDeferred;
    LONG ClashTest;
    MEMORY_ACTION LastAccessOperation;
#if DUMP_STACK
#if ONE_STACK
    LPVOID Stack[8];    // should be variable
#else
    LPVOID CreateStack[4];
    LPVOID LastAccessStack[4];
#endif // ONE_STACK
#endif // DUMP_STACK
    DWORD Guard[2];

    //
    // sizeof(MEMORY_SIGNATURE) currently 24 DWORDs in Win32
    //

} DEBUG_MEMORY_HEADER, *LPDEBUG_MEMORY_HEADER;

//
// DEBUG_MEMORY_FOOTER - used to check for overwrites
//

typedef struct {
    DWORD Guard[4];
    DWORD Signature;
    SIZE_T BlockLength;  // should be the same as the header
    DWORD Guard2[2];

    //
    // sizeof(DEBUG_MEMORY_FOOTER) currently 8 DWORDs in Win32
    //

} DEBUG_MEMORY_FOOTER, *LPDEBUG_MEMORY_FOOTER;

//
// private data
//

PRIVATE BOOL MemoryPackageInitialized = FALSE;

//
// InternetDebugMemFlags - bitfield of flags controlling debug memory usage.
// The default is no debug alloc (don't create header + footers) and to use
// LocalAlloc() etc.
//

//
// BUGBUG - I'm making an assumption that the compiler thinks the bits have the
//          same values as I think they have. If not, it could mess up the
//          registry/environment flags
//

PRIVATE struct {                                            // default value
    DWORD bNoDebugAlloc             : 1;    //  0x00000001          TRUE
    DWORD bUseLocalAlloc            : 1;    //  0x00000002          TRUE
    DWORD bUseSymbols               : 1;    //  0x00000004          FALSE
    DWORD bAssertOnMemoryErrors     : 1;    //  0x00000008          FALSE
    DWORD bFillMemoryOnAlloc        : 1;    //  0x00000010          FALSE
    DWORD bFillMemoryOnFree         : 1;    //  0x00000020          FALSE
    DWORD bReportMemoryUsage        : 1;    //  0x00000040          FALSE
    DWORD bReportUnfreedBlocks      : 1;    //  0x00000080          FALSE
    DWORD bReportMemoryFooters      : 1;    //  0x00000100          FALSE
    DWORD bReportUserData           : 1;    //  0x00000200          FALSE
    DWORD bStopDumpIfBadBlock       : 1;    //  0x00000400          FALSE
    DWORD bLimitUnfreedBlocks       : 1;    //  0x00000800          FALSE
    DWORD bLimitUserData            : 1;    //  0x00001000          FALSE
    DWORD bDumpAsDwords             : 1;    //  0x00002000          FALSE
    DWORD bHeapNoSerialize          : 1;    //  0x00004000          FALSE
    DWORD bHeapGenerateExceptions   : 1;    //  0x00008000          FALSE
    DWORD bHeapIsGrowable           : 1;    //  0x00010000          FALSE
    DWORD bDeferFree                : 1;    //  0x00020000          FALSE
    DWORD bDumpToFile               : 1;    //  0x00040000          FALSE
} InternetDebugMemFlags = {
    TRUE,   // no debug alloc
    TRUE,   // use LocalAlloc()
    FALSE,  // don't load debug symbols
    FALSE,  // don't assert on memory errors
    FALSE,  // don't fill memory on alloc
    FALSE,  // don't fill memory on free
    FALSE,  // don't report memory usage (stats)
    FALSE,  // don't report unfreed blocks
    FALSE,  // don't report memory footers (irrelevant)
    FALSE,  // don't report user data (irrelevant)
    FALSE,  // don't stop dump if bad block (irrelevant)
    FALSE,  // don't limit dump of unfreed blocks (irrelevant)
    FALSE,  // don't limit dump of user data (irrelevant)
    FALSE,  // don't dump user data as DWORDs (irrelevant)
    FALSE,  // serialize access to heap (irrelevant)
    FALSE,  // don't generate heap exceptions (irrelevant)
    TRUE,   // heap is growable (irrelevant)
    FALSE,  // don't defer frees
    FALSE   // don't dump to wininet log file
};

//
// defines to make using InternetDebugMemFlags easier
//

#define bNoDebugAlloc           InternetDebugMemFlags.bNoDebugAlloc
#define bUseLocalAlloc          InternetDebugMemFlags.bUseLocalAlloc
#define bUseSymbols             InternetDebugMemFlags.bUseSymbols
#define bAssertOnMemoryErrors   InternetDebugMemFlags.bAssertOnMemoryErrors
#define bFillMemoryOnAlloc      InternetDebugMemFlags.bFillMemoryOnAlloc
#define bFillMemoryOnFree       InternetDebugMemFlags.bFillMemoryOnFree
#define bReportMemoryUsage      InternetDebugMemFlags.bReportMemoryUsage
#define bReportUnfreedBlocks    InternetDebugMemFlags.bReportUnfreedBlocks
#define bReportMemoryFooters    InternetDebugMemFlags.bReportMemoryFooters
#define bReportUserData         InternetDebugMemFlags.bReportUserData
#define bStopDumpIfBadBlock     InternetDebugMemFlags.bStopDumpIfBadBlock
#define bLimitUnfreedBlocks     InternetDebugMemFlags.bLimitUnfreedBlocks
#define bLimitUserData          InternetDebugMemFlags.bLimitUserData
#define bDumpAsDwords           InternetDebugMemFlags.bDumpAsDwords
#define bHeapNoSerialize        InternetDebugMemFlags.bHeapNoSerialize
#define bHeapGenerateExceptions InternetDebugMemFlags.bHeapGenerateExceptions
#define bHeapIsGrowable         InternetDebugMemFlags.bHeapIsGrowable
#define bDeferFree              InternetDebugMemFlags.bDeferFree
#define bDumpToFile             InternetDebugMemFlags.bDumpToFile

PRIVATE DWORD MaxBlocksDumped = DEFAULT_MAX_BLOCKS_DUMPED;
PRIVATE DWORD MaxUserDataDumped = DEFAULT_MAX_DATA_DUMPED;
PRIVATE DWORD StackBacktraceDepth = DEFAULT_BACKTRACE_DEPTH;

//
// heap variables
//

PRIVATE HANDLE hDebugHeap = NULL;
PRIVATE DWORD InitialHeapSize = DEFAULT_INITIAL_HEAP_SIZE;
PRIVATE DWORD MaximumHeapSize = DEFAULT_MAXIMUM_HEAP_SIZE;
PRIVATE HEAP_COMPACT_TYPE HeapCompactControl = HEAP_COMPACT_NEVER;
PRIVATE HEAP_VALIDATE_TYPE HeapValidateControl = HEAP_VALIDATE_NEVER;

//
// debug mem signatures etc.
//

PRIVATE DWORD AllocAlignment = DEFAULT_ALLOC_ALIGNMENT;
PRIVATE DWORD HeaderGuardSize = DEFAULT_HEADER_GUARD_SIZE;
PRIVATE DWORD FooterGuardSize = DEFAULT_FOOTER_GUARD_SIZE;
PRIVATE DWORD AllocMemoryFiller = DWORD_ALLOC_FILL;
PRIVATE DWORD FreeMemoryFiller = DWORD_FREE_FILL;

//
// usage variables - access using some sort of lock (critsec/interlocked)
//

PRIVATE CRITICAL_SECTION MemoryVarsCritSec;
PRIVATE SIZE_T TotalActualMemoryAllocated = 0;  // cumulative
PRIVATE SIZE_T TotalBlockMemoryAllocated = 0;    //     "
PRIVATE SIZE_T TotalRealMemoryAllocated = 0;    //     "
PRIVATE SIZE_T TotalActualMemoryFreed = 0;      //     "
PRIVATE SIZE_T TotalBlockMemoryFreed = 0;        //     "
PRIVATE SIZE_T TotalRealMemoryFreed = 0;         //     "
PRIVATE SIZE_T ActualMemoryAllocated = 0;        // difference
PRIVATE SIZE_T BlockLengthAllocated = 0;         //     "
PRIVATE SIZE_T RealLengthAllocated = 0;          //     "
PRIVATE DWORD MemoryAllocations = 0;            // cumulative
PRIVATE DWORD GoodMemoryAllocations = 0;        //     "
PRIVATE DWORD MemoryReAllocations = 0;          //     "
PRIVATE DWORD GoodMemoryReAllocations = 0;      //     "
PRIVATE DWORD MemoryFrees = 0;                  //     "
PRIVATE DWORD GoodMemoryFrees = 0;              //     "
PRIVATE SIZE_T LargestBlockRequested = 0;
PRIVATE SIZE_T LargestBlockAllocated = 0;
PRIVATE LPSTR LargestBlockRequestedFile = NULL;
PRIVATE DWORD LargestBlockRequestedLine = 0;
PRIVATE SIZE_T SmallestBlockRequested = (SIZE_T)-1;
PRIVATE SIZE_T SmallestBlockAllocated = (SIZE_T)-1;
PRIVATE LPSTR SmallestBlockRequestedFile = NULL;
PRIVATE DWORD SmallestBlockRequestedLine = 0;
PRIVATE DWORD DeferFreeTime = 0;

//
// lists
//

PRIVATE SERIALIZED_LIST AllocatedBlockList;
PRIVATE SERIALIZED_LIST DeferredFreeList;

//
// macros
//

#define MEMORY_ASSERT(x) \
    if (bAssertOnMemoryErrors) { \
        INET_ASSERT(x); \
    } else { \
        /* NOTHING */ \
    }

//
// private prototypes
//

PRIVATE
VOID
DebugFillMem(
    IN LPVOID Pointer,
    IN SIZE_T Size,
    IN DWORD dwFiller
    );

PRIVATE
HLOCAL
InternetAlloc(
    IN UINT Flags,
    IN SIZE_T Size
    );

PRIVATE
HLOCAL
InternetFree(
    IN HLOCAL hLocal
    );

PRIVATE
HLOCAL
InternetReAlloc(
    IN HLOCAL hLocal,
    IN SIZE_T Size,
    IN UINT Flags
    );

PRIVATE
SIZE_T
InternetSize(
    IN HLOCAL hLocal
    );

PRIVATE
HLOCAL
InternetHeapAlloc(
    IN UINT Flags,
    IN SIZE_T Size
    );

PRIVATE
HLOCAL
InternetHeapReAlloc(
    IN HLOCAL hLocal,
    IN SIZE_T Size,
    IN UINT Flags
    );

PRIVATE
HLOCAL
InternetHeapFree(
    IN HLOCAL hLocal
    );

PRIVATE
SIZE_T
InternetHeapSize(
    IN HLOCAL hLocal
    );

PRIVATE
BOOL
InternetDebugCheckMemBlock(
    IN LPDEBUG_MEMORY_HEADER lpHeader
    );

PRIVATE
VOID
InternetDebugMemTest(
    VOID
    );

PRIVATE
VOID
ReportMemoryUsage(
    VOID
    );

PRIVATE
VOID
ReportMemoryBlocks(
    VOID
    );

PRIVATE
VOID
DumpDeferredFreeList(
    VOID
    );

PRIVATE
VOID
DumpMemoryList(
    IN LPSERIALIZED_LIST lpList
    );

PRIVATE
VOID
FindAndDumpDeferredBlock(
    IN HLOCAL hLocal
    );

PRIVATE
BOOL
DumpBlock(
    IN LPDEBUG_MEMORY_HEADER lpHeader
    );

PRIVATE
BOOL
DumpDebugMemoryHeader(
    LPDEBUG_MEMORY_HEADER lpHeader
    );

PRIVATE
BOOL
DumpDebugMemoryFooter(
    LPDEBUG_MEMORY_FOOTER lpFooter
    );

PRIVATE
VOID
DumpUserData(
    LPDEBUG_MEMORY_HEADER lpHeader
    );

PRIVATE
LPSTR
MapLastAccessOperation(
    MEMORY_ACTION Action
    );

PRIVATE
LPSTR
MapMemoryFlags(
    DWORD Flags,
    LPSTR Buffer
    );

PRIVATE
LPSTR
DbgMemGetDebugSymbol(
    DWORD Address,
    LPDWORD Offset
    );

//
// functions
//


VOID
InternetDebugMemInitialize(
    VOID
    )

/*++

Routine Description:

    Initializes debug memory allocator

Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOL init;

    init = (BOOL)InterlockedExchange((LPLONG)&MemoryPackageInitialized, TRUE);
    if (init) {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("Memory package already initialized\n"
                    ));

        DEBUG_BREAK(MEMALLOC);

        return;
    }

    InitializeSerializedList(&AllocatedBlockList);
    InitializeSerializedList(&DeferredFreeList);
    InitializeCriticalSection(&MemoryVarsCritSec);

    //
    // sleaze: disable any debug output until we finish this. Debug log
    // routines want to allocate memory(!). InternetReadRegistryDword()
    // (called from InternetGetDebugVariable()) wants to perform DEBUG_ENTER
    // etc.
    //

    DWORD debugControlFlags = InternetDebugControlFlags;

    InternetDebugControlFlags = DBG_NO_DEBUG;

    //
    // if "WininetMem" is set then we set up to use debug memory - we use our
    // own heap, full debugging & reporting etc. (basically max memory debugging
    // as defined by me)
    //

    DWORD useDefaultDebugMemoryFlags = FALSE;

    InternetGetDebugVariable("WininetMem",
                             &useDefaultDebugMemoryFlags
                             );
    if (useDefaultDebugMemoryFlags) {
        bNoDebugAlloc = FALSE;          // use full debug allocator (header + footers, etc.)
        bUseLocalAlloc = FALSE;         // use our own heap
        bUseSymbols = FALSE;            // don't load debug symbols
        bAssertOnMemoryErrors = TRUE;   // assert to debugger/log if memory errors
        bFillMemoryOnAlloc = TRUE;      // fill user data w/ signature if not zeroinit
        bFillMemoryOnFree = TRUE;       // fill freed memory (useful on Win95/non-debug on NT)
        bReportMemoryUsage = TRUE;      // dump memory usage stats
        bReportUnfreedBlocks = TRUE;    // dump unfreed blocks (headers)
        bReportMemoryFooters = TRUE;    // dump unfreed block footers
        bReportUserData = TRUE;         // dump unfreed block user data
        bStopDumpIfBadBlock = TRUE;     // stop dumping if error occurs
        bLimitUnfreedBlocks = TRUE;     // limit block dump in case of loop in list
        bLimitUserData = TRUE;          // limit user data dump in case of bad length
        bDumpAsDwords = TRUE;           // dump data in dc format vs. db
        bHeapNoSerialize = FALSE;       // heap functions are serialized
        bHeapGenerateExceptions = FALSE;// heap functions return errors
        bHeapIsGrowable = FALSE;        // limit heap to maximum size (1 Meg)
        if (useDefaultDebugMemoryFlags == 2) {
            bDumpToFile = TRUE;
        }
    } else {

        //
        // no use-debug-mem override, see if there are any specific flags set
        //

        InternetGetDebugVariable("WininetDebugMemFlags",
                                 (LPDWORD)&InternetDebugMemFlags
                                 );
    }

    //
    // we used to load IMAGEHLP.DLL here and not use its functions until we were
    // dumping still in-use memory during DLL shutdown. Problem is that the
    // system has probably already freed IMAGEHLP.DLL by the time we come to use
    // it, resulting in GPF, so now we only load it at the time we're about to
    // use it
    //

    //if (bUseSymbols) {
    //    InitSymLib();
    //}

    if (!bUseLocalAlloc) {

        //
        // not using LocalAlloc(), using HeapAlloc(). Create heap
        //

        InitialHeapSize = DEFAULT_INITIAL_HEAP_SIZE;
        InternetGetDebugVariable("WininetDebugHeapInitialSize",
                                 &InitialHeapSize
                                 );

        MaximumHeapSize = DEFAULT_MAXIMUM_HEAP_SIZE;
        InternetGetDebugVariable("WininetDebugHeapMaximumSize",
                                 &MaximumHeapSize
                                 );

        if (bHeapIsGrowable) {
            MaximumHeapSize = 0;
        }

        hDebugHeap = HeapCreate((bHeapGenerateExceptions
                                    ? HEAP_GENERATE_EXCEPTIONS
                                    : 0)
                                | (bHeapNoSerialize
                                    ? HEAP_NO_SERIALIZE
                                    : 0),
                                InitialHeapSize,
                                MaximumHeapSize
                                );
        if (hDebugHeap == NULL) {

            DEBUG_PUT(("HeapCreate() failed - %d\n",
                        GetLastError()
                        ));

            bUseLocalAlloc = TRUE;
        } else {
            HeapCompactControl = HEAP_COMPACT_NEVER;
            InternetGetDebugVariable("WininetDebugHeapCompactControl",
                                     (LPDWORD)&HeapCompactControl
                                     );

            HeapValidateControl = HEAP_VALIDATE_NEVER;
            InternetGetDebugVariable("WininetDebugHeapValidateControl",
                                     (LPDWORD)&HeapValidateControl
                                     );

            DEBUG_PUT(("Wininet heap = %#x\n",
                        hDebugHeap
                        ));

        }
    }

    //
    // restore default debug flags
    //

    InternetDebugControlFlags = debugControlFlags;

    //InternetDebugMemTest();
}


VOID
InternetDebugMemTerminate(
    IN BOOL bReport
    )

/*++

Routine Description:

    Frees resources allocated in InternetDebugMemInitialize, after checking that
    all memory is freed

Arguments:

    bReport - TRUE if in-use blocks reported at termination

Return Value:

    None.

--*/

{
    BOOL bOpened = bReport ? InternetDebugMemReport(TRUE, FALSE) : FALSE;

    InternetDebugCheckMemFreed(FALSE);
    DeleteCriticalSection(&MemoryVarsCritSec);
    TerminateSerializedList(&AllocatedBlockList);
    TerminateSerializedList(&DeferredFreeList);

    if (hDebugHeap != NULL) {

        //
        // any future allocations(!) must use process heap
        //

        bUseLocalAlloc = TRUE;

        if (!HeapDestroy(hDebugHeap)) {

            DWORD error = GetLastError();

            DEBUG_PRINT(MEMALLOC,
                        ERROR,
                        ("HeapDestroy(%#x) returns %s (%d)\n",
                        hDebugHeap,
                        InternetMapError(error),
                        error
                        ));

            MEMORY_ASSERT(FALSE);

        }
    }
    if (bOpened) {
        InternetCloseDebugFile();
    }
    MemoryPackageInitialized = FALSE;
}


HLOCAL
InternetDebugAllocMem(
    IN UINT Flags,
    IN UINT Size,
    IN LPSTR File,
    IN DWORD Line
    )

/*++

Routine Description:

    Debug memory allocator. If this succeeds, then the real block is put on our
    list and has its head & tail (& possibly contents) initialized. The caller
    gets an pointer which is an offset to the user area in the block

Arguments:

    Flags   - controlling flags (normally passed to LocalAlloc)

    Size    - of block to allocate

    File    - from where alloc called

    Line    - in File

Return Value:

    HLOCAL
        Success - pointer to caller's start of allocated block

        Failure - NULL

--*/

{
    if (!MemoryPackageInitialized) {
        return NULL;
    }

//dprintf("InternetDebugAllocMem(%#x, %d) = ", Flags, Size);
    InterlockedIncrement((LPLONG)&MemoryAllocations);

    //
    // keep these tests separate so we don't have to look up the flags #defines
    //

    INET_ASSERT(!(Flags & LMEM_MOVEABLE));
    INET_ASSERT(!(Flags & LMEM_DISCARDABLE));

    if (Size == 0) {

        DEBUG_PRINT(MEMALLOC,
                    WARNING,
                    ("InternetDebugAllocMem(%#x, %d)\n",
                    Flags,
                    Size
                    ));

        MEMORY_ASSERT(FALSE);

    }

    SIZE_T blockLength;

    if (bNoDebugAlloc) {
        blockLength = Size;
    } else {
        if (Size > LargestBlockRequested) {
            LargestBlockRequested = Size;
            LargestBlockRequestedFile = File;
            LargestBlockRequestedLine = Line;
        } else if (Size < SmallestBlockRequested) {
            SmallestBlockRequested = Size;
            SmallestBlockRequestedFile = File;
            SmallestBlockRequestedLine = Line;
        }
        blockLength = ROUND_UP_DWORD(Size)
                    + sizeof(DEBUG_MEMORY_HEADER)
                    + sizeof(DEBUG_MEMORY_FOOTER);
    }

    //
    // possible problem: if Size + signatures would overflow UINT. Only really
    // problematic on 16-bit platforms
    //

    if (blockLength < Size) {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("can't allocate %lu bytes: would overflow\n",
                    (DWORD)Size
                    ));

        DEBUG_BREAK(MEMALLOC);

//dprintf("NULL\n");
        return NULL;
    }

    //
    // BUGBUG - allocating 0 bytes?
    //

    HLOCAL hLocal = InternetAlloc(Flags, blockLength);

    if (hLocal != NULL) {
        InterlockedIncrement((LPLONG)&GoodMemoryAllocations);
    } else {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("failed to allocate %u bytes memory\n",
                    blockLength
                    ));

        DEBUG_BREAK(MEMALLOC);

//dprintf("NULL\n");
        return NULL;
    }

    SIZE_T actualLength = InternetSize(hLocal);
    SIZE_T requestedLength;

    if (bNoDebugAlloc) {
        blockLength = actualLength;
        requestedLength = actualLength;
    } else {
        requestedLength = Size;
        if (actualLength > LargestBlockAllocated) {
            LargestBlockAllocated = actualLength;
        } else if (actualLength < SmallestBlockAllocated) {
            SmallestBlockAllocated = actualLength;
        }
    }

    EnterCriticalSection(&MemoryVarsCritSec);
    TotalActualMemoryAllocated += actualLength;
    TotalBlockMemoryAllocated += blockLength;
    TotalRealMemoryAllocated += requestedLength;
    ActualMemoryAllocated += actualLength;
    BlockLengthAllocated += blockLength;
    RealLengthAllocated += requestedLength;
    LeaveCriticalSection(&MemoryVarsCritSec);

    if (bNoDebugAlloc || (hLocal == NULL)) {
        if ((hLocal != NULL) && !(Flags & LMEM_ZEROINIT) && bFillMemoryOnAlloc) {
            DebugFillMem(hLocal, Size, AllocMemoryFiller);
        }
//dprintf("%#x\n", hLocal);
        return hLocal;
    }

    LPDEBUG_MEMORY_HEADER lpHeader = (LPDEBUG_MEMORY_HEADER)hLocal;

    //InitializeListHead(&lpHeader->List);
    lpHeader->ThreadId = GetCurrentThreadId();
    lpHeader->CreatedFile = File;
    lpHeader->CreatedLine = Line;
    lpHeader->AccessedFile = File;
    lpHeader->AccessedLine = Line;
    lpHeader->RequestedLength = Size;
    lpHeader->BlockLength = blockLength;
    lpHeader->ActualLength = actualLength;
    lpHeader->Signature = HEADER_SIGNATURE;
    lpHeader->Flags = Flags;
    lpHeader->TimeDeferred = 0;
    lpHeader->ClashTest = -1;
    lpHeader->LastAccessOperation = MemAllocate;

#if DUMP_STACK
#if ONE_STACK

    memset(lpHeader->Stack, 0, sizeof(lpHeader->Stack));
    GET_CALL_STACK(lpHeader->Stack);

#else

    GET_CALLERS_ADDRESS(&lpHeader->CreateStack[0],
                        &lpHeader->CreateStack[1]
                        );

    memset(lpHeader->CreateStack, 0, sizeof(lpHeader->CreateStack));

    GET_CALL_STACK(lpHeader->CreateStack);

    memcpy(lpHeader->LastAccessStack,
           lpHeader->CreateStack,
           sizeof(lpHeader->LastAccessStack)
           );

#endif // ONE_STACK
#endif // DUMP_STACK

    UINT i;

    for (i = 0; i < ARRAY_ELEMENTS(lpHeader->Guard); ++i) {
        lpHeader->Guard[i] = GUARD_DWORD_FILL;
    }

    //
    // BUGBUG - should be using AllocAlignment - could be > sizeof(DWORD)
    //

    if (!(Flags & LMEM_ZEROINIT) && bFillMemoryOnAlloc) {
        DebugFillMem(lpHeader + 1, Size, AllocMemoryFiller);
    }

    UINT bFillLength2 = (Size % sizeof(DWORD)) ? (sizeof(DWORD) - (Size % sizeof(DWORD))) : 0;
    LPBYTE lpbUserPointer = (LPBYTE)(lpHeader + 1) + Size;

    for (i = 0; i < bFillLength2; ++i) {
        *lpbUserPointer++ = BYTE_ALLOC_FILL_EXTRA;
    }

    LPDEBUG_MEMORY_FOOTER lpFooter = (LPDEBUG_MEMORY_FOOTER)lpbUserPointer;

    for (i = 0; i < ARRAY_ELEMENTS(lpFooter->Guard); ++i) {
        lpFooter->Guard[i] = GUARD_DWORD_FILL;
    }

    lpFooter->BlockLength = blockLength;
    lpFooter->Signature = FOOTER_SIGNATURE;

    for (i = 0; i < ARRAY_ELEMENTS(lpFooter->Guard2); ++i) {
        lpFooter->Guard2[i] = GUARD_DWORD_FILL;
    }

    if (!CheckEntryOnSerializedList(&AllocatedBlockList, &lpHeader->List, FALSE)) {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("InternetDebugAllocMem(%d): %#x already on list?\n",
                    Size,
                    lpHeader
                    ));

        MEMORY_ASSERT(FALSE);

    }

    //
    // put at the tail of list so we can view unfreed blocks in chronological
    // order
    //

    InsertAtTailOfSerializedList(&AllocatedBlockList, &lpHeader->List);

//dprintf("%#x\n", lpHeader + 1);
    return (HLOCAL)(lpHeader + 1);
}


HLOCAL
InternetDebugFreeMem(
    IN HLOCAL hLocal,
    IN LPSTR File,
    IN DWORD Line
    )

/*++

Routine Description:

    Frees a block of memory allocated by InternetDebugAllocMem(). Checks that
    the block is on our allocated block list, and that the header and footer
    areas are still intact

Arguments:

    hLocal  - handle (pointer) of block to free

    File    - from where alloc called

    Line    - in File

Return Value:

    HLOCAL
        Success - NULL

        Failure - hLocal

--*/

{
    if (!MemoryPackageInitialized) {
        return NULL;
    }

//dprintf("InternetDebugFreeMem(%#x)\n", hLocal);
    InterlockedIncrement((LPLONG)&MemoryFrees);

    if (hLocal == NULL) {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("InternetDebugFreeMem(NULL)\n"
                    ));

        MEMORY_ASSERT(FALSE);

        return InternetFree(hLocal);
    }

    HLOCAL hLocalOriginal = hLocal;
    SIZE_T actualLength;
    SIZE_T blockLength;
    SIZE_T realLength;

    if (bNoDebugAlloc) {
        actualLength = InternetSize(hLocal);
        blockLength = actualLength;
        realLength = actualLength;
    } else {
        hLocal = (HLOCAL)((LPDEBUG_MEMORY_HEADER)hLocal - 1);
        actualLength = InternetSize(hLocal);

        LPDEBUG_MEMORY_HEADER lpHeader = (LPDEBUG_MEMORY_HEADER)hLocal;

        if (CheckEntryOnSerializedList(&AllocatedBlockList, &lpHeader->List, TRUE)) {
            RemoveFromSerializedList(&AllocatedBlockList, &lpHeader->List);

            if (!((lpHeader->ActualLength == actualLength)
            && (lpHeader->BlockLength <= actualLength)
            && !(lpHeader->BlockLength & (sizeof(DWORD) - 1))
            && (lpHeader->RequestedLength < lpHeader->BlockLength))) {

                DEBUG_PRINT(MEMALLOC,
                            ERROR,
                            ("InternetDebugFreeMem(%#x): block lengths mismatch\n",
                            hLocalOriginal
                            ));

                MEMORY_ASSERT(FALSE);
            }
            if (InternetDebugCheckMemBlock(lpHeader)) {
                blockLength = lpHeader->BlockLength;
                realLength = lpHeader->RequestedLength;
            } else {
                blockLength = 0;
                realLength = 0;
            }
            if (bDeferFree) {

#if DUMP_STACK
#if ONE_STACK

                memset(lpHeader->Stack, 0, sizeof(lpHeader->Stack));
                GET_CALL_STACK(lpHeader->Stack);

#else

                GET_CALLERS_ADDRESS(&lpHeader->CreateStack[0],
                                    &lpHeader->CreateStack[1]
                                    );

                memset(lpHeader->CreateStack, 0, sizeof(lpHeader->CreateStack));

                GET_CALL_STACK(lpHeader->CreateStack);

                memcpy(lpHeader->LastAccessStack,
                       lpHeader->CreateStack,
                       sizeof(lpHeader->LastAccessStack)
                       );

#endif // ONE_STACK
#endif // DUMP_STACK

                InsertAtTailOfSerializedList(&DeferredFreeList, &lpHeader->List);
                hLocal = NULL;
            }
        } else {

            DEBUG_PRINT(MEMALLOC,
                        ERROR,
                        ("InternetDebugFreeMem(%#x): can't find %#x\n",
                        hLocalOriginal,
                        &lpHeader->List
                        ));

            MEMORY_ASSERT(FALSE);

            FindAndDumpDeferredBlock(hLocal);
        }
    }

    if (hLocal && bFillMemoryOnFree) {
        DebugFillMem(hLocal, actualLength, FreeMemoryFiller);
    }

    hLocal = InternetFree(hLocal);

    if (hLocal == NULL) {
        InterlockedIncrement((LPLONG)&GoodMemoryFrees);
        EnterCriticalSection(&MemoryVarsCritSec);
        TotalActualMemoryFreed += actualLength;
        TotalBlockMemoryFreed += blockLength;
        TotalRealMemoryFreed += realLength;
        ActualMemoryAllocated -= actualLength;
        BlockLengthAllocated -= blockLength;
        RealLengthAllocated -= realLength;
        LeaveCriticalSection(&MemoryVarsCritSec);
    } else {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("InternetDebugFreeMem(%#x) failed\n",
                    hLocalOriginal
                    ));

        MEMORY_ASSERT(FALSE);

        hLocal = hLocalOriginal;
    }

    return hLocal;
}


HLOCAL
InternetDebugReAllocMem(
    IN HLOCAL hLocal,
    IN UINT Size,
    IN UINT Flags,
    IN LPSTR File,
    IN DWORD Line
    )

/*++

Routine Description:

    Reallocates a debug memory block allocated by InternetDebugAllocMem()

Arguments:

    hLocal  - the handle (pointer) of the allocated block

    Size    - requested size of new block; can be larger or smaller than current
              size

    Flags   - controlling flags (normally passed to LocalReAlloc)

    File    - from where alloc called

    Line    - in File

Return Value:

    HLOCAL
        Success - pointer to new block. May be same or different than previous
                  pointer, depending on flags

        Failure - NULL

--*/

{
    if (!MemoryPackageInitialized) {
        return NULL;
    }

//dprintf("InternetDebugReAllocMem(%#x, %d, %#x)\n", hLocal, Size, Flags);
    InterlockedIncrement((LPLONG)&MemoryReAllocations);

    //
    // we can't handle the following flags
    //

    INET_ASSERT(!(Flags & LMEM_MODIFY));

    //
    // can't handle reallocating down to zero
    //

    if (Size == 0) {

        MEMORY_ASSERT(FALSE);

    }

    HLOCAL hLocalOriginal = hLocal;
    LPDEBUG_MEMORY_HEADER lpHeader;
    SIZE_T actualLength;
    SIZE_T blockLength;
    SIZE_T requestedLength;
    SIZE_T oldRequestedLength;

    if (bNoDebugAlloc) {
        actualLength = InternetSize(hLocal);
        blockLength = actualLength;
        requestedLength = actualLength;
    } else {
        if (Size > LargestBlockRequested) {
            LargestBlockRequested = Size;
            LargestBlockRequestedFile = File;
            LargestBlockRequestedLine = Line;
        } else if (Size < SmallestBlockRequested) {
            SmallestBlockRequested = Size;
            SmallestBlockRequestedFile = File;
            SmallestBlockRequestedLine = Line;
        }
        lpHeader = (LPDEBUG_MEMORY_HEADER)hLocal - 1;
        hLocal = (HLOCAL)lpHeader;
        if (!CheckEntryOnSerializedList(&AllocatedBlockList, &lpHeader->List, TRUE)) {

            DEBUG_PRINT(MEMALLOC,
                        ERROR,
                        ("InternetDebugReAllocMem(%#x): can't find %#x\n",
                        hLocalOriginal
                        ));

            MEMORY_ASSERT(FALSE);

            return hLocalOriginal;
        }
        RemoveFromSerializedList(&AllocatedBlockList, &lpHeader->List);
        InternetDebugCheckMemBlock(lpHeader);
        actualLength = InternetSize((HLOCAL)lpHeader);
        blockLength = lpHeader->BlockLength;
        requestedLength = lpHeader->RequestedLength;
        oldRequestedLength = requestedLength;
        if (!((lpHeader->ActualLength == actualLength)
        && (lpHeader->BlockLength <= actualLength)
        && !(lpHeader->BlockLength & (sizeof(DWORD) - 1))
        && (lpHeader->RequestedLength < lpHeader->BlockLength))) {

            DEBUG_PRINT(MEMALLOC,
                        ERROR,
                        ("InternetDebugReAllocMem(%#x): block lengths mismatch\n",
                        hLocalOriginal
                        ));

            MEMORY_ASSERT(FALSE);
        }
    }
    EnterCriticalSection(&MemoryVarsCritSec);
    ActualMemoryAllocated -= actualLength;
    BlockLengthAllocated -= blockLength;
    RealLengthAllocated -= requestedLength;
    LeaveCriticalSection(&MemoryVarsCritSec);
    requestedLength = Size;
    if (bNoDebugAlloc) {
        blockLength = Size;
    } else {
        blockLength = ROUND_UP_DWORD(Size)
                    + sizeof(DEBUG_MEMORY_HEADER)
                    + sizeof(DEBUG_MEMORY_FOOTER);
    }
    hLocal = InternetReAlloc(hLocal, blockLength, Flags);
    if (hLocal != NULL) {
        InterlockedIncrement((LPLONG)&GoodMemoryReAllocations);
        actualLength = InternetSize(hLocal);
        if (bNoDebugAlloc) {
            blockLength = actualLength;
        } else {
            lpHeader = (LPDEBUG_MEMORY_HEADER)hLocal;
            //InitializeListHead(&lpHeader->List);
            lpHeader->ThreadId = GetCurrentThreadId();
            lpHeader->AccessedFile = File;
            lpHeader->AccessedLine = Line;
            lpHeader->RequestedLength = requestedLength;
            lpHeader->BlockLength = blockLength;
            lpHeader->ActualLength = actualLength;
            lpHeader->Flags = Flags;
            lpHeader->TimeDeferred = 0;
            lpHeader->ClashTest = -1;
            lpHeader->LastAccessOperation = MemReallocate;

#if DUMP_STACK
#if ONE_STACK
#else

            //GET_CALLERS_ADDRESS(&lpHeader->LastAccessStack[0],
            //                    &lpHeader->LastAccessStack[1]
            //                    );

            memset(lpHeader->LastAccessStack, 0, sizeof(lpHeader->LastAccessStack));

            GET_CALL_STACK(lpHeader->LastAccessStack);

#endif // ONE_STACK
#endif // DUMP_STACK

            LPBYTE extraPointer;
            UINT dwFillLength;
            UINT i;

            if ((requestedLength > oldRequestedLength)
            && bFillMemoryOnAlloc && !(Flags & LMEM_ZEROINIT)) {

                extraPointer = (LPBYTE)(lpHeader + 1) + oldRequestedLength;

                SIZE_T difference = requestedLength - oldRequestedLength;
                DWORD dwFiller = AllocMemoryFiller;
                SIZE_T syncLength = oldRequestedLength & (sizeof(DWORD) - 1);

                if (syncLength) {
                    syncLength = sizeof(DWORD) - syncLength;
                    syncLength = min(syncLength, difference);
                    difference -= syncLength;
                    for (i = 0; i < syncLength; ++i) {
                        *extraPointer++ = ((LPBYTE)&dwFiller)[i];
                    }
                }

                //dwFillLength = difference / sizeof(DWORD);
                //difference %= sizeof(DWORD);
                //while (dwFillLength--) {
                //    *(LPDWORD)extraPointer = 0;
                //    extraPointer += sizeof(DWORD);
                //}
                //while (difference--) {
                //    *extraPointer++ = 0;
                //}

                if (difference) {
                    DebugFillMem(extraPointer, difference, dwFiller);
                    extraPointer += difference;
                }
            } else {
                extraPointer = (LPBYTE)(lpHeader + 1) + requestedLength;
            }

            SIZE_T bFillLength = (sizeof(DWORD) - (requestedLength % sizeof(DWORD))) & (sizeof(DWORD) - 1);

            while (bFillLength--) {
                *extraPointer++ = BYTE_ALLOC_FILL_EXTRA;
            }

            LPDEBUG_MEMORY_FOOTER lpFooter = (LPDEBUG_MEMORY_FOOTER)extraPointer;

            INET_ASSERT(lpFooter == (LPDEBUG_MEMORY_FOOTER)
                ((LPBYTE)(lpHeader + 1) + ROUND_UP_DWORD(requestedLength)));

            for (i = 0; i < ARRAY_ELEMENTS(lpFooter->Guard); ++i) {
                lpFooter->Guard[i] = GUARD_DWORD_FILL;
            }
            lpFooter->Signature = FOOTER_SIGNATURE;
            lpFooter->BlockLength = blockLength;
            for (i = 0; i < ARRAY_ELEMENTS(lpFooter->Guard2); ++i) {
                lpFooter->Guard2[i] = GUARD_DWORD_FILL;
            }
            InsertAtTailOfSerializedList(&AllocatedBlockList, &lpHeader->List);
            hLocal = (HLOCAL)(lpHeader + 1);
        }
        EnterCriticalSection(&MemoryVarsCritSec);
        ActualMemoryAllocated += actualLength;
        BlockLengthAllocated += blockLength;
        RealLengthAllocated += requestedLength;
        LeaveCriticalSection(&MemoryVarsCritSec);
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


SIZE_T
InternetDebugSizeMem(
    IN HLOCAL hLocal,
    IN LPSTR File,
    IN DWORD Line
    )

/*++

Routine Description:

    Returns actual allocated block size

Arguments:

    hLocal  - pointer to allocated block

    File    - from where alloc called

    Line    - in File

Return Value:

    SIZE_T
        size of allocated block

--*/

{
    if (!MemoryPackageInitialized) {
        return 0;
    }

//dprintf("InternetDebugSizeMem(%#x)\n", hLocal);
    SIZE_T size = InternetSize(hLocal);

    if (!bNoDebugAlloc) {

        LPDEBUG_MEMORY_HEADER lpHeader = (LPDEBUG_MEMORY_HEADER)hLocal - 1;

        INET_ASSERT(lpHeader->Signature == HEADER_SIGNATURE);

        SIZE_T sizeInHeader = lpHeader->BlockLength
                          - (sizeof(DEBUG_MEMORY_HEADER) + sizeof(DEBUG_MEMORY_FOOTER));

        INET_ASSERT((sizeInHeader <= size)
                    && (size >= sizeof(DEBUG_MEMORY_HEADER) + sizeof(DEBUG_MEMORY_FOOTER))
                    && (lpHeader->ActualLength == size)
                    );

        size = sizeInHeader;
    }

    return size;
}


BOOL
InternetDebugCheckMemFreed(
    IN BOOL bReport
    )

/*++

Routine Description:

    Called when we're about to quit. Checks that all allocated memory has been
    cleaned up

Arguments:

    bReport - TRUE if in-use blocks reported

Return Value:

    BOOL
        TRUE    - all allocated memory freed

        FALSE   - we failed to clean up

--*/

{
    if (bReport) {
        if (bReportMemoryUsage) {
            ReportMemoryUsage();
        }
        if (bReportUnfreedBlocks) {
            ReportMemoryBlocks();
        }
    }
    if (ElementsOnSerializedList(&AllocatedBlockList) != 0) {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("InternetDebugCheckMemFreed(): %d memory blocks still allocated\n",
                    MemoryAllocations - MemoryFrees
                    ));

        MEMORY_ASSERT(FALSE);

        return FALSE;
    }
    return TRUE;
}


BOOL
InternetDebugMemReport(
    IN BOOL bTerminateSymbols,
    IN BOOL bCloseFile
    )

/*++

Routine Description:

    Dumps in-use blocks to debugger and/or file

Arguments:

    bTerminateSymbols   - TRUE if we are to terminate symbols here

    bCloseFile          - TRUE if we are to close debug log file here

Return Value:

    BOOL    - TRUE if we opened debug log file

--*/

{
    BOOL bOpened = FALSE;

    if (bDumpToFile) {
        bOpened = InternetOpenDebugFile();
        if (bOpened) {
            InternetDebugResetControlFlags(DBG_NO_DEBUG);
            InternetDebugSetControlFlags(DBG_TO_FILE | DBG_NO_ASSERT_BREAK);
        }
    }
    ReportMemoryUsage();
    ReportMemoryBlocks();
    if (bUseSymbols && bTerminateSymbols) {
        TermSymLib();
    }
    if (bOpened && bCloseFile) {
        InternetCloseDebugFile();
    }
    return bOpened;
}

//
// private functions
//


PRIVATE
VOID
DebugFillMem(
    IN LPVOID Pointer,
    IN SIZE_T Size,
    IN DWORD dwFiller
    )

/*++

Routine Description:

    Fills memory with repeating debug pattern. Performs DWORD fill then finishes
    off any remaining bytes with character fill (rep movsd/rep movsb (ideally)
    (x86!))

Arguments:

    Pointer     - memory to fill

    Size        - of Pointer in bytes

    dwFiller    - DWORD value to use

Return Value:

    None.

--*/

{
    INET_ASSERT(((DWORD_PTR)Pointer & (sizeof(DWORD) - 1)) == 0);

    SIZE_T dwFillLength = Size / sizeof(DWORD);
    SIZE_T bFillLength = Size % sizeof(DWORD);

    //
    // assume > 0 DWORDs to fill
    //

    LPDWORD lpdwPointer = (LPDWORD)Pointer;
    SIZE_T i;

    for (i = 0; i < dwFillLength; ++i) {
        *lpdwPointer++ = dwFiller;
    }

    if (bFillLength) {

        LPBYTE lpbPointer = (LPBYTE)lpdwPointer;

        for (i = 0; i < bFillLength; ++i) {
            *lpbPointer++ = ((LPBYTE)&dwFiller)[i];
        }
    }
}


PRIVATE
HLOCAL
InternetAlloc(
    IN UINT Flags,
    IN SIZE_T Size
    )

/*++

Routine Description:

    Allocator - uses process (local) heap or component (debug) heap based on
    global flag setting

Arguments:

    Flags   - LocalAlloc flags

    Size    - of block to allocate

Return Value:

    HLOCAL
        Success - pointer to allocated block

        Failure - NULL

--*/

{
    if (bUseLocalAlloc) {
        return LocalAlloc(Flags, Size);
    } else {
        return InternetHeapAlloc(Flags, Size);
    }
}


PRIVATE
HLOCAL
InternetFree(
    IN HLOCAL hLocal
    )

/*++

Routine Description:

    Deallocator - uses process (local) heap or component (debug) heap based on
    global flag setting

Arguments:

    hLocal  - pointer to block to deallocate

Return Value:

    HLOCAL
        Success - NULL

        Failure - pointer to still allocated block

--*/

{
    if (bUseLocalAlloc) {
        return LocalFree(hLocal);
    } else {
        return InternetHeapFree(hLocal);
    }
}


PRIVATE
HLOCAL
InternetReAlloc(
    IN HLOCAL hLocal,
    IN SIZE_T Size,
    IN UINT Flags
    )

/*++

Routine Description:

    Reallocator - uses process (local) heap or component (debug) heap based on
    global flag setting

Arguments:

    hLocal  - pointer to block to reallocate

    Flags   - LocalAlloc flags

    Size    - of block to allocate

Return Value:

    HLOCAL
        Success - pointer to allocated block

        Failure - NULL

--*/

{
    if (bUseLocalAlloc) {
        return LocalReAlloc(hLocal, Size, Flags);
    } else {
        return InternetHeapReAlloc(hLocal, Size, Flags);
    }
}


PRIVATE
SIZE_T
InternetSize(
    IN HLOCAL hLocal
    )

/*++

Routine Description:

    Block sizer - uses process (local) heap or component (debug) heap based on
    global flag setting

Arguments:

    hLocal  - pointer to block to size

Return Value:

    SIZE_T
        Success - size of block

        Failure - 0

--*/

{
    if (bUseLocalAlloc) {
        return LocalSize(hLocal);
    } else {
        return InternetHeapSize(hLocal);
    }
}


PRIVATE
HLOCAL
InternetHeapAlloc(
    IN UINT Flags,
    IN SIZE_T Size
    )

/*++

Routine Description:

    Allocate memory from debug heap

Arguments:

    Flags   - passed to LocalAlloc

    Size    - of block to allocate

Return Value:

    HLOCAL

--*/

{
    HLOCAL hLocal;

    if (hDebugHeap != NULL) {
        hLocal = (HLOCAL)HeapAlloc(hDebugHeap,
                                   (bHeapNoSerialize
                                        ? HEAP_NO_SERIALIZE
                                        : 0)
                                   | (bHeapGenerateExceptions
                                        ? HEAP_GENERATE_EXCEPTIONS
                                        : 0)
                                   | ((Flags & LMEM_ZEROINIT)
                                        ? HEAP_ZERO_MEMORY
                                        : 0),
                                   Size
                                   );
    } else {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("InternetHeapAlloc(): hDebugHeap is NULL\n"
                    ));

        MEMORY_ASSERT(FALSE);

        hLocal = LocalAlloc(Flags, Size);
    }
    if (hLocal == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }
    return hLocal;
}


PRIVATE
HLOCAL
InternetHeapReAlloc(
    IN HLOCAL hLocal,
    IN SIZE_T Size,
    IN UINT Flags
    )

/*++

Routine Description:

    Reallocate memory from debug heap

Arguments:

    hLocal  - pointer to block to reallocate

    Size    - new size

    Flags   - to LocalReAlloc

Return Value:

    HLOCAL
        Success - pointer to new block

        Failure - NULL

--*/

{
    if (hDebugHeap != NULL) {
        hLocal = (HLOCAL)HeapReAlloc(hDebugHeap,
                                     (bHeapNoSerialize
                                        ? HEAP_NO_SERIALIZE
                                        : 0)
                                     | (bHeapGenerateExceptions
                                        ? HEAP_GENERATE_EXCEPTIONS
                                        : 0)
                                     | ((Flags & LMEM_MOVEABLE)
                                        ? 0
                                        : HEAP_REALLOC_IN_PLACE_ONLY)
                                     | ((Flags & LMEM_ZEROINIT)
                                        ? HEAP_ZERO_MEMORY
                                        : 0),
                                     (LPVOID)hLocal,
                                     Size
                                     );
    } else {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("InternetHeapReAlloc(): hDebugHeap is NULL\n"
                    ));

        MEMORY_ASSERT(FALSE);

        //
        // block still allocated
        //

        hLocal = NULL;
    }
    return hLocal;
}


PRIVATE
HLOCAL
InternetHeapFree(
    IN HLOCAL hLocal
    )

/*++

Routine Description:

    Free memory to debug heap

Arguments:

    hLocal  - to free

Return Value:

    HLOCAL
        Success - NULL

        Failure - hLocal

--*/

{
    BOOL ok;

    if (hDebugHeap != NULL) {
        ok = HeapFree(hDebugHeap,
                      bHeapNoSerialize ? HEAP_NO_SERIALIZE : 0,
                      (LPVOID)hLocal
                      );
        if (!ok) {

            DWORD error = GetLastError();

            DEBUG_PRINT(MEMALLOC,
                        ERROR,
                        ("HeapFree() returns %s (%d)\n",
                        InternetMapError(error),
                        error
                        ));

            MEMORY_ASSERT(FALSE);

        }
    } else {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("InternetHeapFree(): hDebugHeap is NULL\n"
                    ));

        MEMORY_ASSERT(FALSE);

        ok = FALSE;
    }
    return ok ? NULL : hLocal;
}


PRIVATE
SIZE_T
InternetHeapSize(
    IN HLOCAL hLocal
    )

/*++

Routine Description:

    Determines size of block allocated from debug heap

Arguments:

    hLocal  - handle (pointer) of block for which to get size

Return Value:

    SIZE_T
        Success - size of block

        Failure - 0

--*/

{
    SIZE_T size;

    if (hDebugHeap != NULL) {
        size = HeapSize(hDebugHeap,
                        bHeapNoSerialize ? HEAP_NO_SERIALIZE : 0,
                        (LPCVOID)hLocal
                        );
    } else {

        DEBUG_PRINT(MEMALLOC,
                    ERROR,
                    ("InternetHeapSize(): hDebugHeap is NULL\n"
                    ));

        MEMORY_ASSERT(FALSE);

        size = (SIZE_T)-1;
    }
    if (size == (SIZE_T)-1) {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    } else {
        return size;
    }
}


PRIVATE
BOOL
InternetDebugCheckMemBlock(
    IN LPDEBUG_MEMORY_HEADER lpHeader
    )

/*++

Routine Description:

    Checks the consistency of a debug memory block

Arguments:

    lpHeader    - pointer to what we think is DEBUG_MEMORY_HEADER

Return Value:

    None.

--*/

{
    BOOL result;

    __try {
        LPDEBUG_MEMORY_FOOTER lpFooter = (LPDEBUG_MEMORY_FOOTER)
            ((LPBYTE)lpHeader
                + (lpHeader->BlockLength - sizeof(DEBUG_MEMORY_FOOTER)));

        BOOL headerGuardOverrun = FALSE;
        UINT i;

        for (i = 0; i < ARRAY_ELEMENTS(lpHeader->Guard); ++i) {
            if (lpHeader->Guard[i] != GUARD_DWORD_FILL) {
                headerGuardOverrun = TRUE;
                break;
            }
        }

        BOOL footerGuardOverrun = FALSE;

        for (i = 0; i < ARRAY_ELEMENTS(lpFooter->Guard); ++i) {
            if (lpFooter->Guard[i] != GUARD_DWORD_FILL) {
                footerGuardOverrun = TRUE;
                break;
            }
        }

        BOOL footerGuard2Overrun = FALSE;

        for (i = 0; i < ARRAY_ELEMENTS(lpFooter->Guard2); ++i) {
            if (lpFooter->Guard2[i] != GUARD_DWORD_FILL) {
                footerGuard2Overrun = TRUE;
                break;
            }
        }

        LPBYTE lpExtraMemory = (LPBYTE)(lpHeader + 1) + lpHeader->RequestedLength;
        BOOL extraMemoryOverrun = FALSE;
        SIZE_T byteLength = ROUND_UP_DWORD(lpHeader->RequestedLength) - lpHeader->RequestedLength;

        for (i = 0; i < byteLength; ++i) {
            if (lpExtraMemory[i] != BYTE_ALLOC_FILL_EXTRA) {
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
                        ("Bad block: %#x\n",
                        lpHeader
                        ));

            MEMORY_ASSERT(FALSE);

            result = FALSE;
        } else {
            result = TRUE;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        DEBUG_PRINT(MEMALLOC,
                    FATAL,
                    ("Invalid block %#x - exception occurred\n",
                    lpHeader
                    ));

        MEMORY_ASSERT(FALSE);

        result = FALSE;
    }
    return result;
}


PRIVATE
VOID
InternetDebugMemTest(
    VOID
    )
{
    //
    // test
    //

    LPVOID p;

    p = (LPVOID)ALLOCATE_MEMORY(LMEM_FIXED, 1);
    *((LPBYTE)p + 1) = 'X';
    p = (LPVOID)FREE_MEMORY((HLOCAL)p);

    INET_ASSERT(p == NULL);

    p = (LPVOID)ALLOCATE_MEMORY(LMEM_FIXED, 1);
    p = (LPVOID)REALLOCATE_MEMORY(p, 1111, LMEM_MOVEABLE | LMEM_ZEROINIT);
    p = (LPVOID)REALLOCATE_MEMORY(p, 439, LMEM_MOVEABLE);
    p = (LPVOID)REALLOCATE_MEMORY(p, 720, LMEM_MOVEABLE);
    p = (LPVOID)REALLOCATE_MEMORY(p, 256, LMEM_MOVEABLE | LMEM_ZEROINIT);
    p = (LPVOID)REALLOCATE_MEMORY(p, 16, LMEM_MOVEABLE | LMEM_ZEROINIT);
    p = (LPVOID)REALLOCATE_MEMORY(p, 128, LMEM_MOVEABLE);
    p = (LPVOID)REALLOCATE_MEMORY(p, 32, LMEM_MOVEABLE | LMEM_ZEROINIT);
    p = (LPVOID)REALLOCATE_MEMORY(p, 4, LMEM_MOVEABLE);
    p = (LPVOID)REALLOCATE_MEMORY(p, 64, LMEM_MOVEABLE);
    p = (LPVOID)REALLOCATE_MEMORY(p, 63, LMEM_MOVEABLE | LMEM_ZEROINIT);
    p = (LPVOID)REALLOCATE_MEMORY(p, 64, LMEM_MOVEABLE | LMEM_ZEROINIT);
    p = (LPVOID)REALLOCATE_MEMORY(p, 65, LMEM_MOVEABLE | LMEM_ZEROINIT);
    p = (LPVOID)REALLOCATE_MEMORY(p, 65, LMEM_MOVEABLE);
    p = (LPVOID)REALLOCATE_MEMORY(p, 64, LMEM_MOVEABLE);
    p = (LPVOID)REALLOCATE_MEMORY(p, 64, LMEM_MOVEABLE);
    p = (LPVOID)FREE_MEMORY((HLOCAL)p);

    INET_ASSERT(p == NULL);

    p = (LPVOID)ALLOCATE_MEMORY(LMEM_FIXED, 8);
    p = (LPVOID)REALLOCATE_MEMORY(p, 8, LMEM_FIXED);
    p = (LPVOID)REALLOCATE_MEMORY(p, 100000, LMEM_FIXED);
    p = (LPVOID)FREE_MEMORY((HLOCAL)p);

    INET_ASSERT(p == NULL);

    InternetDebugCheckMemFreed(TRUE);
}


PRIVATE
VOID
ReportMemoryUsage(
    VOID
    )
{
    //
    // make copies of variables in case debug print functions want to allocate
    // debug memory (!)
    //

    SIZE_T totalActualMemoryAllocated;
    SIZE_T totalBlockMemoryAllocated;
    SIZE_T totalRealMemoryAllocated;
    SIZE_T totalActualMemoryFreed;
    SIZE_T totalBlockMemoryFreed;
    SIZE_T totalRealMemoryFreed;
    SIZE_T actualMemoryAllocated;
    SIZE_T blockLengthAllocated;
    SIZE_T realLengthAllocated;
    DWORD memoryAllocations;
    DWORD goodMemoryAllocations;
    DWORD memoryReAllocations;
    DWORD goodMemoryReAllocations;
    DWORD memoryFrees;
    DWORD goodMemoryFrees;
    SIZE_T largestBlockRequested;
    SIZE_T largestBlockAllocated;
    SIZE_T smallestBlockRequested;
    SIZE_T smallestBlockAllocated;

    EnterCriticalSection(&MemoryVarsCritSec);

    totalActualMemoryAllocated = TotalActualMemoryAllocated;
    totalBlockMemoryAllocated = TotalBlockMemoryAllocated;
    totalRealMemoryAllocated = TotalRealMemoryAllocated;
    totalActualMemoryFreed = TotalActualMemoryFreed;
    totalBlockMemoryFreed = TotalBlockMemoryFreed;
    totalRealMemoryFreed = TotalRealMemoryFreed;
    actualMemoryAllocated = ActualMemoryAllocated;
    blockLengthAllocated = BlockLengthAllocated;
    realLengthAllocated = RealLengthAllocated;
    memoryAllocations = MemoryAllocations;
    goodMemoryAllocations = GoodMemoryAllocations;
    memoryReAllocations = MemoryReAllocations;
    goodMemoryReAllocations = GoodMemoryReAllocations;
    memoryFrees = MemoryFrees;
    goodMemoryFrees = GoodMemoryFrees;
    largestBlockRequested = LargestBlockRequested;
    largestBlockAllocated = LargestBlockAllocated;
    smallestBlockRequested = SmallestBlockRequested;
    smallestBlockAllocated = SmallestBlockAllocated;

    LeaveCriticalSection(&MemoryVarsCritSec);

#ifdef _WIN64
    char numBuf[64];
#else
    char numBuf[32];
#endif

    DEBUG_PUT(("********************************************************************************\n"
               "\n"
               "WinInet Debug Memory Usage:\n"
               "\n"
               "\tInternetDebugMemFlags = %#08x\n"
               "\n",
               InternetDebugMemFlags
               ));
    DEBUG_PUT(("\tTotal Memory Allocated. . . , . . . . %s\n", NiceNum(numBuf, totalActualMemoryAllocated, 0)));
    DEBUG_PUT(("\tTotal Block Length Allocated. . . . . %s\n", NiceNum(numBuf, totalBlockMemoryAllocated, 0)));
    DEBUG_PUT(("\tTotal User Length Allocated . . . . . %s\n", NiceNum(numBuf, totalRealMemoryAllocated, 0)));
    DEBUG_PUT(("\tTotal Memory Freed. . . . . . . . . . %s\n", NiceNum(numBuf, totalActualMemoryFreed, 0)));
    DEBUG_PUT(("\tTotal Block Length Freed. . . . . . . %s\n", NiceNum(numBuf, totalBlockMemoryFreed, 0)));
    DEBUG_PUT(("\tTotal User Length Freed . . . . . . . %s\n", NiceNum(numBuf, totalRealMemoryFreed, 0)));
    DEBUG_PUT(("\tMemory Still Allocated. . . . . . . . %s\n", NiceNum(numBuf, actualMemoryAllocated, 0)));
    DEBUG_PUT(("\tBlock Length Still Allocated. . . . . %s\n", NiceNum(numBuf, blockLengthAllocated, 0)));
    DEBUG_PUT(("\tUser Length Still Allocated . . . . . %s\n", NiceNum(numBuf, realLengthAllocated, 0)));
    DEBUG_PUT(("\tAttempted Memory Allocations. . . . . %s\n", NiceNum(numBuf, memoryAllocations, 0)));
    DEBUG_PUT(("\tGood Memory Allocations . . . . . . . %s\n", NiceNum(numBuf, goodMemoryAllocations, 0)));
    DEBUG_PUT(("\tAttempted Memory Reallocations. . . . %s\n", NiceNum(numBuf, memoryReAllocations, 0)));
    DEBUG_PUT(("\tGood Memory Reallocations . . . . . . %s\n", NiceNum(numBuf, goodMemoryReAllocations, 0)));
    DEBUG_PUT(("\tAttempted Memory Frees. . . . . . . . %s\n", NiceNum(numBuf, memoryFrees, 0)));
    DEBUG_PUT(("\tGood Memory Frees . . . . . . . . . . %s\n", NiceNum(numBuf, goodMemoryFrees, 0)));
    DEBUG_PUT(("\tLargest Block Requested . . . . . . . %s\n", NiceNum(numBuf, largestBlockRequested, 0)));
    DEBUG_PUT(("\tLargest Block Allocated . . . . . . . %s\n", NiceNum(numBuf, largestBlockAllocated, 0)));
    DEBUG_PUT(("\tLargest Block Requested From. . . . . %s!%d\n", SourceFilename(LargestBlockRequestedFile), LargestBlockRequestedLine));
    DEBUG_PUT(("\tSmallest Block Requested. . . . . . . %s\n", NiceNum(numBuf, smallestBlockRequested, 0)));
    DEBUG_PUT(("\tSmallest Block Allocated. . . . . . . %s\n", NiceNum(numBuf, smallestBlockAllocated, 0)));
    DEBUG_PUT(("\tSmallest Block Requested From . . . . %s!%d\n", SourceFilename(SmallestBlockRequestedFile), SmallestBlockRequestedLine));
    DEBUG_PUT(("\n"
               "\tBlocks Still Allocated. . . . . . . . %s\n", NiceNum(numBuf, goodMemoryAllocations - goodMemoryFrees, 0)));
    DEBUG_PUT(("\tMemory Still Allocated. . . . . . . . %s\n", NiceNum(numBuf, totalActualMemoryAllocated - totalActualMemoryFreed, 0)));
    DEBUG_PUT(("\n"
               "********************************************************************************\n"
               "\n"));
}


PRIVATE
VOID
ReportMemoryBlocks(
    VOID
    )
{
    DEBUG_PUT(("ReportMemoryBlocks\n\n"));

    DEBUG_PUT(("AllocatedBlockList:\n\n"));

    DumpMemoryList(&AllocatedBlockList);
    if (bDeferFree) {
        DumpDeferredFreeList();
    }
}


PRIVATE
VOID
DumpDeferredFreeList(
    VOID
    )
{
    DEBUG_PUT(("DeferredFreeList:\n\n"));

    DumpMemoryList(&DeferredFreeList);
}


PRIVATE
VOID
DumpMemoryList(
    IN LPSERIALIZED_LIST lpList
    )
{
    LPDEBUG_MEMORY_HEADER lpHeader;
    int counter = 1;

    if (bUseSymbols) {

        //
        // have to load IMAGEHLP.DLL here because we're in DLL_PROCESS_DETACH
        // and if we loaded it earlier, there's a good chance the system has
        // already freed it
        //

        InitSymLib();
    }

    LockSerializedList(lpList);
    lpHeader = (LPDEBUG_MEMORY_HEADER)HeadOfSerializedList(lpList);
    while (lpHeader != (LPDEBUG_MEMORY_HEADER)SlSelf(lpList)) {

        DEBUG_PUT(("Block # %d\n", counter));


        if (!DumpBlock(lpHeader)) {
            break;
        }

        DEBUG_PUT(("********************************************************************************\n\n"));

        lpHeader = (LPDEBUG_MEMORY_HEADER)(lpHeader->List.Flink);
        ++counter;
    }
    UnlockSerializedList(lpList);
}


PRIVATE
VOID
FindAndDumpDeferredBlock(
    IN HLOCAL hLocal
    )
{
    LPDEBUG_MEMORY_HEADER lpHeader;

    LockSerializedList(&DeferredFreeList);

    lpHeader = (LPDEBUG_MEMORY_HEADER)HeadOfSerializedList(&DeferredFreeList);
    while (lpHeader != (LPDEBUG_MEMORY_HEADER)SlSelf(&DeferredFreeList)) {
        if (hLocal == (HLOCAL)lpHeader) {
            DumpBlock(lpHeader);
            break;
        }
        lpHeader = (LPDEBUG_MEMORY_HEADER)(lpHeader->List.Flink);
    }

    UnlockSerializedList(&DeferredFreeList);
}


PRIVATE
BOOL
DumpBlock(
    IN LPDEBUG_MEMORY_HEADER lpHeader
    )
{
    BOOL ok = DumpDebugMemoryHeader(lpHeader);

    if (!ok && bStopDumpIfBadBlock) {

        DEBUG_PUT(("*** stopping block dump: header @ %#x is bad\n", lpHeader));

        return FALSE;
    }
    if (bReportUserData) {
        DumpUserData(lpHeader);
    }
    if (bReportMemoryFooters) {

        LPDEBUG_MEMORY_FOOTER lpFooter;

        lpFooter = (LPDEBUG_MEMORY_FOOTER)
                        ((LPBYTE)lpHeader
                        + sizeof(*lpHeader)
                        + ROUND_UP_DWORD(lpHeader->RequestedLength));
        ok = DumpDebugMemoryFooter(lpFooter);
        if (!ok && bStopDumpIfBadBlock) {

            DEBUG_PUT(("*** stopping block dump: footer @ %#x is bad\n", lpFooter));

            return FALSE;
        }
    }
    return TRUE;
}


PRIVATE
BOOL
DumpDebugMemoryHeader(
    LPDEBUG_MEMORY_HEADER lpHeader
    )
{
    char numBuf[32];
    BOOL result;
    LPSTR symbol;
    DWORD offset;
    int i;
    char flagsBuf[256];

    __try {
        DEBUG_PUT(("DEBUG_MEMORY_HEADER @ %#x:\n"
                   "\n",
                   lpHeader
                   ));
        DEBUG_PUT(("\tList. . . . . . . . . F=%#x B=%#x\n",
                   lpHeader->List.Flink,
                   lpHeader->List.Blink
                   ));
        DEBUG_PUT(("\tThread. . . . . . . . %#x\n",
                   lpHeader->ThreadId
                   ));
        DEBUG_PUT(("\tAllocated From. . . . %s!%d\n",
                   SourceFilename(lpHeader->CreatedFile),
                   lpHeader->CreatedLine
                   ));
        DEBUG_PUT(("\tLast Accessed From. . %s!%d\n",
                   SourceFilename(lpHeader->AccessedFile),
                   lpHeader->AccessedLine
                   ));
        DEBUG_PUT(("\tRequested Length. . . %s\n",
                   NiceNum(numBuf, lpHeader->RequestedLength, 0)
                   ));
        DEBUG_PUT(("\tBlock Length. . . . . %s\n",
                   NiceNum(numBuf, lpHeader->BlockLength, 0)
                   ));
        DEBUG_PUT(("\tActual Length . . . . %s\n",
                   NiceNum(numBuf, lpHeader->ActualLength, 0)
                   ));
        DEBUG_PUT(("\tSignature . . . . . . %x (%s)\n",
                   lpHeader->Signature,
                   (lpHeader->Signature == HEADER_SIGNATURE) ? "Good" : "BAD!!!"
                   ));
        DEBUG_PUT(("\tFlags . . . . . . . . %08x %s\n",
                   lpHeader->Flags,
                   MapMemoryFlags(lpHeader->Flags, flagsBuf)
                   ));
        DEBUG_PUT(("\tTime Deferred . . . . %08x\n",
                   lpHeader->TimeDeferred
                   ));
        DEBUG_PUT(("\tClash Test. . . . . . %d\n",
                   lpHeader->ClashTest
                   ));
        DEBUG_PUT(("\tLast Operation. . . . %s\n",
                   MapLastAccessOperation(lpHeader->LastAccessOperation)
                   ));

#if DUMP_STACK
#if ONE_STACK

        if (lpHeader->Stack[0]) {
            symbol = DbgMemGetDebugSymbol((DWORD)lpHeader->Stack[0], &offset);
        } else {
            symbol = "";
            offset = 0;
        }
        DEBUG_PUT(("\tStack . . . . . . . . %08x %s+%#x\n",
                   lpHeader->Stack[0],
                   symbol,
                   offset
                   ));
        for (i = 1; i < ARRAY_ELEMENTS(lpHeader->Stack); ++i) {
            //if (!lpHeader->lpHeader->Stack[i]) {
            //    break;
            //}
            if (lpHeader->Stack[i]) {
                symbol = DbgMemGetDebugSymbol((DWORD)lpHeader->Stack[i], &offset);
            } else {
                symbol = "";
                offset = 0;
            }
            DEBUG_PUT(("\t. . . . . . . . . . . %08x %s+%#x\n",
                       lpHeader->Stack[i],
                       symbol,
                       offset
                       ));
        }

#else

        if (lpHeader->LastAccessStack[0]) {
            symbol = DbgMemGetDebugSymbol((DWORD)lpHeader->LastAccessStack[0], &offset);
        } else {
            symbol = "";
            offset = 0;
        }
        DEBUG_PUT(("\tLastAccessStack . . . %08x %s+%#x\n",
                   lpHeader->LastAccessStack[0],
                   symbol,
                   offset
                   ));
        for (i = 1; i < ARRAY_ELEMENTS(lpHeader->LastAccessStack); ++i) {
            //if (!lpHeader->LastAccessStack[i]) {
            //    break;
            //}
            if (lpHeader->LastAccessStack[i]) {
                symbol = DbgMemGetDebugSymbol((DWORD)lpHeader->LastAccessStack[i], &offset);
            } else {
                symbol = "";
                offset = 0;
            }
            DEBUG_PUT(("\t. . . . . . . . . . . %08x %s+%#x\n",
                       lpHeader->LastAccessStack[i],
                       symbol,
                       offset
                       ));
        }
        if (lpHeader->CreateStack[0]) {
            symbol = DbgMemGetDebugSymbol((DWORD)lpHeader->CreateStack[0], &offset);
        } else {
            symbol = "";
            offset = 0;
        }
        DEBUG_PUT(("\tCreateStack . . . . . %08x %s+%#x\n",
                   lpHeader->CreateStack[0],
                   symbol,
                   offset
                   ));
        for (i = 1; i < ARRAY_ELEMENTS(lpHeader->CreateStack); ++i) {
            //if (!lpHeader->lpHeader->CreateStack[i]) {
            //    break;
            //}
            if (lpHeader->CreateStack[i]) {
                symbol = DbgMemGetDebugSymbol((DWORD)lpHeader->CreateStack[i], &offset);
            } else {
                symbol = "";
                offset = 0;
            }
            DEBUG_PUT(("\t. . . . . . . . . . . %08x %s+%#x\n",
                       lpHeader->CreateStack[i],
                       symbol,
                       offset
                       ));
        }

#endif // ONE_STACK
#endif // DUMP_STACK

        DEBUG_PUT(("\tGuard . . . . . . . . %08x\n"
                   "\n",
                   lpHeader->Guard[0]
                   ));
        result = TRUE;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        //DEBUG_PUT(("DEBUG_MEMORY_HEADER @ %#x is BAD\n", lpHeader));

        result = FALSE;
    }
    return result;
}


PRIVATE
BOOL
DumpDebugMemoryFooter(
    LPDEBUG_MEMORY_FOOTER lpFooter
    )
{
    char numBuf[32];
    BOOL result;

    _try {
        DEBUG_PUT(("DEBUG_MEMORY_FOOTER @ %#x:\n"
                   "\n",
                   lpFooter
                   ));
        DEBUG_PUT(("\tGuard . . . . . . . . %08x %08x %08x %08x\n",
                   lpFooter->Guard[0],
                   lpFooter->Guard[1],
                   lpFooter->Guard[2],
                   lpFooter->Guard[3]
                   ));
        DEBUG_PUT(("\tSignature . . . . . . %x (%s)\n",
                   lpFooter->Signature,
                   (lpFooter->Signature == FOOTER_SIGNATURE) ? "Good" : "BAD!!!"
                   ));
        DEBUG_PUT(("\tBlock Length. . . . . %s\n",
                   NiceNum(numBuf, lpFooter->BlockLength, 0)
                   ));
        DEBUG_PUT(("\tGuard2. . . . . . . . %08x %08x\n"
                   "\n",
                   lpFooter->Guard2[0],
                   lpFooter->Guard2[1]
                   ));
        result = TRUE;
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        //DEBUG_PUT(("DEBUG_MEMORY_FOOTER @ %#x is BAD\n", lpFooter));

        result = FALSE;
    }
    return result;
}


PRIVATE
VOID
DumpUserData(
    LPDEBUG_MEMORY_HEADER lpHeader
    )
{
    static char spaces[] = "                                              ";    // 15 * 3 + 2
    SIZE_T userSize = lpHeader->RequestedLength;
    SIZE_T Size = ROUND_UP_DWORD(userSize);
    LPBYTE Address = (LPBYTE)(lpHeader + 1);

    DEBUG_PUT(("\t%d (%#x) bytes of user data (rounded to %d (%#x)) @ %#x\n\n",
               userSize,
               userSize,
               Size,
               Size,
               Address
               ));

    if (bLimitUserData && (Size > MaxUserDataDumped)) {

        DEBUG_PUT(("*** User data length %d too large: limited to %d (probably bad block)\n",
                   Size,
                   MaxUserDataDumped
                   ));

        Size = MaxUserDataDumped;
    }

    //
    // dump out the data, debug style
    //

    while (Size) {

        char buf[128];
        int len;
        int clen;

        rsprintf(buf, "\t%08x  ", Address);

        clen = (int)min(Size, 16);
        if (bDumpAsDwords) {
            len = clen / 4;
        } else {
            len = clen;
        }

        //
        // dump the hex representation of each character - up to 16 per line
        //

        int i;

        for (i = 0; i < len; ++i) {
            if (bDumpAsDwords) {
                rsprintf(&buf[11 + i * 9], "%08x ", ((LPDWORD)Address)[i]);
            } else {
                rsprintf(&buf[11 + i * 3],
                         ((i & 15) == 7) ? "%02.2x-" : "%02.2x ",
                         Address[i] & 0xff
                         );
            }
        }

        //
        // write as many spaces as required to tab to ASCII field
        //

        int offset;

        if (bDumpAsDwords) {
            memcpy(&buf[11 + i * 9], spaces, (4 - len) * 9 + 2);
            offset = 49;
        } else {
            memcpy(&buf[11 + i * 3], spaces, (16 - len) * 3 + 2);
            offset = 60;
        }

        //
        // dump ASCII representation of each character
        //

        for (i = 0; i < clen; ++i) {

            char ch;

            ch = Address[i];
            buf[offset + i] = ((ch < 32) || (ch > 127)) ? '.' : ch;
        }

        buf[offset + i++] = '\r';
        buf[offset + i++] = '\n';
        buf[offset + i] = 0;

        //
        // InternetDebugOut() - no printf expansion (%s in data!), no prefixes
        //

        InternetDebugOut(buf, FALSE);

        Address += clen;
        Size -= clen;
    }

    InternetDebugOut("\r\n", FALSE);
}


PRIVATE
LPSTR
MapLastAccessOperation(
    MEMORY_ACTION Action
    )
{
    switch (Action) {
    case MemAllocate:
        return "Alloc";

    case MemReallocate:
        return "Realloc";

    case MemLock:
        return "Lock";

    case MemUnlock:
        return "Unlock";

    case MemFree:
        return "Free";

    case MemSize:
        return "Size";
    }
    return "?";
}


PRIVATE
LPSTR
MapMemoryFlags(
    DWORD Flags,
    LPSTR Buffer
    )
{
    LPSTR buf = Buffer;
    int i = 0;

    *buf++ = '(';
    if (Flags & LMEM_DISCARDABLE) {
        buf += wsprintf(buf, "DISCARDABLE");
        ++i;
    }
    if (Flags & LMEM_ZEROINIT) {
        if (i) {
            buf += wsprintf(buf, ", ");
        }
        ++i;
        buf += wsprintf(buf, "ZEROINIT");
    }
    if (Flags & LMEM_NODISCARD) {
        if (i) {
            buf += wsprintf(buf, ", ");
        }
        ++i;
        buf += wsprintf(buf, "NODISCARD");
    }
    if (Flags & LMEM_NOCOMPACT) {
        if (i) {
            buf += wsprintf(buf, ", ");
        }
        ++i;
        buf += wsprintf(buf, "NOCOMPACT");
    }
    if (i) {
        buf += wsprintf(buf, ", ");
    }
    ++i;
    buf += wsprintf(buf, (Flags & LMEM_MOVEABLE) ? "MOVEABLE" : "FIXED");
    *buf++ = ')';
    *buf++ = '\0';
    return Buffer;
}


PRIVATE
LPSTR
DbgMemGetDebugSymbol(
    DWORD Address,
    LPDWORD Offset
    ) {
    //if (!bUseSymbols) {
    //    return "?";
    //}

    //
    // RLF 04/14/98 - IMAGEHLP blowing up probably because we are doing this at
    //                process detach time. Just return offset and run convsym
    //                utility on wininet.log
    //

    //return GetDebugSymbol(Address, Offset);
    *Offset = Address;
    return "";
}

#endif // defined(USE_DEBUG_MEMORY)
