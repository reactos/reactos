/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/time.c
 * PURPOSE:         Time and Timezone Management
 * PROGRAMMERS:     Eric Kohl
 *                  Thomas Weidenmueller
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define TICKSPERMINUTE  600000000

/* GLOBALS ******************************************************************/

/* Note: Bias[minutes] = UTC - local time */
TIME_ZONE_INFORMATION ExpTimeZoneInfo;
ULONG ExpLastTimeZoneBias = -1;
LARGE_INTEGER ExpTimeZoneBias;
ULONG ExpAltTimeZoneBias;
ULONG ExpTimeZoneId;
ULONG ExpTickCountMultiplier;
ERESOURCE ExpTimeRefreshLock;

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
ExAcquireTimeRefreshLock(BOOLEAN Wait)
{
    /* Simply acquire the Resource */
    KeEnterCriticalRegion();
    if (!(ExAcquireResourceExclusiveLite(&ExpTimeRefreshLock, Wait)))
    {
        /* We failed! */
        KeLeaveCriticalRegion();
        return FALSE;
    }

    /* Success */
    return TRUE;
}

VOID
NTAPI
ExReleaseTimeRefreshLock(VOID)
{
    /* Simply release the Resource */
    ExReleaseResourceLite(&ExpTimeRefreshLock);
    KeLeaveCriticalRegion();
}

VOID
NTAPI
ExUpdateSystemTimeFromCmos(IN BOOLEAN UpdateInterruptTime,
                           IN ULONG MaxSepInSeconds)
{
    /* FIXME: TODO */
    return;
}

BOOLEAN
NTAPI
ExRefreshTimeZoneInformation(IN PLARGE_INTEGER CurrentBootTime)
{
    LARGE_INTEGER CurrentTime;
    NTSTATUS Status;

    /* Read time zone information from the registry */
    Status = RtlQueryTimeZoneInformation(&ExpTimeZoneInfo);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, clear all data */
        RtlZeroMemory(&ExpTimeZoneInfo, sizeof(TIME_ZONE_INFORMATION));
        ExpTimeZoneBias.QuadPart = (LONGLONG)0;
        ExpTimeZoneId = TIME_ZONE_ID_UNKNOWN;
    }
    else
    {
        /* FIXME: Calculate transition dates */

        /* Set bias and ID */
        ExpTimeZoneBias.QuadPart = ((LONGLONG)(ExpTimeZoneInfo.Bias +
            ExpTimeZoneInfo.StandardBias)) *
            TICKSPERMINUTE;
        ExpTimeZoneId = TIME_ZONE_ID_STANDARD;
    }

    /* Change it for user-mode applications */
    SharedUserData->TimeZoneBias.High1Time = ExpTimeZoneBias.u.HighPart;
    SharedUserData->TimeZoneBias.High2Time = ExpTimeZoneBias.u.HighPart;
    SharedUserData->TimeZoneBias.LowPart = ExpTimeZoneBias.u.LowPart;
    SharedUserData->TimeZoneId = ExpTimeZoneId;

    /* Convert boot time from local time to UTC */
    KeBootTime.QuadPart += ExpTimeZoneBias.QuadPart;

    /* Convert system time from local time to UTC */
    do
    {
        CurrentTime.u.HighPart = SharedUserData->SystemTime.High1Time;
        CurrentTime.u.LowPart = SharedUserData->SystemTime.LowPart;
    } while (CurrentTime.u.HighPart != SharedUserData->SystemTime.High2Time);

    /* Change it for user-mode applications */
    CurrentTime.QuadPart += ExpTimeZoneBias.QuadPart;
    SharedUserData->SystemTime.LowPart = CurrentTime.u.LowPart;
    SharedUserData->SystemTime.High1Time = CurrentTime.u.HighPart;
    SharedUserData->SystemTime.High2Time = CurrentTime.u.HighPart;

    /* Return success */
    return TRUE;
}

NTSTATUS
ExpSetTimeZoneInformation(PTIME_ZONE_INFORMATION TimeZoneInformation)
{
    LARGE_INTEGER LocalTime, SystemTime, OldTime;
    TIME_FIELDS TimeFields;
    DPRINT("ExpSetTimeZoneInformation() called\n");

    DPRINT("Old time zone bias: %d minutes\n", ExpTimeZoneInfo.Bias);
    DPRINT("Old time zone standard bias: %d minutes\n",
            ExpTimeZoneInfo.StandardBias);
    DPRINT("New time zone bias: %d minutes\n", TimeZoneInformation->Bias);
    DPRINT("New time zone standard bias: %d minutes\n",
            TimeZoneInformation->StandardBias);

    /* Get the local time */
    HalQueryRealTimeClock(&TimeFields);
    RtlTimeFieldsToTime(&TimeFields, &LocalTime);

    /* FIXME: Calculate transition dates */

    /* Calculate the bias and set the ID */
    ExpTimeZoneBias.QuadPart = ((LONGLONG)(TimeZoneInformation->Bias +
                                           TimeZoneInformation->StandardBias)) *
                                           TICKSPERMINUTE;
    ExpTimeZoneId = TIME_ZONE_ID_STANDARD;

    /* Copy the timezone information */
    RtlCopyMemory(&ExpTimeZoneInfo,
                  TimeZoneInformation,
                  sizeof(TIME_ZONE_INFORMATION));

    /* Set the new time zone information */
    SharedUserData->TimeZoneBias.High1Time = ExpTimeZoneBias.u.HighPart;
    SharedUserData->TimeZoneBias.High2Time = ExpTimeZoneBias.u.HighPart;
    SharedUserData->TimeZoneBias.LowPart = ExpTimeZoneBias.u.LowPart;
    SharedUserData->TimeZoneId = ExpTimeZoneId;

    DPRINT("New time zone bias: %I64d minutes\n",
            ExpTimeZoneBias.QuadPart / TICKSPERMINUTE);

    /* Calculate the new system time */
    ExLocalTimeToSystemTime(&LocalTime, &SystemTime);

    /* Set the new system time */
    KeSetSystemTime(&SystemTime, &OldTime, FALSE, NULL);

    /* Return success */
    DPRINT("ExpSetTimeZoneInformation() done\n");
    return STATUS_SUCCESS;
}

/*
 * FUNCTION: Sets the system time.
 * PARAMETERS:
 *        NewTime - Points to a variable that specified the new time
 *        of day in the standard time format.
 *        OldTime - Optionally points to a variable that receives the
 *        old time of day in the standard time format.
 * RETURNS: Status
 */
NTSTATUS
NTAPI
NtSetSystemTime(IN PLARGE_INTEGER SystemTime,
                OUT PLARGE_INTEGER PreviousTime OPTIONAL)
{
    LARGE_INTEGER OldSystemTime;
    LARGE_INTEGER NewSystemTime;
    LARGE_INTEGER LocalTime;
    TIME_FIELDS TimeFields;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if we were called from user-mode */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* Verify the time pointers */
            NewSystemTime = ProbeForReadLargeInteger(SystemTime);
            if(PreviousTime) ProbeForWriteLargeInteger(PreviousTime);
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* If the pointers were invalid, bail out */
        if(!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Reuse the pointer */
        NewSystemTime = *SystemTime;
    }

    /* Make sure we have permission to change the time */
    if(!SeSinglePrivilegeCheck(SeSystemtimePrivilege, PreviousMode))
    {
        DPRINT1("NtSetSystemTime: Caller requires the "
                "SeSystemtimePrivilege privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Convert the time and set it in HAL */
    ExSystemTimeToLocalTime(&NewSystemTime, &LocalTime);
    RtlTimeToTimeFields(&LocalTime, &TimeFields);
    HalSetRealTimeClock(&TimeFields);

    /* Now set system time */
    KeSetSystemTime(&NewSystemTime, &OldSystemTime, FALSE, NULL);

    /* Check if caller wanted previous time */
    if(PreviousTime)
    {
        /* Enter SEH Block for return */
        _SEH_TRY
        {
            /* Return the previous time */
            *PreviousTime = OldSystemTime;
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Return status */
    return Status;
}

/*
 * FUNCTION: Retrieves the system time.
 * PARAMETERS:
 *          CurrentTime - Points to a variable that receives the current
 *          time of day in the standard time format.
 */
NTSTATUS
NTAPI
NtQuerySystemTime(OUT PLARGE_INTEGER SystemTime)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if we were called from user-mode */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* Verify the time pointer */
            ProbeForWriteLargeInteger(SystemTime);

            /*
             * It's safe to pass the pointer directly to KeQuerySystemTime as
             * it's just a basic copy to this pointer. If it raises an
             * exception nothing dangerous can happen!
             */
            KeQuerySystemTime(SystemTime);
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
        Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }
    else
    {
        /* Query the time directly */
        KeQuerySystemTime(SystemTime);
    }

    /* Return status to caller */
    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
ExLocalTimeToSystemTime(PLARGE_INTEGER LocalTime,
                        PLARGE_INTEGER SystemTime)
{
    SystemTime->QuadPart = LocalTime->QuadPart + ExpTimeZoneBias.QuadPart;
}

/*
 * @unimplemented
 */
ULONG
NTAPI
ExSetTimerResolution(IN ULONG DesiredTime,
                     IN BOOLEAN SetResolution)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
VOID
NTAPI
ExSystemTimeToLocalTime(PLARGE_INTEGER SystemTime,
                        PLARGE_INTEGER LocalTime)
{
    LocalTime->QuadPart = SystemTime->QuadPart - ExpTimeZoneBias.QuadPart;
}

/* EOF */
