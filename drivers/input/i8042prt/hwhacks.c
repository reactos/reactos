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

#define NDEBUG
#include <debug.h>

const GUID MSSmBios_RawSMBiosTables_GUID = SMBIOS_DATA_GUID;
PVOID i8042SMBiosTables;

static
VOID
i8042ParseSMBiosTables(
    _In_reads_bytes_(TableSize) PVOID SMBiosTables,
    _In_ ULONG TableSize)
{
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
}

