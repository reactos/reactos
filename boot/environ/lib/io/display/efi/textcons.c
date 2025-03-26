/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/display/efi/textcons.c
 * PURPOSE:         Boot Library EFI Text Console Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

/* FUNCTIONS *****************************************************************/

BL_COLOR
ConsoleEfiTextGetColorForeground (
    _In_ UINT32 Attributes
    )
{
    /* Read the foreground color attribute and convert to CGA color index */
    switch (Attributes & 0x0F)
    {
        case EFI_BLACK:
            return Black;
        case EFI_BLUE:
            return Blue;
        case EFI_GREEN:
            return Green;
        case EFI_RED:
            return Red;
        case EFI_CYAN:
            return Cyan;
        case EFI_MAGENTA:
            return Magenta;
        case EFI_BROWN:
            return Brown;
        case EFI_LIGHTGRAY:
            return LtGray;
        case EFI_DARKGRAY:
            return Gray;
        case EFI_LIGHTBLUE:
            return LtBlue;
        case EFI_LIGHTGREEN:
            return LtGreen;
        case EFI_LIGHTCYAN:
            return LtCyan;
        case EFI_LIGHTRED:
            return LtRed;
        case EFI_LIGHTMAGENTA:
            return LtMagenta;
        case EFI_YELLOW:
            return Yellow;
        case EFI_WHITE:
        default:
            return White;
    }
}

BL_COLOR
ConsoleEfiTextGetColorBackground (
    _In_ UINT32 Attributes
    )
{
    /* Read the background color attribute and convert to CGA color index */
    switch (Attributes & 0xF0)
    {
        case EFI_BACKGROUND_MAGENTA:
            return Magenta;
        case EFI_BACKGROUND_BROWN:
            return Brown;
        case EFI_BACKGROUND_LIGHTGRAY:
            return White;
        case EFI_BACKGROUND_BLACK:
        default:
            return Black;
        case EFI_BACKGROUND_RED:
            return Red;
        case EFI_BACKGROUND_GREEN:
            return Green;
        case EFI_BACKGROUND_CYAN:
            return Cyan;
        case EFI_BACKGROUND_BLUE:
            return Blue;
    }
}

ULONG
ConsoleEfiTextGetEfiColorBackground (
    _In_ BL_COLOR Color
    )
{
    /* Convert the CGA color index into an EFI background attribute */
    switch (Color)
    {
        case Blue:
        case LtBlue:
            return EFI_BACKGROUND_BLUE;
        case Green:
        case LtGreen:
            return EFI_BACKGROUND_GREEN;
        case Cyan:
        case LtCyan:
            return EFI_BACKGROUND_CYAN;
        case Red:
        case LtRed:
            return EFI_BACKGROUND_RED;
        case Magenta:
        case LtMagenta:
            return EFI_BACKGROUND_MAGENTA;
        case Brown:
        case Yellow:
            return EFI_BACKGROUND_BROWN;
        case LtGray:
        case White:
            return EFI_BACKGROUND_LIGHTGRAY;
        case Black:
        case Gray:
        default:
            return EFI_BACKGROUND_BLACK;
    }
}

ULONG
ConsoleEfiTextGetEfiColorForeground (
    _In_ BL_COLOR Color
    )
{
    /* Convert the CGA color index into an EFI foreground attribute */
    switch (Color)
    {
        case Black:
            return EFI_BLACK;
        case Blue:
            return EFI_BLUE;
        case Green:
            return EFI_GREEN;
        case Cyan:
            return EFI_CYAN;
        case Red:
            return EFI_RED;
        case Magenta:
            return EFI_MAGENTA;
        case Brown:
            return EFI_BROWN;
        case LtGray:
            return EFI_LIGHTGRAY;
        case Gray:
            return EFI_DARKGRAY;
        case LtBlue:
            return EFI_LIGHTBLUE;
        case LtGreen:
            return EFI_LIGHTGREEN;
        case LtCyan:
            return EFI_LIGHTCYAN;
        case LtRed:
            return EFI_LIGHTRED;
        case LtMagenta:
            return EFI_LIGHTMAGENTA;
        case Yellow:
            return EFI_YELLOW;
        case White:
        default:
            return EFI_WHITE;
    }
}

ULONG
ConsoleEfiTextGetAttribute (
    BL_COLOR BgColor,
    BL_COLOR FgColor
    )
{
    /* Convert each part and OR into a single attribute */
    return ConsoleEfiTextGetEfiColorBackground(BgColor) |
           ConsoleEfiTextGetEfiColorForeground(FgColor);
}

VOID
ConsoleEfiTextGetStateFromMode (
    _In_ EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode,
    _Out_ PBL_DISPLAY_STATE State
    )
{
    ULONG TextWidth, TextHeight;

    /* Get all the EFI data and convert it into our own structure */
    BlDisplayGetTextCellResolution(&TextWidth, &TextHeight);
    State->FgColor = ConsoleEfiTextGetColorForeground(Mode->Attribute);
    State->BgColor = ConsoleEfiTextGetColorBackground(Mode->Attribute);
    State->XPos = Mode->CursorColumn * TextWidth;
    State->YPos = Mode->CursorRow * TextHeight;
    State->CursorVisible = Mode->CursorVisible != FALSE;
}

NTSTATUS
ConsoleFirmwareTextSetState (
    _In_ PBL_TEXT_CONSOLE TextConsole,
    _In_ UCHAR Mask,
    _In_ PBL_DISPLAY_STATE State
    )
{
    NTSTATUS Status;
    ULONG FgColor, BgColor, Attribute, XPos, YPos, TextHeight, TextWidth;
    BOOLEAN Visible;

    /* Check if foreground state is being set */
    if (Mask & 1)
    {
        /* Check if there's a difference from current */
        FgColor = State->FgColor;
        if (TextConsole->State.FgColor != FgColor)
        {
            /* Ignore invalid color */
            if (FgColor > White)
            {
                return STATUS_INVALID_PARAMETER;
            }

            /* Convert from NT/CGA format to EFI, and then set the attribute */
            Attribute = ConsoleEfiTextGetAttribute(TextConsole->State.BgColor,
                                                   FgColor);
            Status = EfiConOutSetAttribute(TextConsole->Protocol, Attribute);
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            /* Update cached state */
            TextConsole->State.FgColor = FgColor;
        }
    }

    /* Check if background state is being set */
    if (Mask & 2)
    {
        /* Check if there's a difference from current */
        BgColor = State->BgColor;
        if (TextConsole->State.BgColor != BgColor)
        {
            /* Ignore invalid color */
            if (BgColor > White)
            {
                return STATUS_INVALID_PARAMETER;
            }

            /* Convert from NT/CGA format to EFI, and then set the attribute */
            Attribute = ConsoleEfiTextGetAttribute(BgColor,
                                                   TextConsole->State.FgColor);
            Status = EfiConOutSetAttribute(TextConsole->Protocol, Attribute);

            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            /* Update cached state */
            TextConsole->State.BgColor = BgColor;
        }
    }

    /* Check if position state is being set */
    if (Mask & 4)
    {
        /* Check if there's a difference from current */
        XPos = State->XPos;
        YPos = State->YPos;
        if ((TextConsole->State.XPos != XPos) ||
            (TextConsole->State.YPos != YPos))
        {
            /* Set the new cursor position */
            BlDisplayGetTextCellResolution(&TextWidth, &TextHeight);
            Status = EfiConOutSetCursorPosition(TextConsole->Protocol,
                                                XPos/ TextWidth,
                                                YPos / TextHeight);
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            /* Update cached state */
            TextConsole->State.XPos = XPos;
            TextConsole->State.YPos = YPos;
        }
    }

    /* Check if cursor state is being set */
    if (Mask & 8)
    {
        /* Check if there's a difference from current */
        Visible = State->CursorVisible;
        if (TextConsole->State.CursorVisible != Visible)
        {
            /* Ignore invalid state */
            if (Visible >= 3)
            {
                return STATUS_INVALID_PARAMETER;
            }

            /* Set the new cursor state */
            Status = EfiConOutEnableCursor(TextConsole->Protocol, Visible);
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            /* Update cached status */
            TextConsole->State.CursorVisible = Visible;
        }
    }

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
ConsoleEfiTextFindModeFromAllowed (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextProtocol,
    _In_ PBL_DISPLAY_MODE SupportedModes,
    _In_ ULONG MaxIndex,
    _Out_ PULONG SupportedMode
    )
{
    EFI_SIMPLE_TEXT_OUTPUT_MODE ModeInfo;
    ULONG MaxMode, MaxQueriedMode, Mode, i, MatchingMode;
    UINTN HRes, VRes;
    ULONGLONG ModeListSize;
    PBL_DISPLAY_MODE ModeEntry, ModeList, SupportedModeEntry;
    NTSTATUS Status;

    /* Read information on the current mode */
    EfiConOutReadCurrentMode(TextProtocol, &ModeInfo);

    /* Figure out the max mode, and how many modes we'll have to read */
    MaxMode = ModeInfo.MaxMode;
    ModeListSize = sizeof(*ModeEntry) * ModeInfo.MaxMode;
    if (ModeListSize > MAXULONG)
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    /* Allocate a list for all the supported EFI modes */
    ModeList = BlMmAllocateHeap(ModeListSize);
    if (!ModeList)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Scan all the EFI modes */
    EfiPrintf(L"Scanning through %d modes\r\n", MaxMode);
    for (MaxQueriedMode = 0, Mode = 0; Mode < MaxMode; Mode++)
    {
        /* Query information on this mode */
        ModeEntry = &ModeList[MaxQueriedMode];
        if (NT_SUCCESS(EfiConOutQueryMode(TextProtocol,
                                          Mode,
                                          &HRes,
                                          &VRes)))
        {
            /* This mode was successfully queried. Save the data */
            EfiPrintf(L"EFI Firmware Supported Mode %d is H: %d V: %d\r\n", Mode, HRes, VRes);
            ModeEntry->HRes = HRes;
            ModeEntry->VRes = VRes;
            ModeEntry->HRes2 = HRes;
            MaxQueriedMode = Mode + 1;
        }
    }

    /* Loop all the supported mode entries */
    for (i = 0; i < MaxIndex; i++)
    {
        /* Loop all the UEFI queried modes */
        SupportedModeEntry = &SupportedModes[i];
        for (MatchingMode = 0; MatchingMode < MaxQueriedMode; MatchingMode++)
        {
            /* Check if the UEFI mode is compatible with our supported mode */
            ModeEntry = &ModeList[MatchingMode];
            EfiPrintf(L"H1: %d V1: %d - H2: %d - V2: %d\r\n", ModeEntry->HRes, ModeEntry->VRes, SupportedModeEntry->HRes, SupportedModeEntry->VRes);
            if ((ModeEntry->HRes == SupportedModeEntry->HRes) &&
                (ModeEntry->VRes == SupportedModeEntry->VRes))
            {
                /* Yep -- free the mode list and return this mode */
                BlMmFreeHeap(ModeList);
                *SupportedMode = MatchingMode;
                return STATUS_SUCCESS;
            }
        }
    }

    /* We can't do anything -- there are no matching modes */
    Status = STATUS_UNSUCCESSFUL;
    BlMmFreeHeap(ModeList);
    return Status;
}

VOID
ConsoleFirmwareTextClose (
    _In_ PBL_TEXT_CONSOLE TextConsole
    )
{
    ULONG Mode;
    BL_DISPLAY_STATE DisplayState;

    /* Read the original mode, and see if it's different than the one now */
    Mode = TextConsole->OldMode.Mode;
    if (Mode != TextConsole->Mode)
    {
        /* Restore to the original mode */
        EfiConOutSetMode(TextConsole->Protocol, Mode);
    }

    /* Read the EFI settings for the original mode */
    ConsoleEfiTextGetStateFromMode(&TextConsole->OldMode, &DisplayState);

    /* Set the original settings */
    ConsoleFirmwareTextSetState(TextConsole, 0xF, &DisplayState);
}

NTSTATUS
ConsoleFirmwareTextOpen (
    _In_ PBL_TEXT_CONSOLE TextConsole
    )
{
    BL_DISPLAY_MODE DisplayMode;
    EFI_SIMPLE_TEXT_OUTPUT_MODE CurrentMode, NewMode;
    UINTN HRes, VRes;
    ULONG Mode;
    NTSTATUS Status;

    /* Read the current mode and its settings */
    EfiConOutReadCurrentMode(EfiConOut, &CurrentMode);
    Status = EfiConOutQueryMode(EfiConOut, CurrentMode.Mode, &HRes, &VRes);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Save the current mode and its settings */
    NewMode = CurrentMode;
    DisplayMode.VRes = VRes;
    DisplayMode.HRes = HRes;
    DisplayMode.HRes2 = HRes;

    /* Check if the current mode is compatible with one of our modes */
    if (!ConsolepFindResolution(&DisplayMode, ConsoleTextResolutionList, 1))
    {
        /* It isn't -- find a matching EFI mode for what we need */
        EfiPrintf(L"In incorrect mode, scanning for right one\r\n");
        Status = ConsoleEfiTextFindModeFromAllowed(EfiConOut,
                                                   ConsoleTextResolutionList,
                                                   1,
                                                   &Mode);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"Failed to find mode: %lx\r\n", Status);
            return Status;
        }

        /* Set the new EFI mode */
        EfiPrintf(L"Setting new mode: %d\r\n", Mode);
        Status = EfiConOutSetMode(EfiConOut, Mode);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Read the current mode and its settings */
        EfiConOutReadCurrentMode(EfiConOut, &NewMode);
        Status = EfiConOutQueryMode(EfiConOut, Mode, &HRes, &VRes);
        if (!NT_SUCCESS(Status))
        {
            EfiConOutSetMode(EfiConOut, CurrentMode.Mode);
            return Status;
        }

        /* Save the current mode and its settings */
        DisplayMode.HRes = HRes;
        DisplayMode.VRes = VRes;
        DisplayMode.HRes2 = HRes;
    }

    /* Capture all the current settings */
    ConsoleEfiTextGetStateFromMode(&NewMode, &TextConsole->State);
    TextConsole->Mode = NewMode.Mode;
    TextConsole->DisplayMode = DisplayMode;
    TextConsole->Protocol = EfiConOut;
    TextConsole->OldMode = CurrentMode;
    return STATUS_SUCCESS;
}

NTSTATUS
ConsoleInputBaseEraseBuffer (
    _In_ PBL_INPUT_CONSOLE Console,
    _In_opt_ PULONG FillValue
    )
{
    ULONG ValueToFill;
    PULONG i;

    /* Check if we should fill with a particular value */
    if (FillValue)
    {
        /* Use it */
        ValueToFill = *FillValue;
    }
    else
    {
        /* Otherwise, use default */
        ValueToFill = 0x10020;
    }

    /* Set the input buffer to its last location */
    Console->DataStart = Console->DataEnd;

    /* Fill the buffer with the value */
    for (i = Console->Buffer; i < Console->EndBuffer; i++)
    {
        *i = ValueToFill;
    }

    /* All done */
    return STATUS_SUCCESS;
}

NTSTATUS
ConsoleInputLocalEraseBuffer (
    _In_ PBL_INPUT_CONSOLE Console,
    _In_opt_ PULONG FillValue
    )
{
    NTSTATUS Status, EfiStatus;

    /* Erase the software buffer */
    Status = ConsoleInputBaseEraseBuffer(Console, FillValue);

    /* Reset the hardware console */
    EfiStatus = EfiConInEx ? EfiConInExReset() : EfiConInReset();
    if (!NT_SUCCESS(EfiStatus))
    {
        /* Normalize the failure code */
        EfiStatus = STATUS_UNSUCCESSFUL;
    }

    /* Check if software reset worked */
    if (NT_SUCCESS(Status))
    {
        /* Then return the firmware code */
        Status = EfiStatus;
    }

    /* All done */
    return Status;
}

NTSTATUS
ConsoleFirmwareTextClear (
    _In_ PBL_TEXT_CONSOLE Console,
    _In_ BOOLEAN LineOnly
    )
{
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;
    NTSTATUS Status;
    ULONG i, Column, Row, TextWidth, TextHeight;

    /* Get the text resolution */
    BlDisplayGetTextCellResolution(&TextWidth, &TextHeight);

    /* Are we just clearing a line? */
    if (LineOnly)
    {
        /* Get the current column and row */
        Column = Console->State.XPos / TextWidth;
        Row = Console->State.YPos / TextHeight;

        /* Loop over every remaining character */
        for (i = 0; i < Console->DisplayMode.HRes - Column - 1; i++)
        {
            /* Write a space on top of it */
            Status = EfiConOutOutputString(Console->Protocol, L" ");
            if (!NT_SUCCESS(Status))
            {
                break;
            }
        }

        /* And reset the cursor back at the initial position */
        Status = EfiConOutSetCursorPosition(Console->Protocol,
                                            Column,
                                            Row);
    }
    else
    {
        /* Are we in protected mode? */
        OldMode = CurrentExecutionContext->Mode;
        if (OldMode != BlRealMode)
        {
            /* FIXME: Not yet implemented */
            return STATUS_NOT_IMPLEMENTED;
        }

        /* Clear the screen */
        EfiStatus = Console->Protocol->ClearScreen(Console->Protocol);

        /* Switch back to protected mode if we came from there */
        if (OldMode != BlRealMode)
        {
            BlpArchSwitchContext(OldMode);
        }

        /* Conver to NT status -- did that work? */
        Status = EfiGetNtStatusCode(EfiStatus);
        if (NT_SUCCESS(Status))
        {
            /* Reset current positions */
            Console->State.XPos = 0;
            Console->State.YPos = 0;
        }
    }

    /* All done */
    return Status;
}

