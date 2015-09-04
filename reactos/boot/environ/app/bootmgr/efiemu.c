/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Manager
 * FILE:            boot/environ/app/efiemu.c
 * PURPOSE:         UEFI Entrypoint for Boot Manager
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bootmgr.h"

/* FUNCTIONS *****************************************************************/

/*++
 * @name EfiInitCreateInputParametersEx
 *
 *     The EfiInitCreateInputParametersEx routine converts UEFI entrypoint
 *     parameters to the ones expected by Windows Boot Applications
 *
 * @param  ImageHandle
 *         UEFI Image Handle for the current loaded application.
 *
 * @param  SystemTable
 *         Pointer to the UEFI System Table.
 *
 * @return A PBOOT_APPLICATION_PARAMETER_BLOCK structure containing the data
 *         from UEFI, translated to the Boot Library-compatible format.
 *
 *--*/
PBOOT_APPLICATION_PARAMETER_BLOCK
EfiInitCreateInputParametersEx (
    _In_ EFI_HANDLE ImageHandle,
    _In_ EFI_SYSTEM_TABLE *SystemTable
    )
{
    DBG_UNREFERENCED_PARAMETER(ImageHandle);
    DBG_UNREFERENCED_PARAMETER(SystemTable);

    /* Not yet implemented */
    return NULL;
}

/*++
 * @name EfiEntry
 *
 *     The EfiEntry routine implements the UEFI entrypoint for the application.
 *
 * @param  ImageHandle
 *         UEFI Image Handle for the current loaded application.
 *
 * @param  SystemTable
 *         Pointer to the UEFI System Table.
 *
 * @return EFI_SUCCESS if the image was loaded correctly, relevant error code
 *         otherwise.
 *
 *--*/
EFI_STATUS
EfiEntry (
    _In_ EFI_HANDLE ImageHandle,
    _In_ EFI_SYSTEM_TABLE *SystemTable
    )
{
    NTSTATUS Status;
    PBOOT_APPLICATION_PARAMETER_BLOCK BootParameters;

    /* Temporary debugging string */
    SystemTable->ConOut->OutputString(SystemTable->ConsoleOutHandle, L"Hello from EFI\n");

    /* Convert EFI parameters to Windows Boot Application parameters */
    BootParameters = EfiInitCreateInputParametersEx(ImageHandle, SystemTable);
    if (BootParameters != NULL)
    {
        /* Conversion was good -- call the Boot Manager Entrypoint */
        SystemTable->ConOut->OutputString(SystemTable->ConsoleOutHandle, L"EFI input OK!\n");
        Status = BmMain(BootParameters);
    }
    else
    {
        /* Conversion failed, bail out */
        SystemTable->ConOut->OutputString(SystemTable->ConsoleOutHandle, L"EFI input failed\n");
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Convert the NT status code to an EFI code */
    return EfiGetEfiStatusCode(Status);
}

