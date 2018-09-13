/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    exp.h

Abstract:

    This module contains the private (internal) header file for the
    executive.

Author:

    David N. Cutler (davec) 23-May-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#ifndef _EXP_
#define _EXP_

#include "ntos.h"
#include "zwapi.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include "ki.h"
#include "stdio.h"
#include "pool.h"

//
// Executive information initialization structure
//

typedef struct {
    PCALLBACK_OBJECT    *CallBackObject;
    PWSTR               CallbackName;
} EXP_INITIALIZE_GLOBAL_CALLBACKS;

typedef struct _EXP_LICENSE_INFO {
    HANDLE           RegKey;
    ULONG            Count;
    PWSTR            SubKeyName;
    WORK_QUEUE_ITEM  ExpWatchLicenseInfoWorkItem;
    IO_STATUS_BLOCK  ExpLicenseInfoIoSb;
    ULONG            ExpLicenseInfoChangeBuffer;
} EXP_LICENSE_INFO, *PEXP_LICENSE_INFO;


//
// Executive object and other initialization function definitions.
//

BOOLEAN
ExpWorkerInitialization (
    VOID
    );

BOOLEAN
ExpEventInitialization (
    VOID
    );

BOOLEAN
ExpEventPairInitialization (
    VOID
    );

BOOLEAN
ExpMutantInitialization (
    VOID
    );

BOOLEAN
ExpSemaphoreInitialization (
    VOID
    );

BOOLEAN
ExpTimerInitialization (
    VOID
    );

BOOLEAN
ExpWin32Initialization (
    VOID
    );

BOOLEAN
ExpResourceInitialization (
    VOID
    );

PVOID
ExpCheckForResource(
    IN PVOID p,
    IN ULONG Size
    );

BOOLEAN
ExpInitSystemPhase0 (
    VOID
    );

BOOLEAN
ExpInitSystemPhase1 (
    VOID
    );

BOOLEAN
ExpProfileInitialization (
    );

BOOLEAN
ExpUuidInitialization (
    );

VOID
ExpProfileDelete (
    IN PVOID Object
    );


BOOLEAN
ExpInitializeCallbacks (
    VOID
    );

BOOLEAN
ExpSysEventInitialization(
    VOID
    );

VOID
ExpCheckSystemInfoWork (
    IN PVOID Context
    );

VOID
ExInitSystemPhase2 (
    VOID
    );

VOID
ExpCheckSystemInformation (
    PVOID       Context,
    PVOID       InformationClass,
    PVOID       Argument2
    );


VOID
ExpTimeZoneWork(
    IN PVOID Context
    );

VOID
ExpTimeRefreshDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
ExpTimeRefreshWork(
    IN PVOID Context
    );

VOID
ExpTimeZoneDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
ExInitializeTimeRefresh(
    VOID
    );

VOID
ExpExpirationThread(
    IN PVOID StartContext
    );

ULONG
ExpComputeTickCountMultiplier(
    IN ULONG TimeIncrement
    );

BOOLEAN
static
ExpWatchProductTypeInitialization(
    VOID
    );

VOID
static
ExpWatchProductTypeWork(
    IN PVOID Context
    );

VOID
static
ExpWatchLicenseInfoWork(
    IN PVOID Context
    );

VOID
static
ExpWatchSystemPrefixWork(
    IN PVOID Context
    );

VOID
static
ExpWatchExpirationDataWork(
    IN PVOID Context
    );

PVOID
ExpCheckForWorker(
    IN PVOID p,
    IN ULONG Size
    );

ULONG ExpNtExpirationData[3];
BOOLEAN ExpSetupModeDetected;
LARGE_INTEGER ExpSetupSystemPrefix;
HANDLE ExpSetupKey;
BOOLEAN ExpSystemPrefixValid;


#ifdef _PNP_POWER_

extern WORK_QUEUE_ITEM  ExpCheckSystemInfoWorkItem;
extern LONG             ExpCheckSystemInfoBusy;
extern KSPIN_LOCK       ExpCheckSystemInfoLock;
extern WCHAR            ExpWstrSystemInformation[];
extern WCHAR            ExpWstrSystemInformationValue[];

#endif // _PNP_POWER_

extern WCHAR            ExpWstrCallback[];
extern EXP_INITIALIZE_GLOBAL_CALLBACKS  ExpInitializeCallback[];


extern FAST_MUTEX       ExpEnvironmentLock;

extern NPAGED_LOOKASIDE_LIST ExpSmallPagedPoolLookasideLists[POOL_SMALL_LISTS];
extern NPAGED_LOOKASIDE_LIST ExpSmallNPagedPoolLookasideLists[POOL_SMALL_LISTS];

extern LIST_ENTRY ExNPagedLookasideListHead;
extern KSPIN_LOCK ExNPagedLookasideLock;
extern LIST_ENTRY ExPagedLookasideListHead;
extern KSPIN_LOCK ExPagedLookasideLock;
extern LIST_ENTRY ExPoolLookasideListHead;

#endif // _EXP_
