/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/display/efi/guicons.c
 * PURPOSE:         Boot Library EFI GUI Console Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

/* FUNCTIONS *****************************************************************/

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
        EfiPrintf(L"Status: %lx Count: %d\r\n", Status, HandleCount);
        return STATUS_UNSUCCESSFUL;
    }

    /* Scan through the handles we received */
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++)
    {
        /* Try to open each one */
        GraphicsConsole->Handle = HandleArray[HandleIndex];
        Handle = HandleArray[HandleIndex];
        Status = EfiOpenProtocol(Handle, &EfiDevicePathProtocol, &Interface);
        if (NT_SUCCESS(Status))
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
ConsoleFirmwareGraphicalClear (
    _In_ PBL_GRAPHICS_CONSOLE Console,
    _In_ ULONG Color
    )
{
    NTSTATUS Status;
    UCHAR Pixel[4] = { 0 };

    /* Convert the standard color to a firmware pixel color */
    Status = ConsolepConvertColorToPixel(Color, Pixel);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Check if this is GOP or UGA */
    if (Console->Type == BlUgaConsole)
    {
        EfiPrintf(L"Uga not supported\r\n");
        Status = STATUS_NOT_IMPLEMENTED;
    }
    else
    {
        /* For GOP, just fill the screen */
        ConsolepClearBuffer(Console->FrameBuffer,
                            Console->DisplayMode.HRes,
                            Pixel,
                            Console->DisplayMode.VRes,
                            Console->PixelsPerScanLine,
                            Console->PixelDepth);
        Status = STATUS_SUCCESS;
    }

    /* All clear */
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

VOID
ConsoleFirmwareGraphicalDisable (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    )
{
    /* Is this a GOP console? */
    if (GraphicsConsole->Type == BlGopConsole)
    {
        /* Did we map a framebuffer? */
        if (GraphicsConsole->FrameBuffer)
        {
            /* Unmap it */
            BlMmUnmapVirtualAddressEx(GraphicsConsole->FrameBuffer,
                                      GraphicsConsole->FrameBufferSize);
        }
    }
}
