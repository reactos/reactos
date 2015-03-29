/*
 * PROJECT:     ReactOS i8042 (ps/2 keyboard-mouse controller) driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/i8042prt/hwhacks.c
 * PURPOSE:     Mouse specific functions
 * PROGRAMMERS: Timo Kreuzer (timo.kreuzer@reactos.org)
 * REFERENCES:  - http://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.0.0.pdf
 *              -
 */

#include "i8042prt.h"
#include <wmiguid.h>
#include <wmidata.h>
#include <wmistr.h>
#include "dmi.h"

#define NDEBUG
#include <debug.h>

const GUID MSSmBios_RawSMBiosTables_GUID = SMBIOS_DATA_GUID;
PVOID i8042SMBiosTables;
ULONG i8042HwFlags;

enum _ID_STRINGS
{
    ID_NONE = 0,
    BIOS_VENDOR,
    BIOS_VERSION,
    BIOS_DATE,
    SYS_VENDOR,
    SYS_PRODUCT,
    SYS_VERSION,
    SYS_SERIAL,
    BOARD_VENDOR,
    BOARD_NAME,
    BOARD_VERSION,
    BOARD_SERIAL,
    BOARD_ASSET_TAG,


    ID_STRINGS_MAX,
};

typedef struct _MATCHENTRY
{
    ULONG Type;
    PCHAR String;
} MATCHENTRY;

#define MAX_MATCH_ENTRIES 3
typedef struct _HARDWARE_TABLE
{
    MATCHENTRY MatchEntries[MAX_MATCH_ENTRIES];
    ULONG Flags;
} HARDWARE_TABLE;

const HARDWARE_TABLE i8042HardwareTable[] =
{
//    { {{BOARD_VENDOR, "RIOWORKS"}, {BOARD_NAME, "HDAMB"}, {BOARD_VERSION, "Rev E"}}, FL_NOLOOP },
//    { {{BOARD_VENDOR, "ASUSTeK Computer Inc."}, {BOARD_NAME, "G1S"}, {BOARD_VERSION, "1.0"}}, FL_NOLOOP },

    { {{SYS_VENDOR, "Microsoft Corporation"}, {SYS_PRODUCT, "Virtual Machine"}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D530                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D531                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D600                   "}}, FL_INITHACK },

};



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


static
VOID
i8042ParseSMBiosTables(
    _In_reads_bytes_(TableSize) PVOID SMBiosTables,
    _In_ ULONG TableSize)
{
    PMSSmBios_RawSMBiosTables BiosTablesHeader = SMBiosTables;
    PDMI_HEADER Header;
    ULONG Remaining, i, j;
    PCHAR Data;
    PCHAR Strings[ID_STRINGS_MAX] = { 0 };

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

#if 0 // DBG
    DbgPrint("i8042prt: Dumping DMI data:\n");
    DbgPrint("BIOS_VENDOR: %s\n", Strings[BIOS_VENDOR]);
    DbgPrint("BIOS_VERSION: %s\n", Strings[BIOS_VERSION]);
    DbgPrint("BIOS_DATE: %s\n", Strings[BIOS_DATE]);
    DbgPrint("SYS_VENDOR: %s\n", Strings[SYS_VENDOR]);
    DbgPrint("SYS_PRODUCT: %s\n", Strings[SYS_PRODUCT]);
    DbgPrint("SYS_VERSION: %s\n", Strings[SYS_VERSION]);
    DbgPrint("SYS_SERIAL: %s\n", Strings[SYS_SERIAL]);
    DbgPrint("BOARD_VENDOR: %s\n", Strings[BOARD_VENDOR]);
    DbgPrint("BOARD_NAME: %s\n", Strings[BOARD_NAME]);
    DbgPrint("BOARD_VERSION: %s\n", Strings[BOARD_VERSION]);
    DbgPrint("BOARD_SERIAL: %s\n", Strings[BOARD_SERIAL]);
    DbgPrint("BOARD_ASSET_TAG: %s\n", Strings[BOARD_ASSET_TAG]);
#endif

    /* Now loop the hardware table to find a match */
    for (i = 0; i < ARRAYSIZE(i8042HardwareTable); i++)
    {
        for (j = 0; j < MAX_MATCH_ENTRIES; j++)
        {
            ULONG Type = i8042HardwareTable[i].MatchEntries[j].Type;

            if (Type != ID_NONE)
            {
                /* Check for a match */
                if ((Strings[Type] == NULL) ||
                    strcmp(i8042HardwareTable[i].MatchEntries[j].String,
                           Strings[i8042HardwareTable[i].MatchEntries[j].Type]))
                {
                    /* Does not match, try next entry */
                    break;
                }
            }
        }

        if (j == MAX_MATCH_ENTRIES)
        {
            /* All items matched! */
            i8042HwFlags = i8042HardwareTable[i].Flags;
            DPRINT("Found match for hw table index %u\n", i);
            break;
        }
    }
}

VOID
NTAPI
i8042InitializeHwHacks(
    VOID)
{
    NTSTATUS Status;
    PVOID DataBlockObject;
    PWNODE_ALL_DATA AllData;
    ULONG BufferSize;

    /* Open the data block object for the SMBIOS table */
    Status = IoWMIOpenBlock(&MSSmBios_RawSMBiosTables_GUID,
                            WMIGUID_QUERY,
                            &DataBlockObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoWMIOpenBlock failed: 0x%08lx\n", Status);
        return;
    }

    /* Query the required buffer size */
    BufferSize = 0;
    Status = IoWMIQueryAllData(DataBlockObject, &BufferSize, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoWMIOpenBlock failed: 0x%08lx\n", Status);
        return;
    }

    AllData = ExAllocatePoolWithTag(PagedPool, BufferSize, 'BTMS');
    if (AllData == NULL)
    {
        DPRINT1("Failed to allocate %lu bytes for SMBIOS tables\n", BufferSize);
        return;
    }

    /* Query the buffer data */
    Status = IoWMIQueryAllData(DataBlockObject, &BufferSize, AllData);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoWMIOpenBlock failed: 0x%08lx\n", Status);
        return;
    }

    /* Parse the table */
    i8042ParseSMBiosTables(AllData + 1,
                           AllData->WnodeHeader.BufferSize);

    /* Free the buffer */
    ExFreePoolWithTag(AllData, 'BTMS');
}

