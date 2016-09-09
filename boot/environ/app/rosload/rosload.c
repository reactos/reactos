/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI OS Loader
 * FILE:            boot/environ/app/rosload/rosload.c
 * PURPOSE:         OS Loader Entrypoint
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "rosload.h"

/* DATA VARIABLES ************************************************************/

/* FUNCTIONS *****************************************************************/

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
    EfiPrintf(L"ReactOS UEFI OS Loader Initializing...\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

