/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Function stubs
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <uefildr.h>

extern EFI_SYSTEM_TABLE* GlobalSystemTable;

#ifndef _M_ARM
/* TODO: Handle this with custom Disk / partition setup */
UCHAR
DriveMapGetBiosDriveNumber(PCSTR DeviceName)
{
    return 0;
}
#endif

VOID
StallExecutionProcessor(ULONG Microseconds)
{
    if (GlobalSystemTable && GlobalSystemTable->BootServices)
    {
        GlobalSystemTable->BootServices->Stall(Microseconds);
    }
}

VOID
UefiVideoGetFontsFromFirmware(PULONG RomFontPointers)
{

}

VOID
UefiVideoSync(VOID)
{

}

VOID
UefiGetExtendedBIOSData(PULONG ExtendedBIOSDataArea,
                        PULONG ExtendedBIOSDataSize)
{

}

VOID
UefiPcBeep(VOID)
{
    /* Not possible on UEFI, for now */
}

VOID
UefiHwIdle(VOID)
{
    /* Use 1ms delay for idle to prevent CPU spinning */
    StallExecutionProcessor(1000);
}
