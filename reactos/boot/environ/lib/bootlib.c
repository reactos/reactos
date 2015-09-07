/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/bootlib.c
 * PURPOSE:         Boot Library Initialization
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

BL_LIBRARY_PARAMETERS BlpLibraryParameters;
PBL_DEVICE_DESCRIPTOR BlpBootDevice;
PWCHAR BlpApplicationBaseDirectory;
PBOOT_APPLICATION_PARAMETER_BLOCK BlpApplicationParameters;
BL_APPLICATION_ENTRY BlpApplicationEntry;
BOOLEAN BlpLibraryParametersInitialized;

ULONG PdPersistAllocations;
LIST_ENTRY BlBadpListHead;

/* FUNCTIONS *****************************************************************/

/* HACKKKYYY */
EFI_SYSTEM_TABLE* g_SystemTable;

VOID
EarlyPrint(_In_ PWCHAR Format, ...)
{
    WCHAR buffer[1024];
    va_list args;

    va_start(args, Format);

    vswprintf(buffer, Format, args);

    g_SystemTable->ConOut->OutputString(g_SystemTable->ConOut, L"\r");
    g_SystemTable->ConOut->OutputString(g_SystemTable->ConOut, buffer);

    g_SystemTable->BootServices->Stall(200000);

    va_end(args);
}
/* END HACKKKYYY */

/*++
 * @name InitializeLibrary
 *
 *     The InitializeLibrary function initializes the Boot Library.
 *
 * @param  BootParameters
 *         Pointer to the Boot Application Parameter Block.
 *
 * @param  LibraryParameters
 *         Pointer to the Boot Library Parameters.
 *
 * @return NT_SUCCESS if the boot library was loaded correctly, relevant error
 *         otherwise.
 *
 *--*/
NTSTATUS
InitializeLibrary (
    _In_ PBOOT_APPLICATION_PARAMETER_BLOCK BootAppParameters,
    _In_ PBL_LIBRARY_PARAMETERS LibraryParameters
    )
{
    NTSTATUS Status;
    PBL_MEMORY_DATA MemoryData;
    PBL_APPLICATION_ENTRY AppEntry;
    PBL_FIRMWARE_DESCRIPTOR FirmwareDescriptor;
    ULONG_PTR ParamPointer = (ULONG_PTR)BootAppParameters;

    /* Validate correct Boot Application data */
    if (!(BootAppParameters) ||
        (BootAppParameters->Signature[0] != BOOT_APPLICATION_SIGNATURE_1) ||
        (BootAppParameters->Signature[1] != BOOT_APPLICATION_SIGNATURE_2) ||
        (BootAppParameters->Size < sizeof(*BootAppParameters)))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Get sub-structures */
    MemoryData = (PBL_MEMORY_DATA)(ParamPointer + BootAppParameters->MemoryDataOffset);
    FirmwareDescriptor = (PBL_FIRMWARE_DESCRIPTOR)(ParamPointer + BootAppParameters->FirmwareParametersOffset);
    AppEntry = (PBL_APPLICATION_ENTRY)(ParamPointer + BootAppParameters->AppEntryOffset);
    BlpBootDevice = (PBL_DEVICE_DESCRIPTOR)(ParamPointer + BootAppParameters->BootDeviceOffset);
    BlpApplicationBaseDirectory = LibraryParameters->ApplicationBaseDirectory;

    /* Initialize the firmware table */
    Status = BlpFwInitialize(0, FirmwareDescriptor);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Find boot application entry */
    if (strncmp(AppEntry->Signature, BL_APP_ENTRY_SIGNATURE, 7))
    {
        Status = STATUS_INVALID_PARAMETER_9;
        goto Quickie;
    }

    /* Read parameters */
    BlpApplicationParameters = BootAppParameters;
    BlpLibraryParameters = *LibraryParameters;

    /* Save the application entry */
    if (AppEntry->Flags & 2)
    {
        AppEntry->Flags = (AppEntry->Flags & ~0x2) | 0x80;
    }
    BlpApplicationEntry = *AppEntry;

    /* Everything has been captured */
    BlpLibraryParametersInitialized = TRUE;

    /* Initialize the architecture (PM or RM) switching */
    Status = BlpArchInitialize(0);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Initialize the memory manager */
    Status = BlpMmInitialize(MemoryData,
                             BootAppParameters->MemoryTranslationType,
                             LibraryParameters);
    if (!NT_SUCCESS(Status))
    {
        EarlyPrint(L"MM init failed!\n");
        goto Quickie;
    }

    /* Initialize firmware now that the heap, etc works */
    Status = BlpFwInitialize(1, FirmwareDescriptor);
    if (!NT_SUCCESS(Status))
    {
        /* Destroy memory manager in phase 1 */
        //BlpMmDestroy(1);
        EarlyPrint(L"Firmware2 init failed!\n");
        return Status;
    }

#if 0
    /* Modern systems have an undocumented BCD system for the boot frequency */
    Status = BlGetBootOptionInteger(BlpApplicationEntry.BcdData,
                                    0x15000075,
                                    &BootFrequency);
    if (NT_SUCCESS(Status) && (BootFrequency))
    {
        /* Use it if present */
        BlpTimePerformanceFrequency = BootFrequency;
    }
    else
#endif
    {
        /* Use the TSC for calibration */
        Status = BlpTimeCalibratePerformanceCounter();
        if (!NT_SUCCESS(Status))
        {
            /* Destroy memory manager in phase 1 */
            EarlyPrint(L"TSC calibration failed\n");
            //BlpMmDestroy(1);
            return Status;
        }
    }

    /* Now setup the rest of the architecture (IDT, etc) */
    Status = BlpArchInitialize(1);
    if (!NT_SUCCESS(Status))
    {
        /* Destroy memory manager in phase 1 */
        EarlyPrint(L"Arch2 init failed\n");
        //BlpMmDestroy(1);
        return Status;
    }

#ifdef BL_TPM_SUPPORT
    /* Initialize support for Trusted Platform Module v1.2 */
    BlpTpmInitialize();
#endif

#ifdef BL_TPM_SUPPORT
    /* Initialize the event manager */
    EnSubsystemInitialized = 1;
    InitializeListHead(&EnEventNotificationList);
#endif

    /* Initialize the I/O Manager */
    Status = BlpIoInitialize();
    if (!NT_SUCCESS(Status))
    {
        /* Destroy memory manager in phase 1 and the event manager */
        EarlyPrint(L"IO init failed\n");
#ifdef BL_TPM_SUPPORT
        if (EnSubsystemInitialized)
        {
            BlpEnDestroy();
        }
#endif
        //BlpMmDestroy(1);
        return Status;
    }

#ifdef BL_NET_SUPPORT
    /* Initialize the network stack */
    Status = BlNetInitialize();
    if (!NT_SUCCESS(Status))
    {
        /* Destroy the I/O, event, and memory managers in phase 1 */
        BlpIoDestroy();
#ifdef BL_TPM_SUPPORT
        if (EnSubsystemInitialized)
        {
            BlpEnDestroy();
        }
#endif
        BlpMmDestroy(1);
        return Status;
    }
#endif

    /* Initialize the utility library */
    Status = BlUtlInitialize();
    if (!NT_SUCCESS(Status))
    {
        /* Destroy the network, I/O, event, and memory managers in phase 1 */
#ifdef BL_NET_SUPPORT
        BlNetDestroy();
#endif
        //BlpIoDestroy();
#ifdef BL_TPM_SUPPORT
        if (EnSubsystemInitialized)
        {
            BlpEnDestroy();
        }
#endif
        //BlpMmDestroy(1);
        EarlyPrint(L"Util init failed\n");
        return Status;
    }

#ifdef BL_KD_SUPPORT
    /* Initialize PCI Platform Support */
    PltInitializePciConfiguration();
#endif

#ifdef BL_SECURE_BOOT_SUPPORT
    /* Read the current SecureBoot Policy*/
    Status = BlSecureBootSetActivePolicyFromPersistedData();
    if (!NT_SUCCESS(Status))
    {
        /* Destroy everything that we've currently set up */
#ifdef BL_KD_SUPPORT
        PltDestroyPciConfiguration();
#endif
#ifdef BL_NET_SUPPORT
        BlNetDestroy();
#endif
        BlpIoDestroy();
#ifdef BL_TPM_SUPPORT
        if (EnSubsystemInitialized)
        {
            BlpEnDestroy();
        }
#endif
        BlpMmDestroy(1);
        return Status;
    }
#endif

#ifdef BL_TPM_SUPPORT
    /* Initialize phase 0 of the security subsystem */
    SipInitializePhase0();
#endif

#ifdef BL_KD_SUPPORT
    /* Bring up the boot debugger, now that SecureBoot has been processed */
    BlBdInitialize();
#endif

#ifdef BL_ETW_SUPPORT
    /* Initialize internal logging */
    BlpLogInitialize();
#endif

    /* Are graphics enabled? */
    if (!(LibraryParameters->LibraryFlags & BL_LIBRARY_FLAG_NO_DISPLAY))
    {
        /* Initialize the graphics library */
        BlpDisplayInitialize(LibraryParameters->LibraryFlags);
    }

    /* Initialize the boot application persistent data */
    PdPersistAllocations = 0;
    InitializeListHead(&BlBadpListHead);

#ifdef BL_TPM_SUPPORT
    /* Now setup the security subsystem in phase 1 */
    BlpSiInitialize(1);
#endif

#if 0
    /* Setup the text, UI and font resources */
    Status = BlpResourceInitialize();
    if (!NT_SUCCESS(Status))
    {
        /* Tear down everything if this failed */
        if (!(LibraryParameters->LibraryFlags & BL_LIBRARY_FLAG_TEXT_MODE))
        {
//            BlpDisplayDestroy();
        }
        //BlpBdDestroy();
#ifdef BL_KD_SUPPORT
        PltDestroyPciConfiguration();
#endif
#ifdef BL_NET_SUPPORT
        BlNetDestroy();
#endif
        //BlpIoDestroy();
#ifdef BL_TPM_SUPPORT
        if (EnSubsystemInitialized)
        {
            BlpEnDestroy();
        }
#endif
        //BlpMmDestroy(1);
        return Status;
    }
#endif

#if BL_BITLOCKER_SUPPORT
    /* Setup the boot cryptography library */
    g_pEnvironmentData = &SymCryptEnvironmentWindowsBootLibrary;
    if (SymCryptEnvWindowsBootLibInit)
    {
        SymCryptEnvWindowsBootLibInit();
    }
#endif

    /* We are fully initialized, remember this and exit with success */
    BlpLibraryParameters.LibraryFlags |= BL_LIBRARY_FLAG_INITIALIZATION_COMPLETED;
    Status = STATUS_SUCCESS;

Quickie:
    return Status;
}

/*++
 * @name BlInitializeLibrary
 *
 *     The BlInitializeLibrary function initializes, or re-initializes, the
 *     Boot Library.
 *
 * @param  BootParameters
 *         Pointer to the Boot Application Parameter Block.
 *
 * @param  LibraryParameters
 *         Pointer to the Boot Library Parameters.
 *
 * @return NT_SUCCESS if the boot library was loaded correctly, relevant error
 *         otherwise.
 *
 *--*/
NTSTATUS
BlInitializeLibrary(
    _In_ PBOOT_APPLICATION_PARAMETER_BLOCK BootAppParameters,
    _In_ PBL_LIBRARY_PARAMETERS LibraryParameters
    )
{
    NTSTATUS Status;

    /* Are we re-initializing the library? */
    if (LibraryParameters->LibraryFlags & BL_LIBRARY_FLAG_REINITIALIZE)
    {
        /* From scratch? */
        BlpLibraryParameters = *LibraryParameters;
        if (LibraryParameters->LibraryFlags & BL_LIBRARY_FLAG_REINITIALIZE_ALL)
        {
#if 0
            /* Initialize all the core modules again */
            BlpSiInitialize(1);
            BlBdInitialize();
            BlMmRemoveBadMemory();
            BlpMmInitializeConstraints();

            /* Redraw the graphics console as needed */
            BlpDisplayInitialize(LibraryParameters->LibraryFlags);
            BlpResourceInitialize();
#endif
        }

        /* Nothing to do, we're done */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Nope, this is first time initialization */
        Status = InitializeLibrary(BootAppParameters, LibraryParameters);
    }

    /* Return initialization status */
    return Status;
}

