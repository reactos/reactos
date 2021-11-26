/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI OS Loader
 * FILE:            boot/environ/app/rosload/rosload.c
 * PURPOSE:         OS Loader Entrypoint
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "rosload.h"

NTSTATUS
OslArchTransferToKernel (
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ PVOID KernelEntrypoint
    );

/* DATA VARIABLES ************************************************************/

PLOADER_PARAMETER_BLOCK OslLoaderBlock;
PVOID OslEntryPoint;
PVOID UserSharedAddress;
ULONGLONG ArchXCr0BitsToClear;
ULONGLONG ArchCr4BitsToClear;
BOOLEAN BdDebugAfterExitBootServices;
KDESCRIPTOR OslKernelGdt;
KDESCRIPTOR OslKernelIdt;

ULONG_PTR OslImcHiveHandle;
ULONG_PTR OslMachineHiveHandle;
ULONG_PTR OslElamHiveHandle;
ULONG_PTR OslSystemHiveHandle;

PBL_DEVICE_DESCRIPTOR OslLoadDevice;
PCHAR OslLoadOptions;
PWCHAR OslSystemRoot;

LIST_ENTRY OslFreeMemoryDesctiptorsList;
LIST_ENTRY OslFinalMemoryMap;
LIST_ENTRY OslCoreExtensionSubGroups[2];
LIST_ENTRY OslLoadedFirmwareDriverList;

BL_BUFFER_DESCRIPTOR OslFinalMemoryMapDescriptorsBuffer;

GUID OslApplicationIdentifier;

ULONG OslResetBootStatus;
BOOLEAN OslImcProcessingValid;
ULONG OslFreeMemoryDesctiptorsListSize;
PVOID OslMemoryDescriptorBuffer;

BcdObjectType BlpSbdiCurrentApplicationType;

PRTL_BSD_DATA BsdBootStatusData;

OSL_BSD_ITEM_TABLE_ENTRY OslpBootStatusFields[RtlBsdItemMax] =
{
    {
        FIELD_OFFSET(RTL_BSD_DATA, Version),
        sizeof(&BsdBootStatusData->Version)
    },  // RtlBsdItemVersionNumber
    {
        FIELD_OFFSET(RTL_BSD_DATA, ProductType),
        sizeof(&BsdBootStatusData->ProductType)
    },  // RtlBsdItemProductType
    {
        FIELD_OFFSET(RTL_BSD_DATA, AabEnabled),
        sizeof(&BsdBootStatusData->AabEnabled)
    },  // RtlBsdItemAabEnabled
    {
        FIELD_OFFSET(RTL_BSD_DATA, AabTimeout),
        sizeof(&BsdBootStatusData->AabTimeout)
    },  // RtlBsdItemAabTimeout
    {
        FIELD_OFFSET(RTL_BSD_DATA, LastBootSucceeded),
        sizeof(&BsdBootStatusData->LastBootSucceeded)
    },  // RtlBsdItemBootGood
    {
        FIELD_OFFSET(RTL_BSD_DATA, LastBootShutdown),
        sizeof(&BsdBootStatusData->LastBootShutdown)
    },  // RtlBsdItemBootShutdown
    {
        FIELD_OFFSET(RTL_BSD_DATA, SleepInProgress),
        sizeof(&BsdBootStatusData->SleepInProgress)
    },  // RtlBsdSleepInProgress
    {
        FIELD_OFFSET(RTL_BSD_DATA, PowerTransition),
        sizeof(&BsdBootStatusData->PowerTransition)
    },  // RtlBsdPowerTransition
    {
        FIELD_OFFSET(RTL_BSD_DATA, BootAttemptCount),
        sizeof(&BsdBootStatusData->BootAttemptCount)
    },  // RtlBsdItemBootAttemptCount
    {
        FIELD_OFFSET(RTL_BSD_DATA, LastBootCheckpoint),
        sizeof(&BsdBootStatusData->LastBootCheckpoint)
    },  // RtlBsdItemBootCheckpoint
    {
        FIELD_OFFSET(RTL_BSD_DATA, LastBootId),
        sizeof(&BsdBootStatusData->LastBootId)
    },  // RtlBsdItemBootId
    {
        FIELD_OFFSET(RTL_BSD_DATA, LastSuccessfulShutdownBootId),
        sizeof(&BsdBootStatusData->LastSuccessfulShutdownBootId)
    },  // RtlBsdItemShutdownBootId
    {
        FIELD_OFFSET(RTL_BSD_DATA, LastReportedAbnormalShutdownBootId),
        sizeof(&BsdBootStatusData->LastReportedAbnormalShutdownBootId)
    },  // RtlBsdItemReportedAbnormalShutdownBootId
    {
        FIELD_OFFSET(RTL_BSD_DATA, ErrorInfo),
        sizeof(&BsdBootStatusData->ErrorInfo)
    },  // RtlBsdItemErrorInfo
    {
        FIELD_OFFSET(RTL_BSD_DATA, PowerButtonPressInfo),
        sizeof(&BsdBootStatusData->PowerButtonPressInfo)
    },  // RtlBsdItemPowerButtonPressInfo
    {
        FIELD_OFFSET(RTL_BSD_DATA, Checksum),
        sizeof(&BsdBootStatusData->Checksum)
    },  // RtlBsdItemChecksum
};

ULONG OslBootAttemptCount;
ULONG OslBootCountUpdateRequestForAbort;
ULONG OslBootAttemptMaximum;

ULONG OslBootCountUpdateIncrement;

BOOLEAN OslCurrentBootCheckpoint;
BOOLEAN OslCurrentBootSucceeded;
BOOLEAN OslCurrentBootShutdown;

/* FUNCTIONS *****************************************************************/

VOID
OslFatalErrorEx (
    _In_ ULONG ErrorCode,
    _In_ ULONG Parameter1,
    _In_ ULONG_PTR Parameter2,
    _In_ ULONG_PTR Parameter3
    )
{
    /* For now just do this */
    BlStatusPrint(L"FATAL ERROR IN ROSLOAD: %lx\n", ErrorCode);
}

VOID
OslAbortBoot (
    _In_ NTSTATUS Status
    )
{
    /* For now just do this */
    BlStatusPrint(L"BOOT ABORTED: %lx\n", Status);
}

NTSTATUS
OslBlStatusErrorHandler (
    _In_ ULONG ErrorCode,
    _In_ ULONG Parameter1,
    _In_ ULONG_PTR Parameter2,
    _In_ ULONG_PTR Parameter3,
    _In_ ULONG_PTR Parameter4
    )
{
    /* We only filter error code 4 */
    if (ErrorCode != 4)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Handle error 4 as a fatal error 3 internally */
    OslFatalErrorEx(3, Parameter1, Parameter2, Parameter3);
    return STATUS_SUCCESS;
}

VOID
OslpSanitizeLoadOptionsString (
    _In_ PWCHAR OptionString,
    _In_ PWCHAR SanitizeString
    )
{
    /* TODO */
    return;
}

VOID
OslpSanitizeStringOptions (
    _In_ PBL_BCD_OPTION BcdOptions
    )
{
    /* TODO */
    return;
}

NTSTATUS
OslpRemoveInternalApplicationOptions (
    VOID
    )
{
    PWCHAR LoadString;
    NTSTATUS Status;

    /* Assume success */
    Status = STATUS_SUCCESS;

    /* Remove attempts to disable integrity checks or ELAM driver load */
    BlRemoveBootOption(BlpApplicationEntry.BcdData,
                       BcdLibraryBoolean_DisableIntegrityChecks);
    BlRemoveBootOption(BlpApplicationEntry.BcdData,
                       BcdOSLoaderBoolean_DisableElamDrivers);

    /* Get the command-line parameters, if any */
    Status = BlGetBootOptionString(BlpApplicationEntry.BcdData,
                                   BcdLibraryString_LoadOptionsString,
                                   &LoadString);
    if (NT_SUCCESS(Status))
    {
        /* Conver to upper case */
        _wcsupr(LoadString);

        /* Remove the existing one */
        BlRemoveBootOption(BlpApplicationEntry.BcdData,
                           BcdLibraryString_LoadOptionsString);

        /* Sanitize strings we don't want */
        OslpSanitizeLoadOptionsString(LoadString, L"DISABLE_INTEGRITY_CHECKS");
        OslpSanitizeLoadOptionsString(LoadString, L"NOINTEGRITYCHECKS");
        OslpSanitizeLoadOptionsString(LoadString, L"DISABLEELAMDRIVERS");

        /* Add the sanitized one back */
        Status = BlAppendBootOptionString(&BlpApplicationEntry,
                                          BcdLibraryString_LoadOptionsString,
                                          LoadString);

        /* Free the original BCD one */
        BlMmFreeHeap(LoadString);
    }

    /* One more pass for secure-boot options */
    OslpSanitizeStringOptions(BlpApplicationEntry.BcdData);

    /* All good */
    return Status;
}

NTSTATUS
OslpCheckForcedFailure (
    VOID
    )
{
    ULONG64 ForceReason;
    NTSTATUS Status;

    /* Read the option */
    Status = BlGetBootOptionInteger(BlpApplicationEntry.BcdData,
                                    BcdOSLoaderInteger_ForceFailure,
                                    &ForceReason);
    if (NT_SUCCESS(Status) && (ForceReason < 4))
    {
        /* For reasons above 3, don't actually do anything */
        if (ForceReason > 3)
        {
            return STATUS_SUCCESS;
        }
    }

    /* If the option isn't there or invalid, always return success */
    return STATUS_SUCCESS;
}

VOID
OslpInitializeBootStatusDataLog (
    VOID
    )
{
    /* TODO */
    return;
}

NTSTATUS
OslpReadWriteBootStatusData (
    _In_ BOOLEAN WriteAccess
    )
{
    /* Are you trying to write? */
    if (WriteAccess)
    {
        /* Have we already read? */
        if (!BsdBootStatusData)
        {
            /* No -- fail */
            return STATUS_UNSUCCESSFUL;
        }
    }
    else if (BsdBootStatusData)
    {
        /* No -- you're trying to read and we already have the data: no-op */
        return STATUS_SUCCESS;
    }

    /* TODO */
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
OslpGetSetBootStatusData (
    _In_ BOOLEAN Read,
    _In_ RTL_BSD_ITEM_TYPE DataClass,
    _Out_ PVOID Buffer,
    _Inout_ PULONG Size
    )
{
    NTSTATUS Status;
    ULONG Length, Offset;

    /* No data has been read yet, fail */
    if (!BsdBootStatusData)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Invalid data item, fail */
    if (DataClass >= RtlBsdItemMax)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Capture the length and offset */
    Length = OslpBootStatusFields[DataClass].Size;
    Offset = OslpBootStatusFields[DataClass].Offset;

    /* Make sure it doesn't overflow past the structure we've read */
    if ((Length + Offset) > BsdBootStatusData->Version)
    {
        return STATUS_REVISION_MISMATCH;
    }

    /* Make sure we have enough space */
    if (*Size >= Length)
    {
        /* We do -- is this a read? */
        if (Read)
        {
            /* Yes, copy into the caller's buffer */
            RtlCopyMemory(Buffer,
                          (PVOID)((ULONG_PTR)BsdBootStatusData + Offset),
                          Length);
        }
        else
        {
            /* It's a write, copy from caller's buffer */
            RtlCopyMemory((PVOID)((ULONG_PTR)BsdBootStatusData + Offset),
                          Buffer,
                          Length);
        }

        /* Set success */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Return size needed and failure code */
        *Size = Length;
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    /* All good */
    return Status;
}

NTSTATUS
OslSetBootStatusData (
    _In_ BOOLEAN LastBootGood,
    _In_ BOOLEAN LastBootShutdown,
    _In_ BOOLEAN LastBootCheckpoint,
    _In_ ULONG UpdateIncrement,
    _In_ ULONG BootAttemptCount
    )
{
    NTSTATUS Status;
    ULONG Size;

    /* Capture the BSD data in our globals, if needed */
    Status = OslpReadWriteBootStatusData(FALSE);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Write last boot shutdown */
    Size = sizeof(LastBootShutdown);
    Status = OslpGetSetBootStatusData(FALSE,
                                      RtlBsdItemBootShutdown,
                                      &LastBootShutdown,
                                      &Size);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Write last boot good */
    Size = sizeof(LastBootGood);
    Status = OslpGetSetBootStatusData(FALSE,
                                      RtlBsdItemBootGood,
                                      &LastBootGood,
                                      &Size);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Write last boot checkpoint */
    Size = sizeof(LastBootCheckpoint);
    Status = OslpGetSetBootStatusData(FALSE,
                                      RtlBsdItemBootCheckpoint,
                                      &LastBootCheckpoint,
                                      &Size);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Write boot attempt count */
    Size = sizeof(BootAttemptCount);
    Status = OslpGetSetBootStatusData(FALSE,
                                      RtlBsdItemBootAttemptCount,
                                      &BootAttemptCount,
                                      &Size);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* TODO: Update Boot ID*/

    /* Now write the data */
    Status = OslpReadWriteBootStatusData(TRUE);

Quickie:
    return Status;
}

NTSTATUS
OslGetBootStatusData (
    _Out_ PBOOLEAN LastBootGood,
    _Out_ PBOOLEAN LastBootShutdown,
    _Out_ PBOOLEAN LastBootCheckpoint,
    _Out_ PULONG LastBootId,
    _Out_ PBOOLEAN BootGood,
    _Out_ PBOOLEAN BootShutdown
    )
{
    NTSTATUS Status;
    ULONG Size;
    ULONG64 BootStatusPolicy;
    BOOLEAN localBootShutdown, localBootGood;

    /* Capture the BSD data in our globals, if needed */
    Status = OslpReadWriteBootStatusData(FALSE);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Read the last boot ID */
    Size = sizeof(*LastBootId);
    Status = OslpGetSetBootStatusData(TRUE, RtlBsdItemBootId, LastBootId, &Size);
    if (!NT_SUCCESS(Status))
    {
        /* Set to zero if we couldn't find it */
        *LastBootId = 0;
    }

    /* Get the boot status policy */
    Status = BlGetBootOptionInteger(BlpApplicationEntry.BcdData,
                                    BcdOSLoaderInteger_BootStatusPolicy,
                                    &BootStatusPolicy);
    if (!NT_SUCCESS(Status))
    {
        /* Apply a default if none exists */
        BootStatusPolicy = IgnoreShutdownFailures;
    }

    /* Check if this was a good shutdown */
    Size = sizeof(localBootShutdown);
    Status = OslpGetSetBootStatusData(TRUE,
                                      RtlBsdItemBootShutdown,
                                      &localBootShutdown,
                                      &Size);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Tell the caller */
    *BootShutdown = localBootShutdown;

    /* Check if this was a good boot */
    Size = sizeof(localBootGood);
    Status = OslpGetSetBootStatusData(TRUE,
                                      RtlBsdItemBootGood,
                                      &localBootGood,
                                      &Size);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Tell the caller*/
    *BootGood = localBootGood;

    /* TODO: Additional logic for checkpoints and such */
    Status = STATUS_NOT_IMPLEMENTED;

Quickie:
    return Status;
}

BOOLEAN
OslpAdvancedOptionsRequested (
    VOID
    )
{
    /* TODO */
    return FALSE;
}

NTSTATUS
OslPrepareTarget (
    _Out_ PULONG ReturnFlags,
    _Out_ PBOOLEAN Jump
    )
{
    PGUID AppId;
    NTSTATUS Status;
    PBL_DEVICE_DESCRIPTOR OsDevice;
    PWCHAR SystemRoot;
    SIZE_T RootLength, RootLengthWithSep;
    ULONG i;
    ULONG64 StartPerf, EndPerf;
    RTL_BSD_DATA_POWER_TRANSITION PowerTransitionData;
    PRTL_BSD_DATA_POWER_TRANSITION PowerBuffer;
    ULONG OsDeviceHandle;
    BOOLEAN LastBootGood, LastBootShutdown, LastBootCheckpoint;
    ULONG BootId;
    BOOLEAN BootGood, BootShutdown;
    ULONG BsdSize;

    /* Initialize locals */
    PowerBuffer = NULL;

    /* Assume no flags */
    *ReturnFlags = 0;

    /* Make all registry handles invalid */
    OslImcHiveHandle = -1;
    OslMachineHiveHandle = -1;
    OslElamHiveHandle = -1;
    OslSystemHiveHandle = -1;

    /* Initialize memory lists */
    InitializeListHead(&OslFreeMemoryDesctiptorsList);
    InitializeListHead(&OslFinalMemoryMap);
    InitializeListHead(&OslLoadedFirmwareDriverList);
    for (i = 0; i < RTL_NUMBER_OF(OslCoreExtensionSubGroups); i++)
    {
        InitializeListHead(&OslCoreExtensionSubGroups[i]);
    }

    /* Initialize the memory map descriptor buffer */
    RtlZeroMemory(&OslFinalMemoryMapDescriptorsBuffer,
                  sizeof(OslFinalMemoryMapDescriptorsBuffer));

    /* Initialize general pointers */
    OslLoadDevice = NULL;
    OslLoadOptions = NULL;
    OslSystemRoot = NULL;
    OslLoaderBlock = NULL;
    OslMemoryDescriptorBuffer = NULL;

    /* Initialize general variables */
    OslResetBootStatus = 0;
    OslImcProcessingValid = FALSE;
    OslFreeMemoryDesctiptorsListSize = 0;

    /* Capture the current TSC */
    StartPerf = BlArchGetPerformanceCounter();

    /* Set our application type for SecureBoot/TPM purposes */
    BlpSbdiCurrentApplicationType.Application.ObjectCode =
        BCD_OBJECT_TYPE_APPLICATION;
    BlpSbdiCurrentApplicationType.Application.ImageCode =
        BCD_IMAGE_TYPE_BOOT_APP;
    BlpSbdiCurrentApplicationType.Application.ApplicationCode =
        BCD_APPLICATION_TYPE_OSLOADER;
    BlpSbdiCurrentApplicationType.Application.Reserved = 0;

    /* Register an error handler */
    BlpStatusErrorHandler = OslBlStatusErrorHandler;

    /* Get the application identifier and save it */
    AppId = BlGetApplicationIdentifier();
    if (AppId)
    {
        OslApplicationIdentifier = *AppId;
    }

    /* Enable tracing */
#ifdef BL_ETW_SUPPORT
    TraceLoggingRegister(&TlgOslBootProviderProv);
#endif

    /* Remove dangerous BCD options */
    Status = OslpRemoveInternalApplicationOptions();
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Fail here: %d\r\n", __LINE__);
        goto Quickie;
    }

    /* Get the OS device */
    Status = BlGetBootOptionDevice(BlpApplicationEntry.BcdData,
                                   BcdOSLoaderDevice_OSDevice,
                                   &OsDevice,
                                   0);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Fail here: %d\r\n", __LINE__);
        goto Quickie;
    }

    /* If the OS device is the boot device, use the one provided by bootlib */
    if (OsDevice->DeviceType == BootDevice)
    {
        OsDevice = BlpBootDevice;
    }

    /* Save it as a global for later */
    OslLoadDevice = OsDevice;

    /* Get the system root */
    Status = BlGetBootOptionString(BlpApplicationEntry.BcdData,
                                   BcdOSLoaderString_SystemRoot,
                                   &SystemRoot);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Fail here: %d\r\n", __LINE__);
        goto Quickie;
    }

    EfiPrintf(L"System root: %s\r\n", SystemRoot);

    /* Get the system root length and make sure it's slash-terminated */
    RootLength = wcslen(SystemRoot);
    if (SystemRoot[RootLength - 1] == OBJ_NAME_PATH_SEPARATOR)
    {
        /* Perfect, set it */
        OslSystemRoot = SystemRoot;
    }
    else
    {
        /* Allocate a new buffer large enough to contain the slash */
        RootLengthWithSep = RootLength + sizeof(OBJ_NAME_PATH_SEPARATOR);
        OslSystemRoot = BlMmAllocateHeap(RootLengthWithSep * sizeof(WCHAR));
        if (!OslSystemRoot)
        {
            /* Bail out if we're out of memory */
            Status = STATUS_NO_MEMORY;
            EfiPrintf(L"Fail here: %d\r\n", __LINE__);
            goto Quickie;
        }

        /* Make a copy of the path, adding the separator */
        wcscpy(OslSystemRoot, SystemRoot);
        wcscat(OslSystemRoot, L"\\");

        /* Free the original one from the BCD library */
        BlMmFreeHeap(SystemRoot);
    }

    /* Initialize access to the BSD */
    OslpInitializeBootStatusDataLog();

    /* Check if we're supposed to fail on purpose */
    Status = OslpCheckForcedFailure();
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Fail here: %d\r\n", __LINE__);
        goto Quickie;
    }

    /* Always disable VGA mode */
    Status = BlAppendBootOptionBoolean(&BlpApplicationEntry,
                                       BcdOSLoaderBoolean_DisableVgaMode,
                                       TRUE);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Fail here: %d\r\n", __LINE__);
        goto Quickie;
    }

    /* Get telemetry data from the last boot */
    Status = OslGetBootStatusData(&LastBootGood,
                                  &LastBootShutdown,
                                  &LastBootCheckpoint,
                                  &BootId,
                                  &BootGood,
                                  &BootShutdown);
    if (!NT_SUCCESS(Status))
    {
        /* Assume this is the very first boot and everything went well */
        BootId = 0;
        LastBootGood = TRUE;
        LastBootShutdown = TRUE;
        LastBootCheckpoint = TRUE;
        BootGood = TRUE;
        BootShutdown = TRUE;

        /* Set 0 boot attempts */
        OslBootAttemptCount = 0;
    }

    /* Set more attempt variables to their initial state */
    OslResetBootStatus = TRUE;
    OslBootCountUpdateRequestForAbort = 0;

    /* Read the current BSD data into the global buffer */
    Status = OslpReadWriteBootStatusData(FALSE);
    if (NT_SUCCESS(Status))
    {
        /* Get the power transition buffer from the BSD */
        BsdSize = sizeof(PowerTransitionData);
        Status = OslpGetSetBootStatusData(TRUE,
                                          RtlBsdPowerTransition,
                                          &PowerTransitionData,
                                          &BsdSize);
        if (NT_SUCCESS(Status))
        {
            /* Save the buffer */
            PowerBuffer = &PowerTransitionData;
        }
    }

    /* Check if this is VHD boot, which gets 3 boot attempts instead of 2 */
    OslBootAttemptMaximum = 2;
    OslBootAttemptMaximum += BlDeviceIsVirtualPartitionDevice(OslLoadDevice, NULL);

    /* Check if the user wants to see the advanced menu */
    if (!OslpAdvancedOptionsRequested())
    {
        /* The last boot failed more than the maximum */
        if (!(LastBootGood) &&
            (OslBootAttemptCount >= OslBootAttemptMaximum))
        {
            /* Return failure due to boot -- launch recovery */
            *ReturnFlags |= 8;

            /* Update the attempt count and status variables */
            OslBootAttemptCount = OslBootAttemptMaximum - 1;
            OslCurrentBootCheckpoint = LastBootCheckpoint;
            OslCurrentBootSucceeded = FALSE;
            OslCurrentBootShutdown = LastBootShutdown;

            /* Crash with code 15 and abort boot */
            OslFatalErrorEx(15, 0, 0, 0);
            Status = STATUS_UNSUCCESSFUL;
            EfiPrintf(L"Fail here: %d\r\n", __LINE__);
            goto Quickie;
        }

        /* We never made it far enough, more than the maximum */
        if (!(LastBootCheckpoint) &&
            (OslBootAttemptCount >= OslBootAttemptMaximum))
        {
            /* Return crash/dirty shutdown during boot attempt */
            *ReturnFlags |= 0x10;

            /* Update the attempt count and status variables */
            OslBootAttemptCount = OslBootAttemptMaximum - 1;
            OslCurrentBootSucceeded = LastBootGood;
            OslCurrentBootShutdown = LastBootShutdown;
            OslCurrentBootCheckpoint = FALSE;

            /* Crash with code 16 and abort boot */
            OslFatalErrorEx(16, 0, 0, 0);
            Status = STATUS_UNSUCCESSFUL;
            EfiPrintf(L"Fail here: %d\r\n", __LINE__);
            goto Quickie;
        }

        /* We failed to shutdown cleanly, and haven't booted yet */
        if (!(LastBootShutdown) && !(OslBootAttemptCount))
        {
            /* Return crash/dirty shutdown */
            *ReturnFlags |= 0x10;

            /* There's no boot attempt, so only update shutdown variables */
            OslCurrentBootSucceeded = LastBootGood;
            OslCurrentBootShutdown = TRUE;
            OslCurrentBootCheckpoint = LastBootCheckpoint;

            /* Crash with code 16 and abort boot */
            OslFatalErrorEx(16, 0, 0, 0);
            Status = STATUS_UNSUCCESSFUL;
            EfiPrintf(L"Fail here: %d\r\n", __LINE__);
            goto Quickie;
        }
    }

    /* Officially increment the number of boot attempts */
    OslBootAttemptCount++;

    /* No success yet, write to boot status file */
    OslCurrentBootCheckpoint = FALSE;
    OslCurrentBootSucceeded = FALSE;
    OslCurrentBootShutdown = FALSE;
    OslSetBootStatusData(FALSE,
                         FALSE,
                         FALSE,
                         OslBootCountUpdateIncrement,
                         OslBootAttemptCount);

    /* Open the OS Loader Device for Read/Write access */
    Status = BlpDeviceOpen(OslLoadDevice,
                           BL_DEVICE_READ_ACCESS | BL_DEVICE_WRITE_ACCESS,
                           0,
                           &OsDeviceHandle);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Fail here: %d\r\n", __LINE__);
        goto Quickie;
    }

    /* That's all for now, folks */
    Status = STATUS_NOT_IMPLEMENTED;
    DBG_UNREFERENCED_LOCAL_VARIABLE(PowerBuffer);

    /* Printf perf */
    EndPerf = BlArchGetPerformanceCounter();
    EfiPrintf(L"Delta: %lld\r\n", EndPerf - StartPerf);

Quickie:
#if BL_BITLOCKER_SUPPORT
    /* Destroy the RNG/AES library for BitLocker */
    SymCryptRngAesUninstantiate();
#endif

    /* Abort the boot */
    OslAbortBoot(Status);

    /* This is a failure path, so never do the jump */
    *Jump = FALSE;

    /* Return error code */
    return Status;
}

NTSTATUS
OslFwpKernelSetupPhase1 (
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
OslArchpKernelSetupPhase0 (
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
ArchRestoreProcessorFeatures (
    VOID
    )
{
    /* Any XCR0 bits to clear? */
    if (ArchXCr0BitsToClear)
    {
        /* Clear them */
#if defined(_MSC_VER) && !defined(__clang__) && !defined(_M_ARM)
        __xsetbv(0, __xgetbv(0) & ~ArchXCr0BitsToClear);
#endif
        ArchXCr0BitsToClear = 0;
    }

    /* Any CR4 bits to clear? */
    if (ArchCr4BitsToClear)
    {
        /* Clear them */
#if !defined(_M_ARM)
        __writecr4(__readcr4() & ~ArchCr4BitsToClear);
#endif
        ArchCr4BitsToClear = 0;
    }
}

NTSTATUS
OslArchKernelSetup (
    _In_ ULONG Phase,
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    /* For phase 0, do architectural setup */
    if (Phase == 0)
    {
        return OslArchpKernelSetupPhase0(LoaderBlock);
    }

    /* Nothing to do for Phase 1 */
    if (Phase == 1)
    {
        return STATUS_SUCCESS;
    }

    /* Relocate the self map */
    BlMmRelocateSelfMap();

    /* Zero out the HAL Heap */
    BlMmZeroVirtualAddressRange((PVOID)MM_HAL_VA_START,
                                MM_HAL_VA_END - MM_HAL_VA_START + 1);

    /* Move shared user data in its place */
    BlMmMoveVirtualAddressRange((PVOID)KI_USER_SHARED_DATA,
                                UserSharedAddress,
                                PAGE_SIZE);

    /* Clear XCR0/CR4 CPU features that should be disabled before boot */
    ArchRestoreProcessorFeatures();

    /* Good to go */
    return STATUS_SUCCESS;
}

NTSTATUS
OslExecuteTransition (
    VOID
    )
{
    NTSTATUS Status;

    /* Is the debugger meant to be kept enabled throughout the boot phase? */
    if (!BdDebugAfterExitBootServices)
    {
#ifdef BL_KD_SUPPORT
        /* No -- disable it */
        BlBdStop();
#endif
    }

    /* Setup Firmware for Phase 1 */
    Status = OslFwpKernelSetupPhase1(OslLoaderBlock);
    if (NT_SUCCESS(Status))
    {
        /* Setup kernel for Phase 2 */
        Status = OslArchKernelSetup(2, OslLoaderBlock);
        if (NT_SUCCESS(Status))
        {
#ifdef BL_KD_SUPPORT
            /* Stop the boot debugger */
            BlBdStop();
#endif
            /* Jump to the kernel entrypoint */
            OslArchTransferToKernel(OslLoaderBlock, OslEntryPoint);

            /* Infinite loop if we got here */
            for (;;);
        }
    }

    /* Return back with the failure code */
    return Status;
}

NTSTATUS
OslpMain (
    _Out_ PULONG ReturnFlags
    )
{
    NTSTATUS Status;
    BOOLEAN ExecuteJump;
#if !defined(_M_ARM)
    CPU_INFO CpuInfo;
    BOOLEAN NxEnabled;
    LARGE_INTEGER MiscMsr;

    /* Check if the CPU supports NX */
    BlArchCpuId(0x80000001, 0, &CpuInfo);
    if (!(CpuInfo.Edx & 0x10000))
    {
        /* It doesn't, check if this is Intel */
        EfiPrintf(L"NX disabled: %lx\r\n", CpuInfo.Edx);
        if (BlArchGetCpuVendor() == CPU_INTEL)
        {
            /* Then turn off the MSR disable feature for it, enabling NX */
            MiscMsr.QuadPart = __readmsr(MSR_IA32_MISC_ENABLE);
            EfiPrintf(L"NX being turned on: %llx\r\n", MiscMsr.QuadPart);
            MiscMsr.HighPart &= MSR_XD_ENABLE_MASK;
            MiscMsr.QuadPart = __readmsr(MSR_IA32_MISC_ENABLE);
            __writemsr(MSR_IA32_MISC_ENABLE, MiscMsr.QuadPart);
            NxEnabled = TRUE;
        }
    }

    /* Turn on NX support with the CPU-generic MSR */
    __writemsr(MSR_EFER, __readmsr(MSR_EFER) | MSR_NXE);

#endif

    /* Load the kernel */
    Status = OslPrepareTarget(ReturnFlags, &ExecuteJump);
    if (NT_SUCCESS(Status) && (ExecuteJump))
    {
        /* Jump to the kernel */
        Status = OslExecuteTransition();
    }

#if !defined(_M_ARM)
    /* Retore NX support */
    __writemsr(MSR_EFER, __readmsr(MSR_EFER) ^ MSR_NXE);

    /* Did we manually enable NX? */
    if (NxEnabled)
    {
        /* Turn it back off */
        MiscMsr.QuadPart = __readmsr(MSR_IA32_MISC_ENABLE);
        MiscMsr.HighPart |= ~MSR_XD_ENABLE_MASK;
        __writemsr(MSR_IA32_MISC_ENABLE, MiscMsr.QuadPart);
    }

#endif
    /* Go back */
    return Status;
}

/*++
 * @name OslMain
 *
 *     The OslMain function implements the Windows Boot Application entrypoint for
 *     the OS Loader.
 *
 * @param  BootParameters
 *         Pointer to the Boot Application Parameter Block.
 *
 * @return NT_SUCCESS if the image was loaded correctly, relevant error code
 *         otherwise.
 *
 *--*/
NTSTATUS
NTAPI
OslMain (
    _In_ PBOOT_APPLICATION_PARAMETER_BLOCK BootParameters
    )
{
    BL_LIBRARY_PARAMETERS LibraryParameters;
    NTSTATUS Status;
    PBL_RETURN_ARGUMENTS ReturnArguments;
    PBL_APPLICATION_ENTRY AppEntry;
    CPU_INFO CpuInfo;
    ULONG Flags;

    /* Get the return arguments structure, and set our version */
    ReturnArguments = (PBL_RETURN_ARGUMENTS)((ULONG_PTR)BootParameters +
                                             BootParameters->ReturnArgumentsOffset);
    ReturnArguments->Version = BL_RETURN_ARGUMENTS_VERSION;

    /* Get the application entry, and validate it */
    AppEntry = (PBL_APPLICATION_ENTRY)((ULONG_PTR)BootParameters +
                                       BootParameters->AppEntryOffset);
    if (!RtlEqualMemory(AppEntry->Signature,
                        BL_APP_ENTRY_SIGNATURE,
                        sizeof(AppEntry->Signature)))
    {
        /* Unrecognized, bail out */
        Status = STATUS_INVALID_PARAMETER_9;
        goto Quickie;
    }

#if !defined(_M_ARM)
    /* Check if CPUID 01h is supported */
    if (BlArchIsCpuIdFunctionSupported(1))
    {
        /* Query CPU features */
        BlArchCpuId(1, 0, &CpuInfo);

        /* Check if PAE is supported */
        if (CpuInfo.Edx & 0x40)
        {
            EfiPrintf(L"PAE Supported, but won't be used\r\n");
        }
    }
#endif

    /* Setup the boot library parameters for this application */
    BlSetupDefaultParameters(&LibraryParameters);
    LibraryParameters.TranslationType = BlVirtual;
    LibraryParameters.LibraryFlags = BL_LIBRARY_FLAG_ZERO_HEAP_ALLOCATIONS_ON_FREE |
                                     BL_LIBRARY_FLAG_REINITIALIZE_ALL;
    LibraryParameters.MinimumAllocationCount = 1024;
    LibraryParameters.MinimumHeapSize = 2 * 1024 * 1024;
    LibraryParameters.HeapAllocationAttributes = BlMemoryKernelRange;
    LibraryParameters.FontBaseDirectory = L"\\Reactos\\Boot\\Fonts";
    LibraryParameters.DescriptorCount = 512;

    /* Initialize the boot library */
    Status = BlInitializeLibrary(BootParameters, &LibraryParameters);
    if (NT_SUCCESS(Status))
    {
        /* For testing, draw the logo */
        OslDrawLogo();

        /* Call the main routine */
        Status = OslpMain(&Flags);

        /* Return the flags, and destroy the boot library */
        ReturnArguments->Flags = Flags;
        BlDestroyLibrary();
    }

Quickie:
    /* Return back to boot manager */
    ReturnArguments->Status = Status;
    return Status;
}

