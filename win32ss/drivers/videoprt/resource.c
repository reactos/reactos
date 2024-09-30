/*
 * VideoPort driver
 *
 * Copyright (C) 2002 - 2005 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "videoprt.h"

#define NDEBUG
#include <debug.h>

extern BOOLEAN VpBaseVideo;

/* PRIVATE FUNCTIONS **********************************************************/

static BOOLEAN
IntIsVgaSaveDriver(
    IN PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension)
{
    UNICODE_STRING VgaSave = RTL_CONSTANT_STRING(L"\\Driver\\VgaSave");
    return RtlEqualUnicodeString(&VgaSave, &DeviceExtension->DriverObject->DriverName, TRUE);
}

NTSTATUS NTAPI
IntVideoPortGetLegacyResources(
    IN PVIDEO_PORT_DRIVER_EXTENSION DriverExtension,
    IN PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension,
    OUT PVIDEO_ACCESS_RANGE *AccessRanges,
    OUT PULONG AccessRangeCount)
{
    PCI_COMMON_CONFIG PciConfig;
    ULONG ReadLength;

    if (!DriverExtension->InitializationData.HwGetLegacyResources &&
        !DriverExtension->InitializationData.HwLegacyResourceCount)
    {
        /* No legacy resources to report */
        *AccessRangeCount = 0;
        return STATUS_SUCCESS;
    }

    if (DriverExtension->InitializationData.HwGetLegacyResources)
    {
        ReadLength = HalGetBusData(PCIConfiguration,
                                   DeviceExtension->SystemIoBusNumber,
                                   DeviceExtension->SystemIoSlotNumber,
                                   &PciConfig,
                                   sizeof(PciConfig));
        if (ReadLength != sizeof(PciConfig))
        {
            /* This device doesn't exist */
            return STATUS_NO_SUCH_DEVICE;
        }

        DriverExtension->InitializationData.HwGetLegacyResources(PciConfig.VendorID,
                                                                 PciConfig.DeviceID,
                                                                 AccessRanges,
                                                                 AccessRangeCount);
    }
    else
    {
        *AccessRanges = DriverExtension->InitializationData.HwLegacyResourceList;
        *AccessRangeCount = DriverExtension->InitializationData.HwLegacyResourceCount;
    }

    INFO_(VIDEOPRT, "Got %d legacy access ranges\n", *AccessRangeCount);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
IntVideoPortFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION IrpStack,
    IN PIRP Irp)
{
    PDRIVER_OBJECT DriverObject;
    PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    PVIDEO_ACCESS_RANGE AccessRanges;
    ULONG AccessRangeCount, ListSize, i;
    PIO_RESOURCE_REQUIREMENTS_LIST ResList;
    PIO_RESOURCE_REQUIREMENTS_LIST OldResList = IrpStack->Parameters.FilterResourceRequirements.IoResourceRequirementList;
    PIO_RESOURCE_DESCRIPTOR CurrentDescriptor;
    NTSTATUS Status;

    DriverObject = DeviceObject->DriverObject;
    DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
    DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    Status = IntVideoPortGetLegacyResources(DriverExtension, DeviceExtension, &AccessRanges, &AccessRangeCount);
    if (!NT_SUCCESS(Status))
        return Status;
    if (!AccessRangeCount)
    {
        /* No legacy resources to report */
        return Irp->IoStatus.Status;
    }

    /* OK, we've got the access ranges now. Let's set up the resource requirements list */

    if (OldResList)
    {
        /* Already one there so let's add to it */
        ListSize = OldResList->ListSize + sizeof(IO_RESOURCE_DESCRIPTOR) * AccessRangeCount;
        ResList = ExAllocatePool(NonPagedPool,
                                 ListSize);
        if (!ResList) return STATUS_NO_MEMORY;

        RtlCopyMemory(ResList, OldResList, OldResList->ListSize);

        ASSERT(ResList->AlternativeLists == 1);

        ResList->ListSize = ListSize;
        ResList->List[0].Count += AccessRangeCount;

        CurrentDescriptor = (PIO_RESOURCE_DESCRIPTOR)((PUCHAR)ResList + OldResList->ListSize);

        ExFreePool(OldResList);
        Irp->IoStatus.Information = 0;
    }
    else
    {
        /* We need to make a new one */
        ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) + sizeof(IO_RESOURCE_DESCRIPTOR) * (AccessRangeCount - 1);
        ResList = ExAllocatePool(NonPagedPool,
                                 ListSize);
        if (!ResList) return STATUS_NO_MEMORY;

        RtlZeroMemory(ResList, ListSize);

        /* We need to initialize some fields */
        ResList->ListSize = ListSize;
        ResList->InterfaceType = DeviceExtension->AdapterInterfaceType;
        ResList->BusNumber = DeviceExtension->SystemIoBusNumber;
        ResList->SlotNumber = DeviceExtension->SystemIoSlotNumber;
        ResList->AlternativeLists = 1;
        ResList->List[0].Version = 1;
        ResList->List[0].Revision = 1;
        ResList->List[0].Count = AccessRangeCount;

        CurrentDescriptor = ResList->List[0].Descriptors;
    }

    for (i = 0; i < AccessRangeCount; i++)
    {
        /* This is a required resource */
        CurrentDescriptor->Option = 0;

        if (AccessRanges[i].RangeInIoSpace)
            CurrentDescriptor->Type = CmResourceTypePort;
        else
            CurrentDescriptor->Type = CmResourceTypeMemory;

        CurrentDescriptor->ShareDisposition =
        (AccessRanges[i].RangeShareable ? CmResourceShareShared : CmResourceShareDeviceExclusive);

        CurrentDescriptor->Flags = 0;

        if (CurrentDescriptor->Type == CmResourceTypePort)
        {
            CurrentDescriptor->u.Port.Length = AccessRanges[i].RangeLength;
            CurrentDescriptor->u.Port.MinimumAddress = AccessRanges[i].RangeStart;
            CurrentDescriptor->u.Port.MaximumAddress.QuadPart = AccessRanges[i].RangeStart.QuadPart + AccessRanges[i].RangeLength - 1;
            CurrentDescriptor->u.Port.Alignment = 1;
            if (AccessRanges[i].RangePassive & VIDEO_RANGE_PASSIVE_DECODE)
                CurrentDescriptor->Flags |= CM_RESOURCE_PORT_PASSIVE_DECODE;
            if (AccessRanges[i].RangePassive & VIDEO_RANGE_10_BIT_DECODE)
                CurrentDescriptor->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
        }
        else
        {
            CurrentDescriptor->u.Memory.Length = AccessRanges[i].RangeLength;
            CurrentDescriptor->u.Memory.MinimumAddress = AccessRanges[i].RangeStart;
            CurrentDescriptor->u.Memory.MaximumAddress.QuadPart = AccessRanges[i].RangeStart.QuadPart + AccessRanges[i].RangeLength - 1;
            CurrentDescriptor->u.Memory.Alignment = 1;
            CurrentDescriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;
        }

        CurrentDescriptor++;
    }

    Irp->IoStatus.Information = (ULONG_PTR)ResList;

    return STATUS_SUCCESS;
}

VOID
IntVideoPortReleaseResources(
    _In_ PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    BOOLEAN ConflictDetected;
    // An empty CM_RESOURCE_LIST
    UCHAR EmptyResourceList[FIELD_OFFSET(CM_RESOURCE_LIST, List)] = {0};

    Status = IoReportResourceForDetection(
                DeviceExtension->DriverObject,
                NULL, 0, /* Driver List */
                DeviceExtension->PhysicalDeviceObject,
                (PCM_RESOURCE_LIST)EmptyResourceList,
                sizeof(EmptyResourceList),
                &ConflictDetected);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("VideoPortReleaseResources IoReportResource failed with 0x%08lx ; ConflictDetected: %s\n",
                Status, ConflictDetected ? "TRUE" : "FALSE");
    }
    /* Ignore the returned status however... */
}

NTSTATUS NTAPI
IntVideoPortMapPhysicalMemory(
   IN HANDLE Process,
   IN PHYSICAL_ADDRESS PhysicalAddress,
   IN ULONG SizeInBytes,
   IN ULONG Protect,
   IN OUT PVOID *VirtualAddress  OPTIONAL)
{
   OBJECT_ATTRIBUTES ObjAttribs;
   UNICODE_STRING UnicodeString;
   HANDLE hMemObj;
   NTSTATUS Status;
   SIZE_T Size;

   /* Initialize object attribs */
   RtlInitUnicodeString(&UnicodeString, L"\\Device\\PhysicalMemory");
   InitializeObjectAttributes(&ObjAttribs,
                              &UnicodeString,
                              OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                              NULL, NULL);

   /* Open physical memory section */
   Status = ZwOpenSection(&hMemObj, SECTION_ALL_ACCESS, &ObjAttribs);
   if (!NT_SUCCESS(Status))
   {
      WARN_(VIDEOPRT, "ZwOpenSection() failed! (0x%x)\n", Status);
      return Status;
   }

   /* Map view of section */
   Size = SizeInBytes;
   Status = ZwMapViewOfSection(hMemObj,
                               Process,
                               VirtualAddress,
                               0,
                               Size,
                               (PLARGE_INTEGER)(&PhysicalAddress),
                               &Size,
                               ViewUnmap,
                               0,
                               Protect);
   ZwClose(hMemObj);
   if (!NT_SUCCESS(Status))
   {
      WARN_(VIDEOPRT, "ZwMapViewOfSection() failed! (0x%x)\n", Status);
   }

   return Status;
}


PVOID NTAPI
IntVideoPortMapMemory(
   IN PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension,
   IN PHYSICAL_ADDRESS IoAddress,
   IN ULONG NumberOfUchars,
   IN ULONG InIoSpace,
   IN HANDLE ProcessHandle,
   OUT VP_STATUS *Status)
{
   PHYSICAL_ADDRESS TranslatedAddress;
   PVIDEO_PORT_ADDRESS_MAPPING AddressMapping;
   ULONG AddressSpace;
   PVOID MappedAddress;
   PLIST_ENTRY Entry;

   INFO_(VIDEOPRT, "- IoAddress: %lx\n", IoAddress.u.LowPart);
   INFO_(VIDEOPRT, "- NumberOfUchars: %lx\n", NumberOfUchars);
   INFO_(VIDEOPRT, "- InIoSpace: %x\n", InIoSpace);

   InIoSpace &= ~VIDEO_MEMORY_SPACE_DENSE;
   if ((InIoSpace & VIDEO_MEMORY_SPACE_P6CACHE) != 0)
   {
      INFO_(VIDEOPRT, "VIDEO_MEMORY_SPACE_P6CACHE not supported, turning off\n");
      InIoSpace &= ~VIDEO_MEMORY_SPACE_P6CACHE;
   }

   if (ProcessHandle != NULL && (InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE) == 0)
   {
      INFO_(VIDEOPRT, "ProcessHandle is not NULL (0x%x) but InIoSpace does not have "
             "VIDEO_MEMORY_SPACE_USER_MODE set! Setting "
             "VIDEO_MEMORY_SPACE_USER_MODE.\n",
             ProcessHandle);
      InIoSpace |= VIDEO_MEMORY_SPACE_USER_MODE;
   }
   else if (ProcessHandle == NULL && (InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE) != 0)
   {
      INFO_(VIDEOPRT, "ProcessHandle is NULL (0x%x) but InIoSpace does have "
             "VIDEO_MEMORY_SPACE_USER_MODE set! Setting ProcessHandle "
             "to NtCurrentProcess()\n",
             ProcessHandle);
      ProcessHandle = NtCurrentProcess();
   }

   if ((InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE) == 0 &&
       !IsListEmpty(&DeviceExtension->AddressMappingListHead))
   {
      Entry = DeviceExtension->AddressMappingListHead.Flink;
      while (Entry != &DeviceExtension->AddressMappingListHead)
      {
         AddressMapping = CONTAINING_RECORD(
            Entry,
            VIDEO_PORT_ADDRESS_MAPPING,
            List);
         if (IoAddress.QuadPart == AddressMapping->IoAddress.QuadPart &&
             NumberOfUchars <= AddressMapping->NumberOfUchars)
         {
            {
               AddressMapping->MappingCount++;
               if (Status)
                  *Status = NO_ERROR;
               return AddressMapping->MappedAddress;
            }
         }
         Entry = Entry->Flink;
      }
   }

   AddressSpace = (ULONG)InIoSpace;
   AddressSpace &= ~VIDEO_MEMORY_SPACE_USER_MODE;
   if (HalTranslateBusAddress(
          DeviceExtension->AdapterInterfaceType,
          DeviceExtension->SystemIoBusNumber,
          IoAddress,
          &AddressSpace,
          &TranslatedAddress) == FALSE)
   {
      if (Status)
         *Status = ERROR_NOT_ENOUGH_MEMORY;

      return NULL;
   }

   /* I/O space */
   if (AddressSpace != 0)
   {
      ASSERT(0 == TranslatedAddress.u.HighPart);
      if (Status)
         *Status = NO_ERROR;

      return (PVOID)(ULONG_PTR)TranslatedAddress.u.LowPart;
   }

   /* user space */
   if ((InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE) != 0)
   {
      NTSTATUS NtStatus;
      MappedAddress = NULL;
      NtStatus = IntVideoPortMapPhysicalMemory(ProcessHandle,
                                               TranslatedAddress,
                                               NumberOfUchars,
                                               PAGE_READWRITE/* | PAGE_WRITECOMBINE*/,
                                               &MappedAddress);
      if (!NT_SUCCESS(NtStatus))
      {
         WARN_(VIDEOPRT, "IntVideoPortMapPhysicalMemory() failed! (0x%x)\n", NtStatus);
         if (Status)
            *Status = NO_ERROR;
         return NULL;
      }
      INFO_(VIDEOPRT, "Mapped user address = 0x%08x\n", MappedAddress);
   }
   else /* kernel space */
   {
      MappedAddress = MmMapIoSpace(
         TranslatedAddress,
         NumberOfUchars,
         MmNonCached);
   }

   if (MappedAddress != NULL)
   {
      if (Status)
      {
         *Status = NO_ERROR;
      }
      if ((InIoSpace & VIDEO_MEMORY_SPACE_USER_MODE) == 0)
      {
         AddressMapping = ExAllocatePoolWithTag(
            PagedPool,
            sizeof(VIDEO_PORT_ADDRESS_MAPPING),
            TAG_VIDEO_PORT);

         if (AddressMapping == NULL)
            return MappedAddress;

         RtlZeroMemory(AddressMapping, sizeof(VIDEO_PORT_ADDRESS_MAPPING));
         AddressMapping->NumberOfUchars = NumberOfUchars;
         AddressMapping->IoAddress = IoAddress;
         AddressMapping->SystemIoBusNumber = DeviceExtension->SystemIoBusNumber;
         AddressMapping->MappedAddress = MappedAddress;
         AddressMapping->MappingCount = 1;
         InsertHeadList(
            &DeviceExtension->AddressMappingListHead,
            &AddressMapping->List);
      }

      return MappedAddress;
   }

   if (Status)
      *Status = NO_ERROR;

   return NULL;
}

VOID NTAPI
IntVideoPortUnmapMemory(
   IN PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension,
   IN PVOID MappedAddress)
{
   PVIDEO_PORT_ADDRESS_MAPPING AddressMapping;
   PLIST_ENTRY Entry;
   NTSTATUS Status;

   Entry = DeviceExtension->AddressMappingListHead.Flink;
   while (Entry != &DeviceExtension->AddressMappingListHead)
   {
      AddressMapping = CONTAINING_RECORD(
         Entry,
         VIDEO_PORT_ADDRESS_MAPPING,
         List);
      if (AddressMapping->MappedAddress == MappedAddress)
      {
         ASSERT(AddressMapping->MappingCount > 0);
         AddressMapping->MappingCount--;
         if (AddressMapping->MappingCount == 0)
         {
            MmUnmapIoSpace(
               AddressMapping->MappedAddress,
               AddressMapping->NumberOfUchars);
            RemoveEntryList(Entry);
            ExFreePool(AddressMapping);
         }
         return;
      }

      Entry = Entry->Flink;
   }

   /* If there was no kernelmode mapping for the given address found we assume
    * that the given address is a usermode mapping and try to unmap it.
    *
    * FIXME: Is it ok to use NtCurrentProcess?
    */
   Status = ZwUnmapViewOfSection(NtCurrentProcess(), MappedAddress);
   if (!NT_SUCCESS(Status))
   {
      WARN_(VIDEOPRT, "Warning: Mapping for address 0x%p not found!\n", MappedAddress);
   }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */

PVOID NTAPI
VideoPortGetDeviceBase(
   IN PVOID HwDeviceExtension,
   IN PHYSICAL_ADDRESS IoAddress,
   IN ULONG NumberOfUchars,
   IN UCHAR InIoSpace)
{
   TRACE_(VIDEOPRT, "VideoPortGetDeviceBase\n");
   return IntVideoPortMapMemory(
      VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension),
      IoAddress,
      NumberOfUchars,
      InIoSpace,
      NULL,
      NULL);
}

/*
 * @implemented
 */

VOID NTAPI
VideoPortFreeDeviceBase(
   IN PVOID HwDeviceExtension,
   IN PVOID MappedAddress)
{
   TRACE_(VIDEOPRT, "VideoPortFreeDeviceBase\n");
   IntVideoPortUnmapMemory(
      VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension),
      MappedAddress);
}

/*
 * @unimplemented
 */

VP_STATUS NTAPI
VideoPortMapBankedMemory(
   IN PVOID HwDeviceExtension,
   IN PHYSICAL_ADDRESS PhysicalAddress,
   IN PULONG Length,
   IN PULONG InIoSpace,
   OUT PVOID *VirtualAddress,
   IN ULONG BankLength,
   IN UCHAR ReadWriteBank,
   IN PBANKED_SECTION_ROUTINE BankRoutine,
   IN PVOID Context)
{
   TRACE_(VIDEOPRT, "VideoPortMapBankedMemory\n");
   UNIMPLEMENTED;
   return ERROR_INVALID_FUNCTION;
}


/*
 * @implemented
 */

VP_STATUS NTAPI
VideoPortMapMemory(
   IN PVOID HwDeviceExtension,
   IN PHYSICAL_ADDRESS PhysicalAddress,
   IN PULONG Length,
   IN PULONG InIoSpace,
   OUT PVOID *VirtualAddress)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   NTSTATUS Status;

   TRACE_(VIDEOPRT, "VideoPortMapMemory\n");
   INFO_(VIDEOPRT, "- *VirtualAddress: 0x%x\n", *VirtualAddress);

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   *VirtualAddress = IntVideoPortMapMemory(
      DeviceExtension,
      PhysicalAddress,
      *Length,
      *InIoSpace,
      (HANDLE)*VirtualAddress,
      &Status);

   return Status;
}

/*
 * @implemented
 */

VP_STATUS NTAPI
VideoPortUnmapMemory(
   IN PVOID HwDeviceExtension,
   IN PVOID VirtualAddress,
   IN HANDLE ProcessHandle)
{
   TRACE_(VIDEOPRT, "VideoPortFreeDeviceBase\n");

   IntVideoPortUnmapMemory(
      VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension),
      VirtualAddress);

   return NO_ERROR;
}

/**
 * @brief
 * Retrieves bus-relative (mainly PCI) hardware resources access ranges
 * and, if possible, claims these resources for the caller.
 *
 * @param[in]   HwDeviceExtension
 * The miniport device extension.
 *
 * @param[in]   NumRequestedResources
 * The number of hardware resources in the @p RequestedResources array.
 *
 * @param[in]   RequestedResources
 * An optional array of IO_RESOURCE_DESCRIPTOR elements describing hardware
 * resources the miniport requires.
 *
 * @param[in]   NumAccessRanges
 * The number of ranges in the @p AccessRanges array the miniport expects
 * to retrieve.
 *
 * @param[out]  AccessRanges
 * A pointer to an array of hardware resource ranges VideoPortGetAccessRanges
 * fills with bus-relative device memory ACCESS_RANGE's for the adapter.
 *
 * @param[in]   VendorId
 * For a PCI device, points to a USHORT-type value that identifies
 * the PCI manufacturer of the adapter. Otherwise, should be NULL.
 *
 * @param[in]   DeviceId
 * For a PCI device, points to a USHORT-type value that identifies
 * a particular PCI adapter model, assigned by the manufacturer.
 * Otherwise, should be NULL.
 *
 * @param[out]  Slot
 * Points to a ULONG value that receives the logical slot / location of
 * the adapter (bus-dependent). For a PCI adapter, @p Slot points to a
 * @p PCI_SLOT_NUMBER structure that locates the adapter on the PCI bus.
 *
 * @return
 * - NO_ERROR if the resources have been successfully claimed or released.
 * - ERROR_INVALID_PARAMETER if an error or a conflict occurred.
 * - ERROR_DEV_NOT_EXIST if the device is not found.
 * - ERROR_MORE_DATA if there exist more device access ranges available
 *     than what is specified by @p NumAccessRanges.
 * - ERROR_NOT_ENOUGH_MEMORY if there is not enough memory available.
 **/
VP_STATUS
NTAPI
VideoPortGetAccessRanges(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ ULONG NumRequestedResources,
    _In_reads_opt_(NumRequestedResources)
        PIO_RESOURCE_DESCRIPTOR RequestedResources,
    _In_ ULONG NumAccessRanges,
    _Out_writes_(NumAccessRanges) PVIDEO_ACCESS_RANGE AccessRanges,
    _In_ PVOID VendorId,
    _In_ PVOID DeviceId,
    _Out_ PULONG Slot)
{
    PCI_SLOT_NUMBER PciSlotNumber;
    ULONG DeviceNumber;
    ULONG FunctionNumber;
    PCI_COMMON_CONFIG Config;
    PCM_RESOURCE_LIST AllocatedResources;
    NTSTATUS Status;
    UINT AssignedCount = 0;
    CM_FULL_RESOURCE_DESCRIPTOR *FullList;
    CM_PARTIAL_RESOURCE_DESCRIPTOR *Descriptor;
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    PVIDEO_PORT_DRIVER_EXTENSION DriverExtension;
    USHORT VendorIdToFind;
    USHORT DeviceIdToFind;
    ULONG ReturnedLength;
    PVIDEO_ACCESS_RANGE LegacyAccessRanges;
    ULONG LegacyAccessRangeCount;
    PDRIVER_OBJECT DriverObject;
    ULONG ListSize;
    PIO_RESOURCE_REQUIREMENTS_LIST ResReqList;
    BOOLEAN DeviceAndVendorFound = FALSE;

    TRACE_(VIDEOPRT, "VideoPortGetAccessRanges(%d, %p, %d, %p)\n",
        NumRequestedResources, RequestedResources, NumAccessRanges, AccessRanges);

    DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
    DriverObject = DeviceExtension->DriverObject;
    DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);

    if (NumRequestedResources == 0)
    {
        AllocatedResources = DeviceExtension->AllocatedResources;
        if (AllocatedResources == NULL &&
            DeviceExtension->AdapterInterfaceType == PCIBus)
        {
            if (DeviceExtension->PhysicalDeviceObject != NULL)
            {
                PciSlotNumber.u.AsULONG = DeviceExtension->SystemIoSlotNumber;

                ReturnedLength = HalGetBusData(PCIConfiguration,
                                               DeviceExtension->SystemIoBusNumber,
                                               PciSlotNumber.u.AsULONG,
                                               &Config,
                                               sizeof(Config));

                if (ReturnedLength != sizeof(Config))
                {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else
            {
                VendorIdToFind = VendorId != NULL ? *(PUSHORT)VendorId : 0;
                DeviceIdToFind = DeviceId != NULL ? *(PUSHORT)DeviceId : 0;

                if (VendorIdToFind == 0 && DeviceIdToFind == 0)
                {
                    /* We're screwed */
                    return ERROR_DEV_NOT_EXIST;
                }

                INFO_(VIDEOPRT, "Looking for VendorId 0x%04x DeviceId 0x%04x\n",
                      VendorIdToFind, DeviceIdToFind);

                /*
                 * Search for the device id and vendor id on this bus.
                 */
                PciSlotNumber.u.bits.Reserved = 0;
                for (DeviceNumber = 0; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber++)
                {
                    PciSlotNumber.u.bits.DeviceNumber = DeviceNumber;
                    for (FunctionNumber = 0; FunctionNumber < PCI_MAX_FUNCTION; FunctionNumber++)
                    {
                        INFO_(VIDEOPRT, "- Function number: %d\n", FunctionNumber);
                        PciSlotNumber.u.bits.FunctionNumber = FunctionNumber;
                        ReturnedLength = HalGetBusData(PCIConfiguration,
                                                       DeviceExtension->SystemIoBusNumber,
                                                       PciSlotNumber.u.AsULONG,
                                                       &Config,
                                                       sizeof(Config));

                        INFO_(VIDEOPRT, "- Length of data: %x\n", ReturnedLength);

                        if (ReturnedLength == sizeof(Config))
                        {
                            INFO_(VIDEOPRT, "- Slot 0x%02x (Device %d Function %d) VendorId 0x%04x "
                                  "DeviceId 0x%04x\n",
                                  PciSlotNumber.u.AsULONG,
                                  PciSlotNumber.u.bits.DeviceNumber,
                                  PciSlotNumber.u.bits.FunctionNumber,
                                  Config.VendorID,
                                  Config.DeviceID);

                            if ((VendorIdToFind == 0 || Config.VendorID == VendorIdToFind) &&
                                (DeviceIdToFind == 0 || Config.DeviceID == DeviceIdToFind))
                            {
                                DeviceAndVendorFound = TRUE;
                                break;
                            }
                        }
                    }
                    if (DeviceAndVendorFound)
                        break;
                }
                if (FunctionNumber == PCI_MAX_FUNCTION)
                {
                    WARN_(VIDEOPRT, "Didn't find device.\n");
                    return ERROR_DEV_NOT_EXIST;
                }
            }

            Status = HalAssignSlotResources(&DeviceExtension->RegistryPath,
                                            NULL,
                                            DeviceExtension->DriverObject,
                                            DeviceExtension->DriverObject->DeviceObject,
                                            DeviceExtension->AdapterInterfaceType,
                                            DeviceExtension->SystemIoBusNumber,
                                            PciSlotNumber.u.AsULONG,
                                            &AllocatedResources);
            if (!NT_SUCCESS(Status))
            {
                WARN_(VIDEOPRT, "HalAssignSlotResources failed with status %x.\n",Status);
                return Status;
            }
            DeviceExtension->AllocatedResources = AllocatedResources;
            DeviceExtension->SystemIoSlotNumber = PciSlotNumber.u.AsULONG;

            /* Add legacy resources to the resources from HAL */
            Status = IntVideoPortGetLegacyResources(DriverExtension, DeviceExtension,
                                                    &LegacyAccessRanges, &LegacyAccessRangeCount);
            if (!NT_SUCCESS(Status))
                return ERROR_DEV_NOT_EXIST;

            if (NumAccessRanges < LegacyAccessRangeCount)
            {
                ERR_(VIDEOPRT, "Too many legacy access ranges found\n");
                return ERROR_NOT_ENOUGH_MEMORY; // ERROR_MORE_DATA;
            }

            RtlCopyMemory(AccessRanges, LegacyAccessRanges, LegacyAccessRangeCount * sizeof(VIDEO_ACCESS_RANGE));
            AssignedCount = LegacyAccessRangeCount;
        }
    }
    else
    {
        ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) + (NumRequestedResources - 1) * sizeof(IO_RESOURCE_DESCRIPTOR);
        ResReqList = ExAllocatePool(NonPagedPool, ListSize);
        if (!ResReqList)
            return ERROR_NOT_ENOUGH_MEMORY;

        ResReqList->ListSize = ListSize;
        ResReqList->InterfaceType = DeviceExtension->AdapterInterfaceType;
        ResReqList->BusNumber = DeviceExtension->SystemIoBusNumber;
        ResReqList->SlotNumber = DeviceExtension->SystemIoSlotNumber;
        ResReqList->AlternativeLists = 1;
        ResReqList->List[0].Version = 1;
        ResReqList->List[0].Revision = 1;
        ResReqList->List[0].Count = NumRequestedResources;

        /* Copy in the caller's resource list */
        RtlCopyMemory(ResReqList->List[0].Descriptors,
                      RequestedResources,
                      NumRequestedResources * sizeof(IO_RESOURCE_DESCRIPTOR));

        Status = IoAssignResources(&DeviceExtension->RegistryPath,
                                   NULL,
                                   DeviceExtension->DriverObject,
                                   DeviceExtension->PhysicalDeviceObject ?
                                   DeviceExtension->PhysicalDeviceObject :
                                   DeviceExtension->DriverObject->DeviceObject,
                                   ResReqList,
                                   &AllocatedResources);

        if (!NT_SUCCESS(Status))
            return Status;

        if (!DeviceExtension->AllocatedResources)
            DeviceExtension->AllocatedResources = AllocatedResources;
    }

    if (AllocatedResources == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Return the slot number if the caller wants it */
    if (Slot != NULL) *Slot = DeviceExtension->SystemIoBusNumber;

    FullList = AllocatedResources->List;
    ASSERT(AllocatedResources->Count == 1);
    INFO_(VIDEOPRT, "InterfaceType %u BusNumber List %u Device BusNumber %u Version %u Revision %u\n",
          FullList->InterfaceType, FullList->BusNumber, DeviceExtension->SystemIoBusNumber,
          FullList->PartialResourceList.Version, FullList->PartialResourceList.Revision);

    ASSERT(FullList->InterfaceType == PCIBus);
    ASSERT(FullList->BusNumber == DeviceExtension->SystemIoBusNumber);
    ASSERT(1 == FullList->PartialResourceList.Version);
    ASSERT(1 == FullList->PartialResourceList.Revision);

    for (Descriptor = FullList->PartialResourceList.PartialDescriptors;
         Descriptor < FullList->PartialResourceList.PartialDescriptors + FullList->PartialResourceList.Count;
         Descriptor++)
    {
        if ((Descriptor->Type == CmResourceTypeMemory ||
             Descriptor->Type == CmResourceTypePort) &&
            AssignedCount >= NumAccessRanges)
        {
            ERR_(VIDEOPRT, "Too many access ranges found\n");
            return ERROR_MORE_DATA;
        }
        else if (Descriptor->Type == CmResourceTypeMemory)
        {
            INFO_(VIDEOPRT, "Memory range starting at 0x%08x length 0x%08x\n",
                  Descriptor->u.Memory.Start.u.LowPart, Descriptor->u.Memory.Length);
            AccessRanges[AssignedCount].RangeStart = Descriptor->u.Memory.Start;
            AccessRanges[AssignedCount].RangeLength = Descriptor->u.Memory.Length;
            AccessRanges[AssignedCount].RangeInIoSpace = 0;
            AccessRanges[AssignedCount].RangeVisible = 0; /* FIXME: Just guessing */
            AccessRanges[AssignedCount].RangeShareable =
                (Descriptor->ShareDisposition == CmResourceShareShared);
            AccessRanges[AssignedCount].RangePassive = 0;
            AssignedCount++;
        }
        else if (Descriptor->Type == CmResourceTypePort)
        {
            INFO_(VIDEOPRT, "Port range starting at 0x%04x length %d\n",
                  Descriptor->u.Port.Start.u.LowPart, Descriptor->u.Port.Length);
            AccessRanges[AssignedCount].RangeStart = Descriptor->u.Port.Start;
            AccessRanges[AssignedCount].RangeLength = Descriptor->u.Port.Length;
            AccessRanges[AssignedCount].RangeInIoSpace = 1;
            AccessRanges[AssignedCount].RangeVisible = 0; /* FIXME: Just guessing */
            AccessRanges[AssignedCount].RangeShareable =
                (Descriptor->ShareDisposition == CmResourceShareShared);
            AccessRanges[AssignedCount].RangePassive = 0;
            if (Descriptor->Flags & CM_RESOURCE_PORT_10_BIT_DECODE)
                AccessRanges[AssignedCount].RangePassive |= VIDEO_RANGE_10_BIT_DECODE;
            if (Descriptor->Flags & CM_RESOURCE_PORT_PASSIVE_DECODE)
                AccessRanges[AssignedCount].RangePassive |= VIDEO_RANGE_PASSIVE_DECODE;
            AssignedCount++;
        }
        else if (Descriptor->Type == CmResourceTypeInterrupt)
        {
            DeviceExtension->InterruptLevel = Descriptor->u.Interrupt.Level;
            DeviceExtension->InterruptVector = Descriptor->u.Interrupt.Vector;
            if (Descriptor->ShareDisposition == CmResourceShareShared)
                DeviceExtension->InterruptShared = TRUE;
            else
                DeviceExtension->InterruptShared = FALSE;
        }
        // else if (Descriptor->Type == CmResourceTypeDma) // TODO!
        else
        {
            ASSERT(FALSE);
            return ERROR_INVALID_PARAMETER;
        }
    }

    return NO_ERROR;
}

/**
 * @brief
 * Claims or releases a range of hardware resources and checks for conflicts.
 *
 * @param[in]   HwDeviceExtension
 * The miniport device extension.
 *
 * @param[in]   NumAccessRanges
 * The number of hardware resource ranges in the @p AccessRanges array.
 * Specify zero to release the hardware resources held by the miniport.
 *
 * @param[in]   AccessRanges
 * The array of hardware resource ranges to claim ownership.
 * Specify NULL to release the hardware resources held by the miniport.
 *
 * @return
 * - NO_ERROR if the resources have been successfully claimed or released.
 * - ERROR_INVALID_PARAMETER if an error or a conflict occurred.
 * - ERROR_NOT_ENOUGH_MEMORY if there is not enough memory available.
 **/
VP_STATUS
NTAPI
VideoPortVerifyAccessRanges(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ ULONG NumAccessRanges,
    _In_reads_opt_(NumAccessRanges) PVIDEO_ACCESS_RANGE AccessRanges)
{
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
    BOOLEAN ConflictDetected;
    ULONG ResourceListSize;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    ULONG i;
    NTSTATUS Status;

    TRACE_(VIDEOPRT, "VideoPortVerifyAccessRanges\n");

    /* Verify parameters */
    if (NumAccessRanges && !AccessRanges)
        return ERROR_INVALID_PARAMETER;

    DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

    if (NumAccessRanges == 0)
    {
        /* Release the resources and do nothing more for now... */
        IntVideoPortReleaseResources(DeviceExtension);
        return NO_ERROR;
    }

    /* Create the resource list */
    ResourceListSize = sizeof(CM_RESOURCE_LIST)
        + (NumAccessRanges - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    ResourceList = ExAllocatePoolWithTag(PagedPool, ResourceListSize, TAG_VIDEO_PORT);
    if (!ResourceList)
    {
        WARN_(VIDEOPRT, "ExAllocatePool() failed\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Fill resource list */
    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = DeviceExtension->AdapterInterfaceType;
    ResourceList->List[0].BusNumber = DeviceExtension->SystemIoBusNumber;
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = NumAccessRanges;
    for (i = 0; i < NumAccessRanges; i++, AccessRanges++)
    {
        PartialDescriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[i];
        if (AccessRanges->RangeInIoSpace)
        {
            PartialDescriptor->Type = CmResourceTypePort;
            PartialDescriptor->u.Port.Start = AccessRanges->RangeStart;
            PartialDescriptor->u.Port.Length = AccessRanges->RangeLength;
        }
        else
        {
            PartialDescriptor->Type = CmResourceTypeMemory;
            PartialDescriptor->u.Memory.Start = AccessRanges->RangeStart;
            PartialDescriptor->u.Memory.Length = AccessRanges->RangeLength;
        }
        if (AccessRanges->RangeShareable)
            PartialDescriptor->ShareDisposition = CmResourceShareShared;
        else
            PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        PartialDescriptor->Flags = 0;
        if (AccessRanges->RangePassive & VIDEO_RANGE_PASSIVE_DECODE)
            PartialDescriptor->Flags |= CM_RESOURCE_PORT_PASSIVE_DECODE;
        if (AccessRanges->RangePassive & VIDEO_RANGE_10_BIT_DECODE)
            PartialDescriptor->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
    }

    /* Try to acquire all resource ranges */
    Status = IoReportResourceForDetection(
                DeviceExtension->DriverObject,
                NULL, 0, /* Driver List */
                DeviceExtension->PhysicalDeviceObject,
                ResourceList, ResourceListSize,
                &ConflictDetected);

    ExFreePoolWithTag(ResourceList, TAG_VIDEO_PORT);

    /* If VgaSave driver is conflicting and we don't explicitely want
     * to use it, ignore the problem (because win32k will try to use
     * this driver only if all other ones are failing). */
    if (Status == STATUS_CONFLICTING_ADDRESSES &&
        IntIsVgaSaveDriver(DeviceExtension) &&
        !VpBaseVideo)
    {
        return NO_ERROR;
    }

    if (!NT_SUCCESS(Status) || ConflictDetected)
        return ERROR_INVALID_PARAMETER;
    else
        return NO_ERROR;
}

/*
 * @unimplemented
 */

VP_STATUS NTAPI
VideoPortGetDeviceData(
   IN PVOID HwDeviceExtension,
   IN VIDEO_DEVICE_DATA_TYPE DeviceDataType,
   IN PMINIPORT_QUERY_DEVICE_ROUTINE CallbackRoutine,
   IN PVOID Context)
{
   TRACE_(VIDEOPRT, "VideoPortGetDeviceData\n");
   UNIMPLEMENTED;
   return ERROR_INVALID_FUNCTION;
}

/*
 * @implemented
 */

PVOID NTAPI
VideoPortAllocatePool(
   IN PVOID HwDeviceExtension,
   IN VP_POOL_TYPE PoolType,
   IN SIZE_T NumberOfBytes,
   IN ULONG Tag)
{
   TRACE_(VIDEOPRT, "VideoPortAllocatePool\n");
   return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

/*
 * @implemented
 */

VOID NTAPI
VideoPortFreePool(
   IN PVOID HwDeviceExtension,
   IN PVOID Ptr)
{
   ExFreePool(Ptr);
}

/*
 * @implemented
 */

VP_STATUS NTAPI
VideoPortAllocateBuffer(
   IN PVOID HwDeviceExtension,
   IN ULONG Size,
   OUT PVOID *Buffer)
{
   TRACE_(VIDEOPRT, "VideoPortAllocateBuffer\n");
   *Buffer = ExAllocatePoolWithTag ( PagedPool, Size, TAG_VIDEO_PORT_BUFFER ) ;
   return *Buffer == NULL ? ERROR_NOT_ENOUGH_MEMORY : NO_ERROR;
}

/*
 * @implemented
 */

VOID NTAPI
VideoPortReleaseBuffer(
   IN PVOID HwDeviceExtension,
   IN PVOID Ptr)
{
   TRACE_(VIDEOPRT, "VideoPortReleaseBuffer\n");
   ExFreePool(Ptr);
}

/*
 * @implemented
 */

PVOID NTAPI
VideoPortLockBuffer(
   IN PVOID HwDeviceExtension,
   IN PVOID BaseAddress,
   IN ULONG Length,
   IN VP_LOCK_OPERATION Operation)
{
    PMDL Mdl;

    Mdl = IoAllocateMdl(BaseAddress, Length, FALSE, FALSE, NULL);
    if (!Mdl)
    {
        return NULL;
    }
    /* FIXME use seh */
    MmProbeAndLockPages(Mdl, KernelMode,Operation);
    return Mdl;
}

/*
 * @implemented
 */

BOOLEAN
NTAPI
VideoPortLockPages(
    IN PVOID HwDeviceExtension,
    IN OUT PVIDEO_REQUEST_PACKET pVrp,
    IN PEVENT pUEvent,
    IN PEVENT pDisplayEvent,
    IN DMA_FLAGS DmaFlags)
{
    PVOID Buffer;

    /* clear output buffer */
    pVrp->OutputBuffer = NULL;

    if (DmaFlags != VideoPortDmaInitOnly)
    {
        /* VideoPortKeepPagesLocked / VideoPortUnlockAfterDma is no-op */
        return FALSE;
    }

    /* lock the buffer */
    Buffer = VideoPortLockBuffer(HwDeviceExtension, pVrp->InputBuffer, pVrp->InputBufferLength, IoModifyAccess);

    if (Buffer)
    {
        /* store result buffer & length */
        pVrp->OutputBuffer = Buffer;
        pVrp->OutputBufferLength = pVrp->InputBufferLength;

        /* operation succeeded */
        return TRUE;
    }

    /* operation failed */
    return FALSE;
}


/*
 * @implemented
 */

VOID NTAPI
VideoPortUnlockBuffer(
   IN PVOID HwDeviceExtension,
   IN PVOID Mdl)
{
    if (Mdl)
    {
        MmUnlockPages((PMDL)Mdl);
        IoFreeMdl(Mdl);
    }
}

/*
 * @unimplemented
 */

VP_STATUS NTAPI
VideoPortSetTrappedEmulatorPorts(
   IN PVOID HwDeviceExtension,
   IN ULONG NumAccessRanges,
   IN PVIDEO_ACCESS_RANGE AccessRange)
{
    UNIMPLEMENTED;
    /* Should store the ranges in the device extension for use by ntvdm. */
    return NO_ERROR;
}

/*
 * @implemented
 */

ULONG NTAPI
VideoPortGetBusData(
   IN PVOID HwDeviceExtension,
   IN BUS_DATA_TYPE BusDataType,
   IN ULONG SlotNumber,
   OUT PVOID Buffer,
   IN ULONG Offset,
   IN ULONG Length)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   TRACE_(VIDEOPRT, "VideoPortGetBusData\n");

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

   if (BusDataType != Cmos)
   {
      /* Legacy vs. PnP behaviour */
      if (DeviceExtension->PhysicalDeviceObject != NULL)
         SlotNumber = DeviceExtension->SystemIoSlotNumber;
   }

   return HalGetBusDataByOffset(
      BusDataType,
      DeviceExtension->SystemIoBusNumber,
      SlotNumber,
      Buffer,
      Offset,
      Length);
}

/*
 * @implemented
 */

ULONG NTAPI
VideoPortSetBusData(
   IN PVOID HwDeviceExtension,
   IN BUS_DATA_TYPE BusDataType,
   IN ULONG SlotNumber,
   IN PVOID Buffer,
   IN ULONG Offset,
   IN ULONG Length)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

   TRACE_(VIDEOPRT, "VideoPortSetBusData\n");

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);

   if (BusDataType != Cmos)
   {
      /* Legacy vs. PnP behaviour */
      if (DeviceExtension->PhysicalDeviceObject != NULL)
         SlotNumber = DeviceExtension->SystemIoSlotNumber;
   }

   return HalSetBusDataByOffset(
      BusDataType,
      DeviceExtension->SystemIoBusNumber,
      SlotNumber,
      Buffer,
      Offset,
      Length);
}
