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
        //BlRemoveBootOption(BlpApplicationEntry.BcdData,
        //                   BcdLibraryString_LoadOptionsString);

        /* Sanitize strings we don't want */
        OslpSanitizeLoadOptionsString(LoadString, L"DISABLE_INTEGRITY_CHECKS");
        OslpSanitizeLoadOptionsString(LoadString, L"NOINTEGRITYCHECKS");
        OslpSanitizeLoadOptionsString(LoadString, L"DISABLEELAMDRIVERS");

        /* Add the sanitized one back */
        //Status = BlAppendBootOptionsString(&BlpApplicationEntry,
        //                                   BcdLibraryString_LoadOptionsString,
        //                                   LoadString);

        /* Free the original BCD one */
        BlMmFreeHeap(LoadString);
    }

    /* One more pass for secure-boot options */
    OslpSanitizeStringOptions(BlpApplicationEntry.BcdData);

    /* All good */
    return Status;
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

#ifdef BL_TPM_SUPPORT
    BlpSbdiCurrentApplicationType = 0x10200003;
#endif

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
        goto Quickie;
    }

    /* Get the OS device */
    Status = BlGetBootOptionDevice(BlpApplicationEntry.BcdData,
                                   BcdOSLoaderDevice_OSDevice,
                                   &OsDevice,
                                   0);
    if (!NT_SUCCESS(Status))
    {
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
            goto Quickie;
        }

        /* Make a copy of the path, adding the separator */
        wcscpy(OslSystemRoot, SystemRoot);
        wcscat(OslSystemRoot, L"\\");

        /* Free the original one from the BCD library */
        BlMmFreeHeap(SystemRoot);
    }

    Status = STATUS_NOT_IMPLEMENTED;

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
#ifdef _MSC_VER
        __xsetbv(0, __xgetbv(0) & ~ArchXCr0BitsToClear);
#endif
        ArchXCr0BitsToClear = 0;
    }

    /* Any CR4 bits to clear? */
    if (ArchCr4BitsToClear)
    {
        /* Clear them */
        __writecr4(__readcr4() & ~ArchCr4BitsToClear);
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
    CPU_INFO CpuInfo;
    BOOLEAN NxEnabled;
    NTSTATUS Status;
    BOOLEAN ExecuteJump;
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

    /* Load the kernel */
    Status = OslPrepareTarget(ReturnFlags, &ExecuteJump);
    if (NT_SUCCESS(Status) && (ExecuteJump))
    {
        /* Jump to the kernel */
        Status = OslExecuteTransition();
    }

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

