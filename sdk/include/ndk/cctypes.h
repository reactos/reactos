/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    cctypes.h

Abstract:

    Type definitions for the Cache Controller.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _CCTYPES_H
#define _CCTYPES_H

//
// Dependencies
//
#include <umtypes.h>

#ifndef NTOS_MODE_USER

//
// Kernel Exported CcData
//
extern ULONG NTSYSAPI CcFastReadNotPossible;
extern ULONG NTSYSAPI CcFastReadWait;
extern ULONG NTSYSAPI CcFastReadResourceMiss;
extern ULONG NTSYSAPI CcFastReadNoWait;
extern ULONG NTSYSAPI CcFastMdlReadNotPossible;

//
// Virtual Address Control BLock
//
typedef struct _VACB
{
    PVOID BaseAddress;
    struct _SHARED_CACHE_MAP *SharedCacheMap;
    union
    {
        LARGE_INTEGER FileOffset;
        USHORT ActiveCount;
    } Overlay;
    LIST_ENTRY LruList;
} VACB, *PVACB;

//
// Private Cache Map Structure and Flags
//
typedef struct _PRIVATE_CACHE_MAP_FLAGS
{
    ULONG DontUse:16;
    ULONG ReadAheadActive:1;
    ULONG ReadAheadEnabled:1;
    ULONG Available:14;
} PRIVATE_CACHE_MAP_FLAGS;

#define PRIVATE_CACHE_MAP_READ_AHEAD_ACTIVE     (1 << 16)
#define PRIVATE_CACHE_MAP_READ_AHEAD_ENABLED    (1 << 17)

typedef struct _PRIVATE_CACHE_MAP
{
    union
    {
        CSHORT NodeTypeCode;
        PRIVATE_CACHE_MAP_FLAGS Flags;
        ULONG UlongFlags;
    };
    ULONG ReadAheadMask;
    PFILE_OBJECT FileObject;
    LARGE_INTEGER FileOffset1;
    LARGE_INTEGER BeyondLastByte1;
    LARGE_INTEGER FileOffset2;
    LARGE_INTEGER BeyondLastByte2;
    LARGE_INTEGER ReadAheadOffset[2];
    ULONG ReadAheadLength[2];
    KSPIN_LOCK ReadAheadSpinLock;
    LIST_ENTRY PrivateLinks;
    PVOID ReadAheadWorkItem;
} PRIVATE_CACHE_MAP, *PPRIVATE_CACHE_MAP;

typedef struct _BITMAP_RANGE
{
    LIST_ENTRY Links;
    LONGLONG BasePage;
    ULONG FirstDirtyPage;
    ULONG LastDirtyPage;
    ULONG DirtyPages;
    PULONG Bitmap;
} BITMAP_RANGE, *PBITMAP_RANGE;

typedef struct _MBCB
{
    CSHORT NodeTypeCode;
    CSHORT NodeIsInZone;
    ULONG PagesToWrite;
    ULONG DirtyPages;
    ULONG Reserved;
    LIST_ENTRY BitmapRanges;
    LONGLONG ResumeWritePage;
    BITMAP_RANGE BitmapRange1;
    BITMAP_RANGE BitmapRange2;
    BITMAP_RANGE BitmapRange3;
} MBCB, *PMBCB;

#ifdef _NTIFS_INCLUDED_

//
// Shared Cache Map
//
typedef struct _SHARED_CACHE_MAP
{
    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;
    ULONG OpenCount;
    LARGE_INTEGER FileSize;
    LIST_ENTRY BcbList;
    LARGE_INTEGER SectionSize;
    LARGE_INTEGER ValidDataLength;
    LARGE_INTEGER ValidDataGoal;
    PVACB InitialVacbs[4];
    PVACB *Vacbs;
    PFILE_OBJECT FileObject;
    PVACB ActiveVacb;
    PVOID NeedToZero;
    ULONG ActivePage;
    ULONG NeedToZeroPage;
    KSPIN_LOCK ActiveVacbSpinLock;
    ULONG VacbActiveCount;
    ULONG DirtyPages;
    LIST_ENTRY SharedCacheMapLinks;
    ULONG Flags;
    LONG Status;
    PMBCB Mbcb;
    PVOID Section;
    PKEVENT CreateEvent;
    PKEVENT WaitOnActiveCount;
    ULONG PagesToWrite;
    LONGLONG BeyondLastFlush;
    PCACHE_MANAGER_CALLBACKS Callbacks;
    PVOID LazyWriteContext;
    LIST_ENTRY PrivateList;
    PVOID LogHandle;
    PFLUSH_TO_LSN FlushToLsnRoutine;
    ULONG DirtyPageThreshold;
    ULONG LazyWritePassCount;
    PCACHE_UNINITIALIZE_EVENT UninitializeEvent;
    PVACB NeedToZeroVacb;
    KSPIN_LOCK BcbSpinLock;
    PVOID Reserved;
    KEVENT Event;
    EX_PUSH_LOCK VacbPushLock;
    PRIVATE_CACHE_MAP PrivateCacheMap;
} SHARED_CACHE_MAP, *PSHARED_CACHE_MAP;

#endif /* _NTIFS_INCLUDED_  */

//
// Deferred Write list entry
//
typedef struct _DEFERRED_WRITE
{
    CSHORT NodeTypeCode;
    CSHORT NodeByteSize;
    PFILE_OBJECT FileObject;
    ULONG BytesToWrite;
    LIST_ENTRY DeferredWriteLinks;
    PKEVENT Event;
    PCC_POST_DEFERRED_WRITE PostRoutine;
    PVOID Context1;
    PVOID Context2;
    BOOLEAN LimitModifiedPages;
} DEFERRED_WRITE, *PDEFERRED_WRITE;

#endif /* NTOS_MODE_USER    */
#endif /* _CCTYPES_H        */

