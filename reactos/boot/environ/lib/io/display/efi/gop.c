/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/display/efi/gop.c
 * PURPOSE:         Boot Library EFI GOP Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

/* FUNCTIONS *****************************************************************/

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
        EarlyPrint(L"GOP OPEN failed: %lx\n", Status);
        return STATUS_NOT_SUPPORTED;
    }

    /* Get the current mode */
    Status = EfiGopGetCurrentMode(GopProtocol, &CurrentMode, &ModeInformation);
    if (!NT_SUCCESS(Status))
    {
        EarlyPrint(L"GOP mode failed: %lx\n", Status);
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
    EarlyPrint(L"Get format failed: %lx\n", Status);
    EfiCloseProtocol(GraphicsConsole->Handle, &EfiGraphicsOutputProtocol);
    return Status;
}

