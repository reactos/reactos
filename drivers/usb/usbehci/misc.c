/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/misc.c
 * PURPOSE:     Misceallenous operations.
 * PROGRAMMERS:
 *              Michael Martin
 */

#include "usbehci.h"

/*
   Get SymblicName from Parameters in Registry Key
   Caller is responsible for freeing pool of returned pointer
*/
PWSTR
GetSymbolicName(PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    HANDLE DevInstRegKey;
    UNICODE_STRING SymbolicName;
    PKEY_VALUE_PARTIAL_INFORMATION KeyPartInfo;
    ULONG SizeNeeded;
    PWCHAR SymbolicNameString = NULL;

    Status = IoOpenDeviceRegistryKey(DeviceObject,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_ALL,
                                     &DevInstRegKey);

    DPRINT("IoOpenDeviceRegistryKey PLUGPLAY_REGKEY_DEVICE Status %x\n", Status);

    if (NT_SUCCESS(Status))
    {
        RtlInitUnicodeString(&SymbolicName, L"SymbolicName");
        Status = ZwQueryValueKey(DevInstRegKey,
                                 &SymbolicName,
                                 KeyValuePartialInformation,
                                 NULL,
                                 0,
                                 &SizeNeeded);

        DPRINT("ZwQueryValueKey status %x, %d\n", Status, SizeNeeded);

        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            KeyPartInfo = (PKEY_VALUE_PARTIAL_INFORMATION ) ExAllocatePool(PagedPool, SizeNeeded);
            if (!KeyPartInfo)
            {
                DPRINT1("OUT OF MEMORY\n");
                return NULL;
            }
            else
            {
                 Status = ZwQueryValueKey(DevInstRegKey,
                                          &SymbolicName,
                                          KeyValuePartialInformation,
                                          KeyPartInfo,
                                          SizeNeeded,
                                          &SizeNeeded);

                 SymbolicNameString = ExAllocatePool(PagedPool, (KeyPartInfo->DataLength + sizeof(WCHAR)));
                 if (!SymbolicNameString)
                 {
                     return NULL;
                 }
                 RtlZeroMemory(SymbolicNameString, KeyPartInfo->DataLength + 2);
                 RtlCopyMemory(SymbolicNameString, KeyPartInfo->Data, KeyPartInfo->DataLength);
            }

            ExFreePool(KeyPartInfo);
        }

        ZwClose(DevInstRegKey);
    }

    return SymbolicNameString;
}

/*
   Get Physical Device Object Name from registry
   Caller is responsible for freeing pool
*/
PWSTR
GetPhysicalDeviceObjectName(PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PWSTR ObjectName = NULL;
    ULONG SizeNeeded;

    Status = IoGetDeviceProperty(DeviceObject,
                                 DevicePropertyPhysicalDeviceObjectName,
                                 0,
                                 NULL,
                                 &SizeNeeded);

    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        DPRINT1("Expected STATUS_BUFFER_TOO_SMALL, got %x!\n", Status);
        return NULL;
    }

    ObjectName = (PWSTR) ExAllocatePool(PagedPool, SizeNeeded + sizeof(WCHAR));
    if (!ObjectName)
    {
        DPRINT1("Out of memory\n");
        return NULL;
    }

    Status = IoGetDeviceProperty(DeviceObject,
                                 DevicePropertyPhysicalDeviceObjectName,
                                 SizeNeeded,
                                 ObjectName,
                                 &SizeNeeded);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to Get Property\n");
        return NULL;
    }

    return ObjectName;
}

