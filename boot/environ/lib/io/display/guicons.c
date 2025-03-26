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
        (PCONSOLE_REINITIALIZE)ConsoleGraphicalReinitialize,
        ConsoleTextBaseGetTextState,
        (PCONSOLE_SET_TEXT_STATE)ConsoleGraphicalSetTextState,
        NULL, // GetTextResolution
        NULL, // SetTextResolution
        (PCONSOLE_CLEAR_TEXT)ConsoleGraphicalClearText
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
ConsoleGraphicalSetTextState (
    _In_ PBL_GRAPHICS_CONSOLE Console,
    _In_ ULONG Mask,
    _In_ PBL_DISPLAY_STATE TextState
    )
{
    /* Is the text console active? */
    if (Console->TextConsole.Active)
    {
        /* Let it handle that */
        return ConsoleFirmwareTextSetState(&Console->TextConsole,
                                           Mask,
                                           TextState);
    }

    /* Not yet */
    EfiPrintf(L"FFX set not implemented\r\n");
    return STATUS_NOT_IMPLEMENTED;
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

VOID
ConsolepClearBuffer (
    _In_ PUCHAR FrameBuffer,
    _In_ ULONG Width,
    _In_ PUCHAR FillColor,
    _In_ ULONG Height,
    _In_ ULONG ScanlineWidth,
    _In_ ULONG PixelDepth
    )
{
    PUCHAR Scanline, Current, FrameBufferEnd, LineEnd;
    ULONG LineBytes, WidthBytes, BytesPerPixel;

    /* Get the BPP */
    BytesPerPixel = PixelDepth / 8;

    /* Using that, calculate the size of a scan line */
    LineBytes = ScanlineWidth * BytesPerPixel;

    /* And the size of line we'll have to clear */
    WidthBytes = Width * BytesPerPixel;

    /* Allocate a scanline */
    Scanline = BlMmAllocateHeap(WidthBytes);
    if (Scanline)
    {
        /* For each remaining pixel on the scanline */
        Current = Scanline;
        while (Width--)
        {
            /* Copy in the fill color */
            RtlCopyMemory(Current, FillColor, BytesPerPixel);
            Current += BytesPerPixel;
        }

        /* For each scanline in the frame buffer */
        while (Height--)
        {
            /* Copy our constructed scanline */
            RtlCopyMemory(FrameBuffer, Scanline, WidthBytes);
            FrameBuffer += LineBytes;
        }
    }
    else
    {
        FrameBufferEnd = FrameBuffer + Height * LineBytes;
        ScanlineWidth = BytesPerPixel * (ScanlineWidth - Width);
        while (FrameBuffer != FrameBufferEnd)
        {
            if (FrameBuffer != (FrameBuffer + WidthBytes))
            {
                LineEnd = FrameBuffer + WidthBytes;
                do
                {
                    RtlCopyMemory(FrameBuffer, FillColor, BytesPerPixel);
                    FrameBuffer += BytesPerPixel;
                }
                while (FrameBuffer != LineEnd);
            }

            FrameBuffer += ScanlineWidth;
        }
    }
}

NTSTATUS
ConsolepConvertColorToPixel (
    _In_ BL_COLOR Color,
    _Out_ PUCHAR Pixel
    )
{
    NTSTATUS Status;

    /* Assume success */
    Status = STATUS_SUCCESS;

    /* Convert the color to a pixel value */
    switch (Color)
    {
    case Black:
        Pixel[1] = 0;
        Pixel[2] = 0;
        Pixel[0] = 0;
        break;
    case Blue:
        Pixel[1] = 0;
        Pixel[2] = 0;
        Pixel[0] = 0x7F;
        break;
    case Green:
        Pixel[1] = 0x7F;
        Pixel[2] = 0;
        Pixel[0] = 0;
        break;
    case Cyan:
        Pixel[1] = 0x7F;
        Pixel[2] = 0;
        Pixel[0] = 0x7F;
        break;
    case Red:
        Pixel[1] = 0;
        Pixel[2] = 0x7F;
        Pixel[0] = 0x7F;
        break;
    case Magenta:
        Pixel[1] = 0;
        Pixel[2] = 0x7F;
        Pixel[0] = 0x7F;
        break;
    case Brown:
        Pixel[1] = 0x3F;
        Pixel[2] = 0x7F;
        Pixel[0] = 0;
        break;
    case LtGray:
        Pixel[1] = 0xBFu;
        Pixel[2] = 0xBFu;
        *Pixel = 0xBFu;
        break;
    case Gray:
        Pixel[1] = 0x7F;
        Pixel[2] = 0x7F;
        Pixel[0] = 0x7F;
        break;
    case LtBlue:
        Pixel[1] = 0;
        Pixel[2] = 0;
        Pixel[0] = 0xFF;
        break;
    case LtGreen:
        Pixel[1] = 0xFF;
        Pixel[2] = 0;
        Pixel[0] = 0;
        break;
    case LtCyan:
        Pixel[1] = 0xFF;
        Pixel[2] = 0;
        Pixel[0] = 0xFF;
        break;
    case LtRed:
        Pixel[1] = 0;
        Pixel[2] = 0xFF;
        Pixel[0] = 0;
        break;
    case LtMagenta:
        Pixel[1] = 0;
        Pixel[2] = 0xFF;
        Pixel[0] = 0xFF;
        break;
    case Yellow:
        Pixel[1] = 0xFF;
        Pixel[2] = 0xFF;
        Pixel[0] = 0;
        break;
    case White:
        Pixel[1] = 0xFF;
        Pixel[2] = 0xFF;
        Pixel[0] = 0xFF;
        break;
    default:
        Status = STATUS_INVALID_PARAMETER;
        break;
    }
    return Status;
}

NTSTATUS
ConsoleGraphicalClearPixels  (
    _In_ PBL_GRAPHICS_CONSOLE Console,
    _In_ ULONG Color
    )
{
    NTSTATUS Status;

    /* Check if the text console is active */
    if (Console->TextConsole.Active)
    {
        /* We shouldn't be here */
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        /* Clear it in graphics mode */
        Status = ConsoleFirmwareGraphicalClear(Console, Color);
    }

    /* All good */
    return Status;
}

NTSTATUS
ConsoleGraphicalClearText (
    _In_ PBL_GRAPHICS_CONSOLE Console,
    _In_ BOOLEAN LineOnly
    )
{
    /* Is the text console active? */
    if (Console->TextConsole.Active)
    {
        /* Let firmware clear do it */
        return ConsoleFirmwareTextClear(&Console->TextConsole, LineOnly);
    }

    /* Are we clearing a line only? */
    if (LineOnly)
    {
        return BfClearToEndOfLine(Console);
    }

    /* Nope -- the whole screen */
    return BfClearScreen(Console);
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
