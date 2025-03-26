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
BL_LOADED_APPLICATION_ENTRY BlpApplicationEntry;
BOOLEAN BlpLibraryParametersInitialized;
ULONG BlpApplicationFlags;

ULONG PdPersistAllocations;
LIST_ENTRY BlpPdListHead;

/* FUNCTIONS *****************************************************************/

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
    LARGE_INTEGER BootFrequency;
    ULONG_PTR ParamPointer;

    /* Validate correct Boot Application data */
    ParamPointer = (ULONG_PTR)BootAppParameters;
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

    /* Check if the caller sent us their internal BCD options */
    if (AppEntry->Flags & BL_APPLICATION_ENTRY_BCD_OPTIONS_INTERNAL)
    {
        /* These are external to us now, as far as we are concerned */
        AppEntry->Flags &= ~BL_APPLICATION_ENTRY_BCD_OPTIONS_INTERNAL;
        AppEntry->Flags |= BL_APPLICATION_ENTRY_BCD_OPTIONS_EXTERNAL;
    }

    /* Save the application entry flags */
    BlpApplicationEntry.Flags = AppEntry->Flags;

    /* Copy the GUID and point to the options */
    BlpApplicationEntry.Guid = AppEntry->Guid;
    BlpApplicationEntry.BcdData = &AppEntry->BcdData;

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
        EfiPrintf(L"MM init failed!\r\n");
        goto Quickie;
    }

    /* Initialize firmware now that the heap, etc works */
    Status = BlpFwInitialize(1, FirmwareDescriptor);
    if (!NT_SUCCESS(Status))
    {
        /* Destroy memory manager in phase 1 */
        //BlpMmDestroy(1);
        EfiPrintf(L"Firmware2 init failed!\r\n");
        return Status;
    }

    /* Modern systems have an undocumented BCD system for the boot frequency */
    Status = BlGetBootOptionInteger(BlpApplicationEntry.BcdData,
                                    0x15000075,
                                    (PULONGLONG)&BootFrequency.QuadPart);
    if (NT_SUCCESS(Status) && (BootFrequency.QuadPart))
    {
        /* Use it if present */
        BlpTimePerformanceFrequency = BootFrequency.QuadPart;
    }
    else
    {
        /* Use the TSC for calibration */
        Status = BlpTimeCalibratePerformanceCounter();
        if (!NT_SUCCESS(Status))
        {
            /* Destroy memory manager in phase 1 */
            EfiPrintf(L"TSC calibration failed\r\n");
            //BlpMmDestroy(1);
            return Status;
        }
    }

    /* Now setup the rest of the architecture (IDT, etc) */
    Status = BlpArchInitialize(1);
    if (!NT_SUCCESS(Status))
    {
        /* Destroy memory manager in phase 1 */
        EfiPrintf(L"Arch2 init failed\r\n");
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
        EfiPrintf(L"IO init failed\r\n");
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
        EfiPrintf(L"Util init failed\r\n");
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
    InitializeListHead(&BlpPdListHead);

#ifdef BL_TPM_SUPPORT
    /* Now setup the security subsystem in phase 1 */
    BlpSiInitialize(1);
#endif

    /* Setup the text, UI and font resources */
    Status = BlpResourceInitialize();
    if (!NT_SUCCESS(Status))
    {
        /* Tear down everything if this failed */
        if (!(LibraryParameters->LibraryFlags & BL_LIBRARY_FLAG_NO_DISPLAY))
        {
//            BlpDisplayDestroy();
        }
#ifdef BL_KD_SUPPORT
        BlpBdDestroy();
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
#ifdef BL_TPM_SUPPORT
            /* Reinitialize the TPM security enclave as BCD hash changed */
            BlpSiInitialize(1);
#endif
#ifdef BL_KD_SUPPORT
            /* Reinitialize the boot debugger as BCD debug options changed */
            BlBdInitialize();
#endif

            /* Reparse the bad page list now that the BCD has been reloaded */
            BlMmRemoveBadMemory();

            /* Reparse the low/high physical address limits as well */
            BlpMmInitializeConstraints();

            /* Redraw the graphics console as needed */
            BlpDisplayInitialize(LibraryParameters->LibraryFlags);

            /* Reinitialize resources (language may have changed) */
            BlpResourceInitialize();
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

VOID
BlDestroyLibrary (
    VOID
    )
{
    EfiPrintf(L"Destroy not yet implemented\r\n");
    return;
}

PGUID
BlGetApplicationIdentifier (
    VOID
    )
{
    /* Return the GUID, if one was present */
    return (BlpApplicationEntry.Flags & BL_APPLICATION_ENTRY_FLAG_NO_GUID) ?
            NULL : &BlpApplicationEntry.Guid;
}

NTSTATUS
BlGetApplicationBaseAndSize (
    _Out_ PVOID* ImageBase,
    _Out_ PULONG ImageSize
    )
{
    /* Fail if output parameters are missing */
    if (!ImageBase || !ImageSize)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Return the requested data */
    *ImageBase = (PVOID)(ULONG_PTR)BlpApplicationParameters->ImageBase;
    *ImageSize = BlpApplicationParameters->ImageSize;
    return STATUS_SUCCESS;
}

VOID
BlDestroyBootEntry (
    _In_ PBL_LOADED_APPLICATION_ENTRY AppEntry
    )
{
    /* Check if we had allocated BCD options */
    if (AppEntry->Flags & BL_APPLICATION_ENTRY_BCD_OPTIONS_INTERNAL)
    {
        BlMmFreeHeap(AppEntry->BcdData);
    }

    /* Free the entry itself */
    BlMmFreeHeap(AppEntry);
}

NTSTATUS
BlPdQueryData (
    _In_ const GUID* DataGuid,
    _In_ PVOID Unknown,
    _Inout_ PBL_PD_DATA_BLOB DataBlob
    )
{
    /* Check for invalid or missing parameters */
    if (!(DataBlob) ||
        !(DataGuid) ||
        ((DataBlob->BlobSize) && !(DataBlob->Data)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if there's no persistent data blobs */
    if (IsListEmpty(&BlpPdListHead))
    {
        return STATUS_NOT_FOUND;
    }

    /* Not yet handled, TODO */
    EfiPrintf(L"Boot persistent data not yet implemented\r\n");
    return STATUS_NOT_IMPLEMENTED;
}
