/*
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/misc/power.c
 * PURPOSE:         Power Management Functions
 * PROGRAMMER:      Dmitry Chapyshev <dmitry@reactos.org>
 *
 * UPDATE HISTORY:
 *                  01/15/2009 Created
 */

#include <k32.h>

#include <ndk/pofuncs.h>

#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
GetSystemPowerStatus(IN LPSYSTEM_POWER_STATUS PowerStatus)
{
    NTSTATUS Status;
    SYSTEM_BATTERY_STATE BattState;
    ULONG Max, Current;

    Status = NtPowerInformation(SystemBatteryState,
                                NULL,
                                0,
                                &BattState,
                                sizeof(SYSTEM_BATTERY_STATE));

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    RtlZeroMemory(PowerStatus, sizeof(SYSTEM_POWER_STATUS));

    PowerStatus->BatteryLifeTime = BATTERY_LIFE_UNKNOWN;
    PowerStatus->BatteryFullLifeTime = BATTERY_LIFE_UNKNOWN;
    PowerStatus->BatteryLifePercent = BATTERY_PERCENTAGE_UNKNOWN;
    PowerStatus->ACLineStatus = AC_LINE_ONLINE;

    Max = BattState.MaxCapacity;
    Current = BattState.RemainingCapacity;
    if (Max)
    {
        if (Current <= Max)
        {
            PowerStatus->BatteryLifePercent = (UCHAR)((100 * Current + Max / 2) / Max);
        }
        else
        {
            PowerStatus->BatteryLifePercent = 100;
        }

        if (PowerStatus->BatteryLifePercent <= 32) PowerStatus->BatteryFlag |= BATTERY_FLAG_LOW;
        if (PowerStatus->BatteryLifePercent >= 67) PowerStatus->BatteryFlag |= BATTERY_FLAG_HIGH;
    }

    if (!BattState.BatteryPresent) PowerStatus->BatteryFlag |= BATTERY_FLAG_NO_BATTERY;

    if (BattState.Charging) PowerStatus->BatteryFlag |= BATTERY_FLAG_CHARGING;

    if (!(BattState.AcOnLine) && (BattState.BatteryPresent)) PowerStatus->ACLineStatus = AC_LINE_OFFLINE;

    if (BattState.EstimatedTime) PowerStatus->BatteryLifeTime = BattState.EstimatedTime;

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetSystemPowerState(IN BOOL fSuspend,
                    IN BOOL fForce)
{
    NTSTATUS Status;

    Status = NtInitiatePowerAction((fSuspend != FALSE) ? PowerActionSleep     : PowerActionHibernate,
                                   (fSuspend != FALSE) ? PowerSystemSleeping1 : PowerSystemHibernate,
                                   fForce != TRUE,
                                   FALSE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDevicePowerState(IN HANDLE hDevice,
                    OUT BOOL *pfOn)
{
    DEVICE_POWER_STATE DevicePowerState;
    NTSTATUS Status;

    Status = NtGetDevicePowerState(hDevice, &DevicePowerState);
    if (NT_SUCCESS(Status))
    {
        *pfOn = (DevicePowerState == PowerDeviceUnspecified) ||
                (DevicePowerState == PowerDeviceD0);
        return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
RequestDeviceWakeup(IN HANDLE hDevice)
{
    NTSTATUS Status;

    Status = NtRequestDeviceWakeup(hDevice);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
RequestWakeupLatency(IN LATENCY_TIME latency)
{
    NTSTATUS Status;

    Status = NtRequestWakeupLatency(latency);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CancelDeviceWakeupRequest(IN HANDLE hDevice)
{
    NTSTATUS Status;

    Status = NtCancelDeviceWakeupRequest(hDevice);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsSystemResumeAutomatic(VOID)
{
    return (BOOL)NtIsSystemResumeAutomatic();
}

/*
 * @implemented
 */
BOOL
WINAPI
SetMessageWaitingIndicator(IN HANDLE hMsgIndicator,
                           IN ULONG ulMsgCount)
{
    /* This is the correct Windows implementation */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
EXECUTION_STATE
WINAPI
SetThreadExecutionState(EXECUTION_STATE esFlags)
{
    NTSTATUS Status;

    Status = NtSetThreadExecutionState(esFlags, &esFlags);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return esFlags;
}
