/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    PnpEisa.c

Abstract:

    This file implements Eisa related code.

Author:

    Shie-Lin Tzong (shielint)

Environment:

    Kernel Mode.

Notes:

Revision History:

--*/

#include "iop.h"
#pragma hdrstop

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'iepP')
#endif

#define EISA_DEVICE_NODE_NAME L"EisaResources"
#define BUFFER_LENGTH 50

NTSTATUS
EisaGetEisaDevicesResources (
    OUT PCM_RESOURCE_LIST *ResourceList,
    OUT PULONG ResourceLength
    );

NTSTATUS
EisaBuildSlotsResources (
    IN ULONG SlotMasks,
    IN ULONG NumberMasks,
    OUT PCM_RESOURCE_LIST *Resource,
    OUT ULONG *Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, EisaBuildEisaDeviceNode)
#pragma alloc_text(INIT, EisaGetEisaDevicesResources)
#pragma alloc_text(INIT, EisaBuildSlotsResources)
#endif

NTSTATUS
EisaBuildEisaDeviceNode (
    VOID
    )

/*++

Routine Description:

    This routine build an registry key to report eisa resources to arbiters.

Arguments:

    None.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS            status;
    ULONG               disposition, tmpValue;
    WCHAR               buffer[BUFFER_LENGTH];

    UNICODE_STRING      unicodeString;
    HANDLE              rootHandle, deviceHandle, instanceHandle, logConfHandle;

    PCM_RESOURCE_LIST   resourceList;
    ULONG               resourceLength;

    status = EisaGetEisaDevicesResources(&resourceList, &resourceLength);
    if (!NT_SUCCESS(status) || resourceList == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    RtlInitUnicodeString(&unicodeString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\Root");
    status = IopOpenRegistryKeyEx( &rootHandle,
                                   NULL,
                                   &unicodeString,
                                   KEY_ALL_ACCESS
                                   );

    if (!NT_SUCCESS(status)) {
        if (resourceList) {
            ExFreePool (resourceList);
        }
        return status;
    }

    RtlInitUnicodeString(&unicodeString, EISA_DEVICE_NODE_NAME);
    status = IopCreateRegistryKeyEx( &deviceHandle,
                                     rootHandle,
                                     &unicodeString,
                                     KEY_ALL_ACCESS,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    ZwClose(rootHandle);
    if (!NT_SUCCESS(status)) {
        if (resourceList) {
            ExFreePool (resourceList);
        }
        return status;
    }

    RtlInitUnicodeString( &unicodeString, L"0000" );
    status = IopCreateRegistryKeyEx( &instanceHandle,
                                     deviceHandle,
                                     &unicodeString,
                                     KEY_ALL_ACCESS,
                                     REG_OPTION_NON_VOLATILE,
                                     &disposition );
    ZwClose(deviceHandle);
    if (NT_SUCCESS(status))  {
        if (disposition == REG_CREATED_NEW_KEY) {

            RtlInitUnicodeString( &unicodeString, L"DeviceDesc" );
            swprintf(buffer, L"%s", L"Device to report Eisa Slot Resources");

            ZwSetValueKey(instanceHandle,
                          &unicodeString,
                          0,
                          REG_SZ,
                          buffer,
                          (wcslen(buffer) + 1) * sizeof(WCHAR)
                          );

            RtlInitUnicodeString( &unicodeString, L"HardwareID" );
            RtlZeroMemory(buffer, BUFFER_LENGTH * sizeof(WCHAR));
            swprintf(buffer, L"%s", L"*Eisa_Resource_Device");

            ZwSetValueKey(instanceHandle,
                          &unicodeString,
                          0,
                          REG_MULTI_SZ,
                          buffer,
                          (wcslen(buffer) + 2) * sizeof(WCHAR)
                          );

            PiWstrToUnicodeString(&unicodeString, REGSTR_VALUE_CONFIG_FLAGS);
            tmpValue = 0;
            ZwSetValueKey(instanceHandle,
                         &unicodeString,
                         TITLE_INDEX_VALUE,
                         REG_DWORD,
                         &tmpValue,
                         sizeof(tmpValue)
                         );

        }

        RtlInitUnicodeString( &unicodeString, REGSTR_KEY_LOGCONF );
        status = IopCreateRegistryKeyEx( &logConfHandle,
                                         instanceHandle,
                                         &unicodeString,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_NON_VOLATILE,
                                         NULL
                                         );
        ZwClose(instanceHandle);
        if (NT_SUCCESS(status))  {
            RtlInitUnicodeString( &unicodeString, REGSTR_VAL_BOOTCONFIG );

            status = ZwSetValueKey(logConfHandle,
                                   &unicodeString,
                                   0,
                                   REG_RESOURCE_LIST,
                                   resourceList,
                                   resourceLength
                                   );
            ZwClose(logConfHandle);
        }
    }
    if (resourceList) {
        ExFreePool (resourceList);
    }
    return status;
}

NTSTATUS
EisaGetEisaDevicesResources (
    OUT PCM_RESOURCE_LIST *ResourceList,
    OUT PULONG ResourceLength
    )

/*++

Routine Description:

    This routine builds a cm resource list for all the eisa slots.

Arguments:

    None.

Return Value:

    A CmResourceList.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PCM_RESOURCE_LIST resources = NULL;
    HANDLE handle;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING unicodeString;
    ULONG slotMasks = 0, numberMasks = 0, i;

    *ResourceList = NULL;
    *ResourceLength = 0;

    //
    // Open LocalMachine\Hardware\Description
    //

    //RtlInitUnicodeString(&unicodeString, L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\SYSTEM\\EisaAdapter\\0");
    RtlInitUnicodeString(&unicodeString, L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\EisaAdapter");
    status = IopOpenRegistryKeyEx( &handle,
                                   NULL,
                                   &unicodeString,
                                   KEY_READ
                                   );
    if (NT_SUCCESS(status)) {
        status = IopGetRegistryValue(handle,
                                     L"Configuration Data",
                                     &keyValueInformation
                                     );
        if (NT_SUCCESS(status)) {
            PCM_FULL_RESOURCE_DESCRIPTOR resourceDescriptor;
            PCM_PARTIAL_RESOURCE_DESCRIPTOR partialResourceDescriptor;

            resourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)
                ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);

            if ((keyValueInformation->DataLength >= sizeof(CM_FULL_RESOURCE_DESCRIPTOR)) &&
                (resourceDescriptor->PartialResourceList.Count > 0) ) {
                LONG eisaInfoLength;
                PCM_EISA_SLOT_INFORMATION eisaInfo;

                partialResourceDescriptor = resourceDescriptor->PartialResourceList.PartialDescriptors;
                if (partialResourceDescriptor->Type == CmResourceTypeDeviceSpecific) {
                    eisaInfo = (PCM_EISA_SLOT_INFORMATION)
                        ((PUCHAR)partialResourceDescriptor + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
                    eisaInfoLength = (LONG)partialResourceDescriptor->u.DeviceSpecificData.DataSize;

                    //
                    // Parse the eisa slot info to find the eisa slots with device installed.
                    //

                    for (i = 0; i < 0x10 && eisaInfoLength > 0; i++) {
                        if (eisaInfo->ReturnCode == EISA_INVALID_SLOT) {
                            break;
                        }
                        if (eisaInfo->ReturnCode != EISA_EMPTY_SLOT && (i != 0)) {
                            slotMasks |= (1 << i);
                            numberMasks++;
                        }
                        if (eisaInfo->ReturnCode == EISA_EMPTY_SLOT) {
                            eisaInfoLength -= sizeof(CM_EISA_SLOT_INFORMATION);
                            eisaInfo++;
                        } else {
                            eisaInfoLength -= sizeof(CM_EISA_SLOT_INFORMATION) + eisaInfo->NumberFunctions * sizeof(CM_EISA_FUNCTION_INFORMATION);
                            eisaInfo = (PCM_EISA_SLOT_INFORMATION)
                                       ((PUCHAR)eisaInfo + eisaInfo->NumberFunctions * sizeof(CM_EISA_FUNCTION_INFORMATION) +
                                           sizeof(CM_EISA_SLOT_INFORMATION));
                        }
                    }

                    if (slotMasks) {
                        status = EisaBuildSlotsResources(slotMasks, numberMasks, ResourceList, ResourceLength);
                    }
                }

            }
            ExFreePool(keyValueInformation);
        }
        ZwClose(handle);
    }
    return status;
}

NTSTATUS
EisaBuildSlotsResources (
    IN ULONG SlotMasks,
    IN ULONG NumberMasks,
    OUT PCM_RESOURCE_LIST *Resources,
    OUT ULONG *Length
    )

/*++

Routine Description:

    This routine build a cm resource list for all the io resources used
    by the eisa devices.

Arguments:

    SlotMask - a mask to indicate the valid eisa slot.

Return Value:

    A pointer to a CM_RESOURCE_LIST.

--*/

{
    PCM_RESOURCE_LIST resources = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDesc;
    ULONG slot;

    *Length = sizeof(CM_RESOURCE_LIST) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (NumberMasks - 1);
    resources = ExAllocatePool(PagedPool, *Length);
    if (resources) {
        resources->Count = 1;
        resources->List[0].InterfaceType = Eisa;
        resources->List[0].BusNumber = 0;
        resources->List[0].PartialResourceList.Version = 0;
        resources->List[0].PartialResourceList.Revision = 0;
        resources->List[0].PartialResourceList.Count = NumberMasks;
        partialDesc = resources->List[0].PartialResourceList.PartialDescriptors;
        slot = 0; // ignore slot 0
        while (SlotMasks) {
            SlotMasks >>= 1;
            slot++;
            if (SlotMasks & 1) {
                partialDesc->Type = CmResourceTypePort;
                partialDesc->ShareDisposition = CmResourceShareDeviceExclusive;
                partialDesc->Flags = CM_RESOURCE_PORT_16_BIT_DECODE + CM_RESOURCE_PORT_IO;
                partialDesc->u.Port.Start.LowPart = slot << 12;
                partialDesc->u.Port.Start.HighPart = 0;
                partialDesc->u.Port.Length = 0x1000;
                partialDesc++;
            }
        }
        *Resources = resources;
        return STATUS_SUCCESS;
    } else {
        return STATUS_NO_MEMORY;
    }
}
