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

typedef enum _BL_COLOR
{
    Black,
    Blue,
    Green,
    Cyan,
    Red,
    Magenta,
    Brown,
    LtGray,
    Gray,
    LtBlue,
    LtGreen,
    LtCyan,
    LtRed,
    LtMagenta,
    Yellow,
    White
} BL_COLOR, *PBL_COLOR;

typedef struct _BL_DISPLAY_STATE
{
    ULONG BgColor;
    ULONG FgColor;
    ULONG XPos;
    ULONG YPos;
    ULONG CursorVisible;
} BL_DISPLAY_STATE, *PBL_DISPLAY_STATE;

typedef struct _BL_DISPLAY_MODE
{
    ULONG HRes;
    ULONG VRes;
    ULONG HRes2;
} BL_DISPLAY_MODE, *PBL_DISPLAY_MODE;

struct _BL_TEXT_CONSOLE;
typedef
NTSTATUS
(*PCONSOLE_DESTRUCT) (
    _In_ struct _BL_TEXT_CONSOLE* Console
    );

typedef
NTSTATUS
(*PCONSOLE_REINITIALIZE) (
    _In_ struct _BL_TEXT_CONSOLE* Console
    );

typedef
NTSTATUS
(*PCONSOLE_GET_TEXT_STATE) (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _Out_ PBL_DISPLAY_STATE TextState
    );

typedef
NTSTATUS
(*PCONSOLE_SET_TEXT_STATE) (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG Flags,
    _In_ PBL_DISPLAY_STATE TextState
    );

typedef
NTSTATUS
(*PCONSOLE_GET_TEXT_RESOLUTION) (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _Out_ PULONG TextResolution
    );

typedef
NTSTATUS
(*PCONSOLE_SET_TEXT_RESOLUTION) (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG NewTextResolution,
    _Out_ PULONG OldTextResolution
    );

typedef
NTSTATUS
(*PCONSOLE_CLEAR_TEXT) (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG Attribute
    );


typedef
NTSTATUS
(*PCONSOLE_WRITE_TEXT) (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ PCHAR Text,
    _In_ ULONG Attribute
    );

typedef struct _BL_TEXT_CONSOLE_VTABLE
{
    PCONSOLE_DESTRUCT Destruct;
    PCONSOLE_REINITIALIZE Reinitialize;
    PCONSOLE_GET_TEXT_STATE GetTextState;
    PCONSOLE_SET_TEXT_STATE SetTextState;
    PCONSOLE_GET_TEXT_RESOLUTION GetTextResolution;
    PCONSOLE_SET_TEXT_RESOLUTION SetTextResolution;
    PCONSOLE_CLEAR_TEXT ClearText;
    PCONSOLE_WRITE_TEXT WriteText;
} BL_TEXT_CONSOLE_VTABLE, *PBL_TEXT_CONSOLE_VTABLE;

typedef struct _BL_GRAPHICS_CONSOLE_VTABLE
{
    BL_TEXT_CONSOLE_VTABLE Text;
    /// more for graphics ///
} BL_GRAPHICS_CONSOLE_VTABLE, *PBL_GRAPHICS_CONSOLE_VTABLE;

typedef struct _BL_TEXT_CONSOLE
{
    PBL_TEXT_CONSOLE_VTABLE Callbacks;
    BL_DISPLAY_STATE State;
    BL_DISPLAY_MODE DisplayMode;
    BOOLEAN Active;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* Protocol;
    ULONG Mode;
    EFI_SIMPLE_TEXT_OUTPUT_MODE OldMode;
} BL_TEXT_CONSOLE, *PBL_TEXT_CONSOLE;

typedef enum _BL_GRAPHICS_CONSOLE_TYPE
{
    BlGopConsole,
    BlUgaConsole
} BL_GRAPHICS_CONSOLE_TYPE;

typedef struct _BL_GRAPHICS_CONSOLE
{
    BL_TEXT_CONSOLE TextConsole;

    BL_DISPLAY_MODE DisplayMode;

    ULONG PixelDepth;

    ULONG FgColor;
    ULONG BgColor;

    BL_DISPLAY_MODE OldDisplayMode;
    ULONG OldPixelDepth;

    EFI_HANDLE Handle;
    BL_GRAPHICS_CONSOLE_TYPE Type;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* Protocol;
    PVOID FrameBuffer;
    ULONG FrameBufferSize;
    ULONG PixelsPerScanLine;
    ULONG Mode;
    ULONG OldMode;
} BL_GRAPHICS_CONSOLE, *PBL_GRAPHICS_CONSOLE;

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

NTSTATUS
ConsoleTextLocalDestruct (
    _In_ struct _BL_TEXT_CONSOLE* Console
    );

NTSTATUS
ConsoleTextLocalReinitialize (
    _In_ struct _BL_TEXT_CONSOLE* Console
    );

NTSTATUS
ConsoleTextBaseGetTextState (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _Out_ PBL_DISPLAY_STATE TextState
    );

NTSTATUS
ConsoleTextLocalSetTextState (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG Flags,
    _In_ PBL_DISPLAY_STATE TextState
    );

NTSTATUS
ConsoleTextBaseGetTextResolution (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _Out_ PULONG TextResolution
    );

NTSTATUS
ConsoleTextLocalSetTextResolution (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG NewTextResolution,
    _Out_ PULONG OldTextResolution
    );

NTSTATUS
ConsoleTextLocalClearText (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG Attribute
    );

NTSTATUS
ConsoleTextLocalWriteText (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ PCHAR Text,
    _In_ ULONG Attribute
    );

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

BL_GRAPHICS_CONSOLE_VTABLE ConsoleGraphicalVtbl =
{
    {NULL},
};

PVOID DspRemoteInputConsole;
PVOID DspTextConsole;
PVOID DspGraphicalConsole;

/* FUNCTIONS *****************************************************************/

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
    EarlyPrint(L"Scanning through %d modes\n", MaxMode);
    for (MaxQueriedMode = 0, Mode = 0; Mode < MaxMode; Mode++)
    {
        /* Query information on this mode */
        ModeEntry = &ModeList[MaxQueriedMode];
        if (NT_SUCCESS(EfiConOutQueryMode(TextProtocol,
                                          Mode,
                                          &HRes,
                                          &VRes)))
        {
            /* This mode was succesfully queried. Save the data */
            EarlyPrint(L"EFI Firmware Supported Mode %d is H: %d V: %d\n", Mode, HRes, VRes);
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
            EarlyPrint(L"H1: %d V1: %d - H2: %d - V2: %d\n", ModeEntry->HRes, ModeEntry->VRes, SupportedModeEntry->HRes, SupportedModeEntry->VRes);
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
        EarlyPrint(L"In incorrect mode, scanning for right one\n");
        Status = ConsoleEfiTextFindModeFromAllowed(EfiConOut,
                                                   ConsoleTextResolutionList,
                                                   1,
                                                   &Mode);
        if (!NT_SUCCESS(Status))
        {
            EarlyPrint(L"Failed to find mode: %lx\n", Status);
            return Status;
        }

        /* Set the new EFI mode */
        EarlyPrint(L"Setting new mode: %d\n", Mode);
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
ConsoleEfiUgaOpen (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    )
{
    EarlyPrint(L"UGA not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConsoleEfiGopGetGraphicalFormat (
    _In_ EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *ModeInfo,
    _Out_ PULONG PixelDepth
    )
{
    /* Convert the format to depth */
    if (ModeInfo->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
    {
        *PixelDepth = 32;
        return STATUS_SUCCESS;
    }
    if (ModeInfo->PixelFormat == PixelBitMask)
    {
        *PixelDepth = 24;
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
ConsoleEfiGopIsPixelFormatSupported (
    _In_ EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Mode
    )
{
    BOOLEAN Supported;
    EFI_PIXEL_BITMASK PixelMask;

    Supported = FALSE;

    /* Check if it's simple BGR8 */
    if (Mode->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
    {
        Supported = TRUE;
    }
    else
    {
        /* Otherwise, we can check if it's a masked format */
        if (Mode->PixelFormat == PixelBitMask)
        {
            /* Check if the masked format is BGR8 */
            PixelMask.BlueMask = 0xFF;
            PixelMask.GreenMask = 0xFF00;
            PixelMask.RedMask = 0xFF0000;
            PixelMask.ReservedMask = 0;
            if (RtlEqualMemory(&Mode->PixelInformation,
                               &PixelMask,
                               sizeof(PixelMask)))
            {
                Supported = TRUE;
            }
        }
    }

    /* Return if the format was supported */
    return Supported;
}

#define BL_DISPLAY_GRAPHICS_FORCED_VIDEO_MODE_FLAG          0x01
#define BL_DISPLAY_GRAPHICS_FORCED_HIGH_RES_MODE_FLAG       0x02

NTSTATUS
ConsoleEfiGopFindModeFromAllowed (
    _In_ EFI_GRAPHICS_OUTPUT_PROTOCOL *GopProtocol,
    _In_ PBL_DISPLAY_MODE SupportedModes,
    _In_ ULONG MaximumIndex,
    _Out_ PULONG SupportedMode
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
ConsoleEfiUgaClose (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    )
{
    return;
}

VOID
ConsoleEfiGopClose (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    )
{
    ULONG OldMode;

    /* Did we switch modes when we turned on the console? */
    OldMode = GraphicsConsole->OldMode;
    if (GraphicsConsole->Mode != OldMode)
    {
        /* Restore the old mode and reset the OEM bitmap in ACPI */
        EfiGopSetMode(GraphicsConsole->Protocol, OldMode);
        //BlDisplayInvalidateOemBitmap();
    }

    /* Close the GOP protocol */
    EfiCloseProtocol(GraphicsConsole->Handle,
                     &EfiGraphicsOutputProtocol);
}

VOID
ConsoleFirmwareGraphicalClose (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    )
{
    /* Call the correct close routine based on the console mode */
    if (GraphicsConsole->Type == BlUgaConsole)
    {
        ConsoleEfiUgaClose(GraphicsConsole);
    }
    else
    {
        ConsoleEfiGopClose(GraphicsConsole);
    }

}
NTSTATUS
ConsoleEfiGopOpen (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    )
{
    NTSTATUS Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *GopProtocol;
    ULONG Mode, PixelDepth;
    UINTN CurrentMode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION ModeInformation;
    BOOLEAN CurrentModeOk;

    /* Open a handle to GOP */
    Status = EfiOpenProtocol(GraphicsConsole->Handle,
                             &EfiGraphicsOutputProtocol,
                             (PVOID*)&GopProtocol);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_NOT_SUPPORTED;
    }

    /* Get the current mode */
    Status = EfiGopGetCurrentMode(GopProtocol, &CurrentMode, &ModeInformation);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    Mode = CurrentMode;

    /* Check if any custom BCD options were provided */
    if (ConsoleGraphicalResolutionListFlags &
        (BL_DISPLAY_GRAPHICS_FORCED_VIDEO_MODE_FLAG |
         BL_DISPLAY_GRAPHICS_FORCED_HIGH_RES_MODE_FLAG))
    {
        /* We'll have to find a mode */
        CurrentModeOk = FALSE;
    }
    else
    {
        /* Then we should be in the default mode, check if the pixel format is OK */
        CurrentModeOk = ConsoleEfiGopIsPixelFormatSupported(&ModeInformation);
    }

    /* Is the mode/format OK? */
    if (!CurrentModeOk)
    {
        /* Nope -- we'll have to go find one */
        Status = ConsoleEfiGopFindModeFromAllowed(GopProtocol,
                                                  ConsoleGraphicalResolutionList,
                                                  ConsoleGraphicalResolutionListSize,
                                                  &Mode);
        if (!NT_SUCCESS(Status))
        {
            goto Quickie;
        }
    }

    /* Store mode information */
    GraphicsConsole->Protocol = GopProtocol;
    GraphicsConsole->Mode = Mode;
    GraphicsConsole->OldMode = CurrentMode;

    /* Get format information */
    Status = ConsoleEfiGopGetGraphicalFormat(&ModeInformation, &PixelDepth);
    if (NT_SUCCESS(Status))
    {
        /* Store it */
        GraphicsConsole->OldDisplayMode.HRes = ModeInformation.HorizontalResolution;
        GraphicsConsole->OldDisplayMode.VRes = ModeInformation.VerticalResolution;
        GraphicsConsole->OldDisplayMode.HRes2 = ModeInformation.PixelsPerScanLine;
        GraphicsConsole->PixelDepth = PixelDepth;
        return STATUS_SUCCESS;
    }

Quickie:
    /* We failed, close the protocol and return the failure code */
    EfiCloseProtocol(GraphicsConsole->Handle, &EfiGraphicsOutputProtocol);
    return Status;
}

NTSTATUS
ConsoleEfiGraphicalOpenProtocol (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole,
    _In_ BL_GRAPHICS_CONSOLE_TYPE Type
    )
{
    ULONG HandleIndex, HandleCount;
    EFI_HANDLE* HandleArray;
    EFI_HANDLE Handle;
    NTSTATUS Status;
    PVOID Interface;

    /* Find a device handle that implements either GOP or UGA */
    HandleCount = 0;
    HandleArray = NULL;
    Status = EfiLocateHandleBuffer(ByProtocol,
                                   (Type == BlGopConsole) ?
                                   &EfiGraphicsOutputProtocol :
                                   &EfiUgaDrawProtocol,
                                   &HandleCount,
                                   &HandleArray);
    if (!NT_SUCCESS(Status))
    {
        /* Nothing supports this (no video card?) */
        return STATUS_UNSUCCESSFUL;
    }

    /* Scan through the handles we received */
    for (HandleIndex = 0; HandleCount < HandleIndex; HandleIndex++)
    {
        /* Try to open each one */
        GraphicsConsole->Handle = HandleArray[HandleIndex];
        Handle = HandleArray[HandleIndex];
        if (NT_SUCCESS(EfiOpenProtocol(Handle,
                                       &EfiDevicePathProtocol,
                                      &Interface)))
        {
            /* Test worked, close the protocol */
            EfiCloseProtocol(Handle, &EfiDevicePathProtocol);

            /* Now open the real protocol we want, either UGA or GOP */
            Status = Type ? ConsoleEfiUgaOpen(GraphicsConsole) :
                            ConsoleEfiGopOpen(GraphicsConsole);
            if (NT_SUCCESS(Status))
            {
                /* It worked -- store the type of console this is */
                GraphicsConsole->Type = Type;
                return STATUS_SUCCESS;
            }
        }
    }

    /* We failed to find a working GOP/UGA protocol provider */
    return STATUS_UNSUCCESSFUL;
}

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

BOOLEAN
DsppGraphicsDisabledByBcd (
    VOID
    )
{
    //EarlyPrint(L"Disabling graphics\n");
    return FALSE;
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

NTSTATUS
ConsoleEfiUgaSetResolution  (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole,
    _In_ PBL_DISPLAY_MODE DisplayMode,
    _In_ ULONG DisplayModeCount
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConsoleEfiGopEnable (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    )
{
    PVOID FrameBuffer;
    UINTN CurrentMode, Dummy;
    ULONG Mode, PixelDepth;
    UINTN FrameBufferSize;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION ModeInformation;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* Protocol;
    NTSTATUS Status;
    PHYSICAL_ADDRESS FrameBufferPhysical;

    /* Capture the current mode and protocol */
    Mode = GraphicsConsole->Mode;
    Protocol = GraphicsConsole->Protocol;

    /* Get the current mode and its information */
    Status = EfiGopGetCurrentMode(Protocol, &CurrentMode, &ModeInformation);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Check if we're not in the mode we should be */
    if (CurrentMode != Mode)
    {
        /* Switch modes */
        Status = EfiGopSetMode(Protocol, Mode);
        if (Status < 0)
        {
            return Status;
        }

        /* Reset the OEM bitmap and get the new more information */
//        BlDisplayInvalidateOemBitmap();
        EfiGopGetCurrentMode(Protocol, &Dummy, &ModeInformation);
    }

    /* Get the pixel depth for this mode */
    Status = ConsoleEfiGopGetGraphicalFormat(&ModeInformation, &PixelDepth);
    if (NT_SUCCESS(Status))
    {
        /* Get the framebuffer for this mode */
        EfiGopGetFrameBuffer(Protocol, &FrameBufferPhysical, &FrameBufferSize);

        /* Map the framebuffer, try as writeback first */
        FrameBuffer = NULL;
        Status = BlMmMapPhysicalAddressEx(&FrameBuffer,
                                          BlMemoryWriteBack,
                                          FrameBufferSize,
                                          FrameBufferPhysical);
        if (!NT_SUCCESS(Status))
        {
            /* That didn't work, so try uncached next */
            Status = BlMmMapPhysicalAddressEx(&FrameBuffer,
                                              BlMemoryUncached,
                                              FrameBufferSize,
                                              FrameBufferPhysical);
        }
    }

    /* Check if getting all the required information worked out */
    if (NT_SUCCESS(Status))
    {
        /* Capture the resolution, depth, and framebuffer information */
        GraphicsConsole->DisplayMode.HRes = ModeInformation.HorizontalResolution;
        GraphicsConsole->DisplayMode.VRes = ModeInformation.VerticalResolution;
        GraphicsConsole->DisplayMode.HRes2 = ModeInformation.PixelsPerScanLine;
        GraphicsConsole->PixelDepth = PixelDepth;
        GraphicsConsole->FrameBuffer = FrameBuffer;
        GraphicsConsole->FrameBufferSize = FrameBufferSize;
        GraphicsConsole->PixelsPerScanLine = ModeInformation.PixelsPerScanLine;

        /* All good */
        Status = STATUS_SUCCESS;
    }
    else if (CurrentMode != GraphicsConsole->Mode)
    {
        /* We failed seomewhere, reset the mode and the OEM bitmap back */
        EfiGopSetMode(Protocol, CurrentMode);
        //BlDisplayInvalidateOemBitmap();
    }

    /* Return back to caller */
    return Status;
}

NTSTATUS
ConsoleFirmwareGraphicalEnable (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    )
{
    NTSTATUS Status;

    /* Check what type of console this is */
    if (GraphicsConsole->Type == BlUgaConsole)
    {
        /* Handle UGA */
        Status = ConsoleEfiUgaSetResolution(GraphicsConsole,
                                            &GraphicsConsole->DisplayMode,
                                            1);
    }
    else
    {
        /* Handle GOP */
        Status = ConsoleEfiGopEnable(GraphicsConsole);
    }

    /* Return back to caller */
    return Status;
}

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
        return Status;
    }

    /* But overwrite its callbacks with ours */
    GraphicsConsole->TextConsole.Callbacks = &ConsoleGraphicalVtbl.Text;

    /* Try to create a GOP console */
    Status = ConsoleEfiGraphicalOpenProtocol(GraphicsConsole, BlGopConsole);
    if (!NT_SUCCESS(Status))
    {
        /* That failed, try an older EFI 1.02 UGA console */
        Status = ConsoleEfiGraphicalOpenProtocol(GraphicsConsole, BlUgaConsole);
        if (!NT_SUCCESS(Status))
        {
            /* That failed too, give up */
            ConsoleTextLocalDestruct(&GraphicsConsole->TextConsole);
            return STATUS_UNSUCCESSFUL;
        }
    }

    /* Enable the console */
    Status = ConsoleFirmwareGraphicalEnable(GraphicsConsole);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to enable it, undo everything */
        ConsoleFirmwareGraphicalClose(GraphicsConsole);
        ConsoleTextLocalDestruct(&GraphicsConsole->TextConsole);
        return STATUS_UNSUCCESSFUL;
    }

    /* Save the graphics text color from the text mode text color */
    GraphicsConsole->FgColor = GraphicsConsole->TextConsole.State.FgColor;
    GraphicsConsole->BgColor = GraphicsConsole->TextConsole.State.BgColor;
    return STATUS_SUCCESS;
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
   // PVOID RemoteConsole;
    PBL_TEXT_CONSOLE TextConsole;

    /* Initialize font data */
    BfiCachedStrikeData = 0;
    InitializeListHead(&BfiDeferredListHead);
    InitializeListHead(&BfiFontFileListHead);

    /* Allocate the font rectangle */
   // BfiGraphicsRectangle = BlMmAllocateHeap(0x5A);
    //if (!BfiGraphicsRectangle)
    //{
        //return STATUS_NO_MEMORY;
    //}

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
            ConsoleGraphicalResolutionListFlags |= 2;
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
                    BlMmFreeHeap(GraphicsConsole);
                    GraphicsConsole = NULL;
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
        ConsoleGraphicalResolutionListFlags &= ~3;
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
    //RemoteConsole = NULL;
    if (!TextConsole)
    {
        EarlyPrint(L"Display path not handled\n");
        return STATUS_NOT_SUPPORTED;
    }

    /* Do we have a remote console? */
    if (!DspRemoteInputConsole)
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
