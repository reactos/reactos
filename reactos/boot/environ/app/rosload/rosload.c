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
    BL_LIBRARY_PARAMETERS LibraryParameters;
    NTSTATUS Status;

    /* Setup the boot library parameters for this application */
    BlSetupDefaultParameters(&LibraryParameters);
    LibraryParameters.TranslationType = BlVirtual;
    LibraryParameters.LibraryFlags = BL_LIBRARY_FLAG_INITIALIZATION_COMPLETED;
    LibraryParameters.MinimumAllocationCount = 1024;
    LibraryParameters.MinimumHeapSize = 2 * 1024 * 1024;
    LibraryParameters.HeapAllocationAttributes = 0x20000;
    LibraryParameters.FontBaseDirectory = L"\\Reactos\\Boot\\Fonts";
    LibraryParameters.DescriptorCount = 512;
    Status = BlInitializeLibrary(BootParameters, &LibraryParameters);

    EfiPrintf(L"ReactOS UEFI OS Loader Initializing...\r\n");
    return Status;
}

