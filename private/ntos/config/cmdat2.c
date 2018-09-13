/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    cmdat2.c

Abstract:

    This module contains data strings that describes the registry space
    and that are exported to the rest of the system.

Author:

    Andre Vachon (andreva) 08-Apr-1992


Environment:

    Kernel mode.

Revision History:

--*/

#include "cmp.h"

//
// ***** PAGE *****
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

//
// control values/overrides read from registry
//
ULONG CmRegistrySizeLimit = { 0 };
ULONG CmRegistrySizeLimitLength = 4;
ULONG CmRegistrySizeLimitType = { 0 };

//
// Maximum number of bytes of Global Quota the registry may use.
// Set to largest positive number for use in boot.  Will be set down
// based on pool and explicit registry values.
//
ULONG   CmpGlobalQuotaAllowed = CM_WRAP_LIMIT;
ULONG   CmpGlobalQuota = CM_WRAP_LIMIT;
ULONG   CmpGlobalQuotaWarning = CM_WRAP_LIMIT;
BOOLEAN CmpQuotaWarningPopupDisplayed = FALSE;

//
// the "disk full" popup has already been displayed
//
BOOLEAN CmpDiskFullWorkerPopupDisplayed = FALSE;
BOOLEAN CmpCannotWriteConfiguration = FALSE;
//
// GQ actually in use
//
ULONG   CmpGlobalQuotaUsed = 0;

//
// State flag to remember when to turn it on
//
BOOLEAN CmpProfileLoaded = FALSE;

PUCHAR CmpStashBuffer = NULL;
ULONG  CmpStashBufferSize = 0;

//
// Shutdown control
//
BOOLEAN HvShutdownComplete = FALSE;     // Set to true after shutdown
                                        // to disable any further I/O

PCM_KEY_CONTROL_BLOCK CmpKeyControlBlockRoot = NULL;

struct {
    PHHIVE      Hive;
    ULONG       Status;
} CmCheckRegistryDebug = { 0 };

//
// The last I/O error status code
//
struct {
    ULONG       Action;
    HANDLE      Handle;
    NTSTATUS    Status;
} CmRegistryIODebug = { 0 };

//
// globals private to check code
//
PHHIVE  CmpCheckHive = { 0 };
BOOLEAN CmpCheckClean = { 0 };

struct {
    PHHIVE      Hive;
    ULONG       Status;
} CmpCheckRegistry2Debug = { 0 };

struct {
    PHHIVE      Hive;
    ULONG       Status;
    HCELL_INDEX Cell;
    PCELL_DATA  CellPoint;
    PVOID       RootPoint;
    ULONG       Index;
} CmpCheckKeyDebug = { 0 };

struct {
    PHHIVE      Hive;
    ULONG       Status;
    PCELL_DATA  List;
    ULONG       Index;
    HCELL_INDEX Cell;
    PCELL_DATA  CellPoint;
} CmpCheckValueListDebug = { 0 };

ULONG CmpUsedStorage = { 0 };

// hivechek.c
struct {
    PHHIVE      Hive;
    ULONG       Status;
    ULONG       Space;
    HCELL_INDEX MapPoint;
    PHBIN       BinPoint;
} HvCheckHiveDebug = { 0 };

struct {
    PHBIN       Bin;
    ULONG       Status;
    PHCELL      CellPoint;
} HvCheckBinDebug = { 0 };

#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg()
#endif
