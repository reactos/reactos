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
} PRIVATE_CACHE_MAP, *PPRIVATE_CACHE_MAP;

#ifdef _NTIFS_INCLUDED_

//
// Shared Cache Map
//
typedef struct _SHARED_CACHE_MAP
{
    SHORT NodeTypeCode;
    SHORT NodeByteSize;
    ULONG OpenCount;
    LARGE_INTEGER FileSize;
    LIST_ENTRY BcbList;
    LARGE_INTEGER SectionSize;
    LARGE_INTEGER ValidDataLength;
    LARGE_INTEGER ValidDataGoal;
    PVACB InitialVacbs[4];
    PVACB *Vacbs;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    EX_FAST_REF FileObjectFastRef;
#else
	PFILE_OBJECT FileObject;
#endif
    PVACB ActiveVacb;
    PVOID NeedToZero;
    ULONG ActivePage;
    ULONG NeedToZeroPage;
    ULONG ActiveVacbSpinLock;
    ULONG VacbActiveCount;
    ULONG DirtyPages;
    LIST_ENTRY SharedCacheMapLinks;
    ULONG Flags;
    ULONG Status;
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
    PVOID FlushToLsnRoutine;
    ULONG DirtyPageThreshold;
    ULONG LazyWritePassCount;
    PCACHE_UNINITIALIZE_EVENT UninitializeEvent;
    PVACB NeedToZeroVacb;
    ULONG BcbSpinLock;
    PVOID Reserved;
    KEVENT Event;
#if (NTDDI_VERSION >= NTDDI_LONGHORN) //guess
    LARGE_INTEGER HighWaterMappingOffset;
#else
     EX_PUSH_LOCK VacbPushLock;
#endif
    PRIVATE_CACHE_MAP PrivateCacheMap;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID WriteBehindWorkQueueEntry;
    PVOLUME_CACHE_MAP VolumeCacheMap;
    ULONG ProcImagePathHash;
    ULONG MappedWritesInProgress;
#endif
} SHARED_CACHE_MAP, *PSHARED_CACHE_MAP;

#endif /* _NTIFS_INCLUDED_  */
#endif /* NTOS_MODE_USER    */
#endif /* _CCTYPES_H        */

