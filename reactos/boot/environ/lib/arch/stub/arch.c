/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/arch/stub/arch.c
 * PURPOSE:         Boot Library Architectural Initialization Skeleton Code
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

PBL_ARCH_CONTEXT CurrentExecutionContext;

/* FUNCTIONS *****************************************************************/

VOID
BlpArchSwitchContext (
    _In_ BL_ARCH_MODE NewMode
    )
{
}

/*++
* @name BlpArchInitialize
*
*     The BlpArchInitialize function initializes the Boot Library.
*
* @param  Phase
*         Pointer to the Boot Application Parameter Block.
*
* @return NT_SUCCESS if the boot library was loaded correctly, relevant error
*         otherwise.
*
*--*/
NTSTATUS
BlpArchInitialize (
    _In_ ULONG Phase
    )
{
    EfiPrintf(L" BlpArchInitialize NOT IMPLEMENTED for this platform\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

