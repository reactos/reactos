/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/display/emscons.c
 * PURPOSE:         Boot Library Remote Console Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

/* FUNCTIONS *****************************************************************/

NTSTATUS
ConsoleRemoteConstruct (
    _In_ PBL_REMOTE_CONSOLE RemoteConsole
    )
{
#ifdef BL_EMS_SUPPORT
#error Implement me
#else
    /* We don't support EMS for now */
    return STATUS_NOT_IMPLEMENTED;
#endif
}

NTSTATUS
ConsoleCreateRemoteConsole (
    _In_ PBL_TEXT_CONSOLE* TextConsole
    )
{
    PBL_REMOTE_CONSOLE RemoteConsole;
    NTSTATUS Status;

    /* Allocate the remote console */
    RemoteConsole = BlMmAllocateHeap(sizeof(*RemoteConsole));
    if (!RemoteConsole)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Construct it */
    Status = ConsoleRemoteConstruct(RemoteConsole);
    if (Status < 0)
    {
        /* Failed to construct it, delete it */
        BlMmFreeHeap(RemoteConsole);
        return Status;
    }

    /* Save the global pointer and return a pointer to the text console */
    DspRemoteInputConsole = RemoteConsole;
    *TextConsole = &RemoteConsole->TextConsole;
    return STATUS_SUCCESS;
}
