/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UEFI "quirks" checker implementation
 * COPYRIGHT:   Copyright 2026 Leandro Friedrich <leandro@works-on-my.computer>
 */

#include <uefildr.h>
#include <uefiquirks.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

extern EFI_SYSTEM_TABLE *GlobalSystemTable;

typedef struct _UEFI_MACHINE_QUIRKS
{
    PCSTR FirmwareVendorPrefix;
    ULONG QuirkFlags;
} UEFI_MACHINE_QUIRKS, *PUEFI_MACHINE_QUIRKS;

static const UEFI_MACHINE_QUIRKS UefiMachineQuirks[] =
{
};

static
BOOLEAN
UefiFirmwareVendorIs(
    _In_ PCSTR Prefix)
{
    PCWSTR Vendor = GlobalSystemTable->FirmwareVendor;
    ULONG Index;

    if (Vendor == NULL)
        return FALSE;

    for (Index = 0; Prefix[Index] != 0; Index++)
    {
        if (Vendor[Index] != (WCHAR)Prefix[Index])
            return FALSE;
    }
    return TRUE;
}

ULONG
UefiGetMachineQuirks(VOID)
{
    ULONG Index;
    TRACE("Checking firmware vendor against known list\n");
    for (Index = 0; Index < RTL_NUMBER_OF(UefiMachineQuirks); Index++)
    {
        if (UefiFirmwareVendorIs(UefiMachineQuirks[Index].FirmwareVendorPrefix))
        {
            TRACE("Firmware vendor '%s' matched with quirks 0x%lx\n",
                  UefiMachineQuirks[Index].FirmwareVendorPrefix,
                  UefiMachineQuirks[Index].QuirkFlags);
            return UefiMachineQuirks[Index].QuirkFlags;
        }
    }

    TRACE("Machine not known to be quirky\n");
    return 0;
}
