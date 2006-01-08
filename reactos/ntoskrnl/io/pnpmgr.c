/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/pnpmgr.c
 * PURPOSE:         Initializes the PnP manager
 *
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

PDEVICE_NODE IopRootDeviceNode;
KSPIN_LOCK IopDeviceTreeLock;

/* DATA **********************************************************************/

PDRIVER_OBJECT IopRootDriverObject;
PIO_BUS_TYPE_GUID_LIST IopBusTypeGuidList = NULL;

// Static CRC table
ULONG crc32Table[256] =
{
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
	0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
	0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
	0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
	0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
	0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,

	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
	0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
	0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
	0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
	0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
	0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
	0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,

	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
	0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
	0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
	0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
	0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
	0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,

	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
	0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
	0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
	0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
	0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
	0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

static NTSTATUS INIT_FUNCTION
IopSetRootDeviceInstanceData(PDEVICE_NODE DeviceNode);

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, IopSetRootDeviceInstanceData)
#pragma alloc_text(INIT, PnpInit)
#pragma alloc_text(INIT, PnpInit2)
#endif


/* FUNCTIONS *****************************************************************/

PDEVICE_NODE
FASTCALL
IopGetDeviceNode(PDEVICE_OBJECT DeviceObject)
{
   return ((PEXTENDED_DEVOBJ_EXTENSION)DeviceObject->DeviceObjectExtension)->DeviceNode;
}

NTSTATUS
STDCALL
IopQueryDeviceCapabilities(PDEVICE_NODE DeviceNode,
                           PDEVICE_CAPABILITIES DeviceCaps)
{
   IO_STATUS_BLOCK StatusBlock;
   IO_STACK_LOCATION Stack;

   /* Set up the Header */
   RtlZeroMemory(DeviceCaps, sizeof(DEVICE_CAPABILITIES));
   DeviceCaps->Size = sizeof(DEVICE_CAPABILITIES);
   DeviceCaps->Version = 1;
   DeviceCaps->Address = -1;
   DeviceCaps->UINumber = -1;

   /* Set up the Stack */
   RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
   Stack.Parameters.DeviceCapabilities.Capabilities = DeviceCaps;

   /* Send the IRP */
   return IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                            &StatusBlock,
                            IRP_MN_QUERY_CAPABILITIES,
                            &Stack);
}

/*
 * @implemented
 */
VOID
STDCALL
IoInvalidateDeviceRelations(IN PDEVICE_OBJECT DeviceObject,
                            IN DEVICE_RELATION_TYPE Type)
{
   IopInvalidateDeviceRelations(IopGetDeviceNode(DeviceObject), Type);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoGetDeviceProperty(IN PDEVICE_OBJECT DeviceObject,
                    IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
                    IN ULONG BufferLength,
                    OUT PVOID PropertyBuffer,
                    OUT PULONG ResultLength)
{
   PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);
   DEVICE_CAPABILITIES DeviceCaps;
   ULONG Length;
   PVOID Data = NULL;
   PWSTR Ptr;
   NTSTATUS Status;

   DPRINT("IoGetDeviceProperty(0x%p %d)\n", DeviceObject, DeviceProperty);

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
         /* Sanity check */
         if ((DeviceNode->ChildBusTypeIndex != 0xFFFF) && 
             (DeviceNode->ChildBusTypeIndex < IopBusTypeGuidList->GuidCount))
         {
            /* Return the GUID */
            *ResultLength = sizeof(GUID);

            /* Check if the buffer given was large enough */
            if (BufferLength < *ResultLength)
            {
                return STATUS_BUFFER_TOO_SMALL;
            }

            /* Copy the GUID */
            RtlCopyMemory(PropertyBuffer, 
                          &(IopBusTypeGuidList->Guids[DeviceNode->ChildBusTypeIndex]),
                          sizeof(GUID));
            return STATUS_SUCCESS;
         }
         else
         {
            return STATUS_OBJECT_NAME_NOT_FOUND;
         }
         break;

      case DevicePropertyLegacyBusType:
         Length = sizeof(INTERFACE_TYPE);
         Data = &DeviceNode->ChildInterfaceType;
         break;

      case DevicePropertyAddress:
         /* Query the device caps */
         Status = IopQueryDeviceCapabilities(DeviceNode, &DeviceCaps);
         if (NT_SUCCESS(Status) && (DeviceCaps.Address != (ULONG)-1))
         {
            /* Return length */
            *ResultLength = sizeof(ULONG);

            /* Check if the buffer given was large enough */
            if (BufferLength < *ResultLength)
            {
               return STATUS_BUFFER_TOO_SMALL;
            }

            /* Return address */
            *(PULONG)PropertyBuffer = DeviceCaps.Address;
            return STATUS_SUCCESS;
         }
         else
         {
            return STATUS_OBJECT_NAME_NOT_FOUND;
         }
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

         DPRINT("KeyNameBuffer: 0x%p, value %S\n", KeyNameBuffer, RegistryPropertyName);

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

         if (!NT_SUCCESS(Status))
         {
            ExFreePool(ValueInformation);
            if (Status == STATUS_BUFFER_OVERFLOW)
               return STATUS_BUFFER_TOO_SMALL;
            else
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
IoInvalidateDeviceState(IN PDEVICE_OBJECT PhysicalDeviceObject)
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
IoOpenDeviceRegistryKey(IN PDEVICE_OBJECT DeviceObject,
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
   static WCHAR DeviceParametersKeyName[] = L"Device Parameters";
   ULONG KeyNameLength;
   LPWSTR KeyNameBuffer;
   UNICODE_STRING KeyName;
   ULONG DriverKeyLength;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PDEVICE_NODE DeviceNode = NULL;
   NTSTATUS Status;

   DPRINT("IoOpenDeviceRegistryKey() called\n");

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
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("IoOpenDeviceRegistryKey(%wZ): Base key doesn't exist, exiting... (Status 0x%08lx)\n", &KeyName, Status);
      ExFreePool(KeyNameBuffer);
      return Status;
   }
   ExFreePool(KeyNameBuffer);

   /*
    * For driver key we're done now.
    */

   if (DevInstKeyType & PLUGPLAY_REGKEY_DRIVER)
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
IoRequestDeviceEject(IN PDEVICE_OBJECT PhysicalDeviceObject)
{
	UNIMPLEMENTED;
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

USHORT
STDCALL
IopGetBusTypeGuidIndex(LPGUID BusTypeGuid)
{
   USHORT i = 0, FoundIndex = 0xFFFF;
   ULONG NewSize;
   PVOID NewList;
    
   /* Acquire the lock */
   ExAcquireFastMutex(&IopBusTypeGuidList->Lock);

   /* Loop all entries */
   while (i < IopBusTypeGuidList->GuidCount)
   {
       /* Try to find a match */
       if (RtlCompareMemory(BusTypeGuid,
                            &IopBusTypeGuidList->Guids[i],
                            sizeof(GUID)))
       {
           /* Found it */
           FoundIndex = i;
           goto Quickie;
       }
       i++;
   }

   /* Check if we have to grow the list */
   if (IopBusTypeGuidList->GuidCount)
   {
       /* Calculate the new size */
       NewSize = sizeof(IO_BUS_TYPE_GUID_LIST) +
                (sizeof(GUID) * IopBusTypeGuidList->GuidCount);

       /* Allocate the new copy */
       NewList = ExAllocatePool(PagedPool, NewSize);

       /* Now copy them, decrease the size too */
       NewSize -= sizeof(GUID);
       RtlCopyMemory(NewList, IopBusTypeGuidList, NewSize);

       /* Free the old list */
       ExFreePool(IopBusTypeGuidList);

       /* Use the new buffer */
       IopBusTypeGuidList = NewList;
   }

   /* Copy the new GUID */
   RtlCopyMemory(&IopBusTypeGuidList->Guids[IopBusTypeGuidList->GuidCount],
                 BusTypeGuid,
                 sizeof(GUID));

   /* The new entry is the index */
   FoundIndex = IopBusTypeGuidList->GuidCount;
   IopBusTypeGuidList->GuidCount++;

Quickie:
   ExReleaseFastMutex(&IopBusTypeGuidList->Lock);
   return FoundIndex;
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

   DPRINT("ParentNode 0x%p PhysicalDeviceObject 0x%p\n",
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

   ((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode = Node;

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
      Node->Level = ParentNode->Level + 1;
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
IopInitiatePnpIrp(PDEVICE_OBJECT DeviceObject,
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
      RtlMoveMemory(&IrpSp->Parameters,
                    &Stack->Parameters,
                    sizeof(Stack->Parameters));
   }

   Status = IoCallDriver(TopDeviceObject, Irp);
   if (Status == STATUS_PENDING)
   {
      KeWaitForSingleObject(&Event,
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
IopTraverseDeviceTreeNode(PDEVICETREE_TRAVERSE_CONTEXT Context)
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
IopTraverseDeviceTree(PDEVICETREE_TRAVERSE_CONTEXT Context)
{
   NTSTATUS Status;

   DPRINT("Context 0x%p\n", Context);

   DPRINT("IopTraverseDeviceTree(DeviceNode 0x%p  FirstDeviceNode 0x%p  Action %x  Context 0x%p)\n",
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


static
NTSTATUS
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


static
NTSTATUS
IopSetDeviceInstanceData(HANDLE InstanceKey,
                         PDEVICE_NODE DeviceNode)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING KeyName;
   HANDLE LogConfKey;
   ULONG ResCount;
   ULONG ListSize, ResultLength;
   NTSTATUS Status;

   DPRINT("IopSetDeviceInstanceData() called\n");

   /* Create the 'LogConf' key */
   RtlInitUnicodeString(&KeyName, L"LogConf");
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

            RtlInitUnicodeString(&KeyName, L"BootConfig");
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
         RtlInitUnicodeString(&KeyName, L"BasicConfigVector");
         Status = ZwSetValueKey(LogConfKey,
                                &KeyName,
                                0,
                                REG_RESOURCE_REQUIREMENTS_LIST,
                                DeviceNode->ResourceRequirements,
                                DeviceNode->ResourceRequirements->ListSize);
      }

      ZwClose(LogConfKey);
   }

   /* Set the 'ConfigFlags' value */
   RtlInitUnicodeString(&KeyName, L"ConfigFlags");
   Status = ZwQueryValueKey(InstanceKey,
                            &KeyName,
                            KeyValueBasicInformation,
                            NULL,
                            0,
                            &ResultLength);
  if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
  {
    /* Write the default value */
    ULONG DefaultConfigFlags = 0;
    Status = ZwSetValueKey(InstanceKey,
                           &KeyName,
                           0,
                           REG_DWORD,
                           &DefaultConfigFlags,
                           sizeof(DefaultConfigFlags));
  }

#if 0
  if (DeviceNode->PhysicalDeviceObject != NULL)
  {
    /* Create the 'Control' key */
    RtlInitUnicodeString(&KeyName,
		         L"Control");
    InitializeObjectAttributes(&ObjectAttributes,
			       &KeyName,
			       OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			       InstanceKey,
			       NULL);
    Status = ZwCreateKey(&LogConfKey,
		         KEY_ALL_ACCESS,
		         &ObjectAttributes,
		         0,
		         NULL,
		         REG_OPTION_VOLATILE,
		         NULL);
    if (NT_SUCCESS(Status))
    {
      ULONG Reference = (ULONG)DeviceNode->PhysicalDeviceObject;
      RtlInitUnicodeString(&KeyName,
			   L"DeviceReference");
      Status = ZwSetValueKey(LogConfKey,
			     &KeyName,
			     0,
			     REG_DWORD,
			     &Reference,
			     sizeof(PVOID));

      ZwClose(LogConfKey);
    }
  }
#endif

  DPRINT("IopSetDeviceInstanceData() done\n");

  return STATUS_SUCCESS;
}


NTSTATUS
IopAssignDeviceResources(PDEVICE_NODE DeviceNode)
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

   IopDeviceNodeSetFlag(DeviceNode, DNF_ASSIGNING_RESOURCES);

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
               INTERFACE_TYPE BusType;
               ULONG SlotNumber;
               ULONG ret;
               UCHAR Irq;

               DescriptorRaw->u.Interrupt.Level = 0;
               DescriptorRaw->u.Interrupt.Vector = ResourceDescriptor->u.Interrupt.MinimumVector;
               /* FIXME: HACK: if we have a PCI device, we try
                * to keep the IRQ assigned by the BIOS */
               if (NT_SUCCESS(IoGetDeviceProperty(
                  DeviceNode->PhysicalDeviceObject,
                  DevicePropertyLegacyBusType,
                  sizeof(INTERFACE_TYPE),
                  &BusType,
                  &ret)) && BusType == PCIBus)
               {
                  /* We have a PCI bus */
                  if (NT_SUCCESS(IoGetDeviceProperty(
                     DeviceNode->PhysicalDeviceObject,
                     DevicePropertyAddress,
                     sizeof(ULONG),
                     &SlotNumber,
                     &ret)) && SlotNumber > 0)
                  {
                     /* We have a good slot number */
                     ret = HalGetBusDataByOffset(PCIConfiguration,
                                                 DeviceNode->ResourceRequirements->BusNumber,
                                                 SlotNumber,
                                                 &Irq,
                                                 0x3c /* PCI_INTERRUPT_LINE */,
                                                 sizeof(UCHAR));
                     if (ret != 0 && ret != 2
                         && ResourceDescriptor->u.Interrupt.MinimumVector <= Irq
                         && ResourceDescriptor->u.Interrupt.MaximumVector >= Irq)
                     {
                        /* The device already has an assigned IRQ */
                        DescriptorRaw->u.Interrupt.Vector = Irq;
                     }
                     else
                     {
                         DPRINT1("Trying to assign IRQ 0x%lx to %wZ\n",
                            DescriptorRaw->u.Interrupt.Vector,
                            &DeviceNode->InstancePath);
                         Irq = (UCHAR)DescriptorRaw->u.Interrupt.Vector;
                         ret = HalSetBusDataByOffset(PCIConfiguration,
                            DeviceNode->ResourceRequirements->BusNumber,
                            SlotNumber,
                            &Irq,
                            0x3c /* PCI_INTERRUPT_LINE */,
                            sizeof(UCHAR));
                         if (ret == 0 || ret == 2)
                            KEBUGCHECK(0);
                     }
                  }
               }

               DescriptorTranslated->u.Interrupt.Level = 0;
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
            case CmResourceTypeBusNumber:
            {
               DescriptorRaw->u.BusNumber.Start = DescriptorTranslated->u.BusNumber.Start
                  = ResourceDescriptor->u.BusNumber.MinBusNumber;
               DescriptorRaw->u.BusNumber.Length = DescriptorTranslated->u.BusNumber.Length
                  = ResourceDescriptor->u.BusNumber.Length;
               DescriptorRaw->u.BusNumber.Reserved = DescriptorTranslated->u.BusNumber.Reserved
                  = ResourceDescriptor->u.BusNumber.Reserved;
               break;
            }
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

   IopDeviceNodeClearFlag(DeviceNode, DNF_ASSIGNING_RESOURCES);
   IopDeviceNodeSetFlag(DeviceNode, DNF_RESOURCE_ASSIGNED);
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

   IopDeviceNodeClearFlag(DeviceNode, DNF_ASSIGNING_RESOURCES);
   return Status;
}


/*
 * IopGetParentIdPrefix
 *
 * Retrieve (or create) a string which identifies a device.
 *
 * Parameters
 *    DeviceNode
 *        Pointer to device node.
 *    ParentIdPrefix
 *        Pointer to the string where is returned the parent node identifier
 *
 * Remarks
 *     If the return code is STATUS_SUCCESS, the ParentIdPrefix string is
 *     valid and its Buffer field is NULL-terminated. The caller needs to
 *     to free the string with RtlFreeUnicodeString when it is no longer
 *     needed.
 */

NTSTATUS
IopGetParentIdPrefix(PDEVICE_NODE DeviceNode,
                     PUNICODE_STRING ParentIdPrefix)
{
   ULONG KeyNameBufferLength;
   PWSTR KeyNameBuffer = NULL;
   PKEY_VALUE_PARTIAL_INFORMATION ParentIdPrefixInformation = NULL;
   UNICODE_STRING KeyName;
   UNICODE_STRING KeyValue;
   UNICODE_STRING ValueName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE hKey = INVALID_HANDLE_VALUE;
   PBYTE currentByte;
   ULONG crc32 = 0;
   ULONG i;
   NTSTATUS Status;

   /* HACK: As long as some devices have a NULL device
    * instance path, the following test is required :(
    */
   if (DeviceNode->Parent->InstancePath.Length == 0)
      return STATUS_UNSUCCESSFUL;

   /* 1. Try to retrieve ParentIdPrefix from registry */
   KeyNameBufferLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]) + MAX_PATH * sizeof(WCHAR);
   ParentIdPrefixInformation = ExAllocatePool(PagedPool, KeyNameBufferLength + sizeof(WCHAR));
   if (!ParentIdPrefixInformation)
   {
       Status = STATUS_INSUFFICIENT_RESOURCES;
       goto cleanup;
   }
   KeyNameBuffer = ExAllocatePool(PagedPool, (49 * sizeof(WCHAR)) + DeviceNode->Parent->InstancePath.Length);
   if (!KeyNameBuffer)
   {
       Status = STATUS_INSUFFICIENT_RESOURCES;
       goto cleanup;
   }
   wcscpy(KeyNameBuffer, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
   wcscat(KeyNameBuffer, DeviceNode->Parent->InstancePath.Buffer);
   RtlInitUnicodeString(&KeyName, KeyNameBuffer);
   InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
   Status = ZwOpenKey(&hKey, KEY_QUERY_VALUE | KEY_SET_VALUE, &ObjectAttributes);
   if (!NT_SUCCESS(Status))
      goto cleanup;
   RtlInitUnicodeString(&ValueName, L"ParentIdPrefix");
   Status = ZwQueryValueKey(
      hKey, &ValueName,
      KeyValuePartialInformation, ParentIdPrefixInformation,
      KeyNameBufferLength, &KeyNameBufferLength);
   if (NT_SUCCESS(Status))
   {
      if (ParentIdPrefixInformation->Type != REG_SZ)
         Status = STATUS_UNSUCCESSFUL;
      else
      {
         KeyValue.Length = KeyValue.MaximumLength = ParentIdPrefixInformation->DataLength;
         KeyValue.Buffer = (PWSTR)ParentIdPrefixInformation->Data;
      }
      goto cleanup;
   }
   if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
   {
      KeyValue.Length = KeyValue.MaximumLength = ParentIdPrefixInformation->DataLength;
      KeyValue.Buffer = (PWSTR)ParentIdPrefixInformation->Data;
      goto cleanup;
   }

   /* 2. Create the ParentIdPrefix value */
   currentByte = (PBYTE)DeviceNode->Parent->InstancePath.Buffer;
   for (i = 0; i < DeviceNode->Parent->InstancePath.Length; i++, currentByte++)
      crc32 = (crc32 >> 8) ^ crc32Table[*currentByte ^ (crc32 & 0xff)];
   crc32 = ~crc32;
   swprintf((PWSTR)ParentIdPrefixInformation->Data, L"%lx&%lx", DeviceNode->Parent->Level, crc32);
   RtlInitUnicodeString(&KeyValue, (PWSTR)ParentIdPrefixInformation->Data);

   /* 3. Try to write the ParentIdPrefix to registry */
   Status = ZwSetValueKey(hKey,
                          &ValueName,
                          0,
                          REG_SZ,
                          (PVOID)KeyValue.Buffer,
                          (wcslen(KeyValue.Buffer) + 1) * sizeof(WCHAR));

cleanup:
   if (NT_SUCCESS(Status))
   {
      /* Duplicate the string to return it */
      Status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, &KeyValue, ParentIdPrefix);
   }
   ExFreePool(ParentIdPrefixInformation);
   ExFreePool(KeyNameBuffer);
   if (hKey != INVALID_HANDLE_VALUE)
      ZwClose(hKey);
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
IopActionInterrogateDeviceStack(PDEVICE_NODE DeviceNode,
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
   ULONG RequiredLength;
   LCID LocaleId;
   HANDLE InstanceKey = NULL;
   UNICODE_STRING ValueName;
   UNICODE_STRING ParentIdPrefix = { 0 };
   DEVICE_CAPABILITIES DeviceCapabilities;

   DPRINT("IopActionInterrogateDeviceStack(%p, %p)\n", DeviceNode, Context);
   DPRINT("PDO 0x%p\n", DeviceNode->PhysicalDeviceObject);

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

   /* Get Locale ID */
   Status = ZwQueryDefaultLocale(FALSE, &LocaleId);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("ZwQueryDefaultLocale() failed with status 0x%lx\n", Status);
      return Status;
   }

   /*
    * FIXME: For critical errors, cleanup and disable device, but always
    * return STATUS_SUCCESS.
    */

   DPRINT("Sending IRP_MN_QUERY_ID.BusQueryDeviceID to device stack\n");

   Stack.Parameters.QueryId.IdType = BusQueryDeviceID;
   Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
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

   DPRINT("Sending IRP_MN_QUERY_CAPABILITIES to device stack\n");

   Status = IopQueryDeviceCapabilities(DeviceNode, &DeviceCapabilities);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopInitiatePnpIrp() failed (Status 0x%08lx)\n", Status);
   }

   DeviceNode->CapabilityFlags = *(PULONG)((ULONG_PTR)&DeviceCapabilities + 4);

   if (!DeviceCapabilities.UniqueID)
   {
      /* Device has not a unique ID. We need to prepend parent bus unique identifier */
      DPRINT("Instance ID is not unique\n");
      Status = IopGetParentIdPrefix(DeviceNode, &ParentIdPrefix);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("IopGetParentIdPrefix() failed (Status 0x%08lx)\n", Status);
      }
   }

   DPRINT("Sending IRP_MN_QUERY_ID.BusQueryInstanceID to device stack\n");

   Stack.Parameters.QueryId.IdType = BusQueryInstanceID;
   Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                              &IoStatusBlock,
                              IRP_MN_QUERY_ID,
                              &Stack);
   if (NT_SUCCESS(Status))
   {
      /* Append the instance id string */
      wcscat(InstancePath, L"\\");
      if (ParentIdPrefix.Length > 0)
      {
         /* Add information from parent bus device to InstancePath */
         wcscat(InstancePath, ParentIdPrefix.Buffer);
         if (IoStatusBlock.Information && *(PWSTR)IoStatusBlock.Information)
            wcscat(InstancePath, L"&");
      }
      if (IoStatusBlock.Information)
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
   RtlFreeUnicodeString(&ParentIdPrefix);

   if (!RtlCreateUnicodeString(&DeviceNode->InstancePath, InstancePath))
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
   Status = IopCreateDeviceKeyPath(KeyBuffer, &InstanceKey);
   ExFreePool(KeyBuffer);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Failed to create the instance key! (Status %lx)\n", Status);
   }


   {
      /* Set 'Capabilities' value */
      RtlInitUnicodeString(&ValueName, L"Capabilities");
      Status = ZwSetValueKey(InstanceKey,
                             &ValueName,
                             0,
                             REG_DWORD,
                             (PVOID)&DeviceNode->CapabilityFlags,
                             sizeof(ULONG));

      /* Set 'UINumber' value */
      if (DeviceCapabilities.UINumber != (ULONG)-1)
      {
         RtlInitUnicodeString(&ValueName, L"UINumber");
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
   Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
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

      RtlInitUnicodeString(&ValueName, L"HardwareID");
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
   if (NT_SUCCESS(Status) && IoStatusBlock.Information)
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

      RtlInitUnicodeString(&ValueName, L"CompatibleIDs");
      Status = ZwSetValueKey(InstanceKey,
         &ValueName,
         0,
         REG_MULTI_SZ,
         (PVOID)IoStatusBlock.Information,
         (TotalLength + 1) * sizeof(WCHAR));
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("ZwSetValueKey() failed (Status %lx) or no Compatible ID returned\n", Status);
      }
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
   }


   DPRINT("Sending IRP_MN_QUERY_DEVICE_TEXT.DeviceTextDescription to device stack\n");

   Stack.Parameters.QueryDeviceText.DeviceTextType = DeviceTextDescription;
   Stack.Parameters.QueryDeviceText.LocaleId = LocaleId;
   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_DEVICE_TEXT,
      &Stack);
   /* This key is mandatory, so even if the Irp fails, we still write it */
   RtlInitUnicodeString(&ValueName, L"DeviceDesc");
   if (ZwQueryValueKey(InstanceKey, &ValueName, KeyValueBasicInformation, NULL, 0, &RequiredLength) == STATUS_OBJECT_NAME_NOT_FOUND)
   {
      if (NT_SUCCESS(Status) &&
         IoStatusBlock.Information &&
         (*(PWSTR)IoStatusBlock.Information != 0))
      {
         /* This key is overriden when a driver is installed. Don't write the
          * new description if another one already exists */
         Status = ZwSetValueKey(InstanceKey,
                                &ValueName,
                                0,
                                REG_SZ,
                                (PVOID)IoStatusBlock.Information,
                                (wcslen((PWSTR)IoStatusBlock.Information) + 1) * sizeof(WCHAR));
      }
      else
      {
         UNICODE_STRING DeviceDesc = RTL_CONSTANT_STRING(L"Unknown device");
         DPRINT("Driver didn't return DeviceDesc (Status 0x%08lx), so place unknown device there\n", Status);

         Status = ZwSetValueKey(InstanceKey,
            &ValueName,
            0,
            REG_SZ,
            DeviceDesc.Buffer,
            DeviceDesc.MaximumLength);

         if (!NT_SUCCESS(Status))
         {
            DPRINT1("ZwSetValueKey() failed (Status 0x%lx)\n", Status);
         }

      }
   }

   DPRINT("Sending IRP_MN_QUERY_DEVICE_TEXT.DeviceTextLocation to device stack\n");

   Stack.Parameters.QueryDeviceText.DeviceTextType = DeviceTextLocationInformation;
   Stack.Parameters.QueryDeviceText.LocaleId = LocaleId;
   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_DEVICE_TEXT,
      &Stack);
   if (NT_SUCCESS(Status) && IoStatusBlock.Information)
   {
      DPRINT("LocationInformation: %S\n", (PWSTR)IoStatusBlock.Information);
      RtlInitUnicodeString(&ValueName, L"LocationInformation");
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
      DPRINT("IopInitiatePnpIrp() failed (Status %x) or IoStatusBlock.Information=NULL\n", Status);
   }

   DPRINT("Sending IRP_MN_QUERY_BUS_INFORMATION to device stack\n");

   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_BUS_INFORMATION,
      NULL);
   if (NT_SUCCESS(Status) && IoStatusBlock.Information)
   {
      PPNP_BUS_INFORMATION BusInformation =
         (PPNP_BUS_INFORMATION)IoStatusBlock.Information;

      DeviceNode->ChildBusNumber = BusInformation->BusNumber;
      DeviceNode->ChildInterfaceType = BusInformation->LegacyBusType;
      DeviceNode->ChildBusTypeIndex = IopGetBusTypeGuidIndex(&BusInformation->BusTypeGuid);
      ExFreePool(BusInformation);
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x) or IoStatusBlock.Information=NULL\n", Status);

      DeviceNode->ChildBusNumber = 0xFFFFFFF0;
      DeviceNode->ChildInterfaceType = InterfaceTypeUndefined;
      DeviceNode->ChildBusTypeIndex = -1;
   }

   DPRINT("Sending IRP_MN_QUERY_RESOURCES to device stack\n");

   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_RESOURCES,
      NULL);
   if (NT_SUCCESS(Status) && IoStatusBlock.Information)
   {
      DeviceNode->BootResources =
         (PCM_RESOURCE_LIST)IoStatusBlock.Information;
      DeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x) or IoStatusBlock.Information=NULL\n", Status);
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
      if (IoStatusBlock.Information)
         IopDeviceNodeSetFlag(DeviceNode, DNF_RESOURCE_REPORTED);
      else
         IopDeviceNodeSetFlag(DeviceNode, DNF_NO_RESOURCE_REQUIRED);
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %08lx)\n", Status);
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
IopActionConfigureChildServices(PDEVICE_NODE DeviceNode,
                                PVOID Context)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[3];
   PDEVICE_NODE ParentDeviceNode;
   PUNICODE_STRING Service;
   UNICODE_STRING ClassGUID;
   UNICODE_STRING NullString = RTL_CONSTANT_STRING(L"");
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
      RtlInitUnicodeString(&ClassGUID, NULL);

      QueryTable[0].Name = L"Service";
      QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
      QueryTable[0].EntryContext = Service;

      QueryTable[1].Name = L"ClassGUID";
      QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
      QueryTable[1].EntryContext = &ClassGUID;
      QueryTable[1].DefaultType = REG_SZ;
      QueryTable[1].DefaultData = &NullString;
      QueryTable[1].DefaultLength = 0;

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

         if (ClassGUID.Length != 0)
         {
            /* Device has a ClassGUID value, but no Service value.
             * Suppose it is using the NULL driver, so state the
             * device is started */
            DPRINT("%wZ is using NULL driver\n", &DeviceNode->InstancePath);
            IopDeviceNodeSetFlag(DeviceNode, DNF_STARTED);
            DeviceNode->Flags |= DN_STARTED;
         }
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
IopActionInitChildServices(PDEVICE_NODE DeviceNode,
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
      PLDR_DATA_TABLE_ENTRY ModuleObject;
      PDRIVER_OBJECT DriverObject;

      Status = IopLoadServiceModule(&DeviceNode->ServiceName, &ModuleObject);
      if (NT_SUCCESS(Status) || Status == STATUS_IMAGE_ALREADY_LOADED)
      {
         if (Status != STATUS_IMAGE_ALREADY_LOADED)
         {
            DeviceNode->Flags |= DN_DRIVER_LOADED;
            Status = IopInitializeDriverModule(DeviceNode, ModuleObject,
               &DeviceNode->ServiceName, FALSE, &DriverObject);
         }
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
IopActionInitAllServices(PDEVICE_NODE DeviceNode,
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
IopActionInitBootServices(PDEVICE_NODE DeviceNode,
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
IopInitializePnpServices(IN PDEVICE_NODE DeviceNode,
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
   }
   else
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
IopInvalidateDeviceRelations(IN PDEVICE_NODE DeviceNode,
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

   DPRINT("DeviceNode 0x%p\n", DeviceNode);

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


static
NTSTATUS
INIT_FUNCTION
IopSetRootDeviceInstanceData(PDEVICE_NODE DeviceNode)
{
#if 0
    PWSTR KeyBuffer;
    HANDLE InstanceKey = NULL;
    NTSTATUS Status;

    /* Create registry key for the instance id, if it doesn't exist yet */
    KeyBuffer = ExAllocatePool(PagedPool,
                               (49 * sizeof(WCHAR)) + DeviceNode->InstancePath.Length);
    wcscpy(KeyBuffer, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
    wcscat(KeyBuffer, DeviceNode->InstancePath.Buffer);
    Status = IopCreateDeviceKeyPath(KeyBuffer,
                                    &InstanceKey);
    ExFreePool(KeyBuffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create the instance key! (Status %lx)\n", Status);
        return Status;
    }

    /* FIXME: Set 'ConfigFlags' value */

    ZwClose(InstanceKey);

    return Status;
#endif
    return STATUS_SUCCESS;
}


VOID INIT_FUNCTION
PnpInit(VOID)
{
   PDEVICE_OBJECT Pdo;
   NTSTATUS Status;

   DPRINT("PnpInit()\n");

   KeInitializeSpinLock(&IopDeviceTreeLock);

   /* Initialize the Bus Type GUID List */
   IopBusTypeGuidList = ExAllocatePool(PagedPool, sizeof(IO_BUS_TYPE_GUID_LIST));
   RtlZeroMemory(IopBusTypeGuidList, sizeof(IO_BUS_TYPE_GUID_LIST));
   ExInitializeFastMutex(&IopBusTypeGuidList->Lock);

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

   if (!RtlCreateUnicodeString(&IopRootDeviceNode->InstancePath,
       L"HTREE\\ROOT\\0"))
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


VOID INIT_FUNCTION
PnpInit2(VOID)
{
   NTSTATUS Status;

   /* Set root device instance data */
   Status = IopSetRootDeviceInstanceData(IopRootDeviceNode);
   if (!NT_SUCCESS(Status))
   {
      CPRINT("Failed to set instance data\n");
      KEBUGCHECKEX(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
   }
}

/* EOF */
