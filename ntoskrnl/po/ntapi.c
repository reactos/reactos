/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power Manager NT API system calls
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

WORK_QUEUE_ITEM PopUnlockMemoryWorkItem;
KEVENT PopUnlockMemoryCompleteEvent;

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * A worker thread used to unlock memory. This is used for hibernation
 * purposes.
 */
_Use_decl_annotations_
VOID
NTAPI
PopUnlockMemoryWorker(
    _In_ PVOID Parameter)
{
    UNIMPLEMENTED;
    return;
}

/**
 * @brief
 * Determines whether the executing calling thread has
 * enough privileges to perform certain power set information
 * operations.
 *
 * @param[in] Level
 * The power information level of which the calling thread
 * is setting information into.
 *
 * @param[in] InputBuffer
 * A pointer to an input buffer.
 *
 * @param[in] PreviousMode
 * The requestor mode of the calling thread.
 *
 * @return
 * Returns TRUE if the caller has enough privileges or if
 * the calling thread in question is called from kernel mode.
 * Otherwise FALSE is returned.
 */
static
BOOLEAN
PopIsCallerPrivileged(
    _In_ POWER_INFORMATION_LEVEL Level,
    _In_ PVOID InputBuffer,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    LUID Privilege;

    PAGED_CODE();

    /* If the caller was coming from the kernel, he has absolute privileges */
    if (PreviousMode == KernelMode)
    {
        return TRUE;
    }

    /* Setting a new power state handler can only be done in KM */
    if ((Level == SystemPowerStateHandler) && (PreviousMode != KernelMode))
    {
        return FALSE;
    }

    /*
     * This is coming from UM of which we cannot trust it, and we might have an
     * input buffer. Ensure that the caller has the appropriate privilege for
     * whatever operation he wants to do.
     */
    if (InputBuffer)
    {
        /*
         * The calling thread is merely asking the Power Manager to do some
         * verifications by providing their version of a power policy from
         * the input buffer. This is a vetting process and information is not
         * set onto the critical Power Manager data so the following information
         * classes are not invasive.
         */
        if (Level == VerifySystemPolicyAc ||
            Level == VerifySystemPolicyDc ||
            Level == VerifyProcessorPowerPolicyAc ||
            Level == VerifyProcessorPowerPolicyDc)
        {
            return TRUE;
        }

        Privilege = (Level == SystemReserveHiberFile) ? SeCreatePagefilePrivilege : SeShutdownPrivilege;
        if (!SeSinglePrivilegeCheck(Privilege, PreviousMode))
        {
            return FALSE;
        }
    }

    /*
     * This is a query operation (understood by InputBuffer being NULL) or
     * the caller has the required privilege, allow him access.
     */
    return TRUE;
}

/* INFORMATION CLASSES ********************************************************/

static const INFORMATION_CLASS_INFO PoPowerInformationClass[] =
{
    /* SystemPowerPolicyAc */
    IQS_NONE,

    /* SystemPowerPolicyDc */
    IQS_NONE,

    /* VerifySystemPolicyAc */
    IQS_NONE,

    /* VerifySystemPolicyDc */
    IQS_NONE,

    /* SystemPowerCapabilities */
    IQS_SAME(SYSTEM_POWER_CAPABILITIES, ULONG, ICIF_QUERY),

    /* SystemBatteryState */
    IQS_SAME(SYSTEM_BATTERY_STATE, ULONG, ICIF_QUERY),

    /* SystemPowerStateHandler */
    IQS_SAME(POWER_STATE_HANDLER, ULONG, ICIF_SET),

    /* ProcessorStateHandler */
    IQS_NONE,

    /* SystemPowerPolicyCurrent */
    IQS_SAME(SYSTEM_POWER_POLICY, ULONG, ICIF_QUERY),

    /* AdministratorPowerPolicy */
    IQS_NONE,

    /* SystemReserveHiberFile */
    IQS_NONE,

    /* ProcessorInformation */
    IQS_SAME(PROCESSOR_POWER_INFORMATION, ULONG, ICIF_QUERY),

    /* SystemPowerInformation */
    IQS_NONE,

    /* ProcessorStateHandler2 */
    IQS_NONE,

    /* LastWakeTime */
    IQS_NONE,

    /* LastSleepTime */
    IQS_NONE,

    /* SystemExecutionState */
    IQS_NONE,

    /* SystemPowerStateNotifyHandler */
    IQS_NONE,

    /* ProcessorPowerPolicyAc */
    IQS_NONE,

    /* ProcessorPowerPolicyDc */
    IQS_NONE,

    /* VerifyProcessorPowerPolicyAc */
    IQS_NONE,

    /* VerifyProcessorPowerPolicyDc */
    IQS_NONE,

    /* ProcessorPowerPolicyCurrent */
    IQS_NONE,

    /* SystemPowerStateLogging */
    IQS_NONE,

    /* SystemPowerLoggingEntry */
    IQS_NONE,

    /* SetPowerSettingValue */
    IQS_NONE,

    /* NotifyUserPowerSetting */
    IQS_NONE,

    /* PowerInformationLevelUnused0 */
    IQS_NONE, // Not used

    /* SystemMonitorHiberBootPowerOff */
    IQS_NONE,

    /* SystemVideoState */
    IQS_NONE,

    /* TraceApplicationPowerMessage */
    IQS_NONE,

    /* TraceApplicationPowerMessageEnd */
    IQS_NONE,

    /* ProcessorPerfStates */
    IQS_NONE,

    /* ProcessorIdleStates */
    IQS_NONE,

    /* ProcessorCap */
    IQS_NONE,

    /* SystemWakeSource */
    IQS_NONE,

    /* SystemHiberFileInformation */
    IQS_NONE,

    /* TraceServicePowerMessage */
    IQS_NONE,

    /* ProcessorLoad */
    IQS_NONE,

    /* PowerShutdownNotification */
    IQS_NONE,

    /* MonitorCapabilities */
    IQS_NONE,

    /* SessionPowerInit */
    IQS_NONE,

    /* SessionDisplayState */
    IQS_NONE,

    /* PowerRequestCreate */
    IQS_NONE,

    /* PowerRequestAction */
    IQS_NONE,

    /* GetPowerRequestList */
    IQS_NONE,

    /* ProcessorInformationEx */
    IQS_NONE,

    /* NotifyUserModeLegacyPowerEvent */
    IQS_NONE,

    /* GroupPark */
    IQS_NONE,

    /* ProcessorIdleDomains */
    IQS_NONE,

    /* WakeTimerList */
    IQS_NONE,

    /* SystemHiberFileSize */
    IQS_NONE,

    /* ProcessorIdleStatesHv */
    IQS_NONE,

    /* ProcessorPerfStatesHv */
    IQS_NONE,

    /* ProcessorPerfCapHv */
    IQS_NONE,

    /* ProcessorSetIdle */
    IQS_NONE,

    /* LogicalProcessorIdling */
    IQS_NONE,

    /* UserPresence */
    IQS_NONE,

    /* PowerSettingNotificationName */
    IQS_NONE,

    /* GetPowerSettingValue */
    IQS_NONE,

    /* IdleResiliency */
    IQS_NONE,

    /* SessionRITState */
    IQS_NONE,

    /* SessionConnectNotification */
    IQS_NONE,

    /* SessionPowerCleanup */
    IQS_NONE,

    /* SessionLockState */
    IQS_NONE,

    /* SystemHiberbootState */
    IQS_NONE,

    /* PlatformInformation */
    IQS_SAME(POWER_PLATFORM_INFORMATION, ULONG, ICIF_QUERY),

    /* PdcInvocation */
    IQS_NONE,

    /* MonitorInvocation */
    IQS_NONE,

    /* FirmwareTableInformationRegistered */
    IQS_NONE,

    /* SetShutdownSelectedTime */
    IQS_NONE,

    /* SuspendResumeInvocation */
    IQS_NONE,

    /* PlmPowerRequestCreate */
    IQS_NONE,

    /* ScreenOff */
    IQS_NONE,

    /* CsDeviceNotification */
    IQS_NONE,

    /* PlatformRole */
    IQS_NONE,

    /* LastResumePerformance */
    IQS_NONE,

    /* DisplayBurst */
    IQS_NONE,

    /* ExitLatencySamplingPercentage */
    IQS_NONE,

    /* RegisterSpmPowerSettings */
    IQS_NONE,

    /* PlatformIdleStates */
    IQS_NONE,

    /* ProcessorIdleVeto */
    IQS_NONE, // Deprecated

    /* PlatformIdleVeto */
    IQS_NONE, // Deprecated

    /* SystemBatteryStatePrecise */
    IQS_NONE,

    /* ThermalEvent */
    IQS_NONE,

    /* PowerRequestActionInternal */
    IQS_NONE,

    /* BatteryDeviceState */
    IQS_NONE,

    /* PowerInformationInternal */
    IQS_NONE,

    /* ThermalStandby */
    IQS_NONE,

    /* SystemHiberFileType */
    IQS_NONE,

    /* PhysicalPowerButtonPress */
    IQS_NONE,

    /* QueryPotentialDripsConstraint */
    IQS_NONE,

    /* EnergyTrackerCreate */
    IQS_NONE,

    /* EnergyTrackerQuery */
    IQS_NONE,

    /* UpdateBlackBoxRecorder */
    IQS_NONE,

    /* SessionAllowExternalDmaDevices */
    IQS_NONE,
};

/* SYSTEM CALLS ***************************************************************/

/**
 * @brief
 * Instantiates a global power action across the whole
 * system and devices. Devices, applications and everything else
 * are notified of this action.
 *
 * @param[in] SystemAction
 * Specifies the system action the Power Manager must take.
 *
 * @param[in] MinSystemState
 * Specifies the minimum system power state the system must take.
 * The Power Manager cannot let the system incur into the lowest
 * system state than what this parameter permits.
 *
 * @param[in] Flags
 * A bitmask flag passed by the caller that changes the behavior of
 * how this function must work. The following flags are:
 *
 * POWER_ACTION_CRITICAL -- The power action to be instantiated is
 *                          extremely critical and must be processed above
 *                          other registered power actions.
 *
 * POWER_ACTION_DISABLE_WAKES -- Tells the Power Mananger that with this power
 *                               action all wake events such as those from HAL
 *                               are disabled.
 *
 * POWER_ACTION_LIGHTEST_FIRST -- Tells this function to use the minimum system
 *                                sleep state provided by MinSystemState first.
 *                                The system action is thereby ignored.
 *
 * POWER_ACTION_LOCK_CONSOLE -- Tells the Power Manager that the user must input
 *                              a password upon resume of the system from a sleep
 *                              state due to this power action.
 *
 * POWER_ACTION_UI_ALLOWED -- Notifies all the applications of an impeding power
 *                            action that is occurring and the user is prompted
 *                            for directions on how to prepare for a change in state
 *                            of system power state like suspension.
 *
 * @param[in] Asynchronous
 * If set to TRUE, the power action operation is done asynchronously.
 * The Power Manager will deploy a DPC to handle the power action outside
 * of the current calling thread. The function will return immediately.
 * If set to FALSE, the following operation is synchronous and blocking,
 * as in, the calling thread will have to wait for the Power Manager until
 * it finishes processing the power action.
 */
NTSTATUS
NTAPI
NtInitiatePowerAction(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags,
    _In_ BOOLEAN Asynchronous)
{
    /* FIXME */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief
 * Queries information from the Power Manager or sets new information.
 *
 * @param[in] PowerInformationLevel
 * Specifies the power information level that is to be queried or set.
 *
 * @param[in] InputBuffer
 * A pointer to an input buffer of which information is used by the
 * Power Mananger to set a new information.
 *
 * @param[in] InputBufferLength
 * The length size of the buffer pointed by InputBuffer.
 *
 * @param[in] OutputBuffer
 * A pointer to an output buffer of which information is retrieved from
 * the Power Manager.
 *
 * @param[in] OutputBufferLength
 * The length size of the buffer pointed by OutputBuffer, this is so
 * the Power Manager understands how much of data is to be written into
 * the OutputBuffer parameter. Note that the buffer size can vary depending
 * on the power information level.
 *
 * @return
 * Returns STATUS_SUCCESS if the operation has succeeded. STATUS_INVALID_PARAMETER
 * is returned if at least one of the parameters is not valid. STATUS_BUFFER_TOO_SMALL
 * is returned if the buffer pointed by OutputBuffer is too small to hold the
 * information. A failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
NtPowerInformation(
    _In_ POWER_INFORMATION_LEVEL PowerInformationLevel,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    SYSTEM_BATTERY_STATE LocalBatteryState = {0};
    PSYSTEM_BATTERY_STATE BatteryState;
    POWER_PLATFORM_INFORMATION LocalPlatformInfo;
    PPOWER_PLATFORM_INFORMATION PlatformInfo;
    PSYSTEM_POWER_CAPABILITIES PowerCapabilities;
    PPOWER_STATE_HANDLER StateHandler;
    PSYSTEM_POWER_POLICY CurrentPolicy;
    PVOID LocalBuffer = NULL;

    PAGED_CODE();

#if DBG
    DPRINT("NtPowerInformation(PowerInformationLevel %S, InputBuffer 0x%p, "
           "InputBufferLength 0x%x, OutputBuffer 0x%p, OutputBufferLength 0x%x)\n",
           PopGetPowerInformationLevelName(PowerInformationLevel),
           InputBuffer, InputBufferLength,
           OutputBuffer, OutputBufferLength);
#endif

    /* Bail out if the caller is doing something special and has no required privilege */
    PreviousMode = ExGetPreviousMode();
    if (!PopIsCallerPrivileged(PowerInformationLevel, InputBuffer, PreviousMode))
    {
        DPRINT1("The caller has no required privilege for this operation, bail out\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Probe the parameters depending on whether the caller queries or sets something */
    if (InputBuffer)
    {
        /* It wants to set new power data, make sure the length was provided */
        ASSERT(InputBufferLength > 0);

        /* And probe the actual buffer data and class validity */
        Status = DefaultSetInfoBufferCheck(PowerInformationLevel,
                                           PoPowerInformationClass,
                                           RTL_NUMBER_OF(PoPowerInformationClass),
                                           InputBuffer,
                                           InputBufferLength,
                                           PreviousMode);
        if (NT_SUCCESS(Status))
        {
            if (PreviousMode == UserMode)
            {
                /*
                 * We are not done here. We must allocate a local buffer to hold the
                 * input data so that we are safe from anybody who frees InputBuffer.
                 */
                LocalBuffer = PopAllocatePool(InputBufferLength,
                                              TRUE,
                                              TAG_PO_INPUT_INFO_CLASS_BUFFER);
                if (LocalBuffer == NULL)
                {
                    DPRINT1("Buffer allocation for input data failed\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    /* Copy the power input data now */
                    RtlCopyMemory(LocalBuffer, InputBuffer, InputBufferLength);
                }
            }
            else
            {
                /*
                 * The caller is coming from the kernel. We do not need to
                 * allocate any buffer because we trust the kernel.
                 */
                LocalBuffer = InputBuffer;
            }
        }
    }
    else
    {
        /* The caller wants to query something from the Power Manager */
        ASSERT(OutputBufferLength > 0);

        /* Probe the parameter and class validity */
        Status = DefaultQueryInfoBufferCheck(PowerInformationLevel,
                                             PoPowerInformationClass,
                                             RTL_NUMBER_OF(PoPowerInformationClass),
                                             ICIF_PROBE_READ_WRITE,
                                             OutputBuffer,
                                             OutputBufferLength,
                                             NULL,
                                             NULL,
                                             PreviousMode);
    }

    /* Probing failed, bail out */
    if (!NT_SUCCESS(Status))
    {
#if DBG
        DPRINT1("Information verification class failed (Status -> 0x%lx, PowerInformationLevel -> %S)\n",
                Status, PopGetPowerInformationLevelName(PowerInformationLevel));
#endif
        return Status;
    }

    /* Do the Query/Set power operations with the policy lock held */
    PopAcquirePowerPolicyLock();

    switch (PowerInformationLevel)
    {
        case SystemPowerCapabilities:
        {
            PowerCapabilities = (PSYSTEM_POWER_CAPABILITIES)OutputBuffer;

            /* The caller provided an input buffer on a Query class, bail out */
            if (InputBuffer)
            {
                DPRINT1("InputBuffer provided on SystemPowerCapabilities class when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /* This is not the right output buffer length, bail out */
            if (OutputBufferLength < sizeof(SYSTEM_POWER_CAPABILITIES))
            {
                DPRINT1("OutputBufferLength too small (length %lu)\n", OutputBufferLength);
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            /* FIXME: We should filter the capabilities if the system has legacy stuff */
            _SEH2_TRY
            {
                RtlCopyMemory(PowerCapabilities,
                              &PopCapabilities,
                              sizeof(SYSTEM_POWER_CAPABILITIES));

                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case SystemBatteryState:
        {
            BatteryState = (PSYSTEM_BATTERY_STATE)OutputBuffer;

            /* The caller provided an input buffer on a Query class, bail out */
            if (InputBuffer)
            {
                DPRINT1("InputBuffer provided on SystemBatteryState class when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /* This is not the right output buffer length, bail out */
            if (OutputBufferLength < sizeof(SYSTEM_BATTERY_STATE))
            {
                DPRINT1("OutputBufferLength too small (length %lu)\n", OutputBufferLength);
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            /* Query the current state of the composite battery */
            PopQueryBatteryState(&LocalBatteryState);

            /* Copy the current state of the composite battery to the caller */
            _SEH2_TRY
            {
                RtlCopyMemory(BatteryState,
                              &LocalBatteryState,
                              sizeof(LocalBatteryState));

                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case PlatformInformation:
        {
            PlatformInfo = (PPOWER_PLATFORM_INFORMATION)OutputBuffer;

            /* The caller provided an input buffer on a Query class, bail out */
            if (InputBuffer)
            {
                DPRINT1("InputBuffer provided on PlatformInformation class when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /* This is not the right output buffer length, bail out */
            if (OutputBufferLength < sizeof(POWER_PLATFORM_INFORMATION))
            {
                DPRINT1("OutputBufferLength too small (length %lu)\n", OutputBufferLength);
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            /* Determine the AoAc capability from the global Power Manager variable */
            LocalPlatformInfo.AoAc = PopAoAcPresent;

            _SEH2_TRY
            {
                RtlCopyMemory(PlatformInfo,
                              &LocalPlatformInfo,
                              sizeof(POWER_PLATFORM_INFORMATION));

                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case SystemPowerStateHandler:
        {
            /* The caller asked for an output buffer on a Set class, bail out */
            if (OutputBuffer)
            {
                DPRINT1("OutputBuffer provided on SystemPowerStateHandler class when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /* This is not the right input buffer length, bail out */
            if (InputBufferLength < sizeof(POWER_STATE_HANDLER))
            {
                DPRINT1("InputBufferLength too small (length %lu)\n", InputBufferLength);
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            /* No input buffer was provided on a Set class, bail out */
            if (!LocalBuffer)
            {
                DPRINT1("No InputBuffer was provided for the SystemPowerStateHandler class\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /*
             * Setting system power state handlers is ACPI turf, the system must be
             * already supporting ACPI when we're being given state handlers. APM
             * and non-ACPI systems cannot use this class so assert this condition.
             */
            ASSERT(PopAcpiPresent == TRUE);

            /* Cache the provided newer state handler */
            StateHandler = (PPOWER_STATE_HANDLER)LocalBuffer;

            /*
             * HALs can only register newer power state handlers only once and
             * not more. Check that we already have a state handler registered
             * of the specified type. We allow HALs to replace our default
             * power off handler with theirs.
             */
            if (StateHandler->Type == PowerStateShutdownOff)
            {
                if (PopDefaultPowerStateHandlers[StateHandler->Type].Handler != PopShutdownHandler)
                {
                    DPRINT1("The HAL already replaced the default power Off state handler, cannot replace it twice\n");
                    Status = STATUS_INVALID_PARAMETER;
                    goto Quit;
                }
            }
            else
            {
                if (PopDefaultPowerStateHandlers[StateHandler->Type].Handler != NULL)
                {
                    DPRINT1("Cannot set new state handler twice for type (%ld)\n", StateHandler->Type);
                    Status = STATUS_INVALID_PARAMETER;
                    goto Quit;
                }
            }

            /* Register a new state handler with the Power Manager */
            Status = PopRegisterSystemStateHandler(StateHandler->Type,
                                                   StateHandler->RtcWake,
                                                   StateHandler->Handler,
                                                   StateHandler->Context);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to register a state handler for type %ld (Status 0x%lx)\n", StateHandler->Type, Status);
                goto Quit;
            }

            /*
             * Registration done successfully. Now we must activate the following
             * system state capability based on the state handler the HAL has given to us.
             */
            PopChangeSystemSystemStateCapability(StateHandler, TRUE);
            break;
        }

        case SystemPowerPolicyCurrent:
        {
            CurrentPolicy = (PSYSTEM_POWER_POLICY)OutputBuffer;

            /* The caller provided an input buffer on a Query class, bail out */
            if (InputBuffer)
            {
                DPRINT1("InputBuffer provided on SystemPowerPolicyCurrent class when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /* This is not the right output buffer length, bail out */
            if (OutputBufferLength < sizeof(SYSTEM_POWER_POLICY))
            {
                DPRINT1("OutputBufferLength too small (length %lu)\n", OutputBufferLength);
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            /* Copy the currently enacted system policy to the caller */
            _SEH2_TRY
            {
                RtlCopyMemory(CurrentPolicy,
                              PopDefaultPowerPolicy,
                              sizeof(SYSTEM_POWER_CAPABILITIES));

                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        default:
        {
#if DBG
            DPRINT1("%S information class is UNIMPLEMENTED\n", PopGetPowerInformationLevelName(PowerInformationLevel));
#endif
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

Quit:
    PopReleasePowerPolicyLock();

    if (LocalBuffer && PreviousMode == UserMode)
    {
        PopFreePool(LocalBuffer, TAG_PO_INPUT_INFO_CLASS_BUFFER);
    }

    return Status;
}

/**
 * @brief
 * Retrieves the power state of a device.
 *
 * @param[in] Device
 * A handle to a device.
 *
 * @param[out] PowerState
 * A pointer to a power state of the device in question,
 * returned to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS if the operation has succeeded.
 * A failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
NtGetDevicePowerState(
    _In_ HANDLE Device,
    _Out_ PDEVICE_POWER_STATE PowerState)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    KPROCESSOR_MODE PreviousMode;
    PDEVICE_OBJECT DeviceObject;
    DEVICE_POWER_STATE DeviceState;
    PEXTENDED_DEVOBJ_EXTENSION DevObjExts;

    PAGED_CODE();

    /* The caller must supply a variable to the output argument */
    ASSERT(PowerState);

    /* Ensure that we can return the device power state to the caller */
    PreviousMode = ExGetPreviousMode();
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteUlong(PowerState);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Reference the device so that we can get its device extensions */
    Status = ObReferenceObjectByHandle(Device,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference the device (Status 0x%lx)\n", Status);
        return Status;
    }

    /*
     * Get the device object and put a reference on it so that it does not
     * die in our arms. We no longer care about the file object.
     */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    ObReferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);

    /* Get the extensions and retrieve the power state of this device */
    DevObjExts = IoGetDevObjExtension(DeviceObject);
    DeviceState = PopGetDoePowerState(DevObjExts, FALSE);

    /* Return the current power state to the caller safely */
    _SEH2_TRY
    {
        *PowerState = DeviceState;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ObDereferenceObject(DeviceObject);
    return Status;
}

/**
 * @brief
 * Determines whether the system was resumed automatically
 * without from a human prompt (like a wake up timeout signal)
 * or not.
 *
 * @return
 * Returns TRUE if the system was resumed automatically without
 * human input, FALSE otherwise.
 */
BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(VOID)
{
    /* Tell the caller whether the system resumed automatically from the Power Manager */
    return PopResumeAutomatic;
}

/**
 * @brief
 * Tells the Power Manager to respect a certain wakeup latency
 * offset when the system wakes up. This makes the Power Manager
 * to choose a low power state that doesn't take much time for
 * the system to wake up that could exceed the said offset limit.
 *
 * @param[in] Latency
 * The wakeup latency time requirement. The following values are:
 *
 * LT_LOWEST_LATENCY -- The lowest latency possible. This means
 *                      the Power Manager will choose a state of
 *                      PowerSystemSleeping1 (aka S1) which is
 *                      sleep-to-RAM and the system is slightly
 *                      powered down.
 *
 * LT_DONT_CARE -- Any latency is permitted, the Power Manager will
 *                 take whatever sleep state when a power action
 *                 is to be taken.
 *
 * @remarks
 * This function is obsolete and meant to exist only for compatibility
 * purposes.
 */
NTSTATUS
NTAPI
NtRequestWakeupLatency(
    _In_ LATENCY_TIME Latency)
{
    /* On Windows Vista and later versions of Windows, this function is a NOP */
    UNREFERENCED_PARAMETER(Latency);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Makes the system busy due to certain activity the current
 * calling thread is currently doing.
 *
 * @param[in] esFlags
 * A bitmask flag provided by the caller. See PoRegisterSystemState for
 * further information about the flags.
 *
 * @param[out] PreviousFlags
 * A pointer to the previous execution state flags of the calling
 * thread, returned to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS if the operation has succeeded. A failure
 * NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
NtSetThreadExecutionState(
    _In_ EXECUTION_STATE esFlags,
    _Out_ EXECUTION_STATE *PreviousFlags)
{
    NTSTATUS Status;
    PETHREAD Thread;
    EXECUTION_STATE PreviousState;
    PPOP_POWER_REQUEST PowerRequest;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    /* The caller passed ES_USER_PRESENT when it's not even supported (see MSDN documentation) */
    if (esFlags & ~(ES_CONTINUOUS | ES_USER_PRESENT))
    {
        DPRINT1("ES_USER_PRESENT is not supported when setting a new thread execution state\n");
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * Probe the output parameter so that we are safe that we can return
     * the previous execution state of the calling thread.
     */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteUlong(PreviousFlags);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Cache the current calling thread and check if it has a power request */
    Thread = PsGetCurrentThread();
    if (Thread->LegacyPowerObject == NULL)
    {
        /*
         * This thread never had a power request object so that means this is
         * the first time it actually sets an execution power state. Create a
         * power request but the legacy one, as we are dealing with legacy
         * execution state flags here.
         */
        Status = PopRegisterPowerRequest(NULL,
                                         RegisterLegacyRequest,
                                         TRUE,
                                         esFlags,
                                         NULL,
                                         &PowerRequest);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to create a legacy power request object for thread 0x%p (Status 0x%lx)\n", Thread, Status);
            return Status;
        }

        /*
         * As this is the first time this thread has a power request, setup
         * the state flags to an initial state, typically 0.
         */
        Thread->LegacyPowerObject = (PVOID)Thread->LegacyPowerObject;
        PreviousState = 0UL | ES_CONTINUOUS;
        goto Exit;
    }

    /* Cache the previous execution state flags before changing them with newer ones */
    PowerRequest = (PPOP_POWER_REQUEST)Thread->LegacyPowerObject;
    PreviousState = PowerRequest->LegacyStateFlags;

    /*
     * This thread already has a power request, the register helper will
     * take care of changing its execution state flags.
     */
    Status = PopRegisterPowerRequest(NULL,
                                     RegisterLegacyRequest,
                                     FALSE,
                                     esFlags,
                                     NULL,
                                     &PowerRequest);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to change execution state flags for thread 0x%p and power request 0x%p (Status 0x%lx)\n",
                Thread, Thread->LegacyPowerObject, Status);
        return Status;
    }

Exit:
    _SEH2_TRY
    {
        *PreviousFlags = PreviousState;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return Status;
}

/**
 * @brief
 * Sets a new power state to the system, thereby powering it
 * down.
 *
 * @param[in] SystemAction
 * Specifies the system action the Power Manager must take.
 *
 * @param[in] MinSystemState
 * Specifies the minimum system power state the system must take.
 * The Power Manager cannot let the system incur into the lowest
 * system state than what this parameter permits.
 *
 * @param[in] Flags
 * A bitmask flag passed by the caller that changes the behavior of
 * how this function must work. For further details, see NtInitiatePowerAction.
 *
 * @return
 * Returns STATUS_SYSTEM_SHUTDOWN to indicate the system is shutting
 * down soon. STATUS_PRIVILEGE_NOT_HELD is returned if the shutdown privilege
 * is not held by the current calling thread to invoke any change to the
 * system power state. STATUS_PRIVILEGE_NOT_HELD is returned if one
 * of the parameters is not valid. A failure NTSTATUS code is returned
 * otherwise.
 */
NTSTATUS
NTAPI
NtSetSystemPowerState(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags)
{
    NTSTATUS Status;
    ULONG FreedPagesCount;
    KPROCESSOR_MODE PreviousMode;
    POP_POWER_ACTION Action;

    /* Look for any invalid argument and bail out */
    if ((MinSystemState >= PowerSystemMaximum) ||
        (MinSystemState <= PowerSystemUnspecified) ||
        (SystemAction > PowerActionWarmEject) ||
        (SystemAction < PowerActionReserved) ||
        (Flags & ~(POWER_ACTION_QUERY_ALLOWED  |
                   POWER_ACTION_UI_ALLOWED     |
                   POWER_ACTION_OVERRIDE_APPS  |
                   POWER_ACTION_LIGHTEST_FIRST |
                   POWER_ACTION_LOCK_CONSOLE   |
                   POWER_ACTION_DISABLE_WAKES  |
                   POWER_ACTION_CRITICAL)))
    {
        DPRINT1("Invalid parameters found:\n");
        DPRINT1("   SystemAction: 0x%x\n", SystemAction);
        DPRINT1("   MinSystemState: 0x%x\n", MinSystemState);
        DPRINT1("   Flags: 0x%x\n", Flags);
        return STATUS_INVALID_PARAMETER;
    }

    /* Is this called from user mode? */
    PreviousMode = ExGetPreviousMode();
    if (PreviousMode != KernelMode)
    {
        /* Make sure that the caller has the required shutdown privilege */
        if (!SeSinglePrivilegeCheck(SeShutdownPrivilege, PreviousMode))
        {
            DPRINT1("Privilege not held for setting a system power state\n");
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        /* Turn this execution into kernel as this is an invasive operation */
        return ZwSetSystemPowerState(SystemAction, MinSystemState, Flags);
    }

    /* Disable lazy registry flushing */
    CmSetLazyFlushState(FALSE);

    /* Setup the power action */
    RtlZeroMemory(&Action, sizeof(Action));
    Action.Action = SystemAction;
    Action.Flags = Flags;

    /* Notify any registered callbacks of an impeding system state change */
    ExNotifyCallback(PowerStateCallback, (PVOID)PO_CB_SYSTEM_STATE_LOCK, NULL);

    /* Do not allow swaping worker threads at this operation */
    ExSwapinWorkerThreads(FALSE);

    /* Lock the entire power policy manager and make our action global */
    PopAcquirePowerPolicyLock();
    PopAction = Action;

    /* Process action requests */
    Status = STATUS_CANCELLED;
    while (TRUE)
    {
        /* No power action was inquired */
        if (Action.Action == PowerActionNone)
        {
            break;
        }

        /* Check if this action is of shutdown type */
        if (Status == STATUS_CANCELLED)
        {
            if ((PopAction.Action == PowerActionShutdown) ||
                (PopAction.Action == PowerActionShutdownReset) ||
                (PopAction.Action == PowerActionShutdownOff))
            {
                PopAction.Shutdown = TRUE;
            }

            Status = STATUS_SUCCESS;
        }

        /* Stop processin action requests if we are at an invalid status */
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        /* Flush all volumes and the registry */
        PopFlushVolumes(PopAction.Shutdown);

        /* Flush dirty cache pages */
        CcRosFlushDirtyPages(MAXULONG, &FreedPagesCount, FALSE, FALSE);

        /* Now execute the graceful shutdown */
        PopAction.IrpMinor = IRP_MN_SET_POWER;
        if (PopAction.Shutdown)
        {
            /* If we are not running in the system context then queue the shutdown worker */
            if (PsGetCurrentProcess() != PsInitialSystemProcess)
            {
                ExInitializeWorkItem(&PopShutdownWorkItem,
                                     &PopGracefulShutdown,
                                     NULL);
                ExQueueWorkItem(&PopShutdownWorkItem, CriticalWorkQueue);

                KeSuspendThread(KeGetCurrentThread());
                Status = STATUS_SYSTEM_SHUTDOWN;
                goto Exit;
            }
            else
            {
                /* We are running within the system context, invoke shutdown directly */
                PopGracefulShutdown(NULL);
            }
        }

        /* There is A LOOOOOOOT OF STUFF TO IMPLEMENT HERE, consider it a stub at the moment */
        DPRINT1("System is still up and running, you may not have chosen a yet supported power option: %u\n", PopAction.Action);
        break;
    }

Exit:
    /* Release the policy manager lock and enable lazy registry flushing */
    PopReleasePowerPolicyLock();
    CmSetLazyFlushState(TRUE);
    return Status;
}

/**
 * @brief
 * Tells the Power Manager that a device requests the
 * system to wake up.
 *
 * @param[in] DeviceHandle
 * A handle to a device that is requesting a wakeup.
 */
NTSTATUS
NTAPI
NtRequestDeviceWakeup(
    _In_ HANDLE DeviceHandle)
{
    /* The following function is not implemented in Windows */
    UNIMPLEMENTED;
    UNREFERENCED_PARAMETER(DeviceHandle);
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief
 * Cancels a wakeup request previously inquired by a device
 * with a call to NtRequestDeviceWakeup.
 *
 * @param[in] DeviceHandle
 * A handle to a device that requested a wakeup of the system.
 */
NTSTATUS
NTAPI
NtCancelDeviceWakeupRequest(
    _In_ HANDLE DeviceHandle)
{
    /* The following function is not implemented in Windows */
    UNIMPLEMENTED;
    UNREFERENCED_PARAMETER(DeviceHandle);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
