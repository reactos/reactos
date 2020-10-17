/*
 * PROJECT:     ReactOS Storage Stack / SCSIPORT storage port library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Registry operations
 * COPYRIGHT:   Eric Kohl (eric.kohl@reactos.org)
 *              Aleksey Bragin (aleksey@reactos.org)
 */

#include "scsiport.h"

#define NDEBUG
#include <debug.h>


VOID
SpiInitOpenKeys(
    _Inout_ PCONFIGURATION_INFO ConfigInfo,
    _In_ PUNICODE_STRING RegistryPath)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    NTSTATUS Status;

    /* Open the service key */
    InitializeObjectAttributes(&ObjectAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&ConfigInfo->ServiceKey,
                       KEY_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Unable to open driver's registry key %wZ, status 0x%08x\n", RegistryPath, Status);
        ConfigInfo->ServiceKey = NULL;
    }

    /* If we could open driver's service key, then proceed to the Parameters key */
    if (ConfigInfo->ServiceKey != NULL)
    {
        RtlInitUnicodeString(&KeyName, L"Parameters");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   ConfigInfo->ServiceKey,
                                   (PSECURITY_DESCRIPTOR) NULL);

        /* Try to open it */
        Status = ZwOpenKey(&ConfigInfo->DeviceKey,
                           KEY_READ,
                           &ObjectAttributes);

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
                                   OBJ_CASE_INSENSITIVE,
                                   ConfigInfo->ServiceKey,
                                   NULL);

        /* We don't check for failure here - not needed */
        ZwOpenKey(&ConfigInfo->DeviceKey,
                  KEY_READ,
                  &ObjectAttributes);
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
SpiBuildDeviceMap(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PUNICODE_STRING RegistryPath)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    WCHAR NameBuffer[64];
    ULONG Disposition;
    HANDLE ScsiKey;
    HANDLE ScsiPortKey = NULL;
    HANDLE ScsiBusKey = NULL;
    HANDLE ScsiInitiatorKey = NULL;
    HANDLE ScsiTargetKey = NULL;
    HANDLE ScsiLunKey = NULL;
    ULONG BusNumber;
    ULONG Target;
    ULONG CurrentTarget;
    ULONG Lun;
    PWCHAR DriverName;
    ULONG UlongData;
    PWCHAR TypeName;
    NTSTATUS Status;

    DPRINT("SpiBuildDeviceMap() called\n");

    if (DeviceExtension == NULL || RegistryPath == NULL)
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
                         &Disposition);
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
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_KERNEL_HANDLE,
                               ScsiKey,
                               NULL);
    Status = ZwCreateKey(&ScsiPortKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &Disposition);
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
    DriverName = wcsrchr(RegistryPath->Buffer, L'\\') + 1;
    RtlInitUnicodeString(&ValueName, L"Driver");
    Status = ZwSetValueKey(ScsiPortKey,
                           &ValueName,
                           0,
                           REG_SZ,
                           DriverName,
                           (ULONG)((wcslen(DriverName) + 1) * sizeof(WCHAR)));
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
    for (BusNumber = 0; BusNumber < DeviceExtension->PortConfig->NumberOfBuses; BusNumber++)
    {
        /* Create 'Scsi Bus X' key */
        DPRINT("    Scsi Bus %lu\n", BusNumber);
        swprintf(NameBuffer,
                 L"Scsi Bus %lu",
                 BusNumber);
        RtlInitUnicodeString(&KeyName, NameBuffer);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   0,
                                   ScsiPortKey,
                                   NULL);
        Status = ZwCreateKey(&ScsiBusKey,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             &Disposition);
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
                                   0,
                                   ScsiBusKey,
                                   NULL);
        Status = ZwCreateKey(&ScsiInitiatorKey,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             &Disposition);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
            goto ByeBye;
        }

        /* FIXME: Are there any initiator values (??) */

        ZwClose(ScsiInitiatorKey);
        ScsiInitiatorKey = NULL;


        /* Enumerate targets */
        CurrentTarget = (ULONG)-1;
        ScsiTargetKey = NULL;
        for (Target = 0; Target < DeviceExtension->PortConfig->MaximumNumberOfTargets; Target++)
        {
            for (Lun = 0; Lun < SCSI_MAXIMUM_LOGICAL_UNITS; Lun++)
            {
                LunExtension = SpiGetLunExtension(DeviceExtension,
                                                  (UCHAR)BusNumber,
                                                  (UCHAR)Target,
                                                  (UCHAR)Lun);
                if (LunExtension == NULL)
                    continue;

                if (Target != CurrentTarget)
                {
                    /* Close old target key */
                    if (ScsiTargetKey != NULL)
                    {
                        ZwClose(ScsiTargetKey);
                        ScsiTargetKey = NULL;
                    }

                    /* Create 'Target Id X' key */
                    DPRINT("      Target Id %lu\n", Target);
                    swprintf(NameBuffer,
                             L"Target Id %lu",
                             Target);
                    RtlInitUnicodeString(&KeyName, NameBuffer);
                    InitializeObjectAttributes(&ObjectAttributes,
                                               &KeyName,
                                               0,
                                               ScsiBusKey,
                                               NULL);
                    Status = ZwCreateKey(&ScsiTargetKey,
                                         KEY_ALL_ACCESS,
                                         &ObjectAttributes,
                                         0,
                                         NULL,
                                         REG_OPTION_VOLATILE,
                                         &Disposition);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
                        goto ByeBye;
                    }

                    CurrentTarget = Target;
                }

                /* Create 'Logical Unit Id X' key */
                DPRINT("        Logical Unit Id %lu\n", Lun);
                swprintf(NameBuffer,
                         L"Logical Unit Id %lu",
                         Lun);
                RtlInitUnicodeString(&KeyName, NameBuffer);
                InitializeObjectAttributes(&ObjectAttributes,
                                           &KeyName,
                                           0,
                                           ScsiTargetKey,
                                           NULL);
                Status = ZwCreateKey(&ScsiLunKey,
                                     KEY_ALL_ACCESS,
                                     &ObjectAttributes,
                                     0,
                                     NULL,
                                     REG_OPTION_VOLATILE,
                                     &Disposition);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
                    goto ByeBye;
                }

                /* Set 'Identifier' (REG_SZ) value */
                swprintf(NameBuffer,
                         L"%.8S%.16S%.4S",
                         LunExtension->InquiryData.VendorId,
                         LunExtension->InquiryData.ProductId,
                         LunExtension->InquiryData.ProductRevisionLevel);
                DPRINT("          Identifier = '%S'\n", NameBuffer);
                RtlInitUnicodeString(&ValueName, L"Identifier");
                Status = ZwSetValueKey(ScsiLunKey,
                                       &ValueName,
                                       0,
                                       REG_SZ,
                                       NameBuffer,
                                       (ULONG)((wcslen(NameBuffer) + 1) * sizeof(WCHAR)));
                if (!NT_SUCCESS(Status))
                {
                    DPRINT("ZwSetValueKey('Identifier') failed (Status %lx)\n", Status);
                    goto ByeBye;
                }

                /* Set 'Type' (REG_SZ) value */
                /*
                 * See https://docs.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-ide-devices
                 * and https://docs.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-scsi-devices
                 * for a list of types with their human-readable forms.
                 */
                switch (LunExtension->InquiryData.DeviceType)
                {
                    case 0:
                        TypeName = L"DiskPeripheral";
                        break;
                    case 1:
                        TypeName = L"TapePeripheral";
                        break;
                    case 2:
                        TypeName = L"PrinterPeripheral";
                        break;
                    // case 3: "ProcessorPeripheral", classified as 'other': fall back to default case.
                    case 4:
                        TypeName = L"WormPeripheral";
                        break;
                    case 5:
                        TypeName = L"CdRomPeripheral";
                        break;
                    case 6:
                        TypeName = L"ScannerPeripheral";
                        break;
                    case 7:
                        TypeName = L"OpticalDiskPeripheral";
                        break;
                    case 8:
                        TypeName = L"MediumChangerPeripheral";
                        break;
                    case 9:
                        TypeName = L"CommunicationsPeripheral";
                        break;

                    /* New peripheral types (SCSI only) */
                    case 10: case 11:
                        TypeName = L"ASCPrePressGraphicsPeripheral";
                        break;
                    case 12:
                        TypeName = L"ArrayPeripheral";
                        break;
                    case 13:
                        TypeName = L"EnclosurePeripheral";
                        break;
                    case 14:
                        TypeName = L"RBCPeripheral";
                        break;
                    case 15:
                        TypeName = L"CardReaderPeripheral";
                        break;
                    case 16:
                        TypeName = L"BridgePeripheral";
                        break;

                    default:
                        TypeName = L"OtherPeripheral";
                        break;
                }
                DPRINT("          Type = '%S'\n", TypeName);
                RtlInitUnicodeString(&ValueName, L"Type");
                Status = ZwSetValueKey(ScsiLunKey,
                                       &ValueName,
                                       0,
                                       REG_SZ,
                                       TypeName,
                                       (ULONG)((wcslen(TypeName) + 1) * sizeof(WCHAR)));
                if (!NT_SUCCESS(Status))
                {
                    DPRINT("ZwSetValueKey('Type') failed (Status %lx)\n", Status);
                    goto ByeBye;
                }

                ZwClose(ScsiLunKey);
                ScsiLunKey = NULL;
            }

            /* Close old target key */
            if (ScsiTargetKey != NULL)
            {
                ZwClose(ScsiTargetKey);
                ScsiTargetKey = NULL;
            }
        }

        ZwClose(ScsiBusKey);
        ScsiBusKey = NULL;
    }

ByeBye:
    if (ScsiLunKey != NULL)
        ZwClose(ScsiLunKey);

    if (ScsiInitiatorKey != NULL)
        ZwClose(ScsiInitiatorKey);

    if (ScsiTargetKey != NULL)
        ZwClose(ScsiTargetKey);

    if (ScsiBusKey != NULL)
        ZwClose(ScsiBusKey);

    if (ScsiPortKey != NULL)
        ZwClose(ScsiPortKey);

    DPRINT("SpiBuildDeviceMap() done\n");

    return Status;
}
