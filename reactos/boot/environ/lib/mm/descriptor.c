/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/mm/descriptor.c
 * PURPOSE:         Boot Library Memory Manager Descriptor Manager
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/


/* FUNCTIONS *****************************************************************/

NTSTATUS
MmMdInitialize (
    _In_ ULONG Phase,
    _In_ PBL_LIBRARY_PARAMETERS LibraryParameters
    )
{
    EarlyPrint(L"Md init\n");
    return STATUS_NOT_IMPLEMENTED;
}
