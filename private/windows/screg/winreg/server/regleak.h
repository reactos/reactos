/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    regleak.h

Abstract:

    This file contains declarations for data structures
    needed for tracking win32 registry leaks

Author:

    Adam Edwards (adamed) 14-Nov-1997

Notes:

--*/

#ifdef LOCAL

#if !defined(_REGLEAK_H_)
#define _REGLEAK_H_

#ifdef LEAK_TRACK

#include "imagehlp.h"

#define MAX_LEAK_STACK_DEPTH 40
#define SYM_PATH_MAX_SIZE    1000

typedef struct _TrackObjectData 
{
    LIST_ENTRY Links;
    HKEY       hKey;
    DWORD      dwStackDepth;
    PVOID*     rgStack;
} TrackObjectData;

void     TrackObjectDataPrint(TrackObjectData* pKeyData);
NTSTATUS TrackObjectDataInit(TrackObjectData* pKeyData, PVOID* rgStack, DWORD dwMaxStackDepth, HKEY hKey);
NTSTATUS TrackObjectDataClear(TrackObjectData* pKeyData);

NTSTATUS GetLeakStack(PVOID** prgStack, DWORD* pdwMaxDepth, DWORD dwMaxDepth);

enum
{
    LEAK_TRACK_FLAG_NONE = 0,
    LEAK_TRACK_FLAG_USER = 1,
    LEAK_TRACK_FLAG_ALL = 0xFFFFFFFF
};

typedef struct _RegLeakTable
{
    TrackObjectData*       pHead;
    DWORD                  cKeys;
    DWORD                  dwFlags;
    BOOL                   bCriticalSectionInitialized;
    RTL_CRITICAL_SECTION   CriticalSection;

} RegLeakTable;


typedef struct _RegLeakTraceInfo {
    DWORD   dwMaxStackDepth;
    LPTSTR  szSymPath;
    BOOL    bEnableLeakTrack;    
    RTL_CRITICAL_SECTION   StackInitCriticalSection;
} RegLeakTraceInfo;

extern RegLeakTraceInfo g_RegLeakTraceInfo;


NTSTATUS RegLeakTableInit(RegLeakTable* pLeakTable, DWORD dwFlags);
NTSTATUS RegLeakTableClear(RegLeakTable* pLeakTable);
NTSTATUS RegLeakTableAddKey(RegLeakTable* pLeakTable, HKEY hKey);
NTSTATUS RegLeakTableRemoveKey(RegLeakTable* pLeakTable, HKEY hKey);

BOOL RegLeakTableIsEmpty(RegLeakTable* pLeakTable);
BOOL RegLeakTableIsTrackedObject(RegLeakTable* pLeakTable, HKEY hKey);

NTSTATUS TrackObject(HKEY hKey);
BOOL     InitializeLeakTrackTable();
BOOL     CleanupLeakTrackTable();
NTSTATUS UnTrackObject(HKEY hKey);

extern RegLeakTable gLeakTable;


#endif // LEAK_TRACK
#endif // _REGLEAK_H_
#endif // LOCAL












