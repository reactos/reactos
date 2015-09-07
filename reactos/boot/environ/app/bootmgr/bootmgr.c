/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Manager
 * FILE:            boot/environ/app/bootmgr.c
 * PURPOSE:         Boot Manager Entrypoint
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bootmgr.h"

/* DATA VARIABLES ************************************************************/

ULONGLONG ApplicationStartTime;
ULONGLONG PostTime;

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

    EarlyPrint(L"ReactOS UEFI Boot Manager Initializing...\n");

    /* Save the start/end-of-POST time */
    ApplicationStartTime = __rdtsc();
    PostTime = ApplicationStartTime;

    /* Setup the boot library parameters for this application */
    BlSetupDefaultParameters(&LibraryParameters);
    LibraryParameters.TranslationType = BlNone;
    LibraryParameters.LibraryFlags = 0x400 | 0x8;
    LibraryParameters.MinimumAllocationCount = 16;
    LibraryParameters.MinimumHeapSize = 512 * 1024;

    /* Initialize the boot library */
    Status = BlInitializeLibrary(BootParameters, &LibraryParameters);

    EarlyPrint(L"ReactOS UEFI Boot Manager Exiting: %lx\n", Status);
    EfiStall(3000000);
    return Status;
}

