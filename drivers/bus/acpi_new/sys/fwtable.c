/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     'ACPI' firmware table provider for GetSystemFirmwareTable
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <debug.h>

/* SystemRegisterFirmwareTableInformationHandler */
#define UACPINT_SYSTEM_REGISTER_FIRMWARE_TABLE_HANDLER  75

/* The 'ACPI' provider, written the way GetSystemFirmwareTable callers pass it */
#define UACPINT_FIRMWARE_PROVIDER_ACPI                  0x41435049

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemInformation(
    _In_ ULONG SystemInformationClass,
    _In_reads_bytes_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength);

typedef struct _UACPINT_TABLE_LIST
{
    PULONG Signatures;  ///< NULL while only counting
    ULONG  Capacity;
    ULONG  Count;
} UACPINT_TABLE_LIST, *PUACPINT_TABLE_LIST;

static BOOLEAN UacpiNtFirmwareTablesRegistered;

/* The DSDT and FACS hang off the FADT, the root table never lists them */
static
BOOLEAN
NTAPI
UacpiNtIsRootListedTable(
    _In_reads_(4) const uacpi_char *Signature)
{
    return !RtlEqualMemory(Signature, ACPI_DSDT_SIGNATURE, 4) &&
           !RtlEqualMemory(Signature, ACPI_FACS_SIGNATURE, 4);
}

static
uacpi_iteration_decision
UacpiNtCollectTableSignature(
    _In_ uacpi_handle User,
    _In_ uacpi_table_info *TableInfo)
{
    PUACPINT_TABLE_LIST List = User;

    if (!UacpiNtIsRootListedTable(TableInfo->signature))
        return UACPI_ITERATION_DECISION_CONTINUE;

    if (List->Signatures && List->Count < List->Capacity)
        RtlCopyMemory(&List->Signatures[List->Count], TableInfo->signature, sizeof(ULONG));

    List->Count++;
    return UACPI_ITERATION_DECISION_CONTINUE;
}

static
NTSTATUS
NTAPI
UacpiNtFirmwareEnumerate(
    _Inout_ PSYSTEM_FIRMWARE_TABLE_INFORMATION TableInfo)
{
    UACPINT_TABLE_LIST List;
    ULONG Required;

    RtlZeroMemory(&List, sizeof(List));
    uacpi_for_each_table(UacpiNtCollectTableSignature, &List);

    if (List.Count == 0)
        return STATUS_NOT_FOUND;

    Required = List.Count * sizeof(ULONG);
    if (TableInfo->TableBufferLength < Required)
    {
        TableInfo->TableBufferLength = Required;
        return STATUS_BUFFER_TOO_SMALL;
    }

    List.Signatures = (PULONG)TableInfo->TableBuffer;
    List.Capacity = List.Count;
    List.Count = 0;
    uacpi_for_each_table(UacpiNtCollectTableSignature, &List);

    TableInfo->TableBufferLength = List.Count * sizeof(ULONG);
    return STATUS_SUCCESS;
}

/* The first table carrying the signature, copied whole */
static
NTSTATUS
NTAPI
UacpiNtFirmwareGet(
    _Inout_ PSYSTEM_FIRMWARE_TABLE_INFORMATION TableInfo)
{
    uacpi_char Signature[4];
    uacpi_table Table;
    NTSTATUS Status;
    ULONG Length;

    RtlCopyMemory(Signature, &TableInfo->TableID, sizeof(Signature));

    if (uacpi_unlikely_error(uacpi_table_find_by_signature(Signature, &Table)))
        return STATUS_NOT_FOUND;

    Length = Table.hdr->length;
    if (TableInfo->TableBufferLength < Length)
    {
        TableInfo->TableBufferLength = Length;
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        RtlCopyMemory(TableInfo->TableBuffer, Table.ptr, Length);
        TableInfo->TableBufferLength = Length;
        Status = STATUS_SUCCESS;
    }

    uacpi_table_unref(&Table);
    return Status;
}

static
NTSTATUS
__cdecl
UacpiNtFirmwareTableHandler(
    _Inout_ PSYSTEM_FIRMWARE_TABLE_INFORMATION TableInfo)
{
    PAGED_CODE();

    switch (TableInfo->Action)
    {
        case SystemFirmwareTable_Enumerate:
            return UacpiNtFirmwareEnumerate(TableInfo);

        case SystemFirmwareTable_Get:
            return UacpiNtFirmwareGet(TableInfo);

        default:
            return STATUS_NOT_IMPLEMENTED;
    }
}

static
NTSTATUS
NTAPI
UacpiNtSetFirmwareTableHandler(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ BOOLEAN Register)
{
    SYSTEM_FIRMWARE_TABLE_HANDLER Handler;

    RtlZeroMemory(&Handler, sizeof(Handler));
    Handler.ProviderSignature = UACPINT_FIRMWARE_PROVIDER_ACPI;
    Handler.Register = Register;
    Handler.FirmwareTableHandler = UacpiNtFirmwareTableHandler;
    Handler.DriverObject = DriverObject;

    return ZwSetSystemInformation(UACPINT_SYSTEM_REGISTER_FIRMWARE_TABLE_HANDLER,
                                  &Handler,
                                  sizeof(Handler));
}

/* Processor drivers read the FADT through this provider and fail to load without it */
VOID
NTAPI
UacpiNtRegisterFirmwareTables(
    _In_ PDRIVER_OBJECT DriverObject)
{
    NTSTATUS Status;

    PAGED_CODE();

    if (UacpiNtFirmwareTablesRegistered)
        return;

    Status = UacpiNtSetFirmwareTableHandler(DriverObject, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: 'ACPI' firmware table provider not registered 0x%lx\n", Status);
        return;
    }

    UacpiNtFirmwareTablesRegistered = TRUE;

#if (NTDDI_VERSION >= NTDDI_WIN8)
    ZwPowerInformation(FirmwareTableInformationRegistered, NULL, 0, NULL, 0);
#endif

    DPRINT("uACPI-NT: 'ACPI' firmware table provider registered\n");
}

VOID
NTAPI
UacpiNtUnregisterFirmwareTables(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    if (!UacpiNtFirmwareTablesRegistered)
        return;

    UacpiNtSetFirmwareTableHandler(DriverObject, FALSE);
    UacpiNtFirmwareTablesRegistered = FALSE;
}
