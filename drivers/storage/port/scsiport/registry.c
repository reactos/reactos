/*
 * PROJECT:     ReactOS Storage Stack / SCSIPORT storage port library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Registry operations
 * COPYRIGHT:   Eric Kohl (eric.kohl@reactos.org)
 *              Aleksey Bragin (aleksey@reactos.org)
 *              2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include "scsiport.h"
#include "scsitypes.h"

#define NDEBUG
#include <debug.h>


VOID
SpiInitOpenKeys(
    _Inout_ PCONFIGURATION_INFO ConfigInfo,
    _In_ PSCSI_PORT_DRIVER_EXTENSION DriverExtension)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    NTSTATUS Status;
    HANDLE parametersKey;

    DriverExtension->IsLegacyDriver = TRUE;

    /* Open the service key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &DriverExtension->RegistryPath,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&ConfigInfo->ServiceKey, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Unable to open driver's registry key %wZ, status 0x%08x\n",
               DriverExtension->RegistryPath, Status);
        ConfigInfo->ServiceKey = NULL;
    }

    /* If we could open driver's service key, then proceed to the Parameters key */
    if (ConfigInfo->ServiceKey != NULL)
    {
        RtlInitUnicodeString(&KeyName, L"Parameters");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   ConfigInfo->ServiceKey,
                                   NULL);

        /* Try to open it */
        Status = ZwOpenKey(&ConfigInfo->DeviceKey, KEY_READ, &ObjectAttributes);

        if (NT_SUCCESS(Status))
        {
            /* Yes, Parameters key exist, and it must be used instead of
               the Service key */
            ZwClose(ConfigInfo->ServiceKey);
            ConfigInfo->ServiceKey = ConfigInfo->DeviceKey;
            ConfigInfo->DeviceKey = NULL;
        }
    }

    if (ConfigInfo->ServiceKey != NULL)
    {
        /* Open the Device key */
        RtlInitUnicodeString(&KeyName, L"Device");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   ConfigInfo->ServiceKey,
                                   NULL);

        /* We don't check for failure here - not needed */
        ZwOpenKey(&ConfigInfo->DeviceKey, KEY_READ, &ObjectAttributes);

        // Detect the driver PnP capabilities via its Parameters\PnpInterface key
        // for example: HKLM\SYSTEM\CurrentControlSet\Services\UNIATA\Parameters\PnpInterface

        RtlInitUnicodeString(&KeyName, L"PnpInterface");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   ConfigInfo->ServiceKey,
                                   NULL);

        Status = ZwOpenKey(&parametersKey, KEY_READ, &ObjectAttributes);

        if (NT_SUCCESS(Status))
        {
            // if the key exists, it's enough for us for now
            // (the proper check should iterate over INTERFACE_TYPE values)
            DriverExtension->IsLegacyDriver = FALSE;
            ZwClose(parametersKey);
        }
    }
}

/**********************************************************************
 * NAME                         INTERNAL
 *  SpiBuildDeviceMap
 *
 * DESCRIPTION
 *  Builds the registry device map of all device which are attached
 *  to the given SCSI HBA port. The device map is located at:
 *    \Registry\Machine\DeviceMap\Scsi
 *
 * RUN LEVEL
 *  PASSIVE_LEVEL
 *
 * ARGUMENTS
 *  DeviceExtension
 *      ...
 *
 *  RegistryPath
 *      Name of registry driver service key.
 *
 * RETURNS
 *  NTSTATUS
 */

NTSTATUS
RegistryInitAdapterKey(
    _Inout_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    WCHAR NameBuffer[64];
    HANDLE ScsiKey;
    HANDLE ScsiPortKey = NULL;
    HANDLE ScsiBusKey = NULL;
    HANDLE ScsiInitiatorKey = NULL;
    ULONG BusNumber;
    ULONG UlongData;
    NTSTATUS Status;

    DPRINT("SpiBuildDeviceMap() called\n");

    if (DeviceExtension == NULL)
    {
        DPRINT1("Invalid parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Open or create the 'Scsi' subkey */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_KERNEL_HANDLE,
                               0,
                               NULL);
    Status = ZwCreateKey(&ScsiKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Create new 'Scsi Port X' subkey */
    DPRINT("Scsi Port %lu\n", DeviceExtension->PortNumber);

    swprintf(NameBuffer,
             L"Scsi Port %lu",
             DeviceExtension->PortNumber);
    RtlInitUnicodeString(&KeyName, NameBuffer);
    InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_KERNEL_HANDLE, ScsiKey, NULL);
    Status = ZwCreateKey(&ScsiPortKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    ZwClose(ScsiKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
        return Status;
    }

    /*
     * Create port-specific values
     */

    /* Set 'DMA Enabled' (REG_DWORD) value */
    UlongData = (ULONG)!DeviceExtension->PortCapabilities.AdapterUsesPio;
    DPRINT("  DMA Enabled = %s\n", UlongData ? "TRUE" : "FALSE");
    RtlInitUnicodeString(&ValueName, L"DMA Enabled");
    Status = ZwSetValueKey(ScsiPortKey,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &UlongData,
                           sizeof(UlongData));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwSetValueKey('DMA Enabled') failed (Status %lx)\n", Status);
        ZwClose(ScsiPortKey);
        return Status;
    }

    /* Set 'Driver' (REG_SZ) value */
    PUNICODE_STRING driverNameU = &DeviceExtension->Common.DeviceObject->DriverObject->DriverName;
    PWCHAR driverName = ExAllocatePoolWithTag(PagedPool,
                                              driverNameU->Length + sizeof(UNICODE_NULL),
                                              TAG_SCSIPORT);
    if (!driverName)
    {
        DPRINT("Failed to allocate driverName!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(driverName, driverNameU->Buffer, driverNameU->Length);
    driverName[driverNameU->Length / sizeof(WCHAR)] = UNICODE_NULL;

    RtlInitUnicodeString(&ValueName, L"Driver");
    Status = ZwSetValueKey(ScsiPortKey,
                           &ValueName,
                           0,
                           REG_SZ,
                           driverName,
                           driverNameU->Length + sizeof(UNICODE_NULL));

    ExFreePoolWithTag(driverName, TAG_SCSIPORT);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwSetValueKey('Driver') failed (Status %lx)\n", Status);
        ZwClose(ScsiPortKey);
        return Status;
    }

    /* Set 'Interrupt' (REG_DWORD) value (NT4 only) */
    UlongData = (ULONG)DeviceExtension->PortConfig->BusInterruptLevel;
    DPRINT("  Interrupt = %lu\n", UlongData);
    RtlInitUnicodeString(&ValueName, L"Interrupt");
    Status = ZwSetValueKey(ScsiPortKey,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &UlongData,
                           sizeof(UlongData));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwSetValueKey('Interrupt') failed (Status %lx)\n", Status);
        ZwClose(ScsiPortKey);
        return Status;
    }

    /* Set 'IOAddress' (REG_DWORD) value (NT4 only) */
    UlongData = ScsiPortConvertPhysicalAddressToUlong((*DeviceExtension->PortConfig->AccessRanges)[0].RangeStart);
    DPRINT("  IOAddress = %lx\n", UlongData);
    RtlInitUnicodeString(&ValueName, L"IOAddress");
    Status = ZwSetValueKey(ScsiPortKey,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &UlongData,
                           sizeof(UlongData));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwSetValueKey('IOAddress') failed (Status %lx)\n", Status);
        ZwClose(ScsiPortKey);
        return Status;
    }

    /* Enumerate buses */
    for (BusNumber = 0; BusNumber < DeviceExtension->NumberOfBuses; BusNumber++)
    {
        /* Create 'Scsi Bus X' key */
        DPRINT("    Scsi Bus %lu\n", BusNumber);
        swprintf(NameBuffer,
                 L"Scsi Bus %lu",
                 BusNumber);
        RtlInitUnicodeString(&KeyName, NameBuffer);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_KERNEL_HANDLE,
                                   ScsiPortKey,
                                   NULL);
        Status = ZwCreateKey(&ScsiBusKey,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
            goto ByeBye;
        }

        /* Create 'Initiator Id X' key */
        DPRINT("      Initiator Id %lu\n",
               DeviceExtension->PortConfig->InitiatorBusId[BusNumber]);
        swprintf(NameBuffer,
                 L"Initiator Id %lu",
                 (ULONG)(UCHAR)DeviceExtension->PortConfig->InitiatorBusId[BusNumber]);
        RtlInitUnicodeString(&KeyName, NameBuffer);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_KERNEL_HANDLE,
                                   ScsiBusKey,
                                   NULL);
        Status = ZwCreateKey(&ScsiInitiatorKey,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
            goto ByeBye;
        }

        /* FIXME: Are there any initiator values (??) */

        ZwClose(ScsiInitiatorKey);
        ScsiInitiatorKey = NULL;

        DeviceExtension->Buses[BusNumber].RegistryMapKey = ScsiBusKey;
        ScsiBusKey = NULL;
    }

ByeBye:
    if (ScsiInitiatorKey != NULL)
        ZwClose(ScsiInitiatorKey);

    if (ScsiBusKey != NULL)
        ZwClose(ScsiBusKey);

    if (ScsiPortKey != NULL)
        ZwClose(ScsiPortKey);

    DPRINT("SpiBuildDeviceMap() done\n");

    return Status;
}

NTSTATUS
RegistryInitLunKey(
    _Inout_ PSCSI_PORT_LUN_EXTENSION LunExtension)
{
    WCHAR nameBuffer[64];
    UNICODE_STRING keyName;
    UNICODE_STRING valueName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE targetKey;
    NTSTATUS status;

    // get the LUN's bus key
    PSCSI_PORT_DEVICE_EXTENSION portExt = LunExtension->Common.LowerDevice->DeviceExtension;
    HANDLE busKey = portExt->Buses[LunExtension->PathId].RegistryMapKey;

    // create/open 'Target Id X' key
    swprintf(nameBuffer, L"Target Id %lu", LunExtension->TargetId);
    RtlInitUnicodeString(&keyName, nameBuffer);
    InitializeObjectAttributes(&objectAttributes, &keyName, OBJ_KERNEL_HANDLE, busKey, NULL);
    status = ZwCreateKey(&targetKey,
                         KEY_ALL_ACCESS,
                         &objectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    if (!NT_SUCCESS(status))
    {
        DPRINT("ZwCreateKey() failed (Status %lx)\n", status);
        return status;
    }

    // Create 'Logical Unit Id X' key
    swprintf(nameBuffer, L"Logical Unit Id %lu", LunExtension->Lun);
    RtlInitUnicodeString(&keyName, nameBuffer);
    InitializeObjectAttributes(&objectAttributes, &keyName, OBJ_KERNEL_HANDLE, targetKey, NULL);
    status = ZwCreateKey(&LunExtension->RegistryMapKey,
                         KEY_ALL_ACCESS,
                         &objectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    if (!NT_SUCCESS(status))
    {
        DPRINT("ZwCreateKey() failed (Status %lx)\n", status);
        goto ByeBye;
    }

    // Set 'Identifier' (REG_SZ) value
    swprintf(nameBuffer,
             L"%.8S%.16S%.4S",
             LunExtension->InquiryData.VendorId,
             LunExtension->InquiryData.ProductId,
             LunExtension->InquiryData.ProductRevisionLevel);
    RtlInitUnicodeString(&valueName, L"Identifier");
    status = ZwSetValueKey(LunExtension->RegistryMapKey,
                           &valueName,
                           0,
                           REG_SZ,
                           nameBuffer,
                           (wcslen(nameBuffer) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(status))
    {
        DPRINT("ZwSetValueKey('Identifier') failed (Status %lx)\n", status);
        goto ByeBye;
    }

    // Set 'Type' (REG_SZ) value
    PWCHAR typeName = (PWCHAR)GetPeripheralTypeW(&LunExtension->InquiryData);
    DPRINT("          Type = '%S'\n", typeName);
    RtlInitUnicodeString(&valueName, L"Type");
    status = ZwSetValueKey(LunExtension->RegistryMapKey,
                           &valueName,
                           0,
                           REG_SZ,
                           typeName,
                           (wcslen(typeName) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(status))
    {
        DPRINT("ZwSetValueKey('Type') failed (Status %lx)\n", status);
        goto ByeBye;
    }

    // Set 'InquiryData' (REG_BINARY) value
    RtlInitUnicodeString(&valueName, L"InquiryData");
    status = ZwSetValueKey(LunExtension->RegistryMapKey,
                           &valueName,
                           0,
                           REG_BINARY,
                           &LunExtension->InquiryData,
                           INQUIRYDATABUFFERSIZE);
    if (!NT_SUCCESS(status))
    {
        DPRINT("ZwSetValueKey('InquiryData') failed (Status %lx)\n", status);
        goto ByeBye;
    }

ByeBye:
    ZwClose(targetKey);
    // TODO: maybe we will need it in future
    ZwClose(LunExtension->RegistryMapKey);

    return status;
}
