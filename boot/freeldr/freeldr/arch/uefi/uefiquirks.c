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
    /* Surface RT v3.5.500 */
    { "OemkS EFI Sep 10 2012 23:23:31",     UEFI_QUIRK_SKIP_FIRST_GOP },
    /* Surface RT v3.12.500 */
    { "OemkS EFI Dec 5 2012 17:20:58",      UEFI_QUIRK_SKIP_FIRST_GOP },
    /* Surface RT v3.30.500 */
    { "OemkS EFI Aug 16 2013 14:55:40",     UEFI_QUIRK_SKIP_FIRST_GOP },
    /* Surface RT v3.31.500 */
    { "OemkS EFI Jan 24 2014 18:00:42",     UEFI_QUIRK_SKIP_FIRST_GOP },
    /* Lenovo Ideapad Yoga 11 v??? */
    { "Venus EFI Dec  5 2012 23:09:45",     UEFI_QUIRK_SKIP_FIRST_GOP },
    { "Venus EFI Dec 5 2012 23:09:45",      UEFI_QUIRK_SKIP_FIRST_GOP },
    /* Asus VivoTab v??? */
    { "ASUS UEFI Jul 24 2013 18:23:22",     UEFI_QUIRK_SKIP_FIRST_GOP },
    /* Surface 2 v2.6.500 */
    { "Covington EFI Aug 26 2013 19:20:25", UEFI_QUIRK_SKIP_FIRST_GOP },
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
