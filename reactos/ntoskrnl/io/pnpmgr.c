/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/pnpmgr.c
 * PURPOSE:         Initializes the PnP manager
 *
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <ddk/wdmguid.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

PDEVICE_NODE IopRootDeviceNode;
KSPIN_LOCK IopDeviceTreeLock;

/* DATA **********************************************************************/

PDRIVER_OBJECT IopRootDriverObject;

/* FUNCTIONS *****************************************************************/

PDEVICE_NODE FASTCALL
IopGetDeviceNode(
  PDEVICE_OBJECT DeviceObject)
{
  return DeviceObject->DeviceObjectExtension->DeviceNode;
}

/*
 * @implemented
 */
VOID
STDCALL
IoInvalidateDeviceRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN DEVICE_RELATION_TYPE Type)
{
  IopInvalidateDeviceRelations(IopGetDeviceNode(DeviceObject), Type);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoGetDeviceProperty(
  IN PDEVICE_OBJECT DeviceObject,
  IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
  IN ULONG BufferLength,
  OUT PVOID PropertyBuffer,
  OUT PULONG ResultLength)
{
  PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);
  ULONG Length;
  PVOID Data = NULL;
  PWSTR Ptr;

  DPRINT("IoGetDeviceProperty(%x %d)\n", DeviceObject, DeviceProperty);

  if (DeviceNode == NULL)
    return STATUS_INVALID_DEVICE_REQUEST;

  switch (DeviceProperty)
  {
    case DevicePropertyBusNumber:
      Length = sizeof(ULONG);
      Data = &DeviceNode->ChildBusNumber;
      break;

    /* Complete, untested */
    case DevicePropertyBusTypeGuid:
      *ResultLength = 39 * sizeof(WCHAR);
      if (BufferLength < (39 * sizeof(WCHAR)))
        return STATUS_BUFFER_TOO_SMALL;
      swprintf((PWSTR)PropertyBuffer,
        L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        DeviceNode->BusTypeGuid.Data1,
        DeviceNode->BusTypeGuid.Data2,
        DeviceNode->BusTypeGuid.Data3,
        DeviceNode->BusTypeGuid.Data4[0],
        DeviceNode->BusTypeGuid.Data4[1],
        DeviceNode->BusTypeGuid.Data4[2],
        DeviceNode->BusTypeGuid.Data4[3],
        DeviceNode->BusTypeGuid.Data4[4],
        DeviceNode->BusTypeGuid.Data4[5],
        DeviceNode->BusTypeGuid.Data4[6],
        DeviceNode->BusTypeGuid.Data4[7]);
      return STATUS_SUCCESS;

    case DevicePropertyLegacyBusType:
      Length = sizeof(INTERFACE_TYPE);
      Data = &DeviceNode->ChildInterfaceType;
      break;

    case DevicePropertyAddress:
      Length = sizeof(ULONG);
      Data = &DeviceNode->Address;
      break;

//    case DevicePropertyUINumber:
//      if (DeviceNode->CapabilityFlags == NULL)
//         return STATUS_INVALID_DEVICE_REQUEST;
//      Length = sizeof(ULONG);
//      Data = &DeviceNode->CapabilityFlags->UINumber;
//      break;

    case DevicePropertyClassName:
    case DevicePropertyClassGuid:
    case DevicePropertyDriverKeyName:
    case DevicePropertyManufacturer:
    case DevicePropertyFriendlyName:
    case DevicePropertyHardwareID:
    case DevicePropertyCompatibleIDs:
    case DevicePropertyDeviceDescription:
    case DevicePropertyLocationInformation:
    case DevicePropertyUINumber:
      {
        LPWSTR RegistryPropertyName, KeyNameBuffer;
        UNICODE_STRING KeyName, ValueName;
        OBJECT_ATTRIBUTES ObjectAttributes;
        KEY_VALUE_PARTIAL_INFORMATION *ValueInformation;
        ULONG ValueInformationLength;
        HANDLE KeyHandle;
        NTSTATUS Status;

        switch (DeviceProperty)
        {
          case DevicePropertyClassName:
            RegistryPropertyName = L"Class"; break;
          case DevicePropertyClassGuid:
            RegistryPropertyName = L"ClassGuid"; break;
          case DevicePropertyDriverKeyName:
            RegistryPropertyName = L"Driver"; break;
          case DevicePropertyManufacturer:
            RegistryPropertyName = L"Mfg"; break;
          case DevicePropertyFriendlyName:
            RegistryPropertyName = L"FriendlyName"; break;
          case DevicePropertyHardwareID:
            RegistryPropertyName = L"HardwareID"; break;
          case DevicePropertyCompatibleIDs:
            RegistryPropertyName = L"CompatibleIDs"; break;
          case DevicePropertyDeviceDescription:
            RegistryPropertyName = L"DeviceDesc"; break;
          case DevicePropertyLocationInformation:
            RegistryPropertyName = L"LocationInformation"; break;
          case DevicePropertyUINumber:
            RegistryPropertyName = L"UINumber"; break;
          default:
            RegistryPropertyName = NULL; break;
        }

        KeyNameBuffer = ExAllocatePool(PagedPool,
          (49 * sizeof(WCHAR)) + DeviceNode->InstancePath.Length);

	DPRINT("KeyNameBuffer: %x, value %S\n",
		KeyNameBuffer, RegistryPropertyName);

        if (KeyNameBuffer == NULL)
          return STATUS_INSUFFICIENT_RESOURCES;

        wcscpy(KeyNameBuffer, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
        wcscat(KeyNameBuffer, DeviceNode->InstancePath.Buffer);
        RtlInitUnicodeString(&KeyName, KeyNameBuffer);
        InitializeObjectAttributes(&ObjectAttributes, &KeyName,
                                   OBJ_CASE_INSENSITIVE, NULL, NULL);

        Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
        ExFreePool(KeyNameBuffer);
        if (!NT_SUCCESS(Status))
          return Status;

        RtlInitUnicodeString(&ValueName, RegistryPropertyName);
        ValueInformationLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,
                                 Data[0]) + BufferLength;
        ValueInformation = ExAllocatePool(PagedPool, ValueInformationLength);
        if (ValueInformation == NULL)
        {
          ZwClose(KeyHandle);
          return STATUS_INSUFFICIENT_RESOURCES;
        }

        Status = ZwQueryValueKey(KeyHandle, &ValueName,
                                 KeyValuePartialInformation, ValueInformation,
                                 ValueInformationLength,
                                 &ValueInformationLength);
        *ResultLength = ValueInformation->DataLength;
        ZwClose(KeyHandle);

        if (ValueInformation->DataLength > BufferLength)
          Status = STATUS_BUFFER_TOO_SMALL;

        if (!NT_SUCCESS(Status))
        {
          ExFreePool(ValueInformation);
          return Status;
        }

        /* FIXME: Verify the value (NULL-terminated, correct format). */

        RtlCopyMemory(PropertyBuffer, ValueInformation->Data,
                      ValueInformation->DataLength);
        ExFreePool(ValueInformation);

        return STATUS_SUCCESS;
      }

    case DevicePropertyBootConfiguration:
      Length = 0;
      if (DeviceNode->BootResources->Count != 0)
      {
	Length = CM_RESOURCE_LIST_SIZE(DeviceNode->BootResources);
      }
      Data = &DeviceNode->BootResources;
      break;

    /* FIXME: use a translated boot configuration instead */
    case DevicePropertyBootConfigurationTranslated:
      Length = 0;
      if (DeviceNode->BootResources->Count != 0)
      {
	Length = CM_RESOURCE_LIST_SIZE(DeviceNode->BootResources);
      }
      Data = &DeviceNode->BootResources;
      break;

    case DevicePropertyEnumeratorName:
      Ptr = wcschr(DeviceNode->InstancePath.Buffer, L'\\');
      if (Ptr != NULL)
      {
	Length = (ULONG)((ULONG_PTR)Ptr - (ULONG_PTR)DeviceNode->InstancePath.Buffer) + sizeof(WCHAR);
	Data = DeviceNode->InstancePath.Buffer;
      }
      else
      {
	Length = 0;
	Data = NULL;
      }
      break;

    case DevicePropertyPhysicalDeviceObjectName:
      Length = DeviceNode->InstancePath.Length + sizeof(WCHAR);
      Data = DeviceNode->InstancePath.Buffer;
      break;

    default:
      return STATUS_INVALID_PARAMETER_2;
  }

  *ResultLength = Length;
  if (BufferLength < Length)
    return STATUS_BUFFER_TOO_SMALL;
  RtlCopyMemory(PropertyBuffer, Data, Length);

  /* Terminate the string */
  if (DeviceProperty == DevicePropertyEnumeratorName
    || DeviceProperty == DevicePropertyPhysicalDeviceObjectName)
  {
    Ptr = (PWSTR)PropertyBuffer;
    Ptr[(Length / sizeof(WCHAR)) - 1] = 0;
  }

  return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
VOID
STDCALL
IoInvalidateDeviceState(
  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
}

/**
 * @name IoOpenDeviceRegistryKey
 *
 * Open a registry key unique for a specified driver or device instance.
 *
 * @param DeviceObject   Device to get the registry key for.
 * @param DevInstKeyType Type of the key to return.
 * @param DesiredAccess  Access mask (eg. KEY_READ | KEY_WRITE).
 * @param DevInstRegKey  Handle to the opened registry key on
 *                       successful return.
 *
 * @return Status.
 *
 * @implemented
 */
NTSTATUS
STDCALL
IoOpenDeviceRegistryKey(
   IN PDEVICE_OBJECT DeviceObject,
   IN ULONG DevInstKeyType,
   IN ACCESS_MASK DesiredAccess,
   OUT PHANDLE DevInstRegKey)
{
   static WCHAR RootKeyName[] =
      L"\\Registry\\Machine\\System\\CurrentControlSet\\";
   static WCHAR ProfileKeyName[] =
      L"Hardware Profiles\\Current\\System\\CurrentControlSet\\";
   static WCHAR ClassKeyName[] = L"Control\\Class\\";
   static WCHAR EnumKeyName[] = L"Enum\\";
   static WCHAR DeviceParametersKeyName[] = L"Device Parameters\\";
   ULONG KeyNameLength;
   LPWSTR KeyNameBuffer;
   UNICODE_STRING KeyName;
   ULONG DriverKeyLength;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PDEVICE_NODE DeviceNode = NULL;
   NTSTATUS Status;

   if ((DevInstKeyType & (PLUGPLAY_REGKEY_DEVICE | PLUGPLAY_REGKEY_DRIVER)) == 0)
      return STATUS_INVALID_PARAMETER;

   /*
    * Calculate the length of the base key name. This is the full
    * name for driver key or the name excluding "Device Parameters"
    * subkey for device key.
    */

   KeyNameLength = sizeof(RootKeyName);
   if (DevInstKeyType & PLUGPLAY_REGKEY_CURRENT_HWPROFILE)
      KeyNameLength += sizeof(ProfileKeyName) - sizeof(UNICODE_NULL);
   if (DevInstKeyType & PLUGPLAY_REGKEY_DRIVER)
   {
      KeyNameLength += sizeof(ClassKeyName) - sizeof(UNICODE_NULL);
      Status = IoGetDeviceProperty(DeviceObject, DevicePropertyDriverKeyName,
                                   0, NULL, &DriverKeyLength);
      if (Status != STATUS_BUFFER_TOO_SMALL)
         return Status;
      KeyNameLength += DriverKeyLength;
   }
   else
   {
      DeviceNode = IopGetDeviceNode(DeviceObject);
      KeyNameLength += sizeof(EnumKeyName) - sizeof(UNICODE_NULL) +
                       DeviceNode->InstancePath.Length;
   }

   /*
    * Now allocate the buffer for the key name...
    */

   KeyNameBuffer = ExAllocatePool(PagedPool, KeyNameLength);
   if (KeyNameBuffer == NULL)
      return STATUS_INSUFFICIENT_RESOURCES;

   KeyName.Length = 0;
   KeyName.MaximumLength = KeyNameLength;
   KeyName.Buffer = KeyNameBuffer;

   /*
    * ...and build the key name.
    */

   KeyName.Length += sizeof(RootKeyName) - sizeof(UNICODE_NULL);
   RtlCopyMemory(KeyNameBuffer, RootKeyName, KeyName.Length);

   if (DevInstKeyType & PLUGPLAY_REGKEY_CURRENT_HWPROFILE)
      RtlAppendUnicodeToString(&KeyName, ProfileKeyName);

   if (DevInstKeyType & PLUGPLAY_REGKEY_DRIVER)
   {
      RtlAppendUnicodeToString(&KeyName, ClassKeyName);
      Status = IoGetDeviceProperty(DeviceObject, DevicePropertyDriverKeyName,
                                   DriverKeyLength, KeyNameBuffer +
                                   (KeyName.Length / sizeof(WCHAR)),
                                   &DriverKeyLength);
      if (!NT_SUCCESS(Status))
      {
         ExFreePool(KeyNameBuffer);
         return Status;
      }
      KeyName.Length += DriverKeyLength - sizeof(UNICODE_NULL);
   }
   else
   {
      RtlAppendUnicodeToString(&KeyName, EnumKeyName);
      Status = RtlAppendUnicodeStringToString(&KeyName, &DeviceNode->InstancePath);
      if (DeviceNode->InstancePath.Length == 0)
      {
         ExFreePool(KeyNameBuffer);
         return Status;
      }
   }

   /*
    * Open the base key.
    */

   InitializeObjectAttributes(&ObjectAttributes, &KeyName,
                              OBJ_CASE_INSENSITIVE, NULL, NULL);
   Status = ZwOpenKey(DevInstRegKey, DesiredAccess, &ObjectAttributes);
   ExFreePool(KeyNameBuffer);

   /*
    * For driver key we're done now. Also if the base key doesn't
    * exist we can bail out with error...
    */

   if ((DevInstKeyType & PLUGPLAY_REGKEY_DRIVER) || !NT_SUCCESS(Status))
      return Status;

   /*
    * Let's go further. For device key we must open "Device Parameters"
    * subkey and create it if it doesn't exist yet.
    */

   RtlInitUnicodeString(&KeyName, DeviceParametersKeyName);
   InitializeObjectAttributes(&ObjectAttributes, &KeyName,
                              OBJ_CASE_INSENSITIVE, *DevInstRegKey, NULL);
   Status = ZwCreateKey(DevInstRegKey, DesiredAccess, &ObjectAttributes,
                        0, NULL, REG_OPTION_NON_VOLATILE, NULL);
   ZwClose(ObjectAttributes.RootDirectory);

   return Status;
}

/*
 * @unimplemented
 */
VOID
STDCALL
IoRequestDeviceEject(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
{
	UNIMPLEMENTED;
}


BOOLEAN
IopCreateUnicodeString(
  PUNICODE_STRING Destination,
  PWSTR Source,
  POOL_TYPE PoolType)
{
  ULONG Length;

  if (!Source)
  {
    RtlInitUnicodeString(Destination, NULL);
    return TRUE;
  }

  Length = (wcslen(Source) + 1) * sizeof(WCHAR);

  Destination->Buffer = ExAllocatePool(PoolType, Length);

  if (Destination->Buffer == NULL)
  {
    return FALSE;
  }

  RtlCopyMemory(Destination->Buffer, Source, Length);

  Destination->MaximumLength = Length;

  Destination->Length = Length - sizeof(WCHAR);

  return TRUE;
}

NTSTATUS
IopGetSystemPowerDeviceObject(PDEVICE_OBJECT *DeviceObject)
{
  KIRQL OldIrql;

  if (PopSystemPowerDeviceNode)
  {
    KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    *DeviceObject = PopSystemPowerDeviceNode->PhysicalDeviceObject;
    KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

    return STATUS_SUCCESS;
  }

  return STATUS_UNSUCCESSFUL;
}

/*
 * DESCRIPTION
 * 	Creates a device node
 *
 * ARGUMENTS
 *   ParentNode           = Pointer to parent device node
 *   PhysicalDeviceObject = Pointer to PDO for device object. Pass NULL
 *                          to have the root device node create one
 *                          (eg. for legacy drivers)
 *   DeviceNode           = Pointer to storage for created device node
 *
 * RETURN VALUE
 * 	Status
 */
NTSTATUS
IopCreateDeviceNode(PDEVICE_NODE ParentNode,
  PDEVICE_OBJECT PhysicalDeviceObject,
  PDEVICE_NODE *DeviceNode)
{
  PDEVICE_NODE Node;
  NTSTATUS Status;
  KIRQL OldIrql;

  DPRINT("ParentNode %x PhysicalDeviceObject %x\n",
    ParentNode, PhysicalDeviceObject);

  Node = (PDEVICE_NODE)ExAllocatePool(NonPagedPool, sizeof(DEVICE_NODE));
  if (!Node)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory(Node, sizeof(DEVICE_NODE));

  if (!PhysicalDeviceObject)
    {
      Status = PnpRootCreateDevice(&PhysicalDeviceObject);
      if (!NT_SUCCESS(Status))
        {
          ExFreePool(Node);
          return Status;
        }

      /* This is for drivers passed on the command line to ntoskrnl.exe */
      IopDeviceNodeSetFlag(Node, DNF_STARTED);
      IopDeviceNodeSetFlag(Node, DNF_LEGACY_DRIVER);
    }

  Node->PhysicalDeviceObject = PhysicalDeviceObject;

  PhysicalDeviceObject->DeviceObjectExtension->DeviceNode = Node;

  if (ParentNode)
    {
      KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
      Node->Parent = ParentNode;
      Node->NextSibling = ParentNode->Child;
      if (ParentNode->Child != NULL)
	{
	  ParentNode->Child->PrevSibling = Node;
	}
      ParentNode->Child = Node;
      KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);
    }

  *DeviceNode = Node;

  return STATUS_SUCCESS;
}

NTSTATUS
IopFreeDeviceNode(PDEVICE_NODE DeviceNode)
{
  KIRQL OldIrql;

  /* All children must be deleted before a parent is deleted */
  ASSERT(!DeviceNode->Child);

  KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);

  ASSERT(DeviceNode->PhysicalDeviceObject);

  ObDereferenceObject(DeviceNode->PhysicalDeviceObject);

  /* Unlink from parent if it exists */

  if ((DeviceNode->Parent) && (DeviceNode->Parent->Child == DeviceNode))
  {
    DeviceNode->Parent->Child = DeviceNode->NextSibling;
  }

  /* Unlink from sibling list */

  if (DeviceNode->PrevSibling)
  {
    DeviceNode->PrevSibling->NextSibling = DeviceNode->NextSibling;
  }

  if (DeviceNode->NextSibling)
  {
    DeviceNode->NextSibling->PrevSibling = DeviceNode->PrevSibling;
  }

  KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

  RtlFreeUnicodeString(&DeviceNode->InstancePath);

  RtlFreeUnicodeString(&DeviceNode->ServiceName);

  if (DeviceNode->ResourceList)
  {
    ExFreePool(DeviceNode->ResourceList);
  }

  if (DeviceNode->ResourceListTranslated)
  {
    ExFreePool(DeviceNode->ResourceListTranslated);
  }

  if (DeviceNode->ResourceRequirements)
  {
    ExFreePool(DeviceNode->ResourceRequirements);
  }

  if (DeviceNode->BootResources)
  {
    ExFreePool(DeviceNode->BootResources);
  }

  ExFreePool(DeviceNode);

  return STATUS_SUCCESS;
}

NTSTATUS
IopInitiatePnpIrp(
  PDEVICE_OBJECT DeviceObject,
  PIO_STATUS_BLOCK IoStatusBlock,
  ULONG MinorFunction,
  PIO_STACK_LOCATION Stack OPTIONAL)
{
  PDEVICE_OBJECT TopDeviceObject;
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;

  /* Always call the top of the device stack */
  TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

  KeInitializeEvent(
    &Event,
    NotificationEvent,
    FALSE);

  Irp = IoBuildSynchronousFsdRequest(
    IRP_MJ_PNP,
    TopDeviceObject,
    NULL,
    0,
    NULL,
    &Event,
    IoStatusBlock);

  /* PNP IRPs are always initialized with a status code of
     STATUS_NOT_IMPLEMENTED */
  Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
  Irp->IoStatus.Information = 0;

  IrpSp = IoGetNextIrpStackLocation(Irp);
  IrpSp->MinorFunction = MinorFunction;

  if (Stack)
  {
    RtlMoveMemory(
      &IrpSp->Parameters,
      &Stack->Parameters,
      sizeof(Stack->Parameters));
  }

  Status = IoCallDriver(TopDeviceObject, Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(
        &Event,
        Executive,
        KernelMode,
        FALSE,
        NULL);
      Status = IoStatusBlock->Status;
    }

  ObDereferenceObject(TopDeviceObject);

  return Status;
}


NTSTATUS
IopTraverseDeviceTreeNode(
  PDEVICETREE_TRAVERSE_CONTEXT Context)
{
  PDEVICE_NODE ParentDeviceNode;
  PDEVICE_NODE ChildDeviceNode;
  NTSTATUS Status;

  /* Copy context data so we don't overwrite it in subsequent calls to this function */
  ParentDeviceNode = Context->DeviceNode;

  /* Call the action routine */
  Status = (Context->Action)(ParentDeviceNode, Context->Context);
  if (!NT_SUCCESS(Status))
  {
    return Status;
  }

  /* Traversal of all children nodes */
  for (ChildDeviceNode = ParentDeviceNode->Child;
       ChildDeviceNode != NULL;
       ChildDeviceNode = ChildDeviceNode->NextSibling)
  {
    /* Pass the current device node to the action routine */
    Context->DeviceNode = ChildDeviceNode;

    Status = IopTraverseDeviceTreeNode(Context);
    if (!NT_SUCCESS(Status))
    {
      return Status;
    }
  }

  return Status;
}


NTSTATUS
IopTraverseDeviceTree(
  PDEVICETREE_TRAVERSE_CONTEXT Context)
{
  NTSTATUS Status;

  DPRINT("Context %x\n", Context);

  DPRINT("IopTraverseDeviceTree(DeviceNode %x  FirstDeviceNode %x  Action %x  Context %x)\n",
    Context->DeviceNode, Context->FirstDeviceNode, Context->Action, Context->Context);

  /* Start from the specified device node */
  Context->DeviceNode = Context->FirstDeviceNode;

  /* Recursively traverse the device tree */
  Status = IopTraverseDeviceTreeNode(Context);
  if (Status == STATUS_UNSUCCESSFUL)
  {
    /* The action routine just wanted to terminate the traversal with status
       code STATUS_SUCCESS */
    Status = STATUS_SUCCESS;
  }

  return Status;
}


static NTSTATUS
IopCreateDeviceKeyPath(PWSTR Path,
		       PHANDLE Handle)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR KeyBuffer[MAX_PATH];
  UNICODE_STRING KeyName;
  HANDLE KeyHandle;
  NTSTATUS Status;
  PWCHAR Current;
  PWCHAR Next;

  *Handle = NULL;

  if (_wcsnicmp(Path, L"\\Registry\\", 10) != 0)
    {
      return STATUS_INVALID_PARAMETER;
    }

  wcsncpy (KeyBuffer, Path, MAX_PATH-1);

  /* Skip \\Registry\\ */
  Current = KeyBuffer;
  Current = wcschr (Current, L'\\') + 1;
  Current = wcschr (Current, L'\\') + 1;

  while (TRUE)
    {
      Next = wcschr (Current, L'\\');
      if (Next == NULL)
	{
	  /* The end */
	}
      else
	{
	  *Next = 0;
	}

      RtlInitUnicodeString (&KeyName, KeyBuffer);
      InitializeObjectAttributes (&ObjectAttributes,
				  &KeyName,
				  OBJ_CASE_INSENSITIVE,
				  NULL,
				  NULL);

      DPRINT("Create '%S'\n", KeyName.Buffer);

      Status = ZwCreateKey (&KeyHandle,
			    KEY_ALL_ACCESS,
			    &ObjectAttributes,
			    0,
			    NULL,
			    0,
			    NULL);
      if (!NT_SUCCESS (Status))
	{
	  DPRINT ("ZwCreateKey() failed with status %x\n", Status);
	  return Status;
	}

      if (Next == NULL)
	{
	  *Handle = KeyHandle;
	  return STATUS_SUCCESS;
	}
      else
	{
	  ZwClose (KeyHandle);
	  *Next = L'\\';
	}

      Current = Next + 1;
    }

  return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
IopSetDeviceInstanceData(HANDLE InstanceKey,
			 PDEVICE_NODE DeviceNode)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  HANDLE LogConfKey;
  ULONG ResCount;
  ULONG ListSize;
  NTSTATUS Status;

  DPRINT("IopSetDeviceInstanceData() called\n");

  /* Create the 'LogConf' key */
  RtlInitUnicodeString(&KeyName,
		       L"LogConf");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     InstanceKey,
			     NULL);
  Status = ZwCreateKey(&LogConfKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       0,
		       NULL);
  if (NT_SUCCESS(Status))
  {
    /* Set 'BootConfig' value */
    if (DeviceNode->BootResources != NULL)
    {
      ResCount = DeviceNode->BootResources->Count;
      if (ResCount != 0)
      {
	ListSize = CM_RESOURCE_LIST_SIZE(DeviceNode->BootResources);

	RtlInitUnicodeString(&KeyName,
			     L"BootConfig");
	Status = ZwSetValueKey(LogConfKey,
			       &KeyName,
			       0,
			       REG_RESOURCE_LIST,
			       &DeviceNode->BootResources,
			       ListSize);
      }
    }

    /* Set 'BasicConfigVector' value */
    if (DeviceNode->ResourceRequirements != NULL &&
	DeviceNode->ResourceRequirements->ListSize != 0)
    {
      RtlInitUnicodeString(&KeyName,
			   L"BasicConfigVector");
      Status = ZwSetValueKey(LogConfKey,
			     &KeyName,
			     0,
			     REG_RESOURCE_REQUIREMENTS_LIST,
			     &DeviceNode->ResourceRequirements,
			     DeviceNode->ResourceRequirements->ListSize);
    }

    ZwClose(LogConfKey);
  }

  DPRINT("IopSetDeviceInstanceData() done\n");

  return STATUS_SUCCESS;
}


NTSTATUS
IopAssignDeviceResources(
   PDEVICE_NODE DeviceNode)
{
   PIO_RESOURCE_LIST ResourceList;
   PIO_RESOURCE_DESCRIPTOR ResourceDescriptor;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR DescriptorRaw, DescriptorTranslated;
   ULONG NumberOfResources = 0;
   ULONG i;
   NTSTATUS Status;
   
   /* Fill DeviceNode->ResourceList and DeviceNode->ResourceListTranslated;
    * by using DeviceNode->ResourceRequirements */
   
   if (!DeviceNode->ResourceRequirements
      || DeviceNode->ResourceRequirements->AlternativeLists == 0)
   {
      DeviceNode->ResourceList = DeviceNode->ResourceListTranslated = NULL;
      return STATUS_SUCCESS;
   }
   
   /* FIXME: that's here that PnP arbiter should go */
   /* Actually, simply use resource list #0 as assigned resource list */
   ResourceList = &DeviceNode->ResourceRequirements->List[0];
   if (ResourceList->Version != 1 || ResourceList->Revision != 1)
   {
      Status = STATUS_REVISION_MISMATCH;
      goto ByeBye;
   }
   
   DeviceNode->ResourceList = ExAllocatePool(PagedPool, 
      sizeof(CM_RESOURCE_LIST) + ResourceList->Count * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
   if (!DeviceNode->ResourceList)
   {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
   }
   
   DeviceNode->ResourceListTranslated = ExAllocatePool(PagedPool, 
      sizeof(CM_RESOURCE_LIST) + ResourceList->Count * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
   if (!DeviceNode->ResourceListTranslated)
   {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
   }
   
   DeviceNode->ResourceList->Count = 1;
   DeviceNode->ResourceList->List[0].InterfaceType = DeviceNode->ResourceRequirements->InterfaceType;
   DeviceNode->ResourceList->List[0].BusNumber = DeviceNode->ResourceRequirements->BusNumber;
   DeviceNode->ResourceList->List[0].PartialResourceList.Version = 1;
   DeviceNode->ResourceList->List[0].PartialResourceList.Revision = 1;
   
   DeviceNode->ResourceListTranslated->Count = 1;
   DeviceNode->ResourceListTranslated->List[0].InterfaceType = DeviceNode->ResourceRequirements->InterfaceType;
   DeviceNode->ResourceListTranslated->List[0].BusNumber = DeviceNode->ResourceRequirements->BusNumber;
   DeviceNode->ResourceListTranslated->List[0].PartialResourceList.Version = 1;
   DeviceNode->ResourceListTranslated->List[0].PartialResourceList.Revision = 1;
   
   for (i = 0; i < ResourceList->Count; i++)
   {
      ResourceDescriptor = &ResourceList->Descriptors[i];
      
      if (ResourceDescriptor->Option == 0 || ResourceDescriptor->Option == IO_RESOURCE_PREFERRED)
      {
         DescriptorRaw = &DeviceNode->ResourceList->List[0].PartialResourceList.PartialDescriptors[NumberOfResources];
         DescriptorTranslated = &DeviceNode->ResourceListTranslated->List[0].PartialResourceList.PartialDescriptors[NumberOfResources];
         NumberOfResources++;
         
         /* Copy ResourceDescriptor to DescriptorRaw and DescriptorTranslated */
         DescriptorRaw->Type = DescriptorTranslated->Type = ResourceDescriptor->Type;
         DescriptorRaw->ShareDisposition = DescriptorTranslated->ShareDisposition = ResourceDescriptor->ShareDisposition;
         DescriptorRaw->Flags = DescriptorTranslated->Flags = ResourceDescriptor->Flags;
         switch (ResourceDescriptor->Type)
         {
            case CmResourceTypePort:
            {
               ULONG AddressSpace = 0; /* IO space */
               DescriptorRaw->u.Port.Start = ResourceDescriptor->u.Port.MinimumAddress;
               DescriptorRaw->u.Port.Length = DescriptorTranslated->u.Port.Length
                  = ResourceDescriptor->u.Port.Length;
               if (!HalTranslateBusAddress(
                  DeviceNode->ResourceRequirements->InterfaceType,
                  DeviceNode->ResourceRequirements->BusNumber,
                  DescriptorRaw->u.Port.Start,
                  &AddressSpace,
                  &DescriptorTranslated->u.Port.Start))
               {
                  Status = STATUS_UNSUCCESSFUL;
                  goto ByeBye;
               }
               break;
            }
            case CmResourceTypeInterrupt:
            {
               DescriptorRaw->u.Interrupt.Level = 0;
               /* FIXME: if IRQ 9 is in the possible range, use it.
                * This should be a PCI device */
               if (ResourceDescriptor->u.Interrupt.MinimumVector <= 9
                  && ResourceDescriptor->u.Interrupt.MaximumVector >= 9)
                  DescriptorRaw->u.Interrupt.Vector = 9;
               else
                  DescriptorRaw->u.Interrupt.Vector = ResourceDescriptor->u.Interrupt.MinimumVector;
               
               DescriptorTranslated->u.Interrupt.Vector = HalGetInterruptVector(
                  DeviceNode->ResourceRequirements->InterfaceType,
                  DeviceNode->ResourceRequirements->BusNumber,
                  DescriptorRaw->u.Interrupt.Level,
                  DescriptorRaw->u.Interrupt.Vector,
                  (PKIRQL)&DescriptorTranslated->u.Interrupt.Level,
                  &DescriptorRaw->u.Interrupt.Affinity);
               DescriptorTranslated->u.Interrupt.Affinity = DescriptorRaw->u.Interrupt.Affinity;
               break;
            }
            case CmResourceTypeMemory:
            {
               ULONG AddressSpace = 1; /* Memory space */
               DescriptorRaw->u.Memory.Start = ResourceDescriptor->u.Memory.MinimumAddress;
               DescriptorRaw->u.Memory.Length = DescriptorTranslated->u.Memory.Length
                  = ResourceDescriptor->u.Memory.Length;
               if (!HalTranslateBusAddress(
                  DeviceNode->ResourceRequirements->InterfaceType,
                  DeviceNode->ResourceRequirements->BusNumber,
                  DescriptorRaw->u.Memory.Start,
                  &AddressSpace,
                  &DescriptorTranslated->u.Memory.Start))
               {
                  Status = STATUS_UNSUCCESSFUL;
                  goto ByeBye;
               }
               break;
            }
            case CmResourceTypeDma:
            {
               DescriptorRaw->u.Dma.Channel = DescriptorTranslated->u.Dma.Channel
                  = ResourceDescriptor->u.Dma.MinimumChannel;
               DescriptorRaw->u.Dma.Port = DescriptorTranslated->u.Dma.Port
                  = 0; /* FIXME */
               DescriptorRaw->u.Dma.Reserved1 = DescriptorTranslated->u.Dma.Reserved1
                  = 0;
               break;
            }
            /*case CmResourceTypeBusNumber:
            {
               DescriptorRaw->u.BusNumber.Start = DescriptorTranslated->u.BusNumber.Start
                  = ResourceDescriptor->u.BusNumber.MinBusNumber;
               DescriptorRaw->u.BusNumber.Length = DescriptorTranslated->u.BusNumber.Length
                  = ResourceDescriptor->u.BusNumber.Length;
               DescriptorRaw->u.BusNumber.Reserved = DescriptorTranslated->u.BusNumber.Reserved
                  = ResourceDescriptor->u.BusNumber.Reserved;
               break;
            }*/
            /*CmResourceTypeDevicePrivate:
            case CmResourceTypePcCardConfig:
            case CmResourceTypeMfCardConfig:
            {
               RtlCopyMemory(
                  &DescriptorRaw->u.DevicePrivate,
                  &ResourceDescriptor->u.DevicePrivate,
                  sizeof(ResourceDescriptor->u.DevicePrivate));
               RtlCopyMemory(
                  &DescriptorTranslated->u.DevicePrivate,
                  &ResourceDescriptor->u.DevicePrivate,
                  sizeof(ResourceDescriptor->u.DevicePrivate));
               break;
            }*/
            default:
               DPRINT1("IopAssignDeviceResources(): unknown resource descriptor type 0x%x\n", ResourceDescriptor->Type);
               NumberOfResources--;
         }
      }
      
   }
   
   DeviceNode->ResourceList->List[0].PartialResourceList.Count = NumberOfResources;
   DeviceNode->ResourceListTranslated->List[0].PartialResourceList.Count = NumberOfResources;
   
   return STATUS_SUCCESS;

ByeBye:
   if (DeviceNode->ResourceList)
   {
      ExFreePool(DeviceNode->ResourceList);
      DeviceNode->ResourceList = NULL;
   }
   if (DeviceNode->ResourceListTranslated)
   {
      ExFreePool(DeviceNode->ResourceListTranslated);
      DeviceNode->ResourceListTranslated = NULL;
   }

   return Status;
}


/*
 * IopActionInterrogateDeviceStack
 *
 * Retrieve information for all (direct) child nodes of a parent node.
 *
 * Parameters
 *    DeviceNode
 *       Pointer to device node.
 *    Context
 *       Pointer to parent node to retrieve child node information for.
 *
 * Remarks
 *    We only return a status code indicating an error (STATUS_UNSUCCESSFUL)
 *    when we reach a device node which is not a direct child of the device
 *    node for which we retrieve information of child nodes for. Any errors
 *    that occur is logged instead so that all child services have a chance
 *    of being interrogated.
 */

NTSTATUS
IopActionInterrogateDeviceStack(
   PDEVICE_NODE DeviceNode,
   PVOID Context)
{
   IO_STATUS_BLOCK IoStatusBlock;
   PDEVICE_NODE ParentDeviceNode;
   WCHAR InstancePath[MAX_PATH];
   IO_STACK_LOCATION Stack;
   NTSTATUS Status;
   PWSTR KeyBuffer;
   PWSTR Ptr;
   USHORT Length;
   USHORT TotalLength;
   HANDLE InstanceKey = NULL;
   UNICODE_STRING ValueName;
   DEVICE_CAPABILITIES DeviceCapabilities;

   DPRINT("IopActionInterrogateDeviceStack(%p, %p)\n", DeviceNode, Context);
   DPRINT("PDO %x\n", DeviceNode->PhysicalDeviceObject);

   ParentDeviceNode = (PDEVICE_NODE)Context;

   /*
    * We are called for the parent too, but we don't need to do special
    * handling for this node
    */

   if (DeviceNode == ParentDeviceNode)
   {
      DPRINT("Success\n");
      return STATUS_SUCCESS;
   }

   /*
    * Make sure this device node is a direct child of the parent device node
    * that is given as an argument
    */

   if (DeviceNode->Parent != ParentDeviceNode)
   {
      /* Stop the traversal immediately and indicate successful operation */
      DPRINT("Stop\n");
      return STATUS_UNSUCCESSFUL;
   }

   /*
    * FIXME: For critical errors, cleanup and disable device, but always
    * return STATUS_SUCCESS.
    */

   DPRINT("Sending IRP_MN_QUERY_ID.BusQueryDeviceID to device stack\n");

   Stack.Parameters.QueryId.IdType = BusQueryDeviceID;
   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_ID,
      &Stack);
   if (NT_SUCCESS(Status))
   {
      /* Copy the device id string */
      wcscpy(InstancePath, (PWSTR)IoStatusBlock.Information);

      /*
       * FIXME: Check for valid characters, if there is invalid characters
       * then bugcheck.
       */
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
   }

   DPRINT("Sending IRP_MN_QUERY_ID.BusQueryInstanceID to device stack\n");

   Stack.Parameters.QueryId.IdType = BusQueryInstanceID;
   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_ID,
      &Stack);
   if (NT_SUCCESS(Status))
   {
      /* Append the instance id string */
      wcscat(InstancePath, L"\\");
      wcscat(InstancePath, (PWSTR)IoStatusBlock.Information);

      /*
       * FIXME: Check for valid characters, if there is invalid characters
       * then bugcheck
       */
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
   }

   RtlZeroMemory(&DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));
   DeviceCapabilities.Size = sizeof(DEVICE_CAPABILITIES);
   DeviceCapabilities.Version = 1;
   DeviceCapabilities.Address = -1;
   DeviceCapabilities.UINumber = -1;

   Stack.Parameters.DeviceCapabilities.Capabilities = &DeviceCapabilities;
   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_CAPABILITIES,
      &Stack);
   if (NT_SUCCESS(Status))
   {
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
   }

   DeviceNode->CapabilityFlags = *(PULONG)((ULONG_PTR)&DeviceCapabilities + 4);
   DeviceNode->Address = DeviceCapabilities.Address;

   if (!DeviceCapabilities.UniqueID)
   {
      DPRINT("Instance ID is not unique\n");
      /* FIXME: Add information from parent bus driver to InstancePath */
   }

   if (!IopCreateUnicodeString(&DeviceNode->InstancePath, InstancePath, PagedPool))
   {
      DPRINT("No resources\n");
      /* FIXME: Cleanup and disable device */
   }

   DPRINT("InstancePath is %S\n", DeviceNode->InstancePath.Buffer);

   /*
    * Create registry key for the instance id, if it doesn't exist yet
    */
   KeyBuffer = ExAllocatePool(
      PagedPool,
      (49 * sizeof(WCHAR)) + DeviceNode->InstancePath.Length);
   wcscpy(KeyBuffer, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
   wcscat(KeyBuffer, DeviceNode->InstancePath.Buffer);
   Status = IopCreateDeviceKeyPath(KeyBuffer,
				   &InstanceKey);
   ExFreePool(KeyBuffer);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Failed to create the instance key! (Status %lx)\n", Status);
   }


   {
      /* Set 'Capabilities' value */
      RtlInitUnicodeString(&ValueName,
			   L"Capabilities");
      Status = ZwSetValueKey(InstanceKey,
			     &ValueName,
			     0,
			     REG_DWORD,
			     (PVOID)&DeviceNode->CapabilityFlags,
			     sizeof(ULONG));

      /* Set 'UINumber' value */
      if (DeviceCapabilities.UINumber != (ULONG)-1)
      {
         RtlInitUnicodeString(&ValueName,
			      L"UINumber");
         Status = ZwSetValueKey(InstanceKey,
				&ValueName,
				0,
				REG_DWORD,
				&DeviceCapabilities.UINumber,
				sizeof(ULONG));
      }
   }

   DPRINT("Sending IRP_MN_QUERY_ID.BusQueryHardwareIDs to device stack\n");

   Stack.Parameters.QueryId.IdType = BusQueryHardwareIDs;
   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_ID,
      &Stack);
   if (NT_SUCCESS(Status))
   {
      /*
       * FIXME: Check for valid characters, if there is invalid characters
       * then bugcheck.
       */
      TotalLength = 0;
      Ptr = (PWSTR)IoStatusBlock.Information;
      DPRINT("Hardware IDs:\n");
      while (*Ptr)
      {
	DPRINT("  %S\n", Ptr);
	Length = wcslen(Ptr) + 1;

	Ptr += Length;
	TotalLength += Length;
      }
      DPRINT("TotalLength: %hu\n", TotalLength);
      DPRINT("\n");

      RtlInitUnicodeString(&ValueName,
			   L"HardwareID");
      Status = ZwSetValueKey(InstanceKey,
			     &ValueName,
			     0,
			     REG_MULTI_SZ,
			     (PVOID)IoStatusBlock.Information,
			     (TotalLength + 1) * sizeof(WCHAR));
      if (!NT_SUCCESS(Status))
	{
	   DPRINT1("ZwSetValueKey() failed (Status %lx)\n", Status);
	}
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
   }

   DPRINT("Sending IRP_MN_QUERY_ID.BusQueryCompatibleIDs to device stack\n");

   Stack.Parameters.QueryId.IdType = BusQueryCompatibleIDs;
   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_ID,
      &Stack);
   if (NT_SUCCESS(Status))
   {
      /*
       * FIXME: Check for valid characters, if there is invalid characters
       * then bugcheck.
       */
      TotalLength = 0;
      Ptr = (PWSTR)IoStatusBlock.Information;
      DPRINT("Compatible IDs:\n");
      while (*Ptr)
      {
	DPRINT("  %S\n", Ptr);
	Length = wcslen(Ptr) + 1;

	Ptr += Length;
	TotalLength += Length;
      }
      DPRINT("TotalLength: %hu\n", TotalLength);
      DPRINT("\n");

      RtlInitUnicodeString(&ValueName,
			   L"CompatibleIDs");
      Status = ZwSetValueKey(InstanceKey,
			     &ValueName,
			     0,
			     REG_MULTI_SZ,
			     (PVOID)IoStatusBlock.Information,
			     (TotalLength + 1) * sizeof(WCHAR));
      if (!NT_SUCCESS(Status))
	{
	   DPRINT1("ZwSetValueKey() failed (Status %lx)\n", Status);
	}
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
   }


   DPRINT("Sending IRP_MN_QUERY_DEVICE_TEXT.DeviceTextDescription to device stack\n");

   Stack.Parameters.QueryDeviceText.DeviceTextType = DeviceTextDescription;
   Stack.Parameters.QueryDeviceText.LocaleId = 0; /* FIXME */
   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_DEVICE_TEXT,
      &Stack);
   if (NT_SUCCESS(Status))
   {
      RtlInitUnicodeString(&ValueName,
			   L"DeviceDesc");
      Status = ZwSetValueKey(InstanceKey,
			     &ValueName,
			     0,
			     REG_SZ,
			     (PVOID)IoStatusBlock.Information,
			     (wcslen((PWSTR)IoStatusBlock.Information) + 1) * sizeof(WCHAR));
      if (!NT_SUCCESS(Status))
	{
	   DPRINT1("ZwSetValueKey() failed (Status %lx)\n", Status);
	}
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
   }

   DPRINT("Sending IRP_MN_QUERY_DEVICE_TEXT.DeviceTextLocation to device stack\n");

   Stack.Parameters.QueryDeviceText.DeviceTextType = DeviceTextLocationInformation;
   Stack.Parameters.QueryDeviceText.LocaleId = 0; // FIXME
   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_DEVICE_TEXT,
      &Stack);
   if (NT_SUCCESS(Status))
   {
      DPRINT("LocationInformation: %S\n", (PWSTR)IoStatusBlock.Information);
      RtlInitUnicodeString(&ValueName,
			   L"LocationInformation");
      Status = ZwSetValueKey(InstanceKey,
			     &ValueName,
			     0,
			     REG_SZ,
			     (PVOID)IoStatusBlock.Information,
			     (wcslen((PWSTR)IoStatusBlock.Information) + 1) * sizeof(WCHAR));
      if (!NT_SUCCESS(Status))
	{
	   DPRINT1("ZwSetValueKey() failed (Status %lx)\n", Status);
	}
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
   }

   DPRINT("Sending IRP_MN_QUERY_BUS_INFORMATION to device stack\n");

   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_BUS_INFORMATION,
      NULL);
   if (NT_SUCCESS(Status))
   {
      PPNP_BUS_INFORMATION BusInformation =
         (PPNP_BUS_INFORMATION)IoStatusBlock.Information;

      DeviceNode->ChildBusNumber = BusInformation->BusNumber;
      DeviceNode->ChildInterfaceType = BusInformation->LegacyBusType;
      memcpy(&DeviceNode->BusTypeGuid,
             &BusInformation->BusTypeGuid,
             sizeof(GUID));
      ExFreePool(BusInformation);
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);

      DeviceNode->ChildBusNumber = -1;
      DeviceNode->ChildInterfaceType = -1;
      memset(&DeviceNode->BusTypeGuid,
             0,
             sizeof(GUID));
   }

   DPRINT("Sending IRP_MN_QUERY_RESOURCES to device stack\n");

   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_RESOURCES,
      NULL);
   if (NT_SUCCESS(Status))
   {
      DeviceNode->BootResources =
         (PCM_RESOURCE_LIST)IoStatusBlock.Information;
      DeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
      DeviceNode->BootResources = NULL;
   }

   DPRINT("Sending IRP_MN_QUERY_RESOURCE_REQUIREMENTS to device stack\n");

   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_RESOURCE_REQUIREMENTS,
      NULL);
   if (NT_SUCCESS(Status))
   {
      DeviceNode->ResourceRequirements =
         (PIO_RESOURCE_REQUIREMENTS_LIST)IoStatusBlock.Information;
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
      DeviceNode->ResourceRequirements = NULL;
   }


   if (InstanceKey != NULL)
   {
      IopSetDeviceInstanceData(InstanceKey, DeviceNode);
   }

   ZwClose(InstanceKey);

   Status = IopAssignDeviceResources(DeviceNode);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopAssignDeviceResources() failed (Status %x)\n", Status);
   }

   DeviceNode->Flags |= DNF_PROCESSED;

   /* Report the device to the user-mode pnp manager */
   IopQueueTargetDeviceEvent(&GUID_DEVICE_ARRIVAL,
                             &DeviceNode->InstancePath);

   return STATUS_SUCCESS;
}

/*
 * IopActionConfigureChildServices
 *
 * Retrieve configuration for all (direct) child nodes of a parent node.
 *
 * Parameters
 *    DeviceNode
 *       Pointer to device node.
 *    Context
 *       Pointer to parent node to retrieve child node configuration for.
 *
 * Remarks
 *    We only return a status code indicating an error (STATUS_UNSUCCESSFUL)
 *    when we reach a device node which is not a direct child of the device
 *    node for which we configure child services for. Any errors that occur is
 *    logged instead so that all child services have a chance of beeing
 *    configured.
 */

NTSTATUS
IopActionConfigureChildServices(
  PDEVICE_NODE DeviceNode,
  PVOID Context)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[2];
   PDEVICE_NODE ParentDeviceNode;
   PUNICODE_STRING Service;
   NTSTATUS Status;

   DPRINT("IopActionConfigureChildServices(%p, %p)\n", DeviceNode, Context);

   ParentDeviceNode = (PDEVICE_NODE)Context;

   /*
    * We are called for the parent too, but we don't need to do special
    * handling for this node
    */
   if (DeviceNode == ParentDeviceNode)
   {
      DPRINT("Success\n");
      return STATUS_SUCCESS;
   }

   /*
    * Make sure this device node is a direct child of the parent device node
    * that is given as an argument
    */
   if (DeviceNode->Parent != ParentDeviceNode)
   {
      /* Stop the traversal immediately and indicate successful operation */
      DPRINT("Stop\n");
      return STATUS_UNSUCCESSFUL;
   }

   if (!IopDeviceNodeHasFlag(DeviceNode, DNF_DISABLED))
   {
      WCHAR RegKeyBuffer[MAX_PATH];
      UNICODE_STRING RegKey;

      RegKey.Length = 0;
      RegKey.MaximumLength = sizeof(RegKeyBuffer);
      RegKey.Buffer = RegKeyBuffer;

      /*
       * Retrieve configuration from Enum key
       */

      Service = &DeviceNode->ServiceName;

      RtlZeroMemory(QueryTable, sizeof(QueryTable));
      RtlInitUnicodeString(Service, NULL);

      QueryTable[0].Name = L"Service";
      QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
      QueryTable[0].EntryContext = Service;

      RtlAppendUnicodeToString(&RegKey, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
      RtlAppendUnicodeStringToString(&RegKey, &DeviceNode->InstancePath);

      Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
         RegKey.Buffer, QueryTable, NULL, NULL);

      if (!NT_SUCCESS(Status))
      {
         DPRINT("RtlQueryRegistryValues() failed (Status %x)\n", Status);
         /* FIXME: Log the error */
         CPRINT("Could not retrieve configuration for device %S (Status %x)\n",
            DeviceNode->InstancePath.Buffer, Status);
         IopDeviceNodeSetFlag(DeviceNode, DNF_DISABLED);
         return STATUS_SUCCESS;
      }

      if (Service->Buffer == NULL)
      {
         IopDeviceNodeSetFlag(DeviceNode, DNF_DISABLED);
         return STATUS_SUCCESS;
      }

      DPRINT("Got Service %S\n", Service->Buffer);
   }

   return STATUS_SUCCESS;
}

/*
 * IopActionInitChildServices
 *
 * Initialize the service for all (direct) child nodes of a parent node
 *
 * Parameters
 *    DeviceNode
 *       Pointer to device node.
 *    Context
 *       Pointer to parent node to initialize child node services for.
 *    BootDrivers
 *       Load only driver marked as boot start.
 *
 * Remarks
 *    If the driver image for a service is not loaded and initialized
 *    it is done here too. We only return a status code indicating an
 *    error (STATUS_UNSUCCESSFUL) when we reach a device node which is
 *    not a direct child of the device node for which we initialize
 *    child services for. Any errors that occur is logged instead so
 *    that all child services have a chance of being initialized.
 */

NTSTATUS
IopActionInitChildServices(
   PDEVICE_NODE DeviceNode,
   PVOID Context,
   BOOLEAN BootDrivers)
{
   PDEVICE_NODE ParentDeviceNode;
   NTSTATUS Status;

   DPRINT("IopActionInitChildServices(%p, %p, %d)\n", DeviceNode, Context,
      BootDrivers);

   ParentDeviceNode = (PDEVICE_NODE)Context;

   /*
    * We are called for the parent too, but we don't need to do special
    * handling for this node
    */
   if (DeviceNode == ParentDeviceNode)
   {
      DPRINT("Success\n");
      return STATUS_SUCCESS;
   }

   /*
    * Make sure this device node is a direct child of the parent device node
    * that is given as an argument
    */
#if 0
   if (DeviceNode->Parent != ParentDeviceNode)
   {
      /*
       * Stop the traversal immediately and indicate unsuccessful operation
       */
      DPRINT("Stop\n");
      return STATUS_UNSUCCESSFUL;
   }
#endif

   if (!IopDeviceNodeHasFlag(DeviceNode, DNF_DISABLED) &&
       !IopDeviceNodeHasFlag(DeviceNode, DNF_ADDED) &&
       !IopDeviceNodeHasFlag(DeviceNode, DNF_STARTED))
   {
      PMODULE_OBJECT ModuleObject;
      PDRIVER_OBJECT DriverObject;

      Status = IopLoadServiceModule(&DeviceNode->ServiceName, &ModuleObject);
      if (NT_SUCCESS(Status) || Status == STATUS_IMAGE_ALREADY_LOADED)
      {
         if (Status != STATUS_IMAGE_ALREADY_LOADED)
            Status = IopInitializeDriverModule(DeviceNode, ModuleObject,
               &DeviceNode->ServiceName, FALSE, &DriverObject);
         else
         {
            /* get existing DriverObject pointer */
            Status = IopGetDriverObject(
               &DriverObject,
               &DeviceNode->ServiceName,
               FALSE);
         }
         if (NT_SUCCESS(Status))
         {
            /* Attach lower level filter drivers. */
            IopAttachFilterDrivers(DeviceNode, TRUE);
            /* Initialize the function driver for the device node */
            Status = IopInitializeDevice(DeviceNode, DriverObject);
            if (NT_SUCCESS(Status))
            {
               /* Attach upper level filter drivers. */
               IopAttachFilterDrivers(DeviceNode, FALSE);
               IopDeviceNodeSetFlag(DeviceNode, DNF_STARTED);

               Status = IopStartDevice(DeviceNode);
            }
         }
      }
      else
      {
         /*
          * Don't disable when trying to load only boot drivers
          */
         if (!BootDrivers)
         {
            IopDeviceNodeSetFlag(DeviceNode, DNF_DISABLED);
            IopDeviceNodeSetFlag(DeviceNode, DNF_START_FAILED);
         }
         /* FIXME: Log the error (possibly in IopInitializeDeviceNodeService) */
         CPRINT("Initialization of service %S failed (Status %x)\n",
           DeviceNode->ServiceName.Buffer, Status);
      }
   } else
   {
      DPRINT("Service %S is disabled or already initialized\n",
         DeviceNode->ServiceName.Buffer);
   }

   return STATUS_SUCCESS;
}

/*
 * IopActionInitAllServices
 *
 * Initialize the service for all (direct) child nodes of a parent node. This
 * function just calls IopActionInitChildServices with BootDrivers = FALSE.
 */

NTSTATUS
IopActionInitAllServices(
  PDEVICE_NODE DeviceNode,
  PVOID Context)
{
   return IopActionInitChildServices(DeviceNode, Context, FALSE);
}

/*
 * IopActionInitBootServices
 *
 * Initialize the boot start services for all (direct) child nodes of a
 * parent node. This function just calls IopActionInitChildServices with
 * BootDrivers = TRUE.
 */
NTSTATUS
IopActionInitBootServices(
   PDEVICE_NODE DeviceNode,
   PVOID Context)
{
   return IopActionInitChildServices(DeviceNode, Context, TRUE);
}

/*
 * IopInitializePnpServices
 *
 * Initialize services for discovered children
 *
 * Parameters
 *    DeviceNode
 *       Top device node to start initializing services.
 *
 *    BootDrivers
 *       When set to TRUE, only drivers marked as boot start will
 *       be loaded. Otherwise, all drivers will be loaded.
 *
 * Return Value
 *    Status
 */
NTSTATUS
IopInitializePnpServices(
   IN PDEVICE_NODE DeviceNode,
   IN BOOLEAN BootDrivers)
{
   DEVICETREE_TRAVERSE_CONTEXT Context;

   DPRINT("IopInitializePnpServices(%p, %d)\n", DeviceNode, BootDrivers);

   if (BootDrivers)
   {
      IopInitDeviceTreeTraverseContext(
         &Context,
         DeviceNode,
         IopActionInitBootServices,
         DeviceNode);
   } else
   {
      IopInitDeviceTreeTraverseContext(
         &Context,
         DeviceNode,
         IopActionInitAllServices,
         DeviceNode);
   }

   return IopTraverseDeviceTree(&Context);
}


NTSTATUS
IopInvalidateDeviceRelations(
   IN PDEVICE_NODE DeviceNode,
   IN DEVICE_RELATION_TYPE Type)
{
   DEVICETREE_TRAVERSE_CONTEXT Context;
   PDEVICE_RELATIONS DeviceRelations;
   IO_STATUS_BLOCK IoStatusBlock;
   PDEVICE_NODE ChildDeviceNode;
   IO_STACK_LOCATION Stack;
   BOOL BootDrivers;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING LinkName;
   HANDLE Handle;
   NTSTATUS Status;
   ULONG i;

   DPRINT("DeviceNode %x\n", DeviceNode);

   DPRINT("Sending IRP_MN_QUERY_DEVICE_RELATIONS to device stack\n");

   Stack.Parameters.QueryDeviceRelations.Type = Type/*BusRelations*/;

   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_DEVICE_RELATIONS,
      &Stack);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopInitiatePnpIrp() failed\n");
      return Status;
   }

   DeviceRelations = (PDEVICE_RELATIONS)IoStatusBlock.Information;

   if ((!DeviceRelations) || (DeviceRelations->Count <= 0))
   {
      DPRINT("No PDOs\n");
      if (DeviceRelations)
      {
         ExFreePool(DeviceRelations);
      }
      return STATUS_SUCCESS;
   }

   DPRINT("Got %d PDOs\n", DeviceRelations->Count);

   /*
    * Create device nodes for all discovered devices
    */

   for (i = 0; i < DeviceRelations->Count; i++)
   {
      Status = IopCreateDeviceNode(
         DeviceNode,
         DeviceRelations->Objects[i],
         &ChildDeviceNode);
      DeviceNode->Flags |= DNF_ENUMERATED;
      if (!NT_SUCCESS(Status))
      {
         DPRINT("No resources\n");
         for (i = 0; i < DeviceRelations->Count; i++)
            ObDereferenceObject(DeviceRelations->Objects[i]);
         ExFreePool(DeviceRelations);
         return STATUS_INSUFFICIENT_RESOURCES;
      }
   }
   ExFreePool(DeviceRelations);

   /*
    * Retrieve information about all discovered children from the bus driver
    */

   IopInitDeviceTreeTraverseContext(
      &Context,
      DeviceNode,
      IopActionInterrogateDeviceStack,
      DeviceNode);

   Status = IopTraverseDeviceTree(&Context);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopTraverseDeviceTree() failed with status (%x)\n", Status);
      return Status;
   }

   /*
    * Retrieve configuration from the registry for discovered children
    */

   IopInitDeviceTreeTraverseContext(
      &Context,
      DeviceNode,
      IopActionConfigureChildServices,
      DeviceNode);

   Status = IopTraverseDeviceTree(&Context);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopTraverseDeviceTree() failed with status (%x)\n", Status);
      return Status;
   }

   /*
    * Get the state of the system boot. If the \\SystemRoot link isn't
    * created yet, we will assume that it's possible to load only boot
    * drivers.
    */

   RtlInitUnicodeString(&LinkName, L"\\SystemRoot");

   InitializeObjectAttributes(
      &ObjectAttributes,
      &LinkName,
      0,
      NULL,
      NULL);

   Status = ZwOpenFile(
      &Handle,
      FILE_ALL_ACCESS,
      &ObjectAttributes,
      &IoStatusBlock,
      0,
      0);
   if(NT_SUCCESS(Status))
   {
     BootDrivers = FALSE;
     ZwClose(Handle);
   }
   else
     BootDrivers = TRUE;

   /*
    * Initialize services for discovered children. Only boot drivers will
    * be loaded from boot driver!
    */

   Status = IopInitializePnpServices(DeviceNode, BootDrivers);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopInitializePnpServices() failed with status (%x)\n", Status);
      return Status;
   }

   return STATUS_SUCCESS;
}


VOID INIT_FUNCTION
PnpInit(VOID)
{
   PDEVICE_OBJECT Pdo;
   NTSTATUS Status;

   DPRINT("PnpInit()\n");

   KeInitializeSpinLock(&IopDeviceTreeLock);

   /* Initialize PnP-Event notification support */
   Status = IopInitPlugPlayEvents();
   if (!NT_SUCCESS(Status))
   {
      CPRINT("IopInitPlugPlayEvents() failed\n");
      KEBUGCHECKEX(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
   }

   /*
    * Create root device node
    */

   Status = IopCreateDriverObject(&IopRootDriverObject, NULL, 0, FALSE, NULL, 0);
   if (!NT_SUCCESS(Status))
   {
      CPRINT("IoCreateDriverObject() failed\n");
      KEBUGCHECKEX(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
   }

   Status = IoCreateDevice(IopRootDriverObject, 0, NULL, FILE_DEVICE_CONTROLLER,
      0, FALSE, &Pdo);
   if (!NT_SUCCESS(Status))
   {
      CPRINT("IoCreateDevice() failed\n");
      KEBUGCHECKEX(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
   }

   Status = IopCreateDeviceNode(NULL, Pdo, &IopRootDeviceNode);
   if (!NT_SUCCESS(Status))
   {
      CPRINT("Insufficient resources\n");
      KEBUGCHECKEX(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
   }

   if (!IopCreateUnicodeString(&IopRootDeviceNode->InstancePath,
       L"HTREE\\Root\\0",
       PagedPool))
   {
     CPRINT("Failed to create the instance path!\n");
     KEBUGCHECKEX(PHASE1_INITIALIZATION_FAILED, STATUS_UNSUCCESSFUL, 0, 0, 0);
   }

   /* Report the device to the user-mode pnp manager */
   IopQueueTargetDeviceEvent(&GUID_DEVICE_ARRIVAL,
                             &IopRootDeviceNode->InstancePath);

   IopRootDeviceNode->PhysicalDeviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;
   PnpRootDriverEntry(IopRootDriverObject, NULL);
   IopRootDriverObject->DriverExtension->AddDevice(
      IopRootDriverObject,
      IopRootDeviceNode->PhysicalDeviceObject);
}

/* EOF */
