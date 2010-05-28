/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpres.c
 * PURPOSE:         Resource handling code
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 *                  ReactOS Portable Systems Group
 */

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
IopDetectResourceConflict(
   IN PCM_RESOURCE_LIST ResourceList,
   IN BOOLEAN Silent,
   OUT OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor);

ULONG
NTAPI
IopCalculateResourceListSize(
   IN PCM_RESOURCE_LIST ResourceList)
{
   ULONG Size, i, j;
   PCM_PARTIAL_RESOURCE_LIST pPartialResourceList;

   Size = FIELD_OFFSET(CM_RESOURCE_LIST, List);
   for (i = 0; i < ResourceList->Count; i++)
   {
      pPartialResourceList = &ResourceList->List[i].PartialResourceList;
      Size += FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList.PartialDescriptors) +
              pPartialResourceList->Count * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
      for (j = 0; j < pPartialResourceList->Count; j++)
      {
         if (pPartialResourceList->PartialDescriptors[j].Type == CmResourceTypeDeviceSpecific)
             Size += pPartialResourceList->PartialDescriptors[j].u.DeviceSpecificData.DataSize;
      }
   }

   return Size;
}

static
BOOLEAN
IopCheckDescriptorForConflict(PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc, OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor)
{
   CM_RESOURCE_LIST CmList;
   NTSTATUS Status;

   CmList.Count = 1;
   CmList.List[0].InterfaceType = InterfaceTypeUndefined;
   CmList.List[0].BusNumber = 0;
   CmList.List[0].PartialResourceList.Version = 1;
   CmList.List[0].PartialResourceList.Revision = 1;
   CmList.List[0].PartialResourceList.Count = 1;
   CmList.List[0].PartialResourceList.PartialDescriptors[0] = *CmDesc;

   Status = IopDetectResourceConflict(&CmList, TRUE, ConflictingDescriptor);
   if (Status == STATUS_CONFLICTING_ADDRESSES)
       return TRUE;

   return FALSE;
}

static
BOOLEAN
IopFindBusNumberResource(
   IN PIO_RESOURCE_DESCRIPTOR IoDesc,
   OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc)
{
   ULONG Start;
   CM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDesc;

   ASSERT(IoDesc->Type == CmDesc->Type);
   ASSERT(IoDesc->Type == CmResourceTypeBusNumber);

   for (Start = IoDesc->u.BusNumber.MinBusNumber;
        Start < IoDesc->u.BusNumber.MaxBusNumber;
        Start++)
   {
        CmDesc->u.BusNumber.Length = IoDesc->u.BusNumber.Length;
        CmDesc->u.BusNumber.Start = Start;

        if (IopCheckDescriptorForConflict(CmDesc, &ConflictingDesc))
        {
            Start += ConflictingDesc.u.BusNumber.Start + ConflictingDesc.u.BusNumber.Length;
        }
        else
        {
            return TRUE;
        }
   }

   return FALSE;
}

static
BOOLEAN
IopFindMemoryResource(
   IN PIO_RESOURCE_DESCRIPTOR IoDesc,
   OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc)
{
   ULONGLONG Start;
   CM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDesc;

   ASSERT(IoDesc->Type == CmDesc->Type);
   ASSERT(IoDesc->Type == CmResourceTypeMemory);

   for (Start = IoDesc->u.Memory.MinimumAddress.QuadPart;
        Start < IoDesc->u.Memory.MaximumAddress.QuadPart;
        Start++)
   {
        CmDesc->u.Memory.Length = IoDesc->u.Memory.Length;
        CmDesc->u.Memory.Start.QuadPart = Start;

        if (IopCheckDescriptorForConflict(CmDesc, &ConflictingDesc))
        {
            Start += ConflictingDesc.u.Memory.Start.QuadPart + ConflictingDesc.u.Memory.Length;
        }
        else
        {
            return TRUE;
        }
   }

   return FALSE;
}

static
BOOLEAN
IopFindPortResource(
   IN PIO_RESOURCE_DESCRIPTOR IoDesc,
   OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc)
{
   ULONGLONG Start;
   CM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDesc;

   ASSERT(IoDesc->Type == CmDesc->Type);
   ASSERT(IoDesc->Type == CmResourceTypePort);

   for (Start = IoDesc->u.Port.MinimumAddress.QuadPart;
        Start < IoDesc->u.Port.MaximumAddress.QuadPart;
        Start++)
   {
        CmDesc->u.Port.Length = IoDesc->u.Port.Length;
        CmDesc->u.Port.Start.QuadPart = Start;

        if (IopCheckDescriptorForConflict(CmDesc, &ConflictingDesc))
        {
            Start += ConflictingDesc.u.Port.Start.QuadPart + ConflictingDesc.u.Port.Length;
        }
        else
        {
            return TRUE;
        }
   }

   return FALSE;
}

static
BOOLEAN
IopFindDmaResource(
   IN PIO_RESOURCE_DESCRIPTOR IoDesc,
   OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc)
{
   ULONG Channel;

   ASSERT(IoDesc->Type == CmDesc->Type);
   ASSERT(IoDesc->Type == CmResourceTypeDma);

   for (Channel = IoDesc->u.Dma.MinimumChannel;
        Channel < IoDesc->u.Dma.MaximumChannel;
        Channel++)
   {
        CmDesc->u.Dma.Channel = Channel;
        CmDesc->u.Dma.Port = 0;

        if (!IopCheckDescriptorForConflict(CmDesc, NULL))
            return TRUE;
   }

   return FALSE;
}

static
BOOLEAN
IopFindInterruptResource(
   IN PIO_RESOURCE_DESCRIPTOR IoDesc,
   OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc)
{
   ULONG Vector;

   ASSERT(IoDesc->Type == CmDesc->Type);
   ASSERT(IoDesc->Type == CmResourceTypeInterrupt);

   for (Vector = IoDesc->u.Interrupt.MinimumVector;
        Vector < IoDesc->u.Interrupt.MaximumVector;
        Vector++)
   {
        CmDesc->u.Interrupt.Vector = Vector;
        CmDesc->u.Interrupt.Level = Vector;
        CmDesc->u.Interrupt.Affinity = (KAFFINITY)-1;

        if (!IopCheckDescriptorForConflict(CmDesc, NULL))
            return TRUE;
   }

   return FALSE;
}

static
NTSTATUS
IopCreateResourceListFromRequirements(
   IN PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList,
   OUT PCM_RESOURCE_LIST *ResourceList)
{
   ULONG i, ii, Size;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDesc;

   Size = FIELD_OFFSET(CM_RESOURCE_LIST, List);
   for (i = 0; i < RequirementsList->AlternativeLists; i++)
   {
      PIO_RESOURCE_LIST ResList = &RequirementsList->List[i];
      Size += FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList.PartialDescriptors)
        + ResList->Count * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
   }

   *ResourceList = ExAllocatePool(PagedPool, Size);
   if (!*ResourceList)
       return STATUS_INSUFFICIENT_RESOURCES;

   (*ResourceList)->Count = 1;
   (*ResourceList)->List[0].BusNumber = RequirementsList->BusNumber;
   (*ResourceList)->List[0].InterfaceType = RequirementsList->InterfaceType;
   (*ResourceList)->List[0].PartialResourceList.Version = 1;
   (*ResourceList)->List[0].PartialResourceList.Revision = 1;
   (*ResourceList)->List[0].PartialResourceList.Count = 0;

   ResDesc = &(*ResourceList)->List[0].PartialResourceList.PartialDescriptors[0];

   for (i = 0; i < RequirementsList->AlternativeLists; i++)
   {
      PIO_RESOURCE_LIST ResList = &RequirementsList->List[i];
      for (ii = 0; ii < ResList->Count; ii++)
      {
         PIO_RESOURCE_DESCRIPTOR ReqDesc = &ResList->Descriptors[ii];

         /* FIXME: Handle alternate ranges */
         if (ReqDesc->Option == IO_RESOURCE_ALTERNATIVE)
             continue;

         ResDesc->Type = ReqDesc->Type;
         ResDesc->Flags = ReqDesc->Flags;
         ResDesc->ShareDisposition = ReqDesc->ShareDisposition;

         switch (ReqDesc->Type)
         {
            case CmResourceTypeInterrupt:
              if (!IopFindInterruptResource(ReqDesc, ResDesc))
              {
                  DPRINT1("Failed to find an available interrupt resource (0x%x to 0x%x)\n",
                           ReqDesc->u.Interrupt.MinimumVector, ReqDesc->u.Interrupt.MaximumVector);

                  if (ReqDesc->Option == 0)
                  {
                      ExFreePool(*ResourceList);
                      return STATUS_CONFLICTING_ADDRESSES;
                  }
              }
              break;

            case CmResourceTypePort:
              if (!IopFindPortResource(ReqDesc, ResDesc))
              {
                  DPRINT1("Failed to find an available port resource (0x%x to 0x%x length: 0x%x)\n",
                          ReqDesc->u.Port.MinimumAddress.QuadPart, ReqDesc->u.Port.MaximumAddress.QuadPart,
                          ReqDesc->u.Port.Length);

                  if (ReqDesc->Option == 0)
                  {
                      ExFreePool(*ResourceList);
                      return STATUS_CONFLICTING_ADDRESSES;
                  }
              }
              break;

            case CmResourceTypeMemory:
              if (!IopFindMemoryResource(ReqDesc, ResDesc))
              {
                  DPRINT1("Failed to find an available memory resource (0x%x to 0x%x length: 0x%x)\n",
                          ReqDesc->u.Memory.MinimumAddress.QuadPart, ReqDesc->u.Memory.MaximumAddress.QuadPart,
                          ReqDesc->u.Memory.Length);

                  if (ReqDesc->Option == 0)
                  {
                      ExFreePool(*ResourceList);
                      return STATUS_CONFLICTING_ADDRESSES;
                  }
              }
              break;

            case CmResourceTypeBusNumber:
              if (!IopFindBusNumberResource(ReqDesc, ResDesc))
              {
                  DPRINT1("Failed to find an available bus number resource (0x%x to 0x%x length: 0x%x)\n",
                          ReqDesc->u.BusNumber.MinBusNumber, ReqDesc->u.BusNumber.MaxBusNumber,
                          ReqDesc->u.BusNumber.Length);

                  if (ReqDesc->Option == 0)
                  {
                      ExFreePool(*ResourceList);
                      return STATUS_CONFLICTING_ADDRESSES;
                  }
              }
              break;

            case CmResourceTypeDma:
              if (!IopFindDmaResource(ReqDesc, ResDesc))
              {
                  DPRINT1("Failed to find an available dma resource (0x%x to 0x%x)\n",
                          ReqDesc->u.Dma.MinimumChannel, ReqDesc->u.Dma.MaximumChannel);

                  if (ReqDesc->Option == 0)
                  {
                      ExFreePool(*ResourceList);
                      return STATUS_CONFLICTING_ADDRESSES;
                  }
              }
              break;

            default:
              DPRINT1("Unsupported resource type: %x\n", ReqDesc->Type);
              break;
         }

         (*ResourceList)->List[0].PartialResourceList.Count++;
         ResDesc++;
      }
   }

   return STATUS_SUCCESS;
}

static
BOOLEAN
IopCheckResourceDescriptor(
   IN PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDesc,
   IN PCM_RESOURCE_LIST ResourceList,
   IN BOOLEAN Silent,
   OUT OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor)
{
   ULONG i, ii;
   BOOLEAN Result = FALSE;

   if (ResDesc->ShareDisposition == CmResourceShareShared)
       return FALSE;

   for (i = 0; i < ResourceList->Count; i++)
   {
      PCM_PARTIAL_RESOURCE_LIST ResList = &ResourceList->List[i].PartialResourceList;
      for (ii = 0; ii < ResList->Count; ii++)
      {
         PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDesc2 = &ResList->PartialDescriptors[ii];

         /* We don't care about shared resources */
         if (ResDesc->ShareDisposition == CmResourceShareShared &&
             ResDesc2->ShareDisposition == CmResourceShareShared)
             continue;

         /* Make sure we're comparing the same types */
         if (ResDesc->Type != ResDesc2->Type)
             continue;

         switch (ResDesc->Type)
         {
             case CmResourceTypeMemory:
                 if ((ResDesc->u.Memory.Start.QuadPart < ResDesc2->u.Memory.Start.QuadPart &&
                      ResDesc->u.Memory.Start.QuadPart + ResDesc->u.Memory.Length >
                      ResDesc2->u.Memory.Start.QuadPart) || (ResDesc2->u.Memory.Start.QuadPart <
                      ResDesc->u.Memory.Start.QuadPart && ResDesc2->u.Memory.Start.QuadPart +
                      ResDesc2->u.Memory.Length > ResDesc->u.Memory.Start.QuadPart))
                 {
                      if (!Silent)
                      {
                          DPRINT1("Resource conflict: Memory (0x%x to 0x%x vs. 0x%x to 0x%x)\n",
                                  ResDesc->u.Memory.Start.QuadPart, ResDesc->u.Memory.Start.QuadPart +
                                  ResDesc->u.Memory.Length, ResDesc2->u.Memory.Start.QuadPart,
                                  ResDesc2->u.Memory.Start.QuadPart + ResDesc2->u.Memory.Length);
                      }

                      Result = TRUE;

                      goto ByeBye;
                 }
                 break;

             case CmResourceTypePort:
                 if ((ResDesc->u.Port.Start.QuadPart < ResDesc2->u.Port.Start.QuadPart &&
                      ResDesc->u.Port.Start.QuadPart + ResDesc->u.Port.Length >
                      ResDesc2->u.Port.Start.QuadPart) || (ResDesc2->u.Port.Start.QuadPart <
                      ResDesc->u.Port.Start.QuadPart && ResDesc2->u.Port.Start.QuadPart +
                      ResDesc2->u.Port.Length > ResDesc->u.Port.Start.QuadPart))
                 {
                      if (!Silent)
                      {
                          DPRINT1("Resource conflict: Port (0x%x to 0x%x vs. 0x%x to 0x%x)\n",
                                  ResDesc->u.Port.Start.QuadPart, ResDesc->u.Port.Start.QuadPart +
                                  ResDesc->u.Port.Length, ResDesc2->u.Port.Start.QuadPart,
                                  ResDesc2->u.Port.Start.QuadPart + ResDesc2->u.Port.Length);
                      }

                      Result = TRUE;

                      goto ByeBye;
                 }
                 break;

             case CmResourceTypeInterrupt:
                 if (ResDesc->u.Interrupt.Vector == ResDesc2->u.Interrupt.Vector)
                 {
                      if (!Silent)
                      {
                          DPRINT1("Resource conflict: IRQ (0x%x 0x%x vs. 0x%x 0x%x)\n",
                                  ResDesc->u.Interrupt.Vector, ResDesc->u.Interrupt.Level,
                                  ResDesc2->u.Interrupt.Vector, ResDesc2->u.Interrupt.Level);
                      }

                      Result = TRUE;

                      goto ByeBye;
                 }
                 break;

             case CmResourceTypeBusNumber:
                 if ((ResDesc->u.BusNumber.Start < ResDesc2->u.BusNumber.Start &&
                      ResDesc->u.BusNumber.Start + ResDesc->u.BusNumber.Length >
                      ResDesc2->u.BusNumber.Start) || (ResDesc2->u.BusNumber.Start <
                      ResDesc->u.BusNumber.Start && ResDesc2->u.BusNumber.Start +
                      ResDesc2->u.BusNumber.Length > ResDesc->u.BusNumber.Start))
                 {
                      if (!Silent)
                      {
                          DPRINT1("Resource conflict: Bus number (0x%x to 0x%x vs. 0x%x to 0x%x)\n",
                                  ResDesc->u.BusNumber.Start, ResDesc->u.BusNumber.Start +
                                  ResDesc->u.BusNumber.Length, ResDesc2->u.BusNumber.Start,
                                  ResDesc2->u.BusNumber.Start + ResDesc2->u.BusNumber.Length);
                      }

                      Result = TRUE;

                      goto ByeBye;
                 }
                 break;

             case CmResourceTypeDma:
                 if (ResDesc->u.Dma.Channel == ResDesc2->u.Dma.Channel)
                 {
                     if (!Silent)
                     {
                         DPRINT1("Resource conflict: Dma (0x%x 0x%x vs. 0x%x 0x%x)\n",
                                 ResDesc->u.Dma.Channel, ResDesc->u.Dma.Port,
                                 ResDesc2->u.Dma.Channel, ResDesc2->u.Dma.Port);
                     }

                     Result = TRUE;

                     goto ByeBye;
                 }
                 break;
         }
      }
   }

ByeBye:

   if (Result && ConflictingDescriptor)
   {
       RtlCopyMemory(ConflictingDescriptor,
                     ResDesc,
                     sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
   }

   return Result;
}

static
NTSTATUS
IopUpdateControlKeyWithResources(IN PDEVICE_NODE DeviceNode)
{
   UNICODE_STRING EnumRoot = RTL_CONSTANT_STRING(ENUM_ROOT);
   UNICODE_STRING Control = RTL_CONSTANT_STRING(L"Control");
   UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"AllocConfig");
   HANDLE EnumKey, InstanceKey, ControlKey;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES ObjectAttributes;

   /* Open the Enum key */
   Status = IopOpenRegistryKeyEx(&EnumKey, NULL, &EnumRoot, KEY_ENUMERATE_SUB_KEYS);
   if (!NT_SUCCESS(Status))
       return Status;

   /* Open the instance key (eg. Root\PNP0A03) */
   Status = IopOpenRegistryKeyEx(&InstanceKey, EnumKey, &DeviceNode->InstancePath, KEY_ENUMERATE_SUB_KEYS);
   ZwClose(EnumKey);

   if (!NT_SUCCESS(Status))
       return Status;

   /* Create/Open the Control key */
   InitializeObjectAttributes(&ObjectAttributes,
                              &Control,
                              OBJ_CASE_INSENSITIVE,
                              InstanceKey,
                              NULL);
   Status = ZwCreateKey(&ControlKey,
                        KEY_SET_VALUE,
                        &ObjectAttributes,
                        0,
                        NULL,
                        REG_OPTION_VOLATILE,
                        NULL);
   ZwClose(InstanceKey);

   if (!NT_SUCCESS(Status))
       return Status;

   /* Write the resource list */
   Status = ZwSetValueKey(ControlKey,
                          &ValueName,
                          0,
                          REG_RESOURCE_LIST,
                          DeviceNode->ResourceList,
                          IopCalculateResourceListSize(DeviceNode->ResourceList));
   ZwClose(ControlKey);

   if (!NT_SUCCESS(Status))
       return Status; 

   return STATUS_SUCCESS;
}

static
NTSTATUS
IopFilterResourceRequirements(IN PDEVICE_NODE DeviceNode)
{
   IO_STACK_LOCATION Stack;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;

   DPRINT("Sending IRP_MN_FILTER_RESOURCE_REQUIREMENTS to device stack\n");

   Stack.Parameters.FilterResourceRequirements.IoResourceRequirementList = DeviceNode->ResourceRequirements;
   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_FILTER_RESOURCE_REQUIREMENTS,
      &Stack);
   if (!NT_SUCCESS(Status) && Status != STATUS_NOT_SUPPORTED)
   {
      DPRINT("IopInitiatePnpIrp(IRP_MN_FILTER_RESOURCE_REQUIREMENTS) failed\n");
      return Status;
   }
   else if (NT_SUCCESS(Status))
   {
      DeviceNode->ResourceRequirements = (PIO_RESOURCE_REQUIREMENTS_LIST)IoStatusBlock.Information;
   }

   return STATUS_SUCCESS;
}


NTSTATUS
IopUpdateResourceMap(IN PDEVICE_NODE DeviceNode, PWCHAR Level1Key, PWCHAR Level2Key)
{
  NTSTATUS Status;
  ULONG Disposition;
  HANDLE PnpMgrLevel1, PnpMgrLevel2, ResourceMapKey;
  UNICODE_STRING KeyName;
  OBJECT_ATTRIBUTES ObjectAttributes;

  RtlInitUnicodeString(&KeyName,
		       L"\\Registry\\Machine\\HARDWARE\\RESOURCEMAP");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     0,
			     NULL);
  Status = ZwCreateKey(&ResourceMapKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       &Disposition);
  if (!NT_SUCCESS(Status))
      return Status;

  RtlInitUnicodeString(&KeyName, Level1Key);
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     ResourceMapKey,
			     NULL);
  Status = ZwCreateKey(&PnpMgrLevel1,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes,
                       0,
                       NULL,
                       REG_OPTION_VOLATILE,
                       &Disposition);
  ZwClose(ResourceMapKey);
  if (!NT_SUCCESS(Status))
      return Status;

  RtlInitUnicodeString(&KeyName, Level2Key);
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     PnpMgrLevel1,
			     NULL);
  Status = ZwCreateKey(&PnpMgrLevel2,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes,
                       0,
                       NULL,
                       REG_OPTION_VOLATILE,
                       &Disposition);
  ZwClose(PnpMgrLevel1);
  if (!NT_SUCCESS(Status))
      return Status;

  if (DeviceNode->ResourceList)
  {
      PWCHAR DeviceName = NULL;
      UNICODE_STRING NameU;
      UNICODE_STRING Suffix;
      ULONG OldLength = 0;

      ASSERT(DeviceNode->ResourceListTranslated);

      Status = IoGetDeviceProperty(DeviceNode->PhysicalDeviceObject,
                                   DevicePropertyPhysicalDeviceObjectName,
                                   0,
                                   NULL,
                                   &OldLength);
     if ((OldLength != 0) && (Status == STATUS_BUFFER_TOO_SMALL))
     {
        DeviceName = ExAllocatePool(NonPagedPool, OldLength);
        ASSERT(DeviceName);

        IoGetDeviceProperty(DeviceNode->PhysicalDeviceObject,
                            DevicePropertyPhysicalDeviceObjectName,
                            OldLength,
                            DeviceName,
                            &OldLength);
                            
        RtlInitUnicodeString(&NameU, DeviceName);
     }
     else
     {
        /* Some failure */
        ASSERT(!NT_SUCCESS(Status));
        return Status;
     }

      RtlInitUnicodeString(&Suffix, L".Raw");
      RtlAppendUnicodeStringToString(&NameU, &Suffix);

      Status = ZwSetValueKey(PnpMgrLevel2,
                             &NameU,
                             0,
                             REG_RESOURCE_LIST,
                             DeviceNode->ResourceList,
                             IopCalculateResourceListSize(DeviceNode->ResourceList));
      if (!NT_SUCCESS(Status))
      {
          ZwClose(PnpMgrLevel2);
          return Status;
      }

      /* "Remove" the suffix by setting the length back to what it used to be */
      NameU.Length = (USHORT)OldLength;

      RtlInitUnicodeString(&Suffix, L".Translated");
      RtlAppendUnicodeStringToString(&NameU, &Suffix);

      Status = ZwSetValueKey(PnpMgrLevel2,
                             &NameU,
                             0,
                             REG_RESOURCE_LIST,
                             DeviceNode->ResourceListTranslated,
                             IopCalculateResourceListSize(DeviceNode->ResourceListTranslated));
      ZwClose(PnpMgrLevel2);
      ASSERT(DeviceName);
      ExFreePool(DeviceName);
      if (!NT_SUCCESS(Status))
          return Status;
  }
  else
  {
      ZwClose(PnpMgrLevel2);
  }

  return STATUS_SUCCESS;
}

NTSTATUS
IopUpdateResourceMapForPnPDevice(IN PDEVICE_NODE DeviceNode)
{
  return IopUpdateResourceMap(DeviceNode, L"PnP Manager", L"PnpManager");
}

static
NTSTATUS
IopTranslateDeviceResources(
   IN PDEVICE_NODE DeviceNode)
{
   PCM_PARTIAL_RESOURCE_LIST pPartialResourceList;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR DescriptorRaw, DescriptorTranslated;
   ULONG i, j, ListSize;
   NTSTATUS Status;

   if (!DeviceNode->ResourceList)
   {
      DeviceNode->ResourceListTranslated = NULL;
      return STATUS_SUCCESS;
   }

   /* That's easy to translate a resource list. Just copy the
    * untranslated one and change few fields in the copy
    */
   ListSize = IopCalculateResourceListSize(DeviceNode->ResourceList);

   DeviceNode->ResourceListTranslated = ExAllocatePool(PagedPool, ListSize);
   if (!DeviceNode->ResourceListTranslated)
   {
      Status =STATUS_NO_MEMORY;
      goto cleanup;
   }
   RtlCopyMemory(DeviceNode->ResourceListTranslated, DeviceNode->ResourceList, ListSize);

   for (i = 0; i < DeviceNode->ResourceList->Count; i++)
   {
      pPartialResourceList = &DeviceNode->ResourceList->List[i].PartialResourceList;
      for (j = 0; j < pPartialResourceList->Count; j++)
      {
         DescriptorRaw = &pPartialResourceList->PartialDescriptors[j];
         DescriptorTranslated = &DeviceNode->ResourceListTranslated->List[i].PartialResourceList.PartialDescriptors[j];
         switch (DescriptorRaw->Type)
         {
            case CmResourceTypePort:
            {
               ULONG AddressSpace = 1; /* IO space */
               if (!HalTranslateBusAddress(
                  DeviceNode->ResourceList->List[i].InterfaceType,
                  DeviceNode->ResourceList->List[i].BusNumber,
                  DescriptorRaw->u.Port.Start,
                  &AddressSpace,
                  &DescriptorTranslated->u.Port.Start))
               {
                  Status = STATUS_UNSUCCESSFUL;
                  goto cleanup;
               }
               break;
            }
            case CmResourceTypeInterrupt:
            {
               DescriptorTranslated->u.Interrupt.Vector = HalGetInterruptVector(
                  DeviceNode->ResourceList->List[i].InterfaceType,
                  DeviceNode->ResourceList->List[i].BusNumber,
                  DescriptorRaw->u.Interrupt.Level,
                  DescriptorRaw->u.Interrupt.Vector,
                  (PKIRQL)&DescriptorTranslated->u.Interrupt.Level,
                  &DescriptorRaw->u.Interrupt.Affinity);
               break;
            }
            case CmResourceTypeMemory:
            {
               ULONG AddressSpace = 0; /* Memory space */
               if (!HalTranslateBusAddress(
                  DeviceNode->ResourceList->List[i].InterfaceType,
                  DeviceNode->ResourceList->List[i].BusNumber,
                  DescriptorRaw->u.Memory.Start,
                  &AddressSpace,
                  &DescriptorTranslated->u.Memory.Start))
               {
                  Status = STATUS_UNSUCCESSFUL;
                  goto cleanup;
               }
            }

            case CmResourceTypeDma:
            case CmResourceTypeBusNumber:
            case CmResourceTypeDeviceSpecific:
               /* Nothing to do */
               break;
            default:
               DPRINT1("Unknown resource descriptor type 0x%x\n", DescriptorRaw->Type);
               Status = STATUS_NOT_IMPLEMENTED;
               goto cleanup;
         }
      }
   }
   return STATUS_SUCCESS;

cleanup:
   /* Yes! Also delete ResourceList because ResourceList and
    * ResourceListTranslated should be a pair! */
   ExFreePool(DeviceNode->ResourceList);
   DeviceNode->ResourceList = NULL;
   if (DeviceNode->ResourceListTranslated)
   {
      ExFreePool(DeviceNode->ResourceListTranslated);
      DeviceNode->ResourceList = NULL;
   }
   return Status;
}

NTSTATUS
NTAPI
IopAssignDeviceResources(
   IN PDEVICE_NODE DeviceNode)
{
   NTSTATUS Status;
   ULONG ListSize;

   IopDeviceNodeSetFlag(DeviceNode, DNF_ASSIGNING_RESOURCES);

   Status = IopFilterResourceRequirements(DeviceNode);
   if (!NT_SUCCESS(Status))
       goto ByeBye;

   if (!DeviceNode->BootResources && !DeviceNode->ResourceRequirements)
   {
      DeviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED;
      DeviceNode->Flags &= ~DNF_ASSIGNING_RESOURCES;

      /* No resource needed for this device */
      DeviceNode->ResourceList = NULL;
      DeviceNode->ResourceListTranslated = NULL;

      return STATUS_SUCCESS;
   }

   /* Fill DeviceNode->ResourceList
    * FIXME: the PnP arbiter should go there!
    * Actually, use the BootResources if provided, else the resource requirements
    */

   if (DeviceNode->BootResources)
   {
      ListSize = IopCalculateResourceListSize(DeviceNode->BootResources);

      DeviceNode->ResourceList = ExAllocatePool(PagedPool, ListSize);
      if (!DeviceNode->ResourceList)
      {
         Status = STATUS_NO_MEMORY;
         goto ByeBye;
      }
      RtlCopyMemory(DeviceNode->ResourceList, DeviceNode->BootResources, ListSize);

      Status = IopDetectResourceConflict(DeviceNode->ResourceList, FALSE, NULL);
      if (NT_SUCCESS(Status) || !DeviceNode->ResourceRequirements)
      {
          if (!NT_SUCCESS(Status) && !DeviceNode->ResourceRequirements)
          {
              DPRINT1("Using conflicting boot resources because no requirements were supplied!\n");
          }

          goto Finish;
      }
      else
      {
          DPRINT1("Boot resources for %wZ cause a resource conflict!\n", &DeviceNode->InstancePath);
          ExFreePool(DeviceNode->ResourceList);
      }
   }

   Status = IopCreateResourceListFromRequirements(DeviceNode->ResourceRequirements,
                                                  &DeviceNode->ResourceList);
   if (!NT_SUCCESS(Status))
       goto ByeBye;

   Status = IopDetectResourceConflict(DeviceNode->ResourceList, FALSE, NULL);
   if (!NT_SUCCESS(Status))
       goto ByeBye;

Finish:
   Status = IopTranslateDeviceResources(DeviceNode);
   if (!NT_SUCCESS(Status))
       goto ByeBye;

   Status = IopUpdateResourceMapForPnPDevice(DeviceNode);
   if (!NT_SUCCESS(Status))
       goto ByeBye;

   Status = IopUpdateControlKeyWithResources(DeviceNode);
   if (!NT_SUCCESS(Status))
       goto ByeBye;

   IopDeviceNodeSetFlag(DeviceNode, DNF_RESOURCE_ASSIGNED);

   IopDeviceNodeClearFlag(DeviceNode, DNF_ASSIGNING_RESOURCES);

   return STATUS_SUCCESS;

ByeBye:
   if (DeviceNode->ResourceList)
   {
      ExFreePool(DeviceNode->ResourceList);
      DeviceNode->ResourceList = NULL;
   }

   DeviceNode->ResourceListTranslated = NULL;

   IopDeviceNodeClearFlag(DeviceNode, DNF_ASSIGNING_RESOURCES);

   return Status;
}

static
BOOLEAN
IopCheckForResourceConflict(
   IN PCM_RESOURCE_LIST ResourceList1,
   IN PCM_RESOURCE_LIST ResourceList2,
   IN BOOLEAN Silent,
   OUT OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor)
{
   ULONG i, ii;
   BOOLEAN Result = FALSE;

   for (i = 0; i < ResourceList1->Count; i++)
   {
      PCM_PARTIAL_RESOURCE_LIST ResList = &ResourceList1->List[i].PartialResourceList;
      for (ii = 0; ii < ResList->Count; ii++)
      {
         PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDesc = &ResList->PartialDescriptors[ii];

         Result = IopCheckResourceDescriptor(ResDesc,
                                             ResourceList2,
                                             Silent,
                                             ConflictingDescriptor);
         if (Result) goto ByeBye;
      }
   }

        
ByeBye:

   return Result;
}

NTSTATUS
IopDetectResourceConflict(
   IN PCM_RESOURCE_LIST ResourceList,
   IN BOOLEAN Silent,
   OUT OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING KeyName;
   HANDLE ResourceMapKey = INVALID_HANDLE_VALUE, ChildKey2 = INVALID_HANDLE_VALUE, ChildKey3 = INVALID_HANDLE_VALUE;
   ULONG KeyInformationLength, RequiredLength, KeyValueInformationLength, KeyNameInformationLength;
   PKEY_BASIC_INFORMATION KeyInformation;
   PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
   PKEY_VALUE_BASIC_INFORMATION KeyNameInformation;
   ULONG ChildKeyIndex1 = 0, ChildKeyIndex2 = 0, ChildKeyIndex3 = 0;
   NTSTATUS Status;

   RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\HARDWARE\\RESOURCEMAP");
   InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, 0, NULL);
   Status = ZwOpenKey(&ResourceMapKey, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &ObjectAttributes);
   if (!NT_SUCCESS(Status))
   {
      /* The key is missing which means we are the first device */
      return STATUS_SUCCESS;
   }

   while (TRUE)
   {
      Status = ZwEnumerateKey(ResourceMapKey,
                              ChildKeyIndex1,
                              KeyBasicInformation,
                              NULL,
                              0,
                              &RequiredLength);
      if (Status == STATUS_NO_MORE_ENTRIES)
          break;
      else if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
      {
          KeyInformationLength = RequiredLength;
          KeyInformation = ExAllocatePool(PagedPool, KeyInformationLength);
          if (!KeyInformation)
          {
              Status = STATUS_INSUFFICIENT_RESOURCES;
              goto cleanup;
          }

          Status = ZwEnumerateKey(ResourceMapKey, 
                                  ChildKeyIndex1,
                                  KeyBasicInformation,
                                  KeyInformation,
                                  KeyInformationLength,
                                  &RequiredLength);
      }
      else
         goto cleanup;
      ChildKeyIndex1++;
      if (!NT_SUCCESS(Status))
          goto cleanup;

      KeyName.Buffer = KeyInformation->Name;
      KeyName.MaximumLength = KeyName.Length = KeyInformation->NameLength;
      InitializeObjectAttributes(&ObjectAttributes,
                                 &KeyName,
                                 OBJ_CASE_INSENSITIVE,
                                 ResourceMapKey,
                                 NULL);
      Status = ZwOpenKey(&ChildKey2, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &ObjectAttributes);
      ExFreePool(KeyInformation);
      if (!NT_SUCCESS(Status))
          goto cleanup;

      while (TRUE)
      {
          Status = ZwEnumerateKey(ChildKey2, 
                                  ChildKeyIndex2,
                                  KeyBasicInformation,
                                  NULL,
                                  0,
                                  &RequiredLength);
          if (Status == STATUS_NO_MORE_ENTRIES)
              break;
          else if (Status == STATUS_BUFFER_TOO_SMALL)
          {
              KeyInformationLength = RequiredLength;
              KeyInformation = ExAllocatePool(PagedPool, KeyInformationLength);
              if (!KeyInformation)
              {
                  Status = STATUS_INSUFFICIENT_RESOURCES;
                  goto cleanup;
              }

              Status = ZwEnumerateKey(ChildKey2,
                                      ChildKeyIndex2,
                                      KeyBasicInformation,
                                      KeyInformation,
                                      KeyInformationLength,
                                      &RequiredLength);
          }
          else
              goto cleanup;
          ChildKeyIndex2++;
          if (!NT_SUCCESS(Status))
              goto cleanup;

          KeyName.Buffer = KeyInformation->Name;
          KeyName.MaximumLength = KeyName.Length = KeyInformation->NameLength;
          InitializeObjectAttributes(&ObjectAttributes,
                                     &KeyName,
                                     OBJ_CASE_INSENSITIVE,
                                     ChildKey2,
                                     NULL);
          Status = ZwOpenKey(&ChildKey3, KEY_QUERY_VALUE, &ObjectAttributes);
          ExFreePool(KeyInformation);
          if (!NT_SUCCESS(Status))
              goto cleanup;

          while (TRUE)
          {
              Status = ZwEnumerateValueKey(ChildKey3,
                                           ChildKeyIndex3,
                                           KeyValuePartialInformation,
                                           NULL,
                                           0,
                                           &RequiredLength);
              if (Status == STATUS_NO_MORE_ENTRIES)
                  break;
              else if (Status == STATUS_BUFFER_TOO_SMALL)
              {
                  KeyValueInformationLength = RequiredLength;
                  KeyValueInformation = ExAllocatePool(PagedPool, KeyValueInformationLength);
                  if (!KeyValueInformation)
                  {
                      Status = STATUS_INSUFFICIENT_RESOURCES;
                      goto cleanup;
                  }

                  Status = ZwEnumerateValueKey(ChildKey3,
                                               ChildKeyIndex3,
                                               KeyValuePartialInformation,
                                               KeyValueInformation,
                                               KeyValueInformationLength,
                                               &RequiredLength);
              }
              else
                  goto cleanup;
              if (!NT_SUCCESS(Status))
                  goto cleanup;

              Status = ZwEnumerateValueKey(ChildKey3,
                                           ChildKeyIndex3,
                                           KeyValueBasicInformation,
                                           NULL,
                                           0,
                                           &RequiredLength);
              if (Status == STATUS_BUFFER_TOO_SMALL)
              {
                  KeyNameInformationLength = RequiredLength;
                  KeyNameInformation = ExAllocatePool(PagedPool, KeyNameInformationLength + sizeof(WCHAR));
                  if (!KeyNameInformation)
                  {
                      Status = STATUS_INSUFFICIENT_RESOURCES;
                      goto cleanup;
                  }

                  Status = ZwEnumerateValueKey(ChildKey3,
                                               ChildKeyIndex3,
                                               KeyValueBasicInformation,
                                               KeyNameInformation,
                                               KeyNameInformationLength,
                                               &RequiredLength);
              }
              else
                  goto cleanup;

              ChildKeyIndex3++;

              if (!NT_SUCCESS(Status))
                  goto cleanup;

              KeyNameInformation->Name[KeyNameInformation->NameLength / sizeof(WCHAR)] = UNICODE_NULL;

              /* Skip translated entries */
              if (wcsstr(KeyNameInformation->Name, L".Translated"))
              {
                  ExFreePool(KeyNameInformation);
                  continue;
              }

              ExFreePool(KeyNameInformation);

              if (IopCheckForResourceConflict(ResourceList,
                                              (PCM_RESOURCE_LIST)KeyValueInformation->Data,
                                              Silent,
                                              ConflictingDescriptor))
              {
                  ExFreePool(KeyValueInformation);
                  Status = STATUS_CONFLICTING_ADDRESSES;
                  goto cleanup;
              }

              ExFreePool(KeyValueInformation);
          }
      }
   }

cleanup:
   if (ResourceMapKey != INVALID_HANDLE_VALUE)
       ZwClose(ResourceMapKey);
   if (ChildKey2 != INVALID_HANDLE_VALUE)
       ZwClose(ChildKey2);
   if (ChildKey3 != INVALID_HANDLE_VALUE)
       ZwClose(ChildKey3);

   if (Status == STATUS_NO_MORE_ENTRIES)
       Status = STATUS_SUCCESS;

   return Status;
}

