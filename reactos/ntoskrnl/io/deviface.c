/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/deviface.c
 * PURPOSE:         Device interface functions
 *
 * PROGRAMMERS:     Filip Navara (xnavara@volny.cz)
 *                  Matthew Brace (ismarc@austin.rr.com)
 *                  Hervé Poussineau (hpoussin@reactos.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

static PWCHAR BaseKeyString = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\DeviceClasses\\";

/*
 * @unimplemented
 */

NTSTATUS STDCALL
IoOpenDeviceInterfaceRegistryKey(
   IN PUNICODE_STRING SymbolicLinkName,
   IN ACCESS_MASK DesiredAccess,
   OUT PHANDLE DeviceInterfaceKey)
{
   return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */

NTSTATUS STDCALL
IoGetDeviceInterfaceAlias(
   IN PUNICODE_STRING SymbolicLinkName,
   IN CONST GUID *AliasInterfaceClassGuid,
   OUT PUNICODE_STRING AliasSymbolicLinkName)
{
   return STATUS_NOT_IMPLEMENTED;
}

/*
 * IoGetDeviceInterfaces
 *
 * Returns a list of device interfaces of a particular device interface class.
 *
 * Parameters
 *    InterfaceClassGuid
 *       Points to a class GUID specifying the device interface class.
 *
 *    PhysicalDeviceObject
 *       Points to an optional PDO that narrows the search to only the
 *       device interfaces of the device represented by the PDO.
 *
 *    Flags
 *       Specifies flags that modify the search for device interfaces. The
 *       DEVICE_INTERFACE_INCLUDE_NONACTIVE flag specifies that the list of
 *       returned symbolic links should contain also disabled device
 *       interfaces in addition to the enabled ones.
 *
 *    SymbolicLinkList
 *       Points to a character pointer that is filled in on successful return
 *       with a list of unicode strings identifying the device interfaces
 *       that match the search criteria. The newly allocated buffer contains
 *       a list of symbolic link names. Each unicode string in the list is
 *       null-terminated; the end of the whole list is marked by an additional
 *       NULL. The caller is responsible for freeing the buffer (ExFreePool)
 *       when it is no longer needed.
 *       If no device interfaces match the search criteria, this routine
 *       returns STATUS_SUCCESS and the string contains a single NULL
 *       character.
 *
 * Status
 *    @implemented
 *
 *    The parameters PhysicalDeviceObject and Flags aren't correctly
 *    processed. Rest of the cases was tested under Windows(R) XP and
 *    the function worked correctly.
 */

NTSTATUS STDCALL
IoGetDeviceInterfaces(
   IN CONST GUID *InterfaceClassGuid,
   IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
   IN ULONG Flags,
   OUT PWSTR *SymbolicLinkList)
{
   PWCHAR BaseInterfaceString = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";
   UNICODE_STRING GuidString;
   UNICODE_STRING BaseKeyName;
   UNICODE_STRING AliasKeyName;
   UNICODE_STRING SymbolicLink;
   UNICODE_STRING Control;
   UNICODE_STRING SubKeyName;
   UNICODE_STRING SymbolicLinkKeyName;
   UNICODE_STRING ControlKeyName;
   UNICODE_STRING TempString;
   HANDLE InterfaceKey;
   HANDLE SubKey;
   HANDLE SymbolicLinkKey;
   PKEY_FULL_INFORMATION fip;
   PKEY_FULL_INFORMATION bfip = NULL;
   PKEY_BASIC_INFORMATION bip;
   PKEY_VALUE_PARTIAL_INFORMATION vpip = NULL;
   PWCHAR SymLinkList = NULL;
   ULONG SymLinkListSize = 0;
   NTSTATUS Status;
   ULONG Size = 0;
   ULONG i = 0;
   ULONG j = 0;
   OBJECT_ATTRIBUTES ObjectAttributes;

   Status = RtlStringFromGUID(InterfaceClassGuid, &GuidString);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("RtlStringFromGUID() Failed.\n");
      return STATUS_INVALID_HANDLE;
   }

   RtlInitUnicodeString(&AliasKeyName, BaseInterfaceString);
   RtlInitUnicodeString(&SymbolicLink, L"SymbolicLink");
   RtlInitUnicodeString(&Control, L"\\Control");
   BaseKeyName.Length = wcslen(BaseKeyString) * sizeof(WCHAR);
   BaseKeyName.MaximumLength = BaseKeyName.Length + (38 * sizeof(WCHAR));
   BaseKeyName.Buffer = ExAllocatePool(
      NonPagedPool,
      BaseKeyName.MaximumLength);
   ASSERT(BaseKeyName.Buffer != NULL);
   wcscpy(BaseKeyName.Buffer, BaseKeyString);
   RtlAppendUnicodeStringToString(&BaseKeyName, &GuidString);

   if (PhysicalDeviceObject)
   {
      WCHAR GuidBuffer[40];
      UNICODE_STRING PdoGuidString;

      RtlFreeUnicodeString(&BaseKeyName);

      IoGetDeviceProperty(
         PhysicalDeviceObject,
         DevicePropertyClassGuid,
         sizeof(GuidBuffer),
         GuidBuffer,
         &Size);

      RtlInitUnicodeString(&PdoGuidString, GuidBuffer);
      if (RtlCompareUnicodeString(&GuidString, &PdoGuidString, TRUE))
      {
         DPRINT("Inconsistent Guid's asked for in IoGetDeviceInterfaces()\n");
         return STATUS_INVALID_HANDLE;
      }

      DPRINT("IoGetDeviceInterfaces() called with PDO, not implemented.\n");
      return STATUS_NOT_IMPLEMENTED;
   }
   else
   {
      InitializeObjectAttributes(
         &ObjectAttributes,
         &BaseKeyName,
         OBJ_CASE_INSENSITIVE,
         NULL,
         NULL);

      Status = ZwOpenKey(
         &InterfaceKey,
         KEY_READ,
         &ObjectAttributes);

      if (!NT_SUCCESS(Status))
      {
         DPRINT("ZwOpenKey() Failed. (0x%X)\n", Status);
         RtlFreeUnicodeString(&BaseKeyName);
         return Status;
      }

      Status = ZwQueryKey(
         InterfaceKey,
         KeyFullInformation,
         NULL,
         0,
         &Size);

      if (Status != STATUS_BUFFER_TOO_SMALL)
      {
         DPRINT("ZwQueryKey() Failed. (0x%X)\n", Status);
         RtlFreeUnicodeString(&BaseKeyName);
         ZwClose(InterfaceKey);
         return Status;
      }

      fip = (PKEY_FULL_INFORMATION)ExAllocatePool(NonPagedPool, Size);
      ASSERT(fip != NULL);

      Status = ZwQueryKey(
         InterfaceKey,
         KeyFullInformation,
         fip,
         Size,
         &Size);

      if (!NT_SUCCESS(Status))
      {
         DPRINT("ZwQueryKey() Failed. (0x%X)\n", Status);
         ExFreePool(fip);
         RtlFreeUnicodeString(&BaseKeyName);
         ZwClose(InterfaceKey);
         return Status;
      }

      for (; i < fip->SubKeys; i++)
      {
         Status = ZwEnumerateKey(
            InterfaceKey,
            i,
            KeyBasicInformation,
            NULL,
            0,
            &Size);

         if (Status != STATUS_BUFFER_TOO_SMALL)
         {
            DPRINT("ZwEnumerateKey() Failed.(0x%X)\n", Status);
            ExFreePool(fip);
            if (SymLinkList != NULL)
               ExFreePool(SymLinkList);
            RtlFreeUnicodeString(&BaseKeyName);
            ZwClose(InterfaceKey);
            return Status;
         }

         bip = (PKEY_BASIC_INFORMATION)ExAllocatePool(NonPagedPool, Size);
         ASSERT(bip != NULL);

         Status = ZwEnumerateKey(
            InterfaceKey,
            i,
            KeyBasicInformation,
            bip,
            Size,
            &Size);

         if (!NT_SUCCESS(Status))
         {
            DPRINT("ZwEnumerateKey() Failed.(0x%X)\n", Status);
            ExFreePool(fip);
            ExFreePool(bip);
            if (SymLinkList != NULL)
               ExFreePool(SymLinkList);
            RtlFreeUnicodeString(&BaseKeyName);
            ZwClose(InterfaceKey);
            return Status;
         }

         SubKeyName.Length = 0;
         SubKeyName.MaximumLength = BaseKeyName.Length + bip->NameLength + sizeof(WCHAR);
         SubKeyName.Buffer = ExAllocatePool(NonPagedPool, SubKeyName.MaximumLength);
         ASSERT(SubKeyName.Buffer != NULL);
         TempString.Length = TempString.MaximumLength = bip->NameLength;
         TempString.Buffer = bip->Name;
         RtlCopyUnicodeString(&SubKeyName, &BaseKeyName);
         RtlAppendUnicodeToString(&SubKeyName, L"\\");
         RtlAppendUnicodeStringToString(&SubKeyName, &TempString);

         ExFreePool(bip);

         InitializeObjectAttributes(
            &ObjectAttributes,
            &SubKeyName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

         Status = ZwOpenKey(
            &SubKey,
            KEY_READ,
            &ObjectAttributes);

         if (!NT_SUCCESS(Status))
         {
            DPRINT("ZwOpenKey() Failed. (0x%X)\n", Status);
            ExFreePool(fip);
            if (SymLinkList != NULL)
               ExFreePool(SymLinkList);
            RtlFreeUnicodeString(&SubKeyName);
            RtlFreeUnicodeString(&BaseKeyName);
            ZwClose(InterfaceKey);
            return Status;
         }

         Status = ZwQueryKey(
            SubKey,
            KeyFullInformation,
            NULL,
            0,
            &Size);

         if (Status != STATUS_BUFFER_TOO_SMALL)
         {
            DPRINT("ZwQueryKey() Failed. (0x%X)\n", Status);
            ExFreePool(fip);
            RtlFreeUnicodeString(&BaseKeyName);
            RtlFreeUnicodeString(&SubKeyName);
            ZwClose(SubKey);
            ZwClose(InterfaceKey);
            return Status;
         }

         bfip = (PKEY_FULL_INFORMATION)ExAllocatePool(NonPagedPool, Size);
         ASSERT(bfip != NULL);

         Status = ZwQueryKey(
            SubKey,
            KeyFullInformation,
            bfip,
            Size,
            &Size);

         if (!NT_SUCCESS(Status))
         {
            DPRINT("ZwQueryKey() Failed. (0x%X)\n", Status);
            ExFreePool(fip);
            RtlFreeUnicodeString(&SubKeyName);
            RtlFreeUnicodeString(&BaseKeyName);
            ZwClose(SubKey);
            ZwClose(InterfaceKey);
            return Status;
         }

         for(j = 0; j < bfip->SubKeys; j++)
         {
            Status = ZwEnumerateKey(
               SubKey,
               j,
               KeyBasicInformation,
               NULL,
               0,
               &Size);

            if (Status == STATUS_NO_MORE_ENTRIES)
               continue;

            if (Status != STATUS_BUFFER_TOO_SMALL)
            {
               DPRINT("ZwEnumerateKey() Failed.(0x%X)\n", Status);
               ExFreePool(bfip);
               ExFreePool(fip);
               if (SymLinkList != NULL)
                  ExFreePool(SymLinkList);
               RtlFreeUnicodeString(&SubKeyName);
               RtlFreeUnicodeString(&BaseKeyName);
               ZwClose(SubKey);
               ZwClose(InterfaceKey);
               return Status;
            }

            bip = (PKEY_BASIC_INFORMATION)ExAllocatePool(NonPagedPool, Size);
            ASSERT(bip != NULL);

            Status = ZwEnumerateKey(
               SubKey,
               j,
               KeyBasicInformation,
               bip,
               Size,
               &Size);

            if (!NT_SUCCESS(Status))
            {
               DPRINT("ZwEnumerateKey() Failed.(0x%X)\n", Status);
               ExFreePool(fip);
               ExFreePool(bfip);
               ExFreePool(bip);
               if (SymLinkList != NULL)
                  ExFreePool(SymLinkList);
               RtlFreeUnicodeString(&SubKeyName);
               RtlFreeUnicodeString(&BaseKeyName);
               ZwClose(SubKey);
               ZwClose(InterfaceKey);
               return Status;
            }

            if (!wcsncmp(bip->Name, L"Control", bip->NameLength))
            {
               continue;
            }

            SymbolicLinkKeyName.Length = 0;
            SymbolicLinkKeyName.MaximumLength = SubKeyName.Length + bip->NameLength + sizeof(WCHAR);
            SymbolicLinkKeyName.Buffer = ExAllocatePool(NonPagedPool, SymbolicLinkKeyName.MaximumLength);
            ASSERT(SymbolicLinkKeyName.Buffer != NULL);
            TempString.Length = TempString.MaximumLength = bip->NameLength;
            TempString.Buffer = bip->Name;
            RtlCopyUnicodeString(&SymbolicLinkKeyName, &SubKeyName);
            RtlAppendUnicodeToString(&SymbolicLinkKeyName, L"\\");
            RtlAppendUnicodeStringToString(&SymbolicLinkKeyName, &TempString);

            ControlKeyName.Length = 0;
            ControlKeyName.MaximumLength = SymbolicLinkKeyName.Length + Control.Length + sizeof(WCHAR);
            ControlKeyName.Buffer = ExAllocatePool(NonPagedPool, ControlKeyName.MaximumLength);
            ASSERT(ControlKeyName.Buffer != NULL);
            RtlCopyUnicodeString(&ControlKeyName, &SymbolicLinkKeyName);
            RtlAppendUnicodeStringToString(&ControlKeyName, &Control);

            ExFreePool(bip);

            InitializeObjectAttributes(
               &ObjectAttributes,
               &SymbolicLinkKeyName,
               OBJ_CASE_INSENSITIVE,
               NULL,
               NULL);

            Status = ZwOpenKey(
               &SymbolicLinkKey,
               KEY_READ,
               &ObjectAttributes);

            if (!NT_SUCCESS(Status))
            {
               DPRINT("ZwOpenKey() Failed. (0x%X)\n", Status);
               ExFreePool(fip);
               ExFreePool(bfip);
               if (SymLinkList != NULL)
                  ExFreePool(SymLinkList);
               RtlFreeUnicodeString(&SymbolicLinkKeyName);
               RtlFreeUnicodeString(&SubKeyName);
               RtlFreeUnicodeString(&BaseKeyName);
               ZwClose(SubKey);
               ZwClose(InterfaceKey);
               return Status;
            }

            Status = ZwQueryValueKey(
               SymbolicLinkKey,
               &SymbolicLink,
               KeyValuePartialInformation,
               NULL,
               0,
               &Size);

            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
               continue;

            if (Status != STATUS_BUFFER_TOO_SMALL)
            {
               DPRINT("ZwQueryValueKey() Failed.(0x%X)\n", Status);
               ExFreePool(fip);
               ExFreePool(bfip);
               if (SymLinkList != NULL)
                  ExFreePool(SymLinkList);
               RtlFreeUnicodeString(&SymbolicLinkKeyName);
               RtlFreeUnicodeString(&SubKeyName);
               RtlFreeUnicodeString(&BaseKeyName);
               ZwClose(SymbolicLinkKey);
               ZwClose(SubKey);
               ZwClose(InterfaceKey);
               return Status;
            }

            vpip = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool(NonPagedPool, Size);
            ASSERT(vpip != NULL);

            Status = ZwQueryValueKey(
               SymbolicLinkKey,
               &SymbolicLink,
               KeyValuePartialInformation,
               vpip,
               Size,
               &Size);

            if (!NT_SUCCESS(Status))
            {
               DPRINT("ZwQueryValueKey() Failed.(0x%X)\n", Status);
               ExFreePool(fip);
               ExFreePool(bfip);
               ExFreePool(vpip);
               if (SymLinkList != NULL)
                  ExFreePool(SymLinkList);
               RtlFreeUnicodeString(&SymbolicLinkKeyName);
               RtlFreeUnicodeString(&SubKeyName);
               RtlFreeUnicodeString(&BaseKeyName);
               ZwClose(SymbolicLinkKey);
               ZwClose(SubKey);
               ZwClose(InterfaceKey);
               return Status;
            }

            Status = RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE, ControlKeyName.Buffer);

            if (NT_SUCCESS(Status))
            {
               /* Put the name in the string here */
               if (SymLinkList == NULL)
               {
                  SymLinkListSize = vpip->DataLength;
                  SymLinkList = ExAllocatePool(NonPagedPool, SymLinkListSize + sizeof(WCHAR));
                  ASSERT(SymLinkList != NULL);
                  RtlCopyMemory(SymLinkList, vpip->Data, vpip->DataLength);
                  SymLinkList[vpip->DataLength / sizeof(WCHAR)] = 0;
                  SymLinkList[1] = '?';
               }
               else
               {
                  PWCHAR OldSymLinkList;
                  ULONG OldSymLinkListSize;
                  PWCHAR SymLinkListPtr;

                  OldSymLinkList = SymLinkList;
                  OldSymLinkListSize = SymLinkListSize;
                  SymLinkListSize += vpip->DataLength;
                  SymLinkList = ExAllocatePool(NonPagedPool, SymLinkListSize + sizeof(WCHAR));
                  ASSERT(SymLinkList != NULL);
                  RtlCopyMemory(SymLinkList, OldSymLinkList, OldSymLinkListSize);
                  ExFreePool(OldSymLinkList);
                  SymLinkListPtr = SymLinkList + (OldSymLinkListSize / sizeof(WCHAR));
                  RtlCopyMemory(SymLinkListPtr, vpip->Data, vpip->DataLength);
                  SymLinkListPtr[vpip->DataLength / sizeof(WCHAR)] = 0;
                  SymLinkListPtr[1] = '?';
               }
            }

            RtlFreeUnicodeString(&SymbolicLinkKeyName);
            RtlFreeUnicodeString(&ControlKeyName);
            ZwClose(SymbolicLinkKey);
         }

         ExFreePool(vpip);
         RtlFreeUnicodeString(&SubKeyName);
         ZwClose(SubKey);
      }

      if (SymLinkList != NULL)
      {
         SymLinkList[SymLinkListSize / sizeof(WCHAR)] = 0;
      }
      else
      {
         SymLinkList = ExAllocatePool(NonPagedPool, 2 * sizeof(WCHAR));
         SymLinkList[0] = 0;
      }

      *SymbolicLinkList = SymLinkList;

      RtlFreeUnicodeString(&BaseKeyName);
      ZwClose(InterfaceKey);
      ExFreePool(bfip);
      ExFreePool(fip);
   }

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */

NTSTATUS STDCALL
IoRegisterDeviceInterface(
   IN PDEVICE_OBJECT PhysicalDeviceObject,
   IN CONST GUID *InterfaceClassGuid,
   IN PUNICODE_STRING ReferenceString OPTIONAL,
   OUT PUNICODE_STRING SymbolicLinkName)
{
   PUNICODE_STRING InstancePath;
   UNICODE_STRING GuidString;
   UNICODE_STRING SubKeyName;
   UNICODE_STRING InterfaceKeyName;
   UNICODE_STRING BaseKeyName;
   UCHAR PdoNameInfoBuffer[sizeof(OBJECT_NAME_INFORMATION) + (256 * sizeof(WCHAR))];
   POBJECT_NAME_INFORMATION PdoNameInfo = (POBJECT_NAME_INFORMATION)PdoNameInfoBuffer;
   UNICODE_STRING DeviceInstance = RTL_CONSTANT_STRING(L"DeviceInstance");
   UNICODE_STRING SymbolicLink = RTL_CONSTANT_STRING(L"SymbolicLink");
   HANDLE ClassKey;
   HANDLE InterfaceKey;
   HANDLE SubKey;
   ULONG StartIndex;
   OBJECT_ATTRIBUTES ObjectAttributes;
   ULONG i;
   NTSTATUS Status;

   if (!(PhysicalDeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE))
   {
     DPRINT("PhysicalDeviceObject 0x%p is not a valid Pdo\n", PhysicalDeviceObject);
     return STATUS_INVALID_PARAMETER_1;
   }

   Status = RtlStringFromGUID(InterfaceClassGuid, &GuidString);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("RtlStringFromGUID() failed with status 0x%08lx\n", Status);
      return Status;
   }

   /* Create Pdo name: \Device\xxxxxxxx (unnamed device) */
   Status = ObQueryNameString(
      PhysicalDeviceObject,
      PdoNameInfo,
      sizeof(PdoNameInfoBuffer),
      &i);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("ObQueryNameString() failed with status 0x%08lx\n", Status);
      return Status;
   }
   ASSERT(PdoNameInfo->Name.Length);

   /* Create base key name for this interface: HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{GUID} */
   ASSERT(PhysicalDeviceObject->DeviceObjectExtension->DeviceNode);
   InstancePath = &PhysicalDeviceObject->DeviceObjectExtension->DeviceNode->InstancePath;
   BaseKeyName.Length = wcslen(BaseKeyString) * sizeof(WCHAR);
   BaseKeyName.MaximumLength = BaseKeyName.Length
      + GuidString.Length;
   BaseKeyName.Buffer = ExAllocatePool(
      NonPagedPool,
      BaseKeyName.MaximumLength);
   if (!BaseKeyName.Buffer)
   {
      DPRINT("ExAllocatePool() failed\n");
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   wcscpy(BaseKeyName.Buffer, BaseKeyString);
   RtlAppendUnicodeStringToString(&BaseKeyName, &GuidString);

   /* Create BaseKeyName key in registry */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &BaseKeyName,
      OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_OPENIF,
      NULL, /* RootDirectory */
      NULL); /* SecurityDescriptor */

   Status = ZwCreateKey(
      &ClassKey,
      KEY_WRITE,
      &ObjectAttributes,
      0, /* TileIndex */
      NULL, /* Class */
      REG_OPTION_VOLATILE,
      NULL); /* Disposition */

   if (!NT_SUCCESS(Status))
   {
      DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
      ExFreePool(BaseKeyName.Buffer);
      return Status;
   }

   /* Create key name for this interface: ##?#ACPI#PNP0501#1#{GUID} */
   InterfaceKeyName.Length = 0;
   InterfaceKeyName.MaximumLength =
      4 * sizeof(WCHAR) + /* 4  = size of ##?# */
      InstancePath->Length +
      sizeof(WCHAR) +     /* 1  = size of # */
      GuidString.Length;
   InterfaceKeyName.Buffer = ExAllocatePool(
      NonPagedPool,
      InterfaceKeyName.MaximumLength);
   if (!InterfaceKeyName.Buffer)
   {
      DPRINT("ExAllocatePool() failed\n");
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlAppendUnicodeToString(&InterfaceKeyName, L"##?#");
   StartIndex = InterfaceKeyName.Length / sizeof(WCHAR);
   RtlAppendUnicodeStringToString(&InterfaceKeyName, InstancePath);
   for (i = 0; i < InstancePath->Length / sizeof(WCHAR); i++)
   {
      if (InterfaceKeyName.Buffer[StartIndex + i] == '\\')
         InterfaceKeyName.Buffer[StartIndex + i] = '#';
   }
   RtlAppendUnicodeToString(&InterfaceKeyName, L"#");
   RtlAppendUnicodeStringToString(&InterfaceKeyName, &GuidString);

   /* Create the interface key in registry */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &InterfaceKeyName,
      OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_OPENIF,
      ClassKey,
      NULL); /* SecurityDescriptor */

   Status = ZwCreateKey(
      &InterfaceKey,
      KEY_WRITE,
      &ObjectAttributes,
      0, /* TileIndex */
      NULL, /* Class */
      REG_OPTION_VOLATILE,
      NULL); /* Disposition */

   if (!NT_SUCCESS(Status))
   {
      DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
      ZwClose(ClassKey);
      ExFreePool(BaseKeyName.Buffer);
      return Status;
   }

   /* Write DeviceInstance entry. Value is InstancePath */
   Status = ZwSetValueKey(
      InterfaceKey,
      &DeviceInstance,
      0, /* TileIndex */
      REG_SZ,
      InstancePath->Buffer,
      InstancePath->Length);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("ZwSetValueKey() failed with status 0x%08lx\n", Status);
      ZwClose(InterfaceKey);
      ZwClose(ClassKey);
      ExFreePool(InterfaceKeyName.Buffer);
      ExFreePool(BaseKeyName.Buffer);
      return Status;
   }

   /* Create subkey. Name is #ReferenceString */
   SubKeyName.Length = 0;
   SubKeyName.MaximumLength = sizeof(WCHAR);
   if (ReferenceString && ReferenceString->Length)
      SubKeyName.MaximumLength += ReferenceString->Length;
   SubKeyName.Buffer = ExAllocatePool(
      NonPagedPool,
      SubKeyName.MaximumLength);
   if (!SubKeyName.Buffer)
   {
      DPRINT("ExAllocatePool() failed\n");
      ZwClose(InterfaceKey);
      ZwClose(ClassKey);
      ExFreePool(InterfaceKeyName.Buffer);
      ExFreePool(BaseKeyName.Buffer);
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   RtlAppendUnicodeToString(&SubKeyName, L"#");
   if (ReferenceString && ReferenceString->Length)
      RtlAppendUnicodeStringToString(&SubKeyName, ReferenceString);

   /* Create SubKeyName key in registry */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &SubKeyName,
      OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
      InterfaceKey, /* RootDirectory */
      NULL); /* SecurityDescriptor */

   Status = ZwCreateKey(
      &SubKey,
      KEY_WRITE,
      &ObjectAttributes,
      0, /* TileIndex */
      NULL, /* Class */
      REG_OPTION_VOLATILE,
      NULL); /* Disposition */

   if (!NT_SUCCESS(Status))
   {
      DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
      ZwClose(InterfaceKey);
      ZwClose(ClassKey);
      ExFreePool(InterfaceKeyName.Buffer);
      ExFreePool(BaseKeyName.Buffer);
      return Status;
   }

   /* Create symbolic link name: \??\ACPI#PNP0501#1#{GUID}\ReferenceString */
   SymbolicLinkName->Length = 0;
   SymbolicLinkName->MaximumLength = SymbolicLinkName->Length
      + 4 * sizeof(WCHAR) /* 4 = size of \??\ */
      + InstancePath->Length
      + sizeof(WCHAR)     /* 1  = size of # */
      + GuidString.Length
      + sizeof(WCHAR);    /* final NULL */
   if (ReferenceString && ReferenceString->Length)
      SymbolicLinkName->MaximumLength += sizeof(WCHAR) + ReferenceString->Length;
   SymbolicLinkName->Buffer = ExAllocatePool(
      NonPagedPool,
      SymbolicLinkName->MaximumLength);
   if (!SymbolicLinkName->Buffer)
   {
      DPRINT("ExAllocatePool() failed\n");
      ZwClose(SubKey);
      ZwClose(InterfaceKey);
      ZwClose(ClassKey);
      ExFreePool(InterfaceKeyName.Buffer);
      ExFreePool(SubKeyName.Buffer);
      ExFreePool(BaseKeyName.Buffer);
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   RtlAppendUnicodeToString(SymbolicLinkName, L"\\??\\");
   StartIndex = SymbolicLinkName->Length / sizeof(WCHAR);
   RtlAppendUnicodeStringToString(SymbolicLinkName, InstancePath);
   for (i = 0; i < InstancePath->Length / sizeof(WCHAR); i++)
   {
      if (SymbolicLinkName->Buffer[StartIndex + i] == '\\')
         SymbolicLinkName->Buffer[StartIndex + i] = '#';
   }
   RtlAppendUnicodeToString(SymbolicLinkName, L"#");
   RtlAppendUnicodeStringToString(SymbolicLinkName, &GuidString);
   if (ReferenceString && ReferenceString->Length)
   {
      RtlAppendUnicodeToString(SymbolicLinkName, L"\\");
      RtlAppendUnicodeStringToString(SymbolicLinkName, ReferenceString);
   }
   SymbolicLinkName->Buffer[SymbolicLinkName->Length] = '\0';

   /* Create symbolic link */
   DPRINT("IoRegisterDeviceInterface(): creating symbolic link %wZ -> %wZ\n", SymbolicLinkName, &PdoNameInfo->Name);
   Status = IoCreateSymbolicLink(SymbolicLinkName, &PdoNameInfo->Name);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IoCreateSymbolicLink() failed with status 0x%08lx\n", Status);
      ZwClose(SubKey);
      ZwClose(InterfaceKey);
      ZwClose(ClassKey);
      ExFreePool(SubKeyName.Buffer);
      ExFreePool(InterfaceKeyName.Buffer);
      ExFreePool(BaseKeyName.Buffer);
      ExFreePool(SymbolicLinkName->Buffer);
      return Status;
   }

   /* Write symbolic link name in registry */
   Status = ZwSetValueKey(
      SubKey,
      &SymbolicLink,
      0, /* TileIndex */
      REG_SZ,
      SymbolicLinkName->Buffer,
      SymbolicLinkName->Length);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("ZwSetValueKey() failed with status 0x%08lx\n", Status);
      ExFreePool(SymbolicLinkName->Buffer);
   }

   ZwClose(SubKey);
   ZwClose(InterfaceKey);
   ZwClose(ClassKey);
   ExFreePool(SubKeyName.Buffer);
   ExFreePool(InterfaceKeyName.Buffer);
   ExFreePool(BaseKeyName.Buffer);

   return Status;
}

/*
 * @implemented
 */

NTSTATUS STDCALL
IoSetDeviceInterfaceState(
   IN PUNICODE_STRING SymbolicLinkName,
   IN BOOLEAN Enable)
{
   PDEVICE_OBJECT PhysicalDeviceObject;
   PFILE_OBJECT FileObject;
   UNICODE_STRING GuidString;
   PWCHAR StartPosition;
   PWCHAR EndPosition;
   NTSTATUS Status;

   if (SymbolicLinkName == NULL)
      return STATUS_INVALID_PARAMETER_1;

   DPRINT("IoSetDeviceInterfaceState('%wZ', %d)\n", SymbolicLinkName, Enable);
   Status = IoGetDeviceObjectPointer(SymbolicLinkName,
      0, /* DesiredAccess */
      &FileObject,
      &PhysicalDeviceObject);
   if (!NT_SUCCESS(Status))
      return Status;

   /* Symbolic link name is \??\ACPI#PNP0501#1#{GUID}\ReferenceString */
   /* Get GUID from SymbolicLinkName */
   StartPosition = wcschr(SymbolicLinkName->Buffer, L'{');
   EndPosition = wcschr(SymbolicLinkName->Buffer, L'}');
   if (!StartPosition ||!EndPosition || StartPosition > EndPosition)
      return STATUS_INVALID_PARAMETER_1;
   GuidString.Buffer = StartPosition;
   GuidString.MaximumLength = GuidString.Length = (ULONG_PTR)(EndPosition + 1) - (ULONG_PTR)StartPosition;

   IopNotifyPlugPlayNotification(
      PhysicalDeviceObject,
      EventCategoryDeviceInterfaceChange,
      Enable ? (LPGUID)&GUID_DEVICE_INTERFACE_ARRIVAL : (LPGUID)&GUID_DEVICE_INTERFACE_REMOVAL,
      &GuidString,
      (PVOID)SymbolicLinkName);

   ObDereferenceObject(FileObject);

   return STATUS_SUCCESS;
}

/* EOF */
