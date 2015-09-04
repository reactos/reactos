/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Manager
 * FILE:            boot/environ/app/bootmgr.c
 * PURPOSE:         Boot Manager Entrypoint
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bootmgr.h"

/* FUNCTIONS *****************************************************************/

/*++
 * @name BmMain
 *
 *     The BmMain function implements the Windows Boot Application entrypoint for
 *     the Boot Manager.
 *
 * @param  BootParameters
 *         Pointer to the Boot Application Parameter Block.
 *
 * @return NT_SUCCESS if the image was loaded correctly, relevant error code
 *         otherwise.
 *
 *--*/
NTSTATUS
BmMain (
    _In_ PBOOT_APPLICATION_PARAMETER_BLOCK BootParameters
    )
{
    NTSTATUS Status;
    BL_LIBRARY_PARAMETERS LibraryParameters;

    Status = BlInitializeLibrary(BootParameters, &LibraryParameters);

    return Status;
}

