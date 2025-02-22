/*
 * PROJECT:     ReactOS DMI/SMBIOS Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dmilib.c
 * PURPOSE:     SMBIOS table parsing functions
 * PROGRAMMERS: Timo Kreuzer (timo.kreuzer@reactos.org)
 * REFERENCES:  http://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.0.0.pdf
 */

#include "precomp.h"

static
PCHAR
GetDmiString(
    _In_ PDMI_HEADER Header,
    _In_ ULONG FieldOffset)
{
    ULONG StringIndex;
    PCHAR String;

    StringIndex = ((PUCHAR)Header)[FieldOffset];
    if (StringIndex == 0)
    {
        return NULL;
    }

    String = (PCHAR)Header + Header->Length;

    while (--StringIndex != 0)
    {
        while (*String != 0)
            String++;

        String++;
    }

    return String;
}

VOID
ParseSMBiosTables(
    _In_reads_bytes_(TableSize) PVOID SMBiosTables,
    _In_ ULONG TableSize,
    _Inout_updates_(ID_STRINGS_MAX) PCHAR * Strings)
{
    PMSSmBios_RawSMBiosTables BiosTablesHeader = SMBiosTables;
    PDMI_HEADER Header;
    ULONG Remaining;
    PCHAR Data;

    Header = (PDMI_HEADER)(&BiosTablesHeader->SMBiosData);
    Remaining = BiosTablesHeader->Size;

    while (Remaining >= sizeof(*Header))
    {
        if (Header->Type == DMI_ENTRY_END_OF_TABLE)
            break;

        switch (Header->Type)
        {
        case DMI_ENTRY_BIOS:
            if (Remaining < DMI_BIOS_SIZE)
                return;
            Strings[BIOS_VENDOR] = GetDmiString(Header, DMI_BIOS_VENDOR);
            Strings[BIOS_VERSION] = GetDmiString(Header, DMI_BIOS_VERSION);
            Strings[BIOS_DATE] = GetDmiString(Header, DMI_BIOS_DATE);
            break;

        case DMI_ENTRY_SYSTEM:
            if (Remaining < DMI_SYS_SIZE)
                return;
            Strings[SYS_VENDOR] = GetDmiString(Header, DMI_SYS_VENDOR);
            Strings[SYS_PRODUCT] = GetDmiString(Header, DMI_SYS_PRODUCT);
            Strings[SYS_VERSION] = GetDmiString(Header, DMI_SYS_VERSION);
            Strings[SYS_SERIAL] = GetDmiString(Header, DMI_SYS_SERIAL);
            Strings[SYS_SKU] = GetDmiString(Header, DMI_SYS_SKU);
            Strings[SYS_FAMILY] = GetDmiString(Header, DMI_SYS_FAMILY);
            break;

        case DMI_ENTRY_BASEBOARD:
            if (Remaining < DMI_BOARD_SIZE)
                return;
            Strings[BOARD_VENDOR] = GetDmiString(Header, DMI_BOARD_VENDOR);
            Strings[BOARD_NAME] = GetDmiString(Header, DMI_BOARD_NAME);
            Strings[BOARD_VERSION] = GetDmiString(Header, DMI_BOARD_VERSION);
            Strings[BOARD_SERIAL] = GetDmiString(Header, DMI_BOARD_SERIAL);
            Strings[BOARD_ASSET_TAG] = GetDmiString(Header, DMI_BOARD_ASSET_TAG);
            break;

        case DMI_ENTRY_CHASSIS:
        case DMI_ENTRY_ONBOARD_DEVICE:
        case DMI_ENTRY_OEMSTRINGS:
        // DMI_ENTRY_IPMI_DEV?
        // DMI_ENTRY_ONBOARD_DEV_EXT?
            break;
        }

        Remaining -= Header->Length;
        Data = (PCHAR)Header + Header->Length;

        /* Now loop until we find 2 zeroes */
        while ((Remaining >= 2) && ((Data[0] != 0) || (Data[1] != 0)))
        {
            Data++;
            Remaining--;
        }

        if (Remaining < 2)
            break;

        /* Go to the next header */
        Remaining -= 2;
        Header = (PDMI_HEADER)((PUCHAR)Data + 2);
    }
}
