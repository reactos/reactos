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
    DBG_UNREFERENCED_PARAMETER(BootAppParameters);
    DBG_UNREFERENCED_PARAMETER(LibraryParameters);

    return STATUS_NOT_IMPLEMENTED;
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
    if (LibraryParameters->LibraryFlags & 2)
    {
        /* From scratch? */
        BlpLibraryParameters = *LibraryParameters;
        if (LibraryParameters->LibraryFlags & 4)
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

