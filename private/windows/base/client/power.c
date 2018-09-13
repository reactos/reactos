/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    power.c

Abstract:

    Stubs for unimplemented power management APIs

Author:

    Steve Wood (stevewo) 18-Nov-1994

Revision History:

--*/

#include "basedll.h"


BOOL
WINAPI
GetSystemPowerStatus(
    LPSYSTEM_POWER_STATUS lpStatus
    )
{
    SYSTEM_BATTERY_STATE    BatteryState;
    NTSTATUS                Status;

    //
    // Get power policy managers Battery State
    //

    Status = NtPowerInformation (
                SystemBatteryState,
                NULL,
                0,
                &BatteryState,
                sizeof (BatteryState)
                );

    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    //
    // Convert it to the legacy System Power State structure
    //

    RtlZeroMemory (lpStatus, sizeof(*lpStatus));

    lpStatus->ACLineStatus = AC_LINE_ONLINE;
    if (BatteryState.BatteryPresent && !BatteryState.AcOnLine) {
        lpStatus->ACLineStatus = AC_LINE_OFFLINE;
    }

    if (BatteryState.Charging) {
        lpStatus->BatteryFlag |= BATTERY_FLAG_CHARGING;
    }

    if (!BatteryState.BatteryPresent) {
        lpStatus->BatteryFlag |= BATTERY_FLAG_NO_BATTERY;
    }

    lpStatus->BatteryLifePercent = BATTERY_PERCENTAGE_UNKNOWN;
    if (BatteryState.MaxCapacity) {
        if (BatteryState.RemainingCapacity > BatteryState.MaxCapacity) {
            
            //
            // Values greater than 100% should not be returned 
            // According to the SDK they are reserved.
            //
            
            lpStatus->BatteryLifePercent = 100;
        } else {
            lpStatus->BatteryLifePercent = (UCHAR)
                (BatteryState.RemainingCapacity * 100 / BatteryState.MaxCapacity);
        }

        if (lpStatus->BatteryLifePercent > 66) {
            lpStatus->BatteryFlag |= BATTERY_FLAG_HIGH;
        }

        if (lpStatus->BatteryLifePercent < 33) {
            lpStatus->BatteryFlag |= BATTERY_FLAG_LOW;
        }
    }

    lpStatus->BatteryLifeTime = BATTERY_LIFE_UNKNOWN;
    lpStatus->BatteryFullLifeTime = BATTERY_LIFE_UNKNOWN;
    if (BatteryState.EstimatedTime) {
        lpStatus->BatteryLifeTime = BatteryState.EstimatedTime;
    }

    return TRUE;
}

BOOL
WINAPI
SetSystemPowerState(
    BOOL fSuspend,
    BOOL fForce
    )
{
    NTSTATUS        Status;

    Status = NtInitiatePowerAction (PowerActionSleep,
                                    fSuspend ? PowerSystemSleeping1 : PowerSystemHibernate,
                                    fForce == TRUE ? 0 : POWER_ACTION_QUERY_ALLOWED,
                                    FALSE);

    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


EXECUTION_STATE
WINAPI
SetThreadExecutionState(
    EXECUTION_STATE esFlags
    )
{
    NTSTATUS            Status;
    EXECUTION_STATE     PreviousFlags;

    Status = NtSetThreadExecutionState (esFlags, &PreviousFlags);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return (EXECUTION_STATE) 0;
    }

    return PreviousFlags;
}

BOOL
WINAPI
RequestWakeupLatency (
    LATENCY_TIME    latency
    )
{
    NTSTATUS        Status;

    Status = NtRequestWakeupLatency (latency);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
WINAPI
GetDevicePowerState(
    HANDLE  h,
    OUT BOOL *pfOn
    )
{
    NTSTATUS Status;
    DEVICE_POWER_STATE PowerState;

    Status = NtGetDevicePowerState(h, &PowerState);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return(FALSE);
    }
    if ((PowerState == PowerDeviceD0) ||
        (PowerState == PowerDeviceUnspecified)) {
        *pfOn = TRUE;
    } else {
        *pfOn = FALSE;
    }
    return TRUE;
}

BOOL
WINAPI
IsSystemResumeAutomatic(
    VOID
    )
{
    return(NtIsSystemResumeAutomatic());
}

BOOL
WINAPI
RequestDeviceWakeup (
    HANDLE  h
    )
{
    NTSTATUS Status;

    Status = NtRequestDeviceWakeup(h);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    } else {
        return(TRUE);
    }
}


BOOL
WINAPI
CancelDeviceWakeupRequest(
    HANDLE  h
    )
{
    NTSTATUS Status;

    Status = NtCancelDeviceWakeupRequest(h);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    } else {
        return(TRUE);
    }
}



BOOL
WINAPI
SetMessageWaitingIndicator (
    IN HANDLE hMsgIndicator,
    IN ULONG ulMsgCount
    )
{
    BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
    return FALSE;
}

