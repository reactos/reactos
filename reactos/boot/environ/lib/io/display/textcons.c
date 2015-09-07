/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/display/textcons.c
 * PURPOSE:         Boot Library Text Console Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

BL_TEXT_CONSOLE_VTABLE ConsoleTextLocalVtbl =
{
    ConsoleTextLocalDestruct,
    ConsoleTextLocalReinitialize,
    ConsoleTextBaseGetTextState,
    ConsoleTextLocalSetTextState,
    ConsoleTextBaseGetTextResolution,
    ConsoleTextLocalSetTextResolution,
    ConsoleTextLocalClearText,
    ConsoleTextLocalWriteText
};

/* FUNCTIONS *****************************************************************/

NTSTATUS
ConsoleTextLocalDestruct (
    _In_ struct _BL_TEXT_CONSOLE* Console
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConsoleTextLocalReinitialize (
    _In_ struct _BL_TEXT_CONSOLE* Console
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConsoleTextBaseGetTextState (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _Out_ PBL_DISPLAY_STATE TextState
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConsoleTextLocalSetTextState (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG Flags,
    _In_ PBL_DISPLAY_STATE TextState
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConsoleTextBaseGetTextResolution (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _Out_ PULONG TextResolution
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConsoleTextLocalSetTextResolution (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG NewTextResolution,
    _Out_ PULONG OldTextResolution
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConsoleTextLocalClearText (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG Attribute
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConsoleTextLocalWriteText (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ PCHAR Text,
    _In_ ULONG Attribute
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConsoleTextLocalConstruct (
    _In_ PBL_TEXT_CONSOLE TextConsole,
    _In_ BOOLEAN Activate
    )
{
    NTSTATUS Status;
    BL_DISPLAY_STATE TextState;

    /* Set our callbacks */
    TextConsole->Callbacks = &ConsoleTextLocalVtbl;

    /* Are we activating this console? */
    if (Activate)
    {
        /* Call firmware to activate it */
        Status = ConsoleFirmwareTextOpen(TextConsole);
        if (!NT_SUCCESS(Status))
        {
            EarlyPrint(L"Failed to activate console: %lx\n", Status);
            return Status;
        }
    }

    /* Set default text state */
    TextState.BgColor = 0;
    TextState.XPos = 0;
    TextState.YPos = 0;
    TextState.CursorVisible = FALSE;
    TextState.FgColor = White;

    /* Are we activating? */
    if (Activate)
    {
        /* Call firmware to set it */
        Status = ConsoleFirmwareTextSetState(TextConsole, 0xF, &TextState);
        if (!NT_SUCCESS(Status))
        {
            /* We failed, back down */
            EarlyPrint(L"Failed to set console state: %lx\n", Status);
            ConsoleFirmwareTextClose(TextConsole);
            return Status;
        }
    }
    else
    {
        /* Just save the state for now, someone else can activate later */
        TextConsole->State = TextState;
    }

    /* Remember if we activated it */
    TextConsole->Active = Activate;
    return STATUS_SUCCESS;
}

BOOLEAN
ConsolepFindResolution (
    _In_ PBL_DISPLAY_MODE Mode,
    _In_ PBL_DISPLAY_MODE List,
    _In_ ULONG MaxIndex
    )
{
    PBL_DISPLAY_MODE ListEnd;

    /* Loop until we hit the maximum supported list index */
    ListEnd = &List[MaxIndex];
    while (List != ListEnd)
    {
        /* Does this resolution match? */
        if ((Mode->HRes == List->HRes) && (Mode->VRes == List->VRes))
        {
            /* Yep -- we got a match */
            return TRUE;

        }

        /* Try another one*/
        List++;
    }

    /* No matches were found */
    return FALSE;
}
