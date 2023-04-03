/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Utils source
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <uefildr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

/* GLOBALS ********************************************************************/

extern EFI_SYSTEM_TABLE *GlobalSystemTable;

/* FUNCTIONS ******************************************************************/

TIMEINFO*
UefiGetTime(VOID)
{
    static TIMEINFO TimeInfo;
    EFI_STATUS Status;
    EFI_TIME time = {0};

    Status = GlobalSystemTable->RuntimeServices->GetTime(&time, NULL);
    if (Status != EFI_SUCCESS)
        ERR("UefiGetTime: cannot get time status %d\n", Status);

    TimeInfo.Year = time.Year;
    TimeInfo.Month = time.Month;
    TimeInfo.Day = time.Day;
    TimeInfo.Hour = time.Hour;
    TimeInfo.Minute = time.Minute;
    TimeInfo.Second = time.Second;
    return &TimeInfo;
}

