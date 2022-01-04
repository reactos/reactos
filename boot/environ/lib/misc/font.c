/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/font.c
 * PURPOSE:         Boot Library Font Functions
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

LIST_ENTRY BfiDeferredListHead;

/* FUNCTIONS *****************************************************************/

NTSTATUS
BfiLoadFontFile (
    _In_ PBL_DEVICE_DESCRIPTOR FontDevice,
    _In_ PWCHAR FontPath
    )
{
    EfiPrintf(L"Cannot load font %s, no font loader exists\r\n", FontPath);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
BfiFreeDeferredFontFile (
    _In_ PBL_DEFERRED_FONT_FILE DeferredFontFile
    )
{
    /* Free the device copy if there was one */
    if (DeferredFontFile->Device)
    {
        BlMmFreeHeap(DeferredFontFile->Device);
    }

    /* Free the path copy if there was one */
    if (DeferredFontFile->FontPath)
    {
        BlMmFreeHeap(DeferredFontFile->FontPath);
    }

    /* Free the whole thing */
    BlMmFreeHeap(DeferredFontFile);
}

NTSTATUS
BfLoadFontFile (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ PWCHAR FontPath
    )
{
    PBL_DEFERRED_FONT_FILE DeferredFont;
    SIZE_T FontPathSize;

    /* Allocate the deferred font structure */
    DeferredFont = (PBL_DEFERRED_FONT_FILE)BlMmAllocateHeap(sizeof(*DeferredFont));
    if (!DeferredFont)
    {
        return STATUS_NO_MEMORY;
    }

    /* Zero it out */
    RtlZeroMemory(DeferredFont, sizeof(*DeferredFont));

    /* Allocate a copy for the file path */
    FontPathSize = sizeof(WCHAR) * wcslen(FontPath) + sizeof(UNICODE_NULL);
    DeferredFont->FontPath = (PWCHAR)BlMmAllocateHeap(FontPathSize);
    if (!DeferredFont->FontPath)
    {
        BfiFreeDeferredFontFile(DeferredFont);
        return STATUS_NO_MEMORY;
    }

    /* Allocate a copy for the device */
    DeferredFont->Device = BlMmAllocateHeap(Device->Size);
    if (!DeferredFont->Device)
    {
        BfiFreeDeferredFontFile(DeferredFont);
        return STATUS_NO_MEMORY;
    }

    /* Copy the path and device */
    RtlCopyMemory(DeferredFont->FontPath, FontPath, FontPathSize);
    RtlCopyMemory(DeferredFont->Device,Device, Device->Size);

    /* Set pending flag? */
    DeferredFont->Flags = 1;

    /* Insert it into the list */
    InsertTailList(&BfiDeferredListHead, &DeferredFont->ListEntry);
    return STATUS_SUCCESS;
}

NTSTATUS
BfLoadDeferredFontFiles (
    VOID
    )
{
    PLIST_ENTRY NextEntry;
    PBL_DEFERRED_FONT_FILE DeferredFont;
    NTSTATUS Status, LoadStatus;

    /* Assume empty list */
    Status = STATUS_SUCCESS;

    /* Parse the list */
    NextEntry = BfiDeferredListHead.Flink;
    while (NextEntry != &BfiDeferredListHead)
    {
        /* Get the font */
        DeferredFont = CONTAINING_RECORD(NextEntry, BL_DEFERRED_FONT_FILE, ListEntry);

        /* Move to the next entry and remove this one */
        NextEntry = NextEntry->Flink;
        RemoveEntryList(&DeferredFont->ListEntry);

        /* Load the font */
        LoadStatus = BfiLoadFontFile(DeferredFont->Device,
                                     DeferredFont->FontPath);
        if (!NT_SUCCESS(LoadStatus))
        {
            /* Remember the load failure if there was one */
            Status = LoadStatus;
        }

        /* Free the deferred font */
        BfiFreeDeferredFontFile(DeferredFont);
    }

    /* Return load status */
    return Status;
}

NTSTATUS
BfiFlipCursorCharacter (
    _In_ PBL_GRAPHICS_CONSOLE Console,
    _In_ BOOLEAN Visible
    )
{
    /* not implemented */
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BfClearToEndOfLine (
    _In_ PBL_GRAPHICS_CONSOLE Console
    )
{
    /* not implemented */
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BfClearScreen  (
    _In_ PBL_GRAPHICS_CONSOLE Console
    )
{
    NTSTATUS Status;

    /* Reset the cursor position */
    Console->TextConsole.State.XPos = 0;
    Console->TextConsole.State.YPos = 0;

    /* Fill the screen with the background color */
    Status = ConsoleGraphicalClearPixels(Console,
                                         Console->TextConsole.State.BgColor);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Check if the cursor should be visible */
    if (Console->TextConsole.State.CursorVisible)
    {
        /* Load any fonts at this time */
        if (!IsListEmpty(&BfiDeferredListHead))
        {
            BfLoadDeferredFontFiles();
        }

        /* Switch the cursor to visible */
        Status = BfiFlipCursorCharacter(Console, TRUE);
    }
    else
    {
        /* Nothing left to do */
        Status = STATUS_SUCCESS;
    }

    /* Return cursor flip result, if any */
    return Status;
}
