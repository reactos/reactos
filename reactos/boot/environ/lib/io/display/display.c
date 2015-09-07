/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/platform/display.c
 * PURPOSE:         Boot Library Display Management Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"
#include <bcd.h>

/* DATA VARIABLES ************************************************************/

PVOID BfiCachedStrikeData;
LIST_ENTRY BfiDeferredListHead;
LIST_ENTRY BfiFontFileListHead;
PVOID BfiGraphicsRectangle;

ULONG ConsoleGraphicalResolutionListFlags;
BL_DISPLAY_MODE ConsoleGraphicalResolutionList[3] =
{
    {1024, 768, 1024},
    {800, 600, 800},
    {1024, 600, 1024}
};
ULONG ConsoleGraphicalResolutionListSize = RTL_NUMBER_OF(ConsoleGraphicalResolutionList);

BL_DISPLAY_MODE ConsoleTextResolutionList[1] =
{
    {80, 25, 80}
};

PVOID DspRemoteInputConsole;
PVOID DspTextConsole;
PVOID DspGraphicalConsole;

/* FUNCTIONS *****************************************************************/

BOOLEAN
DsppGraphicsDisabledByBcd (
    VOID
    )
{
    //EarlyPrint(L"Disabling graphics\n");
    return FALSE;
}

NTSTATUS
DsppInitialize (
    _In_ ULONG Flags
    )
{
    BL_LIBRARY_PARAMETERS LibraryParameters = BlpLibraryParameters;
    BOOLEAN NoGraphics;// , HighestMode;
    NTSTATUS Status;
    PBL_DISPLAY_MODE DisplayMode;
    //ULONG GraphicsResolution;
    PBL_GRAPHICS_CONSOLE GraphicsConsole;
    PBL_TEXT_CONSOLE TextConsole, RemoteConsole;

    /* Initialize font data */
    BfiCachedStrikeData = 0;
    InitializeListHead(&BfiDeferredListHead);
    InitializeListHead(&BfiFontFileListHead);

    /* Allocate the font rectangle */
    BfiGraphicsRectangle = BlMmAllocateHeap(0x5A);
    if (!BfiGraphicsRectangle)
    {
        return STATUS_NO_MEMORY;
    }

    /* Display re-initialization not yet handled */
    if (LibraryParameters.LibraryFlags & BL_LIBRARY_FLAG_REINITIALIZE_ALL)
    {
        EarlyPrint(L"Display path not handled\n");
        return STATUS_NOT_SUPPORTED;
    }

    /* Check if no graphics console is needed */
    if ((Flags & BL_LIBRARY_FLAG_NO_GRAPHICS_CONSOLE) ||
        (DsppGraphicsDisabledByBcd()))
    {
        /* Remember this */
        NoGraphics = TRUE;
    }
    else
    {
        /* No graphics -- remember this */
        NoGraphics = FALSE;
    }

    /* On first load, we always initialize a graphics display */
    GraphicsConsole = NULL;
    if (!(Flags & BL_LIBRARY_FLAG_REINITIALIZE_ALL) || !(NoGraphics))
    {
        /* Default to mode 0 (1024x768) */
        DisplayMode = &ConsoleGraphicalResolutionList[0];

        /* Check what resolution to use*/
#if 0
        Status = BlGetBootOptionInteger(BlpApplicationEntry.BcdData,
                                        BcdLibraryInteger_GraphicsResolution,
                                        &GraphicsResolution);
#else
        //GraphicsResolution = 0;
        Status = STATUS_NOT_FOUND;
#endif
        if (NT_SUCCESS(Status))
        {
            ConsoleGraphicalResolutionListFlags |= BL_DISPLAY_GRAPHICS_FORCED_VIDEO_MODE_FLAG;
            EarlyPrint(L"Display selection not yet handled\n");
            return STATUS_NOT_IMPLEMENTED;
        }

        /* Check if the highest mode should be forced */
#if 0
        Status = BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                                        BcdLibraryBoolean_GraphicsForceHighestMode,
                                        &HighestMode);
#else
        //HighestMode = 0;
        Status = STATUS_NOT_FOUND;
#endif
        if (NT_SUCCESS(Status))
        {
            ConsoleGraphicalResolutionListFlags |= BL_DISPLAY_GRAPHICS_FORCED_HIGH_RES_MODE_FLAG;
            EarlyPrint(L"High res mode not yet handled\n");
            return STATUS_NOT_IMPLEMENTED;
        }

        /* Do we need graphics mode after all? */
        if (!NoGraphics)
        {
            /* Yep -- go allocate it */
            GraphicsConsole = BlMmAllocateHeap(sizeof(*GraphicsConsole));
            if (GraphicsConsole)
            {
                /* Construct it */
                Status = ConsoleGraphicalConstruct(GraphicsConsole);
                if (!NT_SUCCESS(Status))
                {
                    EarlyPrint(L"GFX FAILED: %lx\n", Status);
                    BlMmFreeHeap(GraphicsConsole);
                    GraphicsConsole = NULL;
                }
                else
                {
                    /* TEST */
                    RtlFillMemory(GraphicsConsole->FrameBuffer, GraphicsConsole->FrameBufferSize, 0x55);
                }
            }
        }

        /* Are we using something else than the default mode? */
        if (DisplayMode != &ConsoleGraphicalResolutionList[0])
        {
            EarlyPrint(L"Display path not handled\n");
            return STATUS_NOT_SUPPORTED;
        }

        /* Mask out all the flags now */
        ConsoleGraphicalResolutionListFlags &= ~(BL_DISPLAY_GRAPHICS_FORCED_VIDEO_MODE_FLAG |
                                                 BL_DISPLAY_GRAPHICS_FORCED_HIGH_RES_MODE_FLAG);
    }

    /* Do we have a graphics console? */
    TextConsole = NULL;
    if (!GraphicsConsole)
    {
        /* Nope -- go allocate a text console */
        TextConsole = BlMmAllocateHeap(sizeof(*TextConsole));
        if (TextConsole)
        {
            /* Construct it */
            Status = ConsoleTextLocalConstruct(TextConsole, TRUE);
            if (!NT_SUCCESS(Status))
            {
                BlMmFreeHeap(TextConsole);
                TextConsole = NULL;
            }
        }
    }

    /* Initialize all globals to NULL */
    DspRemoteInputConsole = NULL;
    DspTextConsole = NULL;
    DspGraphicalConsole = NULL;

    /* If we don't have a text console, go get a remote console */
    RemoteConsole = NULL;
    if (!TextConsole)
    {
        ConsoleCreateRemoteConsole(&RemoteConsole);
    }

    /* Do we have a remote console? */
    if (!RemoteConsole)
    {
        /* Nope -- what about a graphical one? */
        if (GraphicsConsole)
        {
            /* Yes, use it for both graphics and text */
            DspGraphicalConsole = GraphicsConsole;
            DspTextConsole = GraphicsConsole;
        }
        else if (TextConsole)
        {
            /* Nope, but we have a text console */
            DspTextConsole = TextConsole;
        }

        /* Console has been setup */
        return STATUS_SUCCESS;
    }

    /* We have a remote console -- have to figure out how to use it*/
    EarlyPrint(L"Display path not handled\n");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
BlpDisplayInitialize (
    _In_ ULONG Flags
    )
{
    NTSTATUS Status;

    /* Are we resetting or initializing? */
    if (Flags & BL_LIBRARY_FLAG_REINITIALIZE)
    {
        /* This is a reset */
        Status = STATUS_NOT_IMPLEMENTED;
#if 0
        Status = DsppReinitialize(Flags);
        if (NT_SUCCESS(Status))
        {
            Status = BlpDisplayReinitialize();
        }
#endif
    }
    else
    {
        /* Initialize the display */
        Status = DsppInitialize(Flags);
    }

    /* Return display initailziation state */
    return Status;
}

VOID
BlDisplayGetTextCellResolution (
    _Out_ PULONG TextWidth,
    _Out_ PULONG TextHeight
    )
{
    NTSTATUS Status;

    /* If the caller doesn't want anything, bail out */
    if (!(TextWidth) || !(TextHeight))
    {
        return;
    }

    /* Do we have a text console? */
    Status = STATUS_UNSUCCESSFUL;
    if (DspTextConsole)
    {
        /* Do we have a graphics console? */
        if (DspGraphicalConsole)
        {
            /* Yep -- query it */
            EarlyPrint(L"Not supported\n");
            Status = STATUS_NOT_IMPLEMENTED;
        }
    }

    /* Check if we failed to get it from the graphics console */
    if (!NT_SUCCESS(Status))
    {
        /* Set default text size */
        *TextWidth = 8;
        *TextHeight = 8;
    }
}
