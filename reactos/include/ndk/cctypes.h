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


#ifdef _NTIFS_INCLUDED_

typedef struct _SHARED_CACHE_MAP {
        SHORT  NodeTypeCode;
        SHORT NodeByteSize;
        ULONG OpenCount;
        LARGE_INTEGER FileSize;
        LIST_ENTRY BcbList;
        LARGE_INTEGER SectionSize;
        LARGE_INTEGER ValidDataLength;
        LARGE_INTEGER ValidDataGoal;
        PVACB InitialVacbs[4];
        PVACB Vacbs;
        PFILE_OBJECT FileObject;
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
        PMCB Mbcb;
        PVOID Section;
        PKEVENT CreateEvent;
        PKEVENT WaitOnActiveCount;
        ULONG PagesToWrite;
        LONGLONG BeyondLastFlush;
        PCACHE_MANAGER_CALLBACKS Callbacks;
        PVOID LazyWriteContext;
        PLIST_ENTRY PrivateList;
        PVOID LogHandle;
        PVOID FlushToLsnRoutine;
        ULONG DirtyPageThreshold;
        ULONG LazyWritePassCount;
        PCACHE_UNINITIALIZE_EVENT UninitializeEvent;
        PVACB NeedToZeroVacb;
        ULONG BcbSpinLock;
        PVOID Reserved;
        KEVENT Event;
        /* FIX ME: This should be PEX_PUSH_LOCK */
        PVOID VacbPushLock;
        PPRIVATE_CACHE_MAP PrivateCacheMap;
   } SHARED_CACHE_MAP;


#endif /* _NTIFS_INCLUDED_  */
#endif /* NTOS_MODE_USER    */
#endif /* _CCTYPES_H        */

