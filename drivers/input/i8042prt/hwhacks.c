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
#include <dmilib.h>
#include <intrin.h>

#define NDEBUG
#include <debug.h>

const GUID MSSmBios_RawSMBiosTables_GUID = SMBIOS_DATA_GUID;
PVOID i8042SMBiosTables;
ULONG i8042HwFlags;

/* Since Windows 11 24H2 the Hyper-V emulated PS/2 mouse reports vertical
 * movement already in Windows orientation instead of PS/2 orientation,
 * see CORE-20561 */
#define HYPERV_Y_WINDOWS_ORIENTATION_MIN_BUILD 26100

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

    { {{SYS_VENDOR, "Microsoft Corporation"}, {SYS_PRODUCT, "Virtual Machine"}}, FL_INITHACK | FL_MICROSOFT_VM },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Inspiron 6000                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D410                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D430                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D530                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D531                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D600                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D610                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D620                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D630                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D810                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude D820                   "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude E4300                  "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude E4310                  "}}, FL_INITHACK },
    { {{SYS_VENDOR, "Dell Inc."}, {SYS_PRODUCT, "Latitude E6400                  "}}, FL_INITHACK },

};



static
VOID
i8042ParseSMBiosTables(
    _In_reads_bytes_(TableSize) PVOID SMBiosTables,
    _In_ ULONG TableSize)
{
    ULONG i, j;
    PCHAR Strings[ID_STRINGS_MAX] = { 0 };

    ParseSMBiosTables(SMBiosTables, TableSize, Strings);

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
            /* All items matched!
             * Use |= so as not to clobber flags set by other detections */
            i8042HwFlags |= i8042HardwareTable[i].Flags;
            DPRINT("Found match for hw table index %u\n", i);
            break;
        }
    }
}

static
VOID
i8042StoreSMBiosTables(
    _In_reads_bytes_(TableSize) PVOID SMBiosTables,
    _In_ ULONG TableSize)
{
    static UNICODE_STRING mssmbiosKeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\mssmbios");
    static UNICODE_STRING DataName = RTL_CONSTANT_STRING(L"Data");
    static UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"SMBiosData");
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle = NULL, SubKeyHandle = NULL;
    NTSTATUS Status;

    /* Create registry key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &mssmbiosKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwCreateKey(&KeyHandle,
                         KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);

    if (!NT_SUCCESS(Status))
    {
        return;
    }

    /* Create sub key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &DataName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               KeyHandle,
                               NULL);
    Status = ZwCreateKey(&SubKeyHandle,
                         KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);

    if (!NT_SUCCESS(Status))
    {
        ZwClose(KeyHandle);
        return;
    }

    /* Write value */
    ZwSetValueKey(SubKeyHandle,
                  &ValueName,
                  0,
                  REG_BINARY,
                  SMBiosTables,
                  TableSize);

    ZwClose(SubKeyHandle);
    ZwClose(KeyHandle);
}

static
BOOLEAN
i8042IsHyperVYAxisWindowsOriented(
    VOID)
{
#if defined(_M_IX86) || defined(_M_AMD64)
    INT CpuInfo[4];
    ULONG MaxHvLeaf;

    /* Check if we are running under a hypervisor */
    __cpuid(CpuInfo, 1);
    if (!(CpuInfo[2] & 0x80000000))
        return FALSE;

    /* Check for the Hyper-V signature "Microsoft Hv" */
    __cpuid(CpuInfo, 0x40000000);
    MaxHvLeaf = (ULONG)CpuInfo[0];
    DPRINT1("Hypervisor CPUID: max leaf 0x%lx, signature '%.4s%.4s%.4s'\n",
            MaxHvLeaf, (PCHAR)&CpuInfo[1], (PCHAR)&CpuInfo[2], (PCHAR)&CpuInfo[3]);
    if (CpuInfo[1] != 0x7263694D || /* "Micr" */
        CpuInfo[2] != 0x666F736F || /* "osof" */
        CpuInfo[3] != 0x76482074)   /* "t Hv" */
        return FALSE;

    if (MaxHvLeaf < 0x40000002)
        return FALSE;

    /* Hypervisor system identity: EAX holds the host OS build number */
    __cpuid(CpuInfo, 0x40000002);
    DPRINT1("Hyper-V interface reports host build %lu\n", (ULONG)CpuInfo[0]);
    return (ULONG)CpuInfo[0] >= HYPERV_Y_WINDOWS_ORIENTATION_MIN_BUILD;
#else
    return FALSE;
#endif
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
        ExFreePoolWithTag(AllData, 'BTMS');
        return;
    }

    /* FIXME: This function should be removed once the mssmbios driver is implemented */
    /* Store SMBios data in registry */
    i8042StoreSMBiosTables(AllData + 1,
                           AllData->FixedInstanceSize);
    DPRINT1("SMBiosTables HACK, see CORE-14867\n");

    /* Parse the table */
    i8042ParseSMBiosTables(AllData + 1,
                           AllData->WnodeHeader.BufferSize);

    /* Free the buffer */
    ExFreePoolWithTag(AllData, 'BTMS');

    /* The Y-axis workaround requires both the Hyper-V SMBIOS identity and
     * an affected host build reported via the CPUID hypervisor interface.
     * The SMBIOS check keeps other hypervisors that expose the Hyper-V
     * CPUID interface (e.g. VMware/VirtualBox running on top of the
     * Windows Hypervisor Platform) unaffected. */
    if ((i8042HwFlags & FL_MICROSOFT_VM) && i8042IsHyperVYAxisWindowsOriented())
    {
        DPRINT1("Hyper-V host reports Windows-oriented PS/2 Y-axis, skipping Y negation\n");
        i8042HwFlags |= FL_HYPERV_SKIP_Y_NEGATION;
    }
}

