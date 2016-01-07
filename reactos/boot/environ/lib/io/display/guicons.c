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
    {
        (PCONSOLE_DESTRUCT)ConsoleGraphicalDestruct,
        (PCONSOLE_REINITIALIZE)ConsoleGraphicalReinitialize
    },
    ConsoleGraphicalIsEnabled,
    ConsoleGraphicalEnable,
    NULL,
    ConsoleGraphicalGetGraphicalResolution,
    ConsoleGraphicalGetOriginalResolution,
    NULL,
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

BOOLEAN
ConsoleGraphicalIsEnabled  (
    _In_ PBL_GRAPHICS_CONSOLE Console
    )
{
    /* Is the text console active? If so, the graphics console isn't */
    return !Console->TextConsole.Active;
}

VOID
ConsoleGraphicalDestruct (
    _In_ PBL_GRAPHICS_CONSOLE Console
    )
{
    /* Is the text console active? */
    if (Console->TextConsole.Active)
    {
        /* Disable it */
        ConsoleFirmwareGraphicalDisable(Console);
    }

    /* Close the firmware protocols */
    ConsoleFirmwareGraphicalClose(Console);

    /* Destroy the console object */
    ConsoleTextLocalDestruct(&Console->TextConsole);
}

NTSTATUS
ConsoleGraphicalReinitialize (
    _In_ PBL_GRAPHICS_CONSOLE Console
    )
{
    /* Is the text console active? */
    if (Console->TextConsole.Active)
    {
        /* Reinitialize it */
        ConsoleTextLocalReinitialize(&Console->TextConsole);
    }

    /* Disable the graphics console */
    ConsoleFirmwareGraphicalDisable(Console);

    /* Then bring it back again */
    return ConsoleFirmwareGraphicalEnable(Console);
}

NTSTATUS
ConsoleGraphicalEnable (
    _In_ PBL_GRAPHICS_CONSOLE Console,
    _In_ BOOLEAN Enable
    )
{
    BOOLEAN Active;
    NTSTATUS Status;

    /* The text mode console state should be the opposite of what we want to do */
    Active = Console->TextConsole.Active;
    if (Active == Enable)
    {
        /* Are we trying to enable graphics? */
        if (Enable)
        {
            /* Enable the console */
            Status = ConsoleFirmwareGraphicalEnable(Console);
            if (NT_SUCCESS(Status))
            {
                return Status;
            }

            /* Is the text console active? */
            if (Console->TextConsole.Active)
            {
                /* Turn it off */
                ConsoleFirmwareTextClose(&Console->TextConsole);
                Console->TextConsole.Active = FALSE;
            }

            /* Preserve the text colors */
            Console->FgColor = Console->TextConsole.State.FgColor;
            Console->BgColor = Console->TextConsole.State.BgColor;
        }
        else
        {
            /* We are turning off graphics -- is the text console active? */
            if (Active != TRUE)
            {
                /* It isn't, so let's turn it on */
                Status = ConsoleFirmwareTextOpen(&Console->TextConsole);
                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }

                /* Remember that it's on */
                Console->TextConsole.Active = TRUE;
            }

            /* Disable the graphics console */
            ConsoleFirmwareGraphicalDisable(Console);
        }
    }

    /* All good */
    return STATUS_SUCCESS;
}

NTSTATUS
ConsoleGraphicalGetGraphicalResolution (
    _In_ PBL_GRAPHICS_CONSOLE Console, 
    _In_ PBL_DISPLAY_MODE DisplayMode
    )
{
    /* Is the text console active? */
    if (Console->TextConsole.Active)
    {
        /* There's no graphics resolution then */
        return STATUS_UNSUCCESSFUL;
    }

    /* Return the current display mode */
    *DisplayMode = Console->DisplayMode;
    return STATUS_SUCCESS;
}

NTSTATUS
ConsoleGraphicalGetOriginalResolution (
    _In_ PBL_GRAPHICS_CONSOLE Console, 
    _In_ PBL_DISPLAY_MODE DisplayMode
    )
{
    /* Is the text console active? */
    if (Console->TextConsole.Active)
    {
        /* There's no graphics resolution then */
        return STATUS_UNSUCCESSFUL;
    }

    /* Return the current display mode */
    *DisplayMode = Console->OldDisplayMode;
    return STATUS_SUCCESS;
}
