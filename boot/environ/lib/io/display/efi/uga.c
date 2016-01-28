/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/display/efi/uga.c
 * PURPOSE:         Boot Library EFI UGA Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

/* FUNCTIONS *****************************************************************/

NTSTATUS
ConsoleEfiUgaOpen (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    )
{
    EfiPrintf(L"UGA not implemented\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

VOID
ConsoleEfiUgaClose (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    )
{
    return;
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
