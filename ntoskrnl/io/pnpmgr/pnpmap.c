/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PnP manager Firmware Mapper functions
 * COPYRIGHT:   Copyright 2006-2007 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2008-2011 Cameron Gutman <cameron.gutman@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
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
    ULONG Counter;
} PNP_DETECT_IDENTIFIER_MAP;

/* DATA **********************************************************************/

static UNICODE_STRING IdentifierU = RTL_CONSTANT_STRING(L"Identifier");
static UNICODE_STRING HardwareIDU = RTL_CONSTANT_STRING(L"HardwareID");
static UNICODE_STRING ConfigurationDataU = RTL_CONSTANT_STRING(L"Configuration Data");
static UNICODE_STRING BootConfigU = RTL_CONSTANT_STRING(L"BootConfig");
static UNICODE_STRING LogConfU = RTL_CONSTANT_STRING(L"LogConf");

/* FIXME: Trailing \0 in structures below are hacks, should be removed.
 * Hardware identifiers also can be mapped using "LegacyXlate" sections
 * of driver INF files. */

DATA_SEG("INITDATA")
static
PNP_MAPPER_DEVICE_ID KeyboardMap[] =
{
    { L"XT_83KEY", L"*PNP0300\0" },
    { L"PCAT_86KEY", L"*PNP0301\0" },
    { L"PCXT_84KEY", L"*PNP0302\0" },
    { L"XT_84KEY", L"*PNP0302\0" },
    { L"101-KEY", L"*PNP0303\0" },
    { L"OLI_83KEY", L"*PNP0304\0" },
    { L"ATT_301", L"*PNP0304\0" },
    { L"OLI_102KEY", L"*PNP0305\0" },
    { L"OLI_86KEY", L"*PNP0306\0" },
    { L"OLI_A101_102KEY", L"*PNP0309\0" },
    { L"ATT_302", L"*PNP030a\0" },
    { L"PCAT_ENHANCED", L"*PNP030b\0" },
    { L"PC98_106KEY", L"*nEC1300\0" },
    { L"PC98_LaptopKEY", L"*nEC1300\0" },
    { L"PC98_N106KEY", L"*PNP0303\0" },
    { NULL, NULL }
};

DATA_SEG("INITDATA")
static
PNP_MAPPER_DEVICE_ID PointerMap[] =
{
    { L"PS2 MOUSE", L"*PNP0F0E\0" },
    { L"SERIAL MOUSE", L"*PNP0F0C\0" },
    { L"MICROSOFT PS2 MOUSE", L"*PNP0F03\0" },
    { L"LOGITECH PS2 MOUSE", L"*PNP0F12\0" },
    { L"MICROSOFT INPORT MOUSE", L"*PNP0F02\0" },
    { L"MICROSOFT SERIAL MOUSE", L"*PNP0F01\0" },
    { L"MICROSOFT BALLPOINT SERIAL MOUSE", L"*PNP0F09\0" },
    { L"LOGITECH SERIAL MOUSE", L"*PNP0F08\0" },
    { L"MICROSOFT BUS MOUSE", L"*PNP0F00\0" },
    { L"NEC PC-9800 BUS MOUSE", L"*nEC1F00\0" },
    { NULL, NULL }
};

DATA_SEG("INITDATA")
static
PNP_DETECT_IDENTIFIER_MAP PnPMap[] =
{
    { L"SerialController", L"*PNP0501\0", NULL, 0 },
    //{ L"KeyboardController", L"*PNP0303\0", NULL, 0 },
    //{ L"PointerController", L"*PNP0F13\0", NULL, 0 },
    { L"KeyboardPeripheral", NULL, KeyboardMap, 0 },
    { L"PointerPeripheral", NULL, PointerMap, 0 },
    { L"ParallelController", L"*PNP0400\0", NULL, 0 },
    { L"FloppyDiskPeripheral", L"*PNP0700\0", NULL, 0 },
    { NULL, NULL, NULL, 0 }
};

/* FUNCTIONS *****************************************************************/

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
            *DeviceIndex = PnPMap[i].Counter++;

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
    _In_ HANDLE hRootKey,
    _In_ BOOLEAN EnumerateSubKeys,
    _In_opt_ PCM_FULL_RESOURCE_DESCRIPTOR BootResources,
    _In_opt_ ULONG BootResourcesLength,
    _In_ PCM_FULL_RESOURCE_DESCRIPTOR ParentBootResources,
    _In_ ULONG ParentBootResourcesLength)
{
    HANDLE hDevicesKey = NULL;
    ULONG KeyIndex = 0;
    PKEY_BASIC_INFORMATION pDeviceInformation = NULL;
    ULONG DeviceInfoLength = sizeof(KEY_BASIC_INFORMATION) + 50 * sizeof(WCHAR);
    NTSTATUS Status;

    if (!BootResources && RelativePath)
    {
        Status = IopOpenRegistryKeyEx(&hDevicesKey, hBaseKey, RelativePath, KEY_ENUMERATE_SUB_KEYS);

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
        OBJECT_ATTRIBUTES ObjectAttributes;
        HANDLE hDeviceKey = NULL;
        HANDLE hLevel1Key, hLevel2Key = NULL, hLogConf;
        UNICODE_STRING Level2NameU;
        WCHAR Level2Name[5];
        PKEY_VALUE_PARTIAL_INFORMATION pValueInformation = NULL;
        ULONG ValueInfoLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 50 * sizeof(WCHAR);
        UNICODE_STRING DeviceName, ValueName;
        ULONG RequiredSize;

        UNICODE_STRING HardwareIdKey;
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
                hRootKey,
                TRUE,
                NULL,
                0,
                BootResources,
                BootResourcesLength);

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

        Status = IopOpenRegistryKeyEx(&hDeviceKey, hDevicesKey, &DeviceName,
            KEY_QUERY_VALUE + (EnumerateSubKeys ? KEY_ENUMERATE_SUB_KEYS : 0));

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
                ZwDeleteKey(hLevel2Key);
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
                hRootKey,
                TRUE,
                BootResources,
                BootResourcesLength,
                ParentBootResources,
                ParentBootResourcesLength);

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

        /* Prepare hardware id key (hardware id value without final \0) */
        HardwareIdKey.Length = (USHORT)wcslen(pHardwareId) * sizeof(WCHAR);
        HardwareIdKey.MaximumLength = HardwareIdKey.Length + sizeof(UNICODE_NULL) * 2;
        HardwareIdKey.Buffer = pHardwareId;

        /* Add the detected device to Root key */
        InitializeObjectAttributes(&ObjectAttributes, &HardwareIdKey, OBJ_KERNEL_HANDLE, hRootKey, NULL);

        Status = ZwCreateKey(
            &hLevel1Key,
            KEY_CREATE_SUB_KEY,
            &ObjectAttributes,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
            goto nextdevice;
        }

        _swprintf(Level2Name, L"%04lu", DeviceIndex);
        RtlInitUnicodeString(&Level2NameU, Level2Name);
        InitializeObjectAttributes(&ObjectAttributes, &Level2NameU, OBJ_KERNEL_HANDLE, hLevel1Key, NULL);

        Status = ZwCreateKey(
            &hLevel2Key,
            KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
            &ObjectAttributes,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            NULL);

        ZwClose(hLevel1Key);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
            goto nextdevice;
        }

        DPRINT("Found %wZ #%lu (%wZ)\n", &ValueName, DeviceIndex, &HardwareIdKey);
        Status = ZwSetValueKey(hLevel2Key, &HardwareIDU, 0, REG_MULTI_SZ, HardwareIdKey.Buffer, HardwareIdKey.MaximumLength);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwSetValueKey() failed with status 0x%08lx\n", Status);
            ZwDeleteKey(hLevel2Key);
            goto nextdevice;
        }

        /* Create 'LogConf' subkey */
        InitializeObjectAttributes(&ObjectAttributes, &LogConfU, OBJ_KERNEL_HANDLE, hLevel2Key, NULL);

        Status = ZwCreateKey(
            &hLogConf,
            KEY_SET_VALUE,
            &ObjectAttributes,
            0,
            NULL,
            REG_OPTION_VOLATILE,
            NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
            ZwDeleteKey(hLevel2Key);
            goto nextdevice;
        }

        if (BootResourcesLength >= sizeof(CM_FULL_RESOURCE_DESCRIPTOR))
        {
            PUCHAR CmResourceList;
            ULONG ListCount;

            CmResourceList = ExAllocatePool(PagedPool, BootResourcesLength + sizeof(ULONG));
            if (!CmResourceList)
            {
                ZwClose(hLogConf);
                ZwDeleteKey(hLevel2Key);
                goto nextdevice;
            }

            /* Add the list count (1st member of CM_RESOURCE_LIST) */
            ListCount = 1;
            RtlCopyMemory(CmResourceList,
                          &ListCount,
                          sizeof(ULONG));

            /* Now add the actual list (2nd member of CM_RESOURCE_LIST) */
            RtlCopyMemory(CmResourceList + sizeof(ULONG),
                          BootResources,
                          BootResourcesLength);

            /* Save boot resources to 'LogConf\BootConfig' */
            Status = ZwSetValueKey(hLogConf,
                                   &BootConfigU,
                                   0,
                                   REG_RESOURCE_LIST,
                                   CmResourceList,
                                   BootResourcesLength + sizeof(ULONG));

            ExFreePool(CmResourceList);

            if (!NT_SUCCESS(Status))
            {
                DPRINT("ZwSetValueKey() failed with status 0x%08lx\n", Status);
                ZwClose(hLogConf);
                ZwDeleteKey(hLevel2Key);
                goto nextdevice;
            }
        }
        ZwClose(hLogConf);

nextdevice:
        if (BootResources && BootResources != ParentBootResources)
        {
            ExFreePool(BootResources);
            BootResources = NULL;
        }
        if (hLevel2Key)
        {
            ZwClose(hLevel2Key);
            hLevel2Key = NULL;
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

static
CODE_SEG("INIT")
BOOLEAN
IopIsFirmwareMapperDisabled(VOID)
{
    UNICODE_STRING KeyPathU = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CURRENTCONTROLSET\\Control\\Pnp");
    UNICODE_STRING KeyNameU = RTL_CONSTANT_STRING(L"DisableFirmwareMapper");
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hPnpKey;
    PKEY_VALUE_PARTIAL_INFORMATION KeyInformation;
    ULONG DesiredLength, Length;
    ULONG KeyValue = 0;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjectAttributes, &KeyPathU, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = ZwOpenKey(&hPnpKey, KEY_QUERY_VALUE, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        Status = ZwQueryValueKey(hPnpKey,
                                 &KeyNameU,
                                 KeyValuePartialInformation,
                                 NULL,
                                 0,
                                 &DesiredLength);
        if ((Status == STATUS_BUFFER_TOO_SMALL) ||
            (Status == STATUS_BUFFER_OVERFLOW))
        {
            Length = DesiredLength;
            KeyInformation = ExAllocatePool(PagedPool, Length);
            if (KeyInformation)
            {
                Status = ZwQueryValueKey(hPnpKey,
                                         &KeyNameU,
                                         KeyValuePartialInformation,
                                         KeyInformation,
                                         Length,
                                         &DesiredLength);
                if (NT_SUCCESS(Status) && KeyInformation->DataLength == sizeof(ULONG))
                {
                    KeyValue = (ULONG)(*KeyInformation->Data);
                }
                else
                {
                    DPRINT1("ZwQueryValueKey(%wZ%wZ) failed\n", &KeyPathU, &KeyNameU);
                }

                ExFreePool(KeyInformation);
            }
            else
            {
                DPRINT1("Failed to allocate memory for registry query\n");
            }
        }
        else
        {
            DPRINT1("ZwQueryValueKey(%wZ%wZ) failed with status 0x%08lx\n", &KeyPathU, &KeyNameU, Status);
        }

        ZwClose(hPnpKey);
    }
    else
    {
        DPRINT1("ZwOpenKey(%wZ) failed with status 0x%08lx\n", &KeyPathU, Status);
    }

    DPRINT("Firmware mapper is %s\n", KeyValue != 0 ? "disabled" : "enabled");

    return (KeyValue != 0) ? TRUE : FALSE;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
IopUpdateRootKey(VOID)
{
    UNICODE_STRING EnumU = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Enum");
    UNICODE_STRING RootPathU = RTL_CONSTANT_STRING(L"Root");
    UNICODE_STRING MultiKeyPathU = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter");
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hEnum, hRoot;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjectAttributes, &EnumU, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = ZwCreateKey(&hEnum, KEY_CREATE_SUB_KEY, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwCreateKey() failed with status 0x%08lx\n", Status);
        return Status;
    }

    InitializeObjectAttributes(&ObjectAttributes, &RootPathU, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, hEnum, NULL);
    Status = ZwCreateKey(&hRoot, KEY_CREATE_SUB_KEY, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    ZwClose(hEnum);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwOpenKey() failed with status 0x%08lx\n", Status);
        return Status;
    }

    if (!IopIsFirmwareMapperDisabled())
    {
        Status = IopOpenRegistryKeyEx(&hEnum, NULL, &MultiKeyPathU, KEY_ENUMERATE_SUB_KEYS);
        if (!NT_SUCCESS(Status))
        {
            /* Nothing to do, don't return with an error status */
            DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
            ZwClose(hRoot);
            return STATUS_SUCCESS;
        }
        Status = IopEnumerateDetectedDevices(
            hEnum,
            NULL,
            hRoot,
            TRUE,
            NULL,
            0,
            NULL,
            0);
        ZwClose(hEnum);
    }
    else
    {
        /* Enumeration is disabled */
        Status = STATUS_SUCCESS;
    }

    ZwClose(hRoot);

    return Status;
}
