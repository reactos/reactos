/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/display/guicons.c
 * PURPOSE:         Boot Library GUI Console Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

BL_GRAPHICS_CONSOLE_VTABLE ConsoleGraphicalVtbl =
{
    { NULL },
};

/* FUNCTIONS *****************************************************************/

NTSTATUS
ConsoleGraphicalConstruct (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    )
{
    NTSTATUS Status;

    /* Create a text console */
    Status = ConsoleTextLocalConstruct(&GraphicsConsole->TextConsole, FALSE);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Text failed: %lx\r\n", Status);
        return Status;
    }

    /* But overwrite its callbacks with ours */
    GraphicsConsole->TextConsole.Callbacks = &ConsoleGraphicalVtbl.Text;

    /* Try to create a GOP console */
    Status = ConsoleEfiGraphicalOpenProtocol(GraphicsConsole, BlGopConsole);
    if (!NT_SUCCESS(Status))
    {
        /* That failed, try an older EFI 1.02 UGA console */
        EfiPrintf(L"GOP open failed!\r\n", Status);
        Status = ConsoleEfiGraphicalOpenProtocol(GraphicsConsole, BlUgaConsole);
        if (!NT_SUCCESS(Status))
        {
            /* That failed too, give up */
            EfiPrintf(L"UGA failed!\r\n", Status);
            ConsoleTextLocalDestruct(&GraphicsConsole->TextConsole);
            return STATUS_UNSUCCESSFUL;
        }
    }

    /* Enable the console */
    Status = ConsoleFirmwareGraphicalEnable(GraphicsConsole);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to enable it, undo everything */
        EfiPrintf(L"Enable failed\r\n");
        ConsoleFirmwareGraphicalClose(GraphicsConsole);
        ConsoleTextLocalDestruct(&GraphicsConsole->TextConsole);
        return STATUS_UNSUCCESSFUL;
    }

    /* Save the graphics text color from the text mode text color */
    GraphicsConsole->FgColor = GraphicsConsole->TextConsole.State.FgColor;
    GraphicsConsole->BgColor = GraphicsConsole->TextConsole.State.BgColor;
    return STATUS_SUCCESS;
}
