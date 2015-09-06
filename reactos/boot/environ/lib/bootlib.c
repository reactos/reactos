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

    g_SystemTable->BootServices->Stall(1000000);

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

    EarlyPrint(L"TODO!\n");
    Status = STATUS_NOT_IMPLEMENTED;

Quickie:
    EarlyPrint(L"Exiting init: %lx\n", Status);
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

