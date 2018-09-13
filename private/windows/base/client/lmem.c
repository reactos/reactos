/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    lmem.c

Abstract:

    This module contains the Win32 Local Memory Management APIs

Author:

    Steve Wood (stevewo) 24-Sep-1990

Revision History:

--*/

#include "basedll.h"

void
BaseDllInitializeMemoryManager( VOID )
{
    BaseHeap = RtlProcessHeap();
    RtlInitializeHandleTable( 0xFFFF,
                              sizeof( BASE_HANDLE_TABLE_ENTRY ),
                              &BaseHeapHandleTable
                            );
    NtQuerySystemInformation(SystemRangeStartInformation,
                             &SystemRangeStart,
                             sizeof(SystemRangeStart),
                             NULL);
}

#if i386
#pragma optimize("y",off)
#endif

HLOCAL
WINAPI
LocalAlloc(
    UINT uFlags,
    SIZE_T uBytes
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    HANDLE hMem;
    ULONG Flags;
    LPSTR p;

    if (uFlags & ~LMEM_VALID_FLAGS) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( NULL );
        }

    Flags = 0;
    if (uFlags & LMEM_ZEROINIT) {
        Flags |= HEAP_ZERO_MEMORY;
        }

    if (!(uFlags & LMEM_MOVEABLE)) {
        p = RtlAllocateHeap( BaseHeap,
                             MAKE_TAG( LMEM_TAG ) | Flags,
                             uBytes
                           );
        if (p == NULL) {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            }

        return( p );
        }

    RtlLockHeap( BaseHeap );
    Flags |= HEAP_NO_SERIALIZE | HEAP_SETTABLE_USER_VALUE | BASE_HEAP_FLAG_MOVEABLE;
    try {
        p = NULL;
        HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)RtlAllocateHandle( &BaseHeapHandleTable, NULL );
        if (HandleEntry == NULL) {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            goto Fail;
            }

        hMem = (HANDLE)&HandleEntry->Object;
        if (uBytes != 0) {
            p = (LPSTR)RtlAllocateHeap( BaseHeap, MAKE_TAG( LMEM_TAG ) | Flags, uBytes );
            if (p == NULL) {
                HandleEntry->Flags = RTL_HANDLE_ALLOCATED;
                RtlFreeHandle( &BaseHeapHandleTable, (PRTL_HANDLE_TABLE_ENTRY)HandleEntry );
                HandleEntry = NULL;
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                }
            else {
                RtlSetUserValueHeap( BaseHeap, HEAP_NO_SERIALIZE, p, hMem );
                }
            }
        else {
            p = NULL;
            }
Fail:   ;
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        p = NULL;
        BaseSetLastNTError( GetExceptionCode() );
        }

    RtlUnlockHeap( BaseHeap );

    if (HandleEntry != NULL) {
        if (HandleEntry->Object = p) {
            HandleEntry->Flags = RTL_HANDLE_ALLOCATED;
            }
        else {
            HandleEntry->Flags = RTL_HANDLE_ALLOCATED | BASE_HANDLE_DISCARDED;
            }

        if (uFlags & LMEM_DISCARDABLE) {
            HandleEntry->Flags |= BASE_HANDLE_DISCARDABLE;
            }

        if (uFlags & LMEM_MOVEABLE) {
            HandleEntry->Flags |= BASE_HANDLE_MOVEABLE;
            }

        p = (LPSTR)hMem;
        }

    return( (HANDLE)p );
}


HLOCAL
WINAPI
LocalReAlloc(
    HLOCAL hMem,
    SIZE_T uBytes,
    UINT uFlags
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    LPSTR p;
    ULONG Flags;

    if ((uFlags & ~(LMEM_VALID_FLAGS | LMEM_MODIFY)) ||
        ((uFlags & LMEM_DISCARDABLE) && !(uFlags & LMEM_MODIFY))
       ) {
#if DBG
        DbgPrint( "*** LocalReAlloc( %lx ) - invalid flags\n", uFlags );
        BaseHeapBreakPoint();
#endif
        SetLastError( ERROR_INVALID_PARAMETER );
        return( NULL );
        }

    Flags = 0;
    if (uFlags & LMEM_ZEROINIT) {
        Flags |= HEAP_ZERO_MEMORY;
        }
    if (!(uFlags & LMEM_MOVEABLE)) {
        Flags |= HEAP_REALLOC_IN_PLACE_ONLY;
        }

    RtlLockHeap( BaseHeap );
    Flags |= HEAP_NO_SERIALIZE;
    try {
        if ((ULONG_PTR)hMem & BASE_HANDLE_MARK_BIT) {
            HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)
                CONTAINING_RECORD( hMem, BASE_HANDLE_TABLE_ENTRY, Object );

            if (!RtlIsValidHandle( &BaseHeapHandleTable, (PRTL_HANDLE_TABLE_ENTRY)HandleEntry )) {
#if DBG
                DbgPrint( "*** LocalReAlloc( %lx ) - invalid handle\n", hMem );
                BaseHeapBreakPoint();
#endif
                SetLastError( ERROR_INVALID_HANDLE );
                hMem = NULL;
                }
            else
            if (uFlags & LMEM_MODIFY) {
                if (uFlags & LMEM_DISCARDABLE) {
                    HandleEntry->Flags |= BASE_HANDLE_DISCARDABLE;
                    }
                else {
                    HandleEntry->Flags &= ~BASE_HANDLE_DISCARDABLE;
                    }
                }
            else {
                p = HandleEntry->Object;
                if (uBytes == 0) {
                    hMem = NULL;
                    if (p != NULL) {
                        if ((uFlags & LMEM_MOVEABLE) && HandleEntry->LockCount == 0) {
                            if (RtlFreeHeap( BaseHeap, Flags | HEAP_NO_SERIALIZE, p )) {
                                HandleEntry->Object = NULL;
                                HandleEntry->Flags |= BASE_HANDLE_DISCARDED;
                                hMem = (HANDLE)&HandleEntry->Object;
                                }
                            }
                        else {
#if DBG
                            DbgPrint( "*** LocalReAlloc( %lx ) - failing with locked handle\n", &HandleEntry->Object );
                            BaseHeapBreakPoint();
#endif
                            }
                        }
                    else {
                        hMem = (HANDLE)&HandleEntry->Object;
                        }
                    }
                else {
                    Flags |= HEAP_SETTABLE_USER_VALUE | BASE_HEAP_FLAG_MOVEABLE;
                    if (p == NULL) {
                        p = RtlAllocateHeap( BaseHeap, MAKE_TAG( LMEM_TAG ) | Flags, uBytes );
                        if (p != NULL) {
                            RtlSetUserValueHeap( BaseHeap, HEAP_NO_SERIALIZE, p, hMem );
                            }
                        }
                    else {
                        if (!(uFlags & LMEM_MOVEABLE) &&
                            HandleEntry->LockCount != 0
                           ) {
                            Flags |= HEAP_REALLOC_IN_PLACE_ONLY;
                            }
                        else {
                            Flags &= ~HEAP_REALLOC_IN_PLACE_ONLY;
                            }

                        p = RtlReAllocateHeap( BaseHeap, MAKE_TAG( LMEM_TAG ) | Flags, p, uBytes );
                        }

                    if (p != NULL) {
                        HandleEntry->Object = p;
                        HandleEntry->Flags &= ~BASE_HANDLE_DISCARDED;
                        }
                    else {
                        hMem = NULL;
                        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                        }
                    }
                }
            }
        else
        if (!(uFlags & LMEM_MODIFY)) {
            hMem = RtlReAllocateHeap( BaseHeap, MAKE_TAG( LMEM_TAG ) | Flags, (PVOID)hMem, uBytes );
            if (hMem == NULL) {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        hMem = NULL;
        BaseSetLastNTError( GetExceptionCode() );
        }

    RtlUnlockHeap( BaseHeap );

    return( (LPSTR)hMem );
}

PVOID
WINAPI
LocalLock(
    HLOCAL hMem
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    LPSTR p;

    if ((ULONG_PTR)hMem & BASE_HANDLE_MARK_BIT) {
        RtlLockHeap( BaseHeap );

        try {
            HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)
                CONTAINING_RECORD( hMem, BASE_HANDLE_TABLE_ENTRY, Object );

            if (!RtlIsValidHandle( &BaseHeapHandleTable, (PRTL_HANDLE_TABLE_ENTRY)HandleEntry )) {
#if DBG
                DbgPrint( "*** LocalLock( %lx ) - invalid handle\n", hMem );
                BaseHeapBreakPoint();
#endif
                SetLastError( ERROR_INVALID_HANDLE );
                p = NULL;
                }
            else {
                p = HandleEntry->Object;
                if (p != NULL) {
                    if (HandleEntry->LockCount++ == LMEM_LOCKCOUNT) {
                        HandleEntry->LockCount--;
                        }
                    }
                else {
                    SetLastError( ERROR_DISCARDED );
                    }
                }
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            p = NULL;
            BaseSetLastNTError( GetExceptionCode() );
            }

        RtlUnlockHeap( BaseHeap );

        return( p );
        }
    else {
        if ( (ULONG_PTR)hMem >= SystemRangeStart ) {
            return NULL;
            }
        return( (LPSTR)hMem );
        }
}

HLOCAL
WINAPI
LocalHandle(
    LPCVOID pMem
    )
{
    HANDLE Handle;
    ULONG Flags;

    RtlLockHeap( BaseHeap );
    try {
        Handle = NULL;
        if (!RtlGetUserInfoHeap( BaseHeap, HEAP_NO_SERIALIZE, (LPVOID)pMem, &Handle, &Flags )) {
            SetLastError( ERROR_INVALID_HANDLE );
            }
        else
        if (Handle == NULL || !(Flags & BASE_HEAP_FLAG_MOVEABLE)) {
            Handle = (HANDLE)pMem;
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        }

    RtlUnlockHeap( BaseHeap );

    return( Handle );
}

BOOL
WINAPI
LocalUnlock(
    HLOCAL hMem
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    BOOL Result;

    Result = FALSE;
    if ((ULONG_PTR)hMem & BASE_HANDLE_MARK_BIT) {
        RtlLockHeap( BaseHeap );
        try {
            HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)
                CONTAINING_RECORD( hMem, BASE_HANDLE_TABLE_ENTRY, Object );

            if (!RtlIsValidHandle( &BaseHeapHandleTable, (PRTL_HANDLE_TABLE_ENTRY)HandleEntry )) {
#if DBG
                DbgPrint( "*** LocalUnlock( %lx ) - invalid handle\n", hMem );
                BaseHeapBreakPoint();
#endif
                SetLastError( ERROR_INVALID_HANDLE );
                }
            else
            if (HandleEntry->LockCount-- == 0) {
                HandleEntry->LockCount++;
                SetLastError( ERROR_NOT_LOCKED );
                }
            else
            if (HandleEntry->LockCount != 0) {
                Result = TRUE;
                }
            else {
                SetLastError( NO_ERROR );
                }
            }
        except (EXCEPTION_EXECUTE_HANDLER) {
            BaseSetLastNTError( GetExceptionCode() );
            }

        RtlUnlockHeap( BaseHeap );
        }
    else {
        SetLastError( ERROR_NOT_LOCKED );
        }

    return( Result );
}


SIZE_T
WINAPI
LocalSize(
    HLOCAL hMem
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    PVOID Handle;
    ULONG Flags;
    SIZE_T uSize;

    uSize = MAXULONG_PTR;
    Flags = 0;
    RtlLockHeap( BaseHeap );
    try {
        if (!((ULONG_PTR)hMem & BASE_HANDLE_MARK_BIT)) {
            Handle = NULL;
            if (!RtlGetUserInfoHeap( BaseHeap, Flags, hMem, &Handle, &Flags )) {
                }
            else
            if (Handle == NULL || !(Flags & BASE_HEAP_FLAG_MOVEABLE)) {
                uSize = RtlSizeHeap( BaseHeap, HEAP_NO_SERIALIZE, (PVOID)hMem );
                }
            else {
                hMem = Handle;
                }
            }

        if ((ULONG_PTR)hMem & BASE_HANDLE_MARK_BIT) {
            HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)
                CONTAINING_RECORD( hMem, BASE_HANDLE_TABLE_ENTRY, Object );

            if (!RtlIsValidHandle( &BaseHeapHandleTable, (PRTL_HANDLE_TABLE_ENTRY)HandleEntry )) {
#if DBG
                DbgPrint( "*** LocalSize( %lx ) - invalid handle\n", hMem );
                BaseHeapBreakPoint();
#endif
                SetLastError( ERROR_INVALID_HANDLE );
                }
            else
            if (HandleEntry->Flags & BASE_HANDLE_DISCARDED) {
                uSize = HandleEntry->Size;
                }
            else {
                uSize = RtlSizeHeap( BaseHeap, HEAP_NO_SERIALIZE, HandleEntry->Object );
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        }

    RtlUnlockHeap( BaseHeap );

    if (uSize == MAXULONG_PTR) {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
        }
    else {
        return uSize;
        }
}


UINT
WINAPI
LocalFlags(
    HLOCAL hMem
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    HANDLE Handle;
    ULONG Flags;
    UINT uFlags;

    uFlags = LMEM_INVALID_HANDLE;
    RtlLockHeap( BaseHeap );
    try {
        if (!((ULONG_PTR)hMem & BASE_HANDLE_MARK_BIT)) {
            Handle = NULL;
            Flags = 0;
            if (!RtlGetUserInfoHeap( BaseHeap, Flags, hMem, &Handle, &Flags )) {
                }
            else
            if (Handle == NULL || !(Flags & BASE_HEAP_FLAG_MOVEABLE)) {
                uFlags = 0;
                }
            else {
                hMem = Handle;
                }
            }

        if ((ULONG_PTR)hMem & BASE_HANDLE_MARK_BIT) {
            HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)
                CONTAINING_RECORD( hMem, BASE_HANDLE_TABLE_ENTRY, Object );

            if (RtlIsValidHandle( &BaseHeapHandleTable, (PRTL_HANDLE_TABLE_ENTRY)HandleEntry )) {
                uFlags = HandleEntry->LockCount & LMEM_LOCKCOUNT;
                if (HandleEntry->Flags & BASE_HANDLE_DISCARDED) {
                    uFlags |= LMEM_DISCARDED;
                    }

                if (HandleEntry->Flags & BASE_HANDLE_DISCARDABLE) {
                    uFlags |= LMEM_DISCARDABLE;
                    }
                }
            }

        if (uFlags == LMEM_INVALID_HANDLE) {
#if DBG
            DbgPrint( "*** LocalFlags( %lx ) - invalid handle\n", hMem );
            BaseHeapBreakPoint();
#endif
            SetLastError( ERROR_INVALID_HANDLE );
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        }

    RtlUnlockHeap( BaseHeap );

    return( uFlags );
}


HLOCAL
WINAPI
LocalFree(
    HLOCAL hMem
    )
{
    PBASE_HANDLE_TABLE_ENTRY HandleEntry;
    LPSTR p;

    try {
        if (!((ULONG_PTR)hMem & BASE_HANDLE_MARK_BIT)) {
            if (RtlFreeHeap( BaseHeap,
                             0,
                             (PVOID)hMem
                           )
               ) {
                return NULL;
                }
            else {
                SetLastError( ERROR_INVALID_HANDLE );
                return hMem;
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        return hMem;
        }

    RtlLockHeap( BaseHeap );
    try {
        if ((ULONG_PTR)hMem & BASE_HANDLE_MARK_BIT) {
            HandleEntry = (PBASE_HANDLE_TABLE_ENTRY)
                CONTAINING_RECORD( hMem, BASE_HANDLE_TABLE_ENTRY, Object );

            if (!RtlIsValidHandle( &BaseHeapHandleTable, (PRTL_HANDLE_TABLE_ENTRY)HandleEntry )) {
#if DBG
                DbgPrint( "*** LocalFree( %lx ) - invalid handle\n", hMem );
                BaseHeapBreakPoint();
#endif
                SetLastError( ERROR_INVALID_HANDLE );
                p = NULL;
                }
            else {
#if DBG
                if (HandleEntry->LockCount != 0) {
                    DbgPrint( "BASE: LocalFree called with a locked object.\n" );
                    BaseHeapBreakPoint();
                    }
#endif
                p = HandleEntry->Object;
                RtlFreeHandle( &BaseHeapHandleTable, (PRTL_HANDLE_TABLE_ENTRY)HandleEntry );
                if (p == NULL) {
                    hMem = NULL;
                    }
                }
            }
        else {
            p = (LPSTR)hMem;
            }

        if (p != NULL) {
            if (RtlFreeHeap( BaseHeap, HEAP_NO_SERIALIZE, p )) {
                hMem = NULL;
                }
            else {
                SetLastError( ERROR_INVALID_HANDLE );
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        BaseSetLastNTError( GetExceptionCode() );
        }

    RtlUnlockHeap( BaseHeap );

    return( hMem );
}


SIZE_T
WINAPI
LocalCompact(
    UINT uMinFree
    )
{
    return RtlCompactHeap( BaseHeap, 0 );
}

SIZE_T
WINAPI
LocalShrink(
    HLOCAL hMem,
    UINT cbNewSize
    )
{
    return RtlCompactHeap( BaseHeap, 0 );
}


HANDLE
WINAPI
HeapCreate(
    DWORD flOptions,
    SIZE_T dwInitialSize,
    SIZE_T dwMaximumSize
    )
{
    HANDLE hHeap;
    ULONG GrowthThreshold;
    ULONG Flags;


    Flags = (flOptions & (HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE)) | HEAP_CLASS_1;
    GrowthThreshold = 0;

    if (dwMaximumSize < BASE_SYSINFO.PageSize) {

        if (dwMaximumSize == 0) {

            GrowthThreshold = BASE_SYSINFO.PageSize * 16;
            Flags |= HEAP_GROWABLE;
            }
        else {
            dwMaximumSize = BASE_SYSINFO.PageSize;
            }
        }

    if (GrowthThreshold == 0 && dwInitialSize > dwMaximumSize) {
        dwMaximumSize = dwInitialSize;
        }

    hHeap = (HANDLE)RtlCreateHeap( Flags,
                                   NULL,
                                   dwMaximumSize,
                                   dwInitialSize,
                                   0,
                                   NULL
                                 );
    if (hHeap == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        }

    return( hHeap );
}

BOOL
WINAPI
HeapDestroy(
    HANDLE hHeap
    )
{
    if (RtlDestroyHeap( (PVOID)hHeap ) == NULL ) {
        return( TRUE );
        }
    else {
        SetLastError( ERROR_INVALID_HANDLE );
        return( FALSE );
        }
}

BOOL
WINAPI
HeapExtend(
    HANDLE hHeap,
    DWORD dwFlags,
    LPVOID lpBase,
    DWORD dwBytes
    )
{
    NTSTATUS Status;

    Status = RtlExtendHeap( hHeap, dwFlags, lpBase, dwBytes );
    if (NT_SUCCESS( Status )) {
        return TRUE;
        }
    else {
        BaseSetLastNTError( Status );
        }
    return FALSE;
}

WINBASEAPI
DWORD
WINAPI
HeapCreateTagsW(
    HANDLE hHeap,
    DWORD dwFlags,
    LPCWSTR lpTagPrefix,
    LPCWSTR lpTagNames
    )
{
    return RtlCreateTagHeap( hHeap, dwFlags, (PWSTR)lpTagPrefix, (PWSTR)lpTagNames );
}

WINBASEAPI
LPCWSTR
WINAPI
HeapQueryTagW(
    HANDLE hHeap,
    DWORD dwFlags,
    WORD wTagIndex,
    BOOL bResetCounters,
    LPHEAP_TAG_INFO TagInfo
    )
{
    ASSERT( sizeof(RTL_HEAP_TAG_INFO) == sizeof(HEAP_TAG_INFO) );
    return RtlQueryTagHeap( hHeap,
                            dwFlags,
                            wTagIndex,
                            (BOOLEAN)bResetCounters,
                            (PRTL_HEAP_TAG_INFO)TagInfo
                          );
}


BOOL
WINAPI
HeapSummary(
    HANDLE hHeap,
    DWORD dwFlags,
    LPHEAP_SUMMARY lpSummary
    )
{
    NTSTATUS Status;
    RTL_HEAP_USAGE HeapInfo;

    if (lpSummary->cb != sizeof( *lpSummary )) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
        }

    HeapInfo.Length = sizeof( HeapInfo );
    Status = RtlUsageHeap( hHeap,
                           dwFlags & ~(HEAP_USAGE_ALLOCATED_BLOCKS |
                                       HEAP_USAGE_FREE_BUFFER
                                      ),
                           &HeapInfo
                         );
    if (NT_SUCCESS( Status )) {
        lpSummary->cbAllocated = HeapInfo.BytesAllocated;
        lpSummary->cbCommitted = HeapInfo.BytesCommitted;
        return TRUE;
        }
    else {
        BaseSetLastNTError( Status );
        return FALSE;
        }
}

BOOL
WINAPI
HeapUsage(
    HANDLE hHeap,
    DWORD dwFlags,
    BOOL bFirstCall,
    BOOL bLastCall,
    PHEAP_USAGE lpUsage
    )
{
    NTSTATUS Status;

    if (lpUsage->cb != sizeof( *lpUsage ) || (bFirstCall & bLastCall)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
        }

    dwFlags &= ~(HEAP_USAGE_ALLOCATED_BLOCKS |
                 HEAP_USAGE_FREE_BUFFER
                );

    if (bLastCall) {
        dwFlags |= HEAP_USAGE_FREE_BUFFER;
        }
    else {
        dwFlags |= HEAP_USAGE_ALLOCATED_BLOCKS;
        if (bFirstCall) {
            RtlZeroMemory( (&lpUsage->cb)+1, sizeof( *lpUsage ) - sizeof( lpUsage->cb ) );
            }
        }

    ASSERT( sizeof(RTL_HEAP_USAGE) == sizeof(HEAP_USAGE) );
    Status = RtlUsageHeap( hHeap, dwFlags, (PRTL_HEAP_USAGE)lpUsage );
    if (NT_SUCCESS( Status )) {
        if (Status == STATUS_MORE_ENTRIES) {
            return TRUE;
            }
        else {
            SetLastError( NO_ERROR );
            return FALSE;
            }
        }
    else {
        BaseSetLastNTError( Status );
        return FALSE;
        }
}

BOOL
WINAPI
HeapValidate(
    HANDLE hHeap,
    DWORD dwFlags,
    LPVOID lpMem
    )
{
    return RtlValidateHeap( hHeap, dwFlags, lpMem );
}

HANDLE
WINAPI
GetProcessHeap( VOID )
{
    return RtlProcessHeap();
}


WINBASEAPI
DWORD
WINAPI
GetProcessHeaps(
    DWORD NumberOfHeaps,
    PHANDLE ProcessHeaps
    )
{
    return RtlGetProcessHeaps( NumberOfHeaps, ProcessHeaps );
}


WINBASEAPI
SIZE_T
WINAPI
HeapCompact(
    HANDLE hHeap,
    DWORD dwFlags
    )
{
    return RtlCompactHeap( hHeap, dwFlags );
}


WINBASEAPI
BOOL
WINAPI
HeapLock(
    HANDLE hHeap
    )
{
    return RtlLockHeap( hHeap );
}


WINBASEAPI
BOOL
WINAPI
HeapUnlock(
    HANDLE hHeap
    )
{
    return RtlUnlockHeap( hHeap );
}

WINBASEAPI
BOOL
WINAPI
HeapWalk(
    HANDLE hHeap,
    LPPROCESS_HEAP_ENTRY lpEntry
    )
{
    RTL_HEAP_WALK_ENTRY Entry;
    NTSTATUS Status;

    if (lpEntry->lpData == NULL) {
        Entry.DataAddress = NULL;
        Status = RtlWalkHeap( hHeap, &Entry );
        }
    else {
        Entry.DataAddress = lpEntry->lpData;
        Entry.SegmentIndex = lpEntry->iRegionIndex;
        if (lpEntry->wFlags & PROCESS_HEAP_REGION) {
            Entry.Flags = RTL_HEAP_SEGMENT;
            }
        else
        if (lpEntry->wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE) {
            Entry.Flags = RTL_HEAP_UNCOMMITTED_RANGE;
            Entry.DataSize = lpEntry->cbData;
            }
        else
        if (lpEntry->wFlags & PROCESS_HEAP_ENTRY_BUSY) {
            Entry.Flags = RTL_HEAP_BUSY;
            }
        else {
            Entry.Flags = 0;
            }

        Status = RtlWalkHeap( hHeap, &Entry );
        }

    if (NT_SUCCESS( Status )) {
        lpEntry->lpData = Entry.DataAddress;
        lpEntry->cbData = (DWORD)Entry.DataSize;
        lpEntry->cbOverhead = Entry.OverheadBytes;
        lpEntry->iRegionIndex = Entry.SegmentIndex;
        if (Entry.Flags & RTL_HEAP_BUSY) {
            lpEntry->wFlags = PROCESS_HEAP_ENTRY_BUSY;
            if (Entry.Flags & BASE_HEAP_FLAG_DDESHARE) {
                lpEntry->wFlags |= PROCESS_HEAP_ENTRY_DDESHARE;
                }

            if (Entry.Flags & BASE_HEAP_FLAG_MOVEABLE) {
                lpEntry->wFlags |= PROCESS_HEAP_ENTRY_MOVEABLE;
                lpEntry->Block.hMem = (HLOCAL)Entry.Block.Settable;
                }

            memset( lpEntry->Block.dwReserved, 0, sizeof( lpEntry->Block.dwReserved ) );
            }
        else
        if (Entry.Flags & RTL_HEAP_SEGMENT) {
            lpEntry->wFlags = PROCESS_HEAP_REGION;
            lpEntry->Region.dwCommittedSize = Entry.Segment.CommittedSize;
            lpEntry->Region.dwUnCommittedSize = Entry.Segment.UnCommittedSize;
            lpEntry->Region.lpFirstBlock = Entry.Segment.FirstEntry;
            lpEntry->Region.lpLastBlock = Entry.Segment.LastEntry;
            }
        else
        if (Entry.Flags & RTL_HEAP_UNCOMMITTED_RANGE) {
            lpEntry->wFlags = PROCESS_HEAP_UNCOMMITTED_RANGE;
            memset( &lpEntry->Region, 0, sizeof( lpEntry->Region ) );
            }
        else {
            lpEntry->wFlags = 0;
            }

        return TRUE;
        }
    else {
        BaseSetLastNTError( Status );
        return FALSE;
        }
}

#if DBG
VOID
BaseHeapBreakPoint( VOID )
{
    if (NtCurrentPeb()->BeingDebugged)
        {
#if i386
        _asm {  int 3 }
#else
        DbgBreakPoint();
#endif
        }
}
#endif
