/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    debugmem.h

Abstract:

    Header for debugmem.cxx

Author:

     Richard L Firth (rfirth) 02-Feb-1995

Revision History:

    02-Feb-1995
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// manifests
//

//
// USE_PRIVATE_HEAP_IN_RETAIL - by default we use the process heap in the retail
// build. Alternative is to use a private (wininet) heap (which we do in the
// debug version if required)
//

#if !defined(USE_PRIVATE_HEAP_IN_RETAIL)
#define USE_PRIVATE_HEAP_IN_RETAIL  0
#endif

//
// prototypes
//

VOID
InternetDebugMemInitialize(
    VOID
    );

VOID
InternetDebugMemTerminate(
    IN BOOL bReport
    );

HLOCAL
InternetDebugAllocMem(
    IN UINT Flags,
    IN UINT Size,
    IN LPSTR File,
    IN DWORD Line
    );

HLOCAL
InternetDebugFreeMem(
    IN HLOCAL hLocal,
    IN LPSTR File,
    IN DWORD Line
    );

HLOCAL
InternetDebugReAllocMem(
    IN HLOCAL hLocal,
    IN UINT Size,
    IN UINT Flags,
    IN LPSTR File,
    IN DWORD Line
    );

SIZE_T
InternetDebugSizeMem(
    IN HLOCAL hLocal,
    IN LPSTR File,
    IN DWORD Line
    );

BOOL
InternetDebugCheckMemFreed(
    IN BOOL bReport
    );

BOOL
InternetDebugMemReport(
    IN BOOL bTerminateSymbols,
    IN BOOL bCloseFile
    );

//
// macros
//

#if defined(USE_DEBUG_MEMORY)

#define ALLOCATOR(Flags, Size) \
    InternetDebugAllocMem(Flags, Size, __FILE__, __LINE__)

#define DEALLOCATOR(hLocal) \
    InternetDebugFreeMem(hLocal, __FILE__, __LINE__)

#define REALLOCATOR(hLocal, Size, Flags) \
    InternetDebugReAllocMem(hLocal, Size, Flags, __FILE__, __LINE__)

#define MEMORYSIZER(hLocal) \
    InternetDebugSizeMem(hLocal, __FILE__, __LINE__)

#define INITIALIZE_DEBUG_MEMORY() \
    InternetDebugMemInitialize()

#define TERMINATE_DEBUG_MEMORY(bReport) \
    InternetDebugMemTerminate(bReport)

#define CHECK_MEMORY_FREED(bReport) \
    InternetDebugCheckMemFreed(bReport)

#define REPORT_DEBUG_MEMORY(bTermSym, bCloseFile) \
    InternetDebugMemReport(bTermSym, bCloseFile)

#else   // retail version

#if USE_PRIVATE_HEAP_IN_RETAIL

#error no other memory allocation schemes defined

#else

#ifndef WININET_UNIX_PRVATE_ALLOCATOR
#define ALLOCATOR(Flags, Size) \
    LocalAlloc(Flags, Size)

#define DEALLOCATOR(hLocal) \
    LocalFree(hLocal)

#define REALLOCATOR(hLocal, Size, Flags) \
    LocalReAlloc(hLocal, Size, Flags)

#define MEMORYSIZER(hLocal) \
    LocalSize(hLocal)
#else

HLOCAL IEUnixLocalAlloc(UINT wFlags, UINT wBytes);
HLOCAL IEUnixLocalReAlloc(HLOCAL hMemory, UINT wBytes, UINT wFlags);
HLOCAL IEUnixLocalFree(HLOCAL hMem);
UINT IEUnixLocalSize(HLOCAL hMem);
LPVOID IEUnixLocalLock(HLOCAL hMem);

#define ALLOCATOR(Flags, Size)\
    IEUnixLocalAlloc(Flags, Size)
#define DEALLOCATOR(hLocal)\
    IEUnixLocalFree(hLocal)
#define REALLOCATOR(hLocal, Size, Flags)\
    IEUnixLocalReAlloc(hLocal, Size, Flags)
#define MEMORYSIZER(hLocal) \
    IEUnixLocalSize(hLocal)
#endif /* unix */
#endif // USE_PRIVATE_HEAP_IN_RETAIL

#define INITIALIZE_DEBUG_MEMORY() \
    /* NOTHING */

#define TERMINATE_DEBUG_MEMORY(bReport) \
    /* NOTHING */

#define CHECK_MEMORY_FREED(bReport) \
    /* NOTHING */

#define REPORT_DEBUG_MEMORY(bTermSym, bCloseFile) \
    /* NOTHING */

#endif // defined(USE_DEBUG_MEMORY)

#define ALLOCATE_ZERO_MEMORY(Size) \
    ALLOCATE_MEMORY(LPTR, (Size))

#define ALLOCATE_FIXED_MEMORY(Size) \
    ALLOCATE_MEMORY(LMEM_FIXED, (Size))

#define ALLOCATE_MEMORY(Flags, Size) \
    ALLOCATOR((UINT)(Flags), (UINT)(Size))

#define FREE_ZERO_MEMORY(hLocal) \
    FREE_MEMORY((HLOCAL)(hLocal))

#define FREE_FIXED_MEMORY(hLocal) \
    FREE_MEMORY((HLOCAL)(hLocal))

#define FREE_MEMORY(hLocal) \
    DEALLOCATOR((HLOCAL)(hLocal))

#define REALLOCATE_MEMORY(hLocal, Size, Flags) \
    REALLOCATOR((HLOCAL)(hLocal), (UINT)(Size), (UINT)(Flags))

#define MEMORY_SIZE(hLocal) \
    MEMORYSIZER((HLOCAL)(hLocal))

#if defined(__cplusplus)
}
#endif

//
// Wininet no longer uses moveable memory
//

#define LOCK_MEMORY(p)          (LPSTR)(p)
#define UNLOCK_MEMORY(p)
