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

LOADER_PARAMETER_BLOCK OslLoaderBlock;
PVOID OslEntryPoint;
PVOID UserSharedAddress;
ULONGLONG ArchXCr0BitsToClear;
ULONGLONG ArchCr4BitsToClear;
BOOLEAN BdDebugAfterExitBootServices;
KDESCRIPTOR OslKernelGdt;
KDESCRIPTOR OslKernelIdt;

/* FUNCTIONS *****************************************************************/

NTSTATUS
OslPrepareTarget (
    _Out_ PULONG ReturnFlags,
    _Out_ PBOOLEAN Jump
    )
{
    return STATUS_NOT_IMPLEMENTED;
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
    Status = OslFwpKernelSetupPhase1(&OslLoaderBlock);
    if (NT_SUCCESS(Status))
    {
        /* Setup kernel for Phase 2 */
        Status = OslArchKernelSetup(2, &OslLoaderBlock);
        if (NT_SUCCESS(Status))
        {
#ifdef BL_KD_SUPPORT
            /* Stop the boot debugger */
            BlBdStop();
#endif
            /* Jump to the kernel entrypoint */
            OslArchTransferToKernel(&OslLoaderBlock, OslEntryPoint);

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

