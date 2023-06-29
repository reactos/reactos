/*
 * PROJECT:     ReactOS HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Routines to parse the ARC device tree
 * COPYRIGHT:   Copyright 2006-2007 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2008-2011 Cameron Gutman <cameron.gutman@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* TYPES *********************************************************************/

typedef struct _PNP_MAPPER_DEVICE_ID
{
    PCWSTR TypeName;
    PWSTR PnPId;
} PNP_MAPPER_DEVICE_ID, *PPNP_MAPPER_DEVICE_ID;

typedef struct _PNP_DETECT_IDENTIFIER_MAP
{
    PCWSTR DetectId;
    PWSTR PnPId;
    PPNP_MAPPER_DEVICE_ID PeripheralMap;
} PNP_DETECT_IDENTIFIER_MAP;

/* DATA **********************************************************************/

static UNICODE_STRING IdentifierU = RTL_CONSTANT_STRING(L"Identifier");
static UNICODE_STRING ConfigurationDataU = RTL_CONSTANT_STRING(L"Configuration Data");

/* FIXME: Hardware identifiers also can be mapped using "LegacyXlate" sections
 * of driver INF files. */

DATA_SEG("INITDATA")
static
PNP_MAPPER_DEVICE_ID KeyboardMap[] =
{
    { L"XT_83KEY", L"PNP0300" },
    { L"PCAT_86KEY", L"PNP0301" },
    { L"PCXT_84KEY", L"PNP0302" },
    { L"XT_84KEY", L"PNP0302" },
    { L"101-KEY", L"PNP0303" },
    { L"OLI_83KEY", L"PNP0304" },
    { L"ATT_301", L"PNP0304" },
    { L"OLI_102KEY", L"PNP0305" },
    { L"OLI_86KEY", L"PNP0306" },
    { L"OLI_A101_102KEY", L"PNP0309" },
    { L"ATT_302", L"PNP030a" },
    { L"PCAT_ENHANCED", L"PNP030b" },
    { L"PC98_106KEY", L"nEC1300" },
    { L"PC98_LaptopKEY", L"nEC1300" },
    { L"PC98_N106KEY", L"PNP0303" },
    { NULL, NULL }
};

DATA_SEG("INITDATA")
static
PNP_MAPPER_DEVICE_ID PointerMap[] =
{
    { L"PS2 MOUSE", L"PNP0F0E" },
    { L"SERIAL MOUSE", L"PNP0F0C" },
    { L"MICROSOFT PS2 MOUSE", L"PNP0F03" },
    { L"LOGITECH PS2 MOUSE", L"PNP0F12" },
    { L"MICROSOFT INPORT MOUSE", L"PNP0F02" },
    { L"MICROSOFT SERIAL MOUSE", L"PNP0F01" },
    { L"MICROSOFT BALLPOINT SERIAL MOUSE", L"PNP0F09" },
    { L"LOGITECH SERIAL MOUSE", L"PNP0F08" },
    { L"MICROSOFT BUS MOUSE", L"PNP0F00" },
    { L"NEC PC-9800 BUS MOUSE", L"nEC1F00" },
    { NULL, NULL }
};

DATA_SEG("INITDATA")
static
PNP_DETECT_IDENTIFIER_MAP PnPMap[] =
{
    { L"SerialController", L"PNP0501", NULL },
    //{ L"KeyboardController", L"PNP0303", NULL },
    //{ L"PointerController", L"PNP0F13", NULL },
    { L"KeyboardPeripheral", NULL, KeyboardMap },
    { L"PointerPeripheral", NULL, PointerMap },
    { L"ParallelController", L"PNP0400", NULL },
    { L"FloppyDiskPeripheral", L"PNP0700", NULL },
    { NULL, NULL, NULL }
};

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
NTSTATUS
HalpCreatePdoDeviceObject(
    _In_ WCHAR PnPID[],
    _In_ PCM_RESOURCE_LIST BootResources,
    _In_ UINT32 BootResourcesLength,
    _In_ PDEVICE_OBJECT Fdo);

static
CODE_SEG("INIT")
PWSTR
IopMapPeripheralId(
    _In_ PCUNICODE_STRING Value,
    _In_ PPNP_MAPPER_DEVICE_ID DeviceList)
{
    ULONG i;
    UNICODE_STRING CmpId;

    for (i = 0; DeviceList[i].TypeName; i++)
    {
        RtlInitUnicodeString(&CmpId, DeviceList[i].TypeName);

        if (RtlCompareUnicodeString(Value, &CmpId, FALSE) == 0)
            break;
    }

    return DeviceList[i].PnPId;
}

static
CODE_SEG("INIT")
PWSTR
IopMapDetectedDeviceId(
    _In_ PUNICODE_STRING DetectId,
    _In_ PUNICODE_STRING Value,
    _Out_ PULONG DeviceIndex)
{
    ULONG i;
    UNICODE_STRING CmpId;

    if (!DetectId)
        return NULL;

    for (i = 0; PnPMap[i].DetectId; i++)
    {
        RtlInitUnicodeString(&CmpId, PnPMap[i].DetectId);

        if (RtlCompareUnicodeString(DetectId, &CmpId, FALSE) == 0)
        {
            if (PnPMap[i].PeripheralMap)
                return IopMapPeripheralId(Value, PnPMap[i].PeripheralMap);
            break;
        }
    }

    return PnPMap[i].PnPId;
}

static
CODE_SEG("INIT")
NTSTATUS
IopEnumerateDetectedDevices(
    _In_ HANDLE hBaseKey,
    _In_opt_ PUNICODE_STRING RelativePath,
    _In_ BOOLEAN EnumerateSubKeys,
    _In_opt_ PCM_FULL_RESOURCE_DESCRIPTOR BootResources,
    _In_opt_ ULONG BootResourcesLength,
    _In_ PCM_FULL_RESOURCE_DESCRIPTOR ParentBootResources,
    _In_ ULONG ParentBootResourcesLength,
    _In_ PDEVICE_OBJECT halDevice)
{
    HANDLE hDevicesKey = NULL;
    ULONG KeyIndex = 0;
    PKEY_BASIC_INFORMATION pDeviceInformation = NULL;
    ULONG DeviceInfoLength = sizeof(KEY_BASIC_INFORMATION) + 50 * sizeof(WCHAR);
    NTSTATUS Status;

    if (!BootResources && RelativePath)
    {
        Status = HalpOpenRegistryKey(
            &hDevicesKey, hBaseKey, RelativePath, KEY_ENUMERATE_SUB_KEYS, FALSE);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
            goto cleanup;
        }
    }
    else
        hDevicesKey = hBaseKey;

    pDeviceInformation = ExAllocatePool(PagedPool, DeviceInfoLength);
    if (!pDeviceInformation)
    {
        DPRINT("ExAllocatePool() failed\n");
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    while (TRUE)
    {
        HANDLE hDeviceKey = NULL;
        PKEY_VALUE_PARTIAL_INFORMATION pValueInformation = NULL;
        ULONG ValueInfoLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 50 * sizeof(WCHAR);
        UNICODE_STRING DeviceName, ValueName;
        ULONG RequiredSize;

        PWSTR pHardwareId;
        ULONG DeviceIndex = 0;

        Status = ZwEnumerateKey(hDevicesKey,
                                KeyIndex,
                                KeyBasicInformation,
                                pDeviceInformation,
                                DeviceInfoLength,
                                &RequiredSize);

        if (Status == STATUS_NO_MORE_ENTRIES)
            break;
        else if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
        {
            ExFreePool(pDeviceInformation);
            DeviceInfoLength = RequiredSize;
            pDeviceInformation = ExAllocatePool(PagedPool, DeviceInfoLength);

            if (!pDeviceInformation)
            {
                DPRINT("ExAllocatePool() failed\n");
                Status = STATUS_NO_MEMORY;
                goto cleanup;
            }

            Status = ZwEnumerateKey(hDevicesKey,
                                    KeyIndex,
                                    KeyBasicInformation,
                                    pDeviceInformation,
                                    DeviceInfoLength,
                                    &RequiredSize);
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwEnumerateKey() failed with status 0x%08lx\n", Status);
            goto cleanup;
        }
        KeyIndex++;

        /* Open device key */
        DeviceName.Length = DeviceName.MaximumLength = (USHORT)pDeviceInformation->NameLength;
        DeviceName.Buffer = pDeviceInformation->Name;

        if (BootResources)
        {
            Status = IopEnumerateDetectedDevices(
                hDevicesKey,
                &DeviceName,
                TRUE,
                NULL,
                0,
                BootResources,
                BootResourcesLength,
                halDevice);

            if (!NT_SUCCESS(Status))
                goto cleanup;

            continue;
        }

        pValueInformation = ExAllocatePool(PagedPool, ValueInfoLength);
        if (!pValueInformation)
        {
            DPRINT("ExAllocatePool() failed\n");
            Status = STATUS_NO_MEMORY;
            goto cleanup;
        }

        Status = HalpOpenRegistryKey(&hDeviceKey, hDevicesKey, &DeviceName,
            KEY_QUERY_VALUE + (EnumerateSubKeys ? KEY_ENUMERATE_SUB_KEYS : 0), FALSE);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
            goto cleanup;
        }

        /* Read boot resources, and add then to parent ones */
        Status = ZwQueryValueKey(hDeviceKey,
                                 &ConfigurationDataU,
                                 KeyValuePartialInformation,
                                 pValueInformation,
                                 ValueInfoLength,
                                 &RequiredSize);

        if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
        {
            ExFreePool(pValueInformation);
            ValueInfoLength = RequiredSize;
            pValueInformation = ExAllocatePool(PagedPool, ValueInfoLength);

            if (!pValueInformation)
            {
                DPRINT("ExAllocatePool() failed\n");
                Status = STATUS_NO_MEMORY;
                goto cleanup;
            }

            Status = ZwQueryValueKey(hDeviceKey,
                                     &ConfigurationDataU,
                                     KeyValuePartialInformation,
                                     pValueInformation,
                                     ValueInfoLength,
                                     &RequiredSize);
        }

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            BootResources = ParentBootResources;
            BootResourcesLength = ParentBootResourcesLength;
        }
        else if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwQueryValueKey() failed with status 0x%08lx\n", Status);
            goto nextdevice;
        }
        else if (pValueInformation->Type != REG_FULL_RESOURCE_DESCRIPTOR)
        {
            DPRINT("Wrong registry type: got 0x%lx, expected 0x%lx\n", pValueInformation->Type, REG_FULL_RESOURCE_DESCRIPTOR);
            goto nextdevice;
        }
        else
        {
            static const ULONG Header = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList.PartialDescriptors);

            /* Concatenate current resources and parent ones */
            if (ParentBootResourcesLength == 0)
                BootResourcesLength = pValueInformation->DataLength;
            else
                BootResourcesLength = ParentBootResourcesLength
                    + pValueInformation->DataLength
                    - Header;

            BootResources = ExAllocatePool(PagedPool, BootResourcesLength);
            if (!BootResources)
            {
                DPRINT("ExAllocatePool() failed\n");
                goto nextdevice;
            }

            if (ParentBootResourcesLength < sizeof(CM_FULL_RESOURCE_DESCRIPTOR))
            {
                RtlCopyMemory(BootResources, pValueInformation->Data, pValueInformation->DataLength);
            }
            else if (ParentBootResources->PartialResourceList.PartialDescriptors[ParentBootResources->PartialResourceList.Count - 1].Type == CmResourceTypeDeviceSpecific)
            {
                RtlCopyMemory(BootResources, pValueInformation->Data, pValueInformation->DataLength);
                RtlCopyMemory(
                    (PVOID)((ULONG_PTR)BootResources + pValueInformation->DataLength),
                    (PVOID)((ULONG_PTR)ParentBootResources + Header),
                    ParentBootResourcesLength - Header);
                BootResources->PartialResourceList.Count += ParentBootResources->PartialResourceList.Count;
            }
            else
            {
                RtlCopyMemory(BootResources, pValueInformation->Data, Header);
                RtlCopyMemory(
                    (PVOID)((ULONG_PTR)BootResources + Header),
                    (PVOID)((ULONG_PTR)ParentBootResources + Header),
                    ParentBootResourcesLength - Header);
                RtlCopyMemory(
                    (PVOID)((ULONG_PTR)BootResources + ParentBootResourcesLength),
                    pValueInformation->Data + Header,
                    pValueInformation->DataLength - Header);
                BootResources->PartialResourceList.Count += ParentBootResources->PartialResourceList.Count;
            }
        }

        if (EnumerateSubKeys)
        {
            Status = IopEnumerateDetectedDevices(
                hDeviceKey,
                RelativePath,
                TRUE,
                BootResources,
                BootResourcesLength,
                ParentBootResources,
                ParentBootResourcesLength,
                halDevice);

            if (!NT_SUCCESS(Status))
                goto cleanup;
        }

        /* Read identifier */
        Status = ZwQueryValueKey(hDeviceKey,
                                 &IdentifierU,
                                 KeyValuePartialInformation,
                                 pValueInformation,
                                 ValueInfoLength,
                                 &RequiredSize);

        if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
        {
            ExFreePool(pValueInformation);
            ValueInfoLength = RequiredSize;
            pValueInformation = ExAllocatePool(PagedPool, ValueInfoLength);

            if (!pValueInformation)
            {
                DPRINT("ExAllocatePool() failed\n");
                Status = STATUS_NO_MEMORY;
                goto cleanup;
            }

            Status = ZwQueryValueKey(hDeviceKey,
                                     &IdentifierU,
                                     KeyValuePartialInformation,
                                     pValueInformation,
                                     ValueInfoLength,
                                     &RequiredSize);
        }

        if (!NT_SUCCESS(Status))
        {
            if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
            {
                DPRINT("ZwQueryValueKey() failed with status 0x%08lx\n", Status);
                goto nextdevice;
            }
            ValueName.Length = ValueName.MaximumLength = 0;
        }
        else if (pValueInformation->Type != REG_SZ)
        {
            DPRINT("Wrong registry type: got 0x%lx, expected 0x%lx\n", pValueInformation->Type, REG_SZ);
            goto nextdevice;
        }
        else
        {
            /* Assign hardware id to this device */
            ValueName.Length = ValueName.MaximumLength = (USHORT)pValueInformation->DataLength;
            ValueName.Buffer = (PWCHAR)pValueInformation->Data;
            if (ValueName.Length >= sizeof(WCHAR) && ValueName.Buffer[ValueName.Length / sizeof(WCHAR) - 1] == UNICODE_NULL)
                ValueName.Length -= sizeof(WCHAR);
        }

        pHardwareId = IopMapDetectedDeviceId(RelativePath, &ValueName, &DeviceIndex);
        if (!pHardwareId)
        {
            /* Unknown key path */
            DPRINT("Unknown key path '%wZ' value '%wZ'\n", RelativePath, &ValueName);
            goto nextdevice;
        }

        // set up boot resources
        PCM_RESOURCE_LIST resList = NULL;
        if (BootResourcesLength >= sizeof(CM_FULL_RESOURCE_DESCRIPTOR))
        {
            resList = ExAllocatePoolWithTag(
                PagedPool, BootResourcesLength + FIELD_OFFSET(CM_RESOURCE_LIST, List), TAG_HAL);
            resList->Count = 1;
            RtlCopyMemory(resList->List, BootResources, BootResourcesLength);
        }

        Status = HalpCreatePdoDeviceObject(pHardwareId, resList, BootResourcesLength + sizeof(ULONG), halDevice);
        if (!NT_SUCCESS(Status))
            DPRINT("Failed to add device \"%S\", status 0x%x\n", pHardwareId, Status);

nextdevice:
        if (BootResources && BootResources != ParentBootResources)
        {
            ExFreePool(BootResources);
            BootResources = NULL;
        }
        if (hDeviceKey)
        {
            ZwClose(hDeviceKey);
            hDeviceKey = NULL;
        }
        if (pValueInformation)
            ExFreePool(pValueInformation);
    }

    Status = STATUS_SUCCESS;

cleanup:
    if (hDevicesKey && hDevicesKey != hBaseKey)
        ZwClose(hDevicesKey);
    if (pDeviceInformation)
        ExFreePool(pDeviceInformation);

    return Status;
}

CODE_SEG("INIT")
VOID
HalpParseArcMap(
    _In_ PDEVICE_OBJECT halDevice)
{
    UNICODE_STRING multiKeyPathU = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter");

    HANDLE hMulti;
    NTSTATUS status = HalpOpenRegistryKey(
        &hMulti, NULL, &multiKeyPathU, KEY_ENUMERATE_SUB_KEYS, FALSE);

    if (NT_SUCCESS(status))
    {
        status = IopEnumerateDetectedDevices(
            hMulti,
            NULL,
            TRUE,
            NULL,
            0,
            NULL,
            0,
            halDevice);
        ZwClose(hMulti);
    }

    if (!NT_SUCCESS(status))
        DPRINT("WARNING! HalpParseArcMap failed with status 0x%x\n", status);
}
