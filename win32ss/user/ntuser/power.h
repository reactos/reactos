/*
 * PROJECT:         ReactOS Win32k subsystem
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Power management of the Win32 kernel-mode subsystem
 * COPYRIGHT:       Copyright 2024 George Bișoc <george.bisoc@reactos.org>
 */

#pragma once

//
// Power Callout type enumeration
//
typedef enum tagPOWER_CALLOUT_TYPE
{
    POWER_CALLOUT_EVENT = 0,
    POWER_CALLOUT_STATE
} POWER_CALLOUT_TYPE;

//
// Power Callout structure
//
typedef struct tagWIN32POWERCALLOUT
{
    /* Links the power callout with the global queue list */
    LIST_ENTRY Link;

    /*
     * Set to POWER_CALLOUT_STATE if the upcoming power callout is a power state
     * (IntHandlePowerState does that), POWER_CALLOUT_EVENT otherwise
     */
    POWER_CALLOUT_TYPE Type;

    /* The captured Win32 power event parameters (the params are set to none if Type == POWER_STATE) */
    WIN32_POWEREVENT_PARAMETERS Params;
} WIN32POWERCALLOUT, *PWIN32POWERCALLOUT;

//
// Ensures the power callout mutex lock is acquired
//
#define ASSERT_POWER_CALLOUT_LOCK_ACQUIRED()    \
    ASSERT(gpPowerCalloutMutexOwnerThread == KeGetCurrentThread())

//
// Power Manager data
//
extern LIST_ENTRY gPowerCalloutsQueueList;
extern PFAST_MUTEX gpPowerCalloutMutexLock;
extern PKEVENT gpPowerRequestCalloutEvent;
extern PKTHREAD gpPowerCalloutMutexOwnerThread;

//
// Function prototypes
//
NTSTATUS
NTAPI
IntInitWin32PowerManagement(
    _In_ HANDLE hPowerRequestEvent);

NTSTATUS
NTAPI
IntWin32PowerManagementCleanup(VOID);

NTSTATUS
NTAPI
IntHandlePowerEvent(
    _In_ PWIN32_POWEREVENT_PARAMETERS pWin32PwrEventParams);

NTSTATUS
NTAPI
IntHandlePowerState(
    _In_ PWIN32_POWERSTATE_PARAMETERS pWin32PwrStateParams);

//
// Power Callout locking mechanism
//
FORCEINLINE
VOID
IntAcquirePowerCalloutLock(VOID)
{
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(gpPowerCalloutMutexLock);
    gpPowerCalloutMutexOwnerThread = KeGetCurrentThread();
}

FORCEINLINE
VOID
IntReleasePowerCalloutLock(VOID)
{
    ExReleaseFastMutexUnsafe(gpPowerCalloutMutexLock);
    gpPowerCalloutMutexOwnerThread = NULL;
    KeLeaveCriticalRegion();
}

//
// Checks if the following thread is a Win32 thread
//
FORCEINLINE
BOOL
IntIsThreadWin32Thread(
    _In_ PETHREAD Thread)
{
    return (Thread->Tcb.Win32Thread != NULL) ? TRUE : FALSE;
}

/* EOF */
