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
RTL_TIME_ZONE_INFORMATION ExpTimeZoneInfo;
ULONG ExpLastTimeZoneBias = -1;
LARGE_INTEGER ExpTimeZoneBias;
ULONG ExpAltTimeZoneBias;
ULONG ExpTimeZoneId;
ULONG ExpTickCountMultiplier;
ERESOURCE ExpTimeRefreshLock;
ULONG ExpKernelResolutionCount = 0;
ULONG ExpTimerResolutionCount = 0;

/* FUNCTIONS ****************************************************************/

/*++
 * @name ExAcquireTimeRefreshLock
 *
 *     The ExReleaseTimeRefreshLock routine acquires the system-wide lock used
 *     to synchronize clock interrupt frequency changes.
 *
 * @param Wait
 *        If TRUE, the system will block the caller thread waiting for the lock
 *        to become available. If FALSE, the routine will fail if the lock has
 *        already been acquired.
 *
 * @return Boolean value indicating success or failure of the lock acquisition.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
NTAPI
ExAcquireTimeRefreshLock(IN BOOLEAN Wait)
{
    /* Block APCs */
    KeEnterCriticalRegion();

    /* Attempt lock acquisition */
    if (!(ExAcquireResourceExclusiveLite(&ExpTimeRefreshLock, Wait)))
    {
        /* Lock was not acquired, enable APCs and fail */
        KeLeaveCriticalRegion();
        return FALSE;
    }

    /* Lock has been acquired */
    return TRUE;
}

/*++
 * @name ExReleaseTimeRefreshLock
 *
 *     The ExReleaseTimeRefreshLock routine releases the system-wide lock used
 *     to synchronize clock interrupt frequency changes.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
ExReleaseTimeRefreshLock(VOID)
{
    /* Release the lock and re-enable APCs */
    ExReleaseResourceLite(&ExpTimeRefreshLock);
    KeLeaveCriticalRegion();
}

/*++
 * @name ExSetTimerResolution
 * @exported
 *
 *     The KiInsertQueueApc routine modifies the frequency at which the system
 *     clock interrupts.
 *
 * @param DesiredTime
 *        Specifies the amount of time that should elapse between each timer
 *        interrupt, in 100-nanosecond units.
 *
 *        This parameter is ignored if SetResolution is FALSE.
 *
 * @param SetResolution
 *        If TRUE, the call is a request to set the clock interrupt frequency to
 *        the value specified by DesiredTime. If FALSE, the call is a request to
 *        restore the clock interrupt frequency to the system's default value.
 *
 * @return New timer resolution, in 100-nanosecond ticks.
 *
 * @remarks (1) The clock frequency is changed only if the DesiredTime value is
 *              less than the current setting.
 *
 *          (2) The routine just returns the current setting if the DesiredTime
 *              value is greater than what is currently set.
 *
 *          (3) If the DesiredTime value is less than the system clock can
 *              support, the routine uses the smallest resolution the system can
 *              support, and returns that value.
 *
 *          (4) If multiple drivers have attempted to change the clock interrupt
 *              frequency, the system will only restore the default frequency
 *              once ALL drivers have called the routine with SetResolution set
 *              to FALSE.
 *
 *          NB. This routine synchronizes with IRP_MJ_POWER requests through the
 *              TimeRefreshLock.
 *
 *--*/
ULONG
NTAPI
ExSetTimerResolution(IN ULONG DesiredTime,
                     IN BOOLEAN SetResolution)
{
    ULONG CurrentIncrement;

    /* Wait for clock interrupt frequency and power requests to synchronize */
    ExAcquireTimeRefreshLock(TRUE);

    /* Obey remark 2*/
    CurrentIncrement = KeTimeIncrement;

    /* Check the type of operation this is */
    if (SetResolution)
    {
        /*
         * If this is the first kernel change, bump the timer resolution change
         * count, then bump the kernel change count as well.
         *
         * These two variables are tracked differently since user-mode processes
         * can also change the timer resolution through the NtSetTimerResolution
         * system call. A per-process flag in the EPROCESS then stores the per-
         * process change state.
         *
         */
        if (!ExpKernelResolutionCount++) ExpTimerResolutionCount++;

        /* Obey remark 3 */
        if (DesiredTime < KeMinimumIncrement) DesiredTime = KeMinimumIncrement;

        /* Obey remark 1 */
        if (DesiredTime < KeTimeIncrement)
        {
            /* Force this thread on CPU zero, since we don't want it to drift */
            KeSetSystemAffinityThread(1);

            /* Now call the platform driver (HAL) to make the change */
            CurrentIncrement = HalSetTimeIncrement(DesiredTime);

            /* Put the thread back to its original affinity */
            KeRevertToUserAffinityThread();

            /* Finally, keep track of the new value in the kernel */
            KeTimeIncrement = CurrentIncrement;
        }
    }
    else
    {
        /* First, make sure that a driver has actually changed the resolution */
        if (ExpKernelResolutionCount)
        {
            /* Obey remark 4 */
            if (--ExpKernelResolutionCount)
            {
                /*
                 * All kernel drivers have requested the original frequency to
                 * be restored, but there might still be user processes with an
                 * ongoing clock interrupt frequency change, so make sure that
                 * this isn't the case.
                 */
                if (--ExpTimerResolutionCount)
                {
                    /* Force this thread on one CPU so that it doesn't drift */
                    KeSetSystemAffinityThread(1);

                    /* Call the HAL to restore the frequency to its default */
                    CurrentIncrement = HalSetTimeIncrement(KeMaximumIncrement);

                    /* Put the thread back to its original affinity */
                    KeRevertToUserAffinityThread();

                    /* Finally, keep track of the new value in the kernel */
                    KeTimeIncrement = CurrentIncrement;
                }
            }
        }
    }

    /* Release the clock interrupt frequency lock since changes are done */
    ExReleaseTimeRefreshLock();

    /* And return the current value -- which could reflect the new frequency */
    return CurrentIncrement;
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
    LARGE_INTEGER StandardTime;
    LARGE_INTEGER DaylightTime;
    LARGE_INTEGER CurrentTime;
    NTSTATUS Status;

    /* Read time zone information from the registry */
    Status = RtlQueryTimeZoneInformation(&ExpTimeZoneInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlQueryTimeZoneInformation() failed (Status 0x%08lx)\n", Status);
        return FALSE;
    }

    /* Get the default bias */
    ExpTimeZoneBias.QuadPart = (LONGLONG)ExpTimeZoneInfo.Bias * TICKSPERMINUTE;

    if (ExpTimeZoneInfo.StandardDate.Month != 0 &&
        ExpTimeZoneInfo.DaylightDate.Month != 0)
    {
        /* Get this years standard start time */
        if (!RtlCutoverTimeToSystemTime(&ExpTimeZoneInfo.StandardDate,
                                        &StandardTime,
                                        CurrentBootTime,
                                        TRUE))
        {
            DPRINT1("RtlCutoverTimeToSystemTime() for StandardDate failed!\n");
            return FALSE;
        }

        /* Get this years daylight start time */
        if (!RtlCutoverTimeToSystemTime(&ExpTimeZoneInfo.DaylightDate,
                                        &DaylightTime,
                                        CurrentBootTime,
                                        TRUE))
        {
            DPRINT1("RtlCutoverTimeToSystemTime() for DaylightDate failed!\n");
            return FALSE;
        }

        /* Determine the time zone id and update the time zone bias */
        if (DaylightTime.QuadPart < StandardTime.QuadPart)
        {
            if ((CurrentBootTime->QuadPart >= DaylightTime.QuadPart) &&
                (CurrentBootTime->QuadPart < StandardTime.QuadPart))
            {
                DPRINT("Daylight time!\n");
                ExpTimeZoneId = TIME_ZONE_ID_DAYLIGHT;
                ExpTimeZoneBias.QuadPart += (LONGLONG)ExpTimeZoneInfo.DaylightBias * TICKSPERMINUTE;
            }
            else
            {
                DPRINT("Standard time!\n");
                ExpTimeZoneId = TIME_ZONE_ID_STANDARD;
                ExpTimeZoneBias.QuadPart += (LONGLONG)ExpTimeZoneInfo.StandardBias * TICKSPERMINUTE;
            }
        }
        else
        {
            if ((CurrentBootTime->QuadPart >= StandardTime.QuadPart) &&
                (CurrentBootTime->QuadPart < DaylightTime.QuadPart))
            {
                DPRINT("Standard time!\n");
                ExpTimeZoneId = TIME_ZONE_ID_STANDARD;
                ExpTimeZoneBias.QuadPart += (LONGLONG)ExpTimeZoneInfo.StandardBias * TICKSPERMINUTE;
            }
            else
            {
                DPRINT("Daylight time!\n");
                ExpTimeZoneId = TIME_ZONE_ID_DAYLIGHT;
                ExpTimeZoneBias.QuadPart += (LONGLONG)ExpTimeZoneInfo.DaylightBias * TICKSPERMINUTE;
            }
        }
    }
    else
    {
        ExpTimeZoneId = TIME_ZONE_ID_UNKNOWN;
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
ExpSetTimeZoneInformation(PRTL_TIME_ZONE_INFORMATION TimeZoneInformation)
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
                  sizeof(RTL_TIME_ZONE_INFORMATION));

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
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Verify the time pointers */
            NewSystemTime = ProbeForReadLargeInteger(SystemTime);
            if(PreviousTime) ProbeForWriteLargeInteger(PreviousTime);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Reuse the pointer */
        NewSystemTime = *SystemTime;
    }

    /* Make sure we have permission to change the time */
    if (!SeSinglePrivilegeCheck(SeSystemtimePrivilege, PreviousMode))
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
    if (PreviousTime)
    {
        /* Enter SEH Block for return */
        _SEH2_TRY
        {
            /* Return the previous time */
            *PreviousTime = OldSystemTime;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
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
    PAGED_CODE();

    /* Check if we were called from user-mode */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Verify the time pointer */
            ProbeForWriteLargeInteger(SystemTime);

            /*
             * It's safe to pass the pointer directly to KeQuerySystemTime
             * as it's just a basic copy to this pointer. If it raises an
             * exception nothing dangerous can happen!
             */
            KeQuerySystemTime(SystemTime);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Query the time directly */
        KeQuerySystemTime(SystemTime);
    }

    /* Return success */
    return STATUS_SUCCESS;
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
 * @implemented
 */
VOID
NTAPI
ExSystemTimeToLocalTime(PLARGE_INTEGER SystemTime,
                        PLARGE_INTEGER LocalTime)
{
    LocalTime->QuadPart = SystemTime->QuadPart - ExpTimeZoneBias.QuadPart;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryTimerResolution(OUT PULONG MinimumResolution,
                       OUT PULONG MaximumResolution,
                       OUT PULONG ActualResolution)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    /* Check if the call came from user-mode */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the parameters */
            ProbeForWriteUlong(MinimumResolution);
            ProbeForWriteUlong(MaximumResolution);
            ProbeForWriteUlong(ActualResolution);

            /*
             * Set the parameters to the actual values.
             *
             * NOTE:
             * MinimumResolution corresponds to the biggest time increment and
             * MaximumResolution corresponds to the smallest time increment.
             */
            *MinimumResolution = KeMaximumIncrement;
            *MaximumResolution = KeMinimumIncrement;
            *ActualResolution  = KeTimeIncrement;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Set the parameters to the actual values */
        *MinimumResolution = KeMaximumIncrement;
        *MaximumResolution = KeMinimumIncrement;
        *ActualResolution  = KeTimeIncrement;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetTimerResolution(IN ULONG DesiredResolution,
                     IN BOOLEAN SetResolution,
                     OUT PULONG CurrentResolution)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PEPROCESS Process = PsGetCurrentProcess();
    ULONG NewResolution;

    /* Check if the call came from user-mode */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the parameter */
            ProbeForWriteUlong(CurrentResolution);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Set and return the new resolution */
    NewResolution = ExSetTimerResolution(DesiredResolution, SetResolution);

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            *CurrentResolution = NewResolution;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        *CurrentResolution = NewResolution;
    }

    if (SetResolution || Process->SetTimerResolution)
    {
        /* The resolution has been changed now or in an earlier call */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* The resolution hasn't been changed */
        Status = STATUS_TIMER_RESOLUTION_NOT_SET;
    }

    /* Update the flag */
    Process->SetTimerResolution = SetResolution;

    return Status;
}

/* EOF */
