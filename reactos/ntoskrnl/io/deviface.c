/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/pnpmgr/devintrf.c
 * PURPOSE:        Device interface functions
 * PROGRAMMER:     Filip Navara (xnavara@volny.cz)
 *                 Matthew Brace (ismarc@austin.rr.com)
 * UPDATE HISTORY:
 *    22/09/2003 FiN Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ole32/guiddef.h>
#ifdef DEFINE_GUID
DEFINE_GUID(GUID_SERENUM_BUS_ENUMERATOR, 0x4D36E978L, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18);
#endif
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

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
   PWCHAR BaseKeyString = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\DeviceClasses\\";
   PWCHAR BaseInterfaceString = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";
   UNICODE_STRING GuidString;
   UNICODE_STRING BaseKeyName;
   UNICODE_STRING AliasKeyName;
   UNICODE_STRING SymbolicLink;
   UNICODE_STRING SubKeyName;
   UNICODE_STRING SymbolicLinkKeyName;
   UNICODE_STRING TempString;
   HANDLE InterfaceKey;
   HANDLE SubKey;
   HANDLE SymbolicLinkKey;
   PKEY_FULL_INFORMATION fip;
   PKEY_BASIC_INFORMATION bip;
   PKEY_VALUE_PARTIAL_INFORMATION vpip;
   PWCHAR SymLinkList = NULL;
   ULONG SymLinkListSize;
   NTSTATUS Status;
   ULONG Size = 0;
   ULONG i = 0;
   OBJECT_ATTRIBUTES ObjectAttributes;
  
   Status = RtlStringFromGUID(InterfaceClassGuid, &GuidString);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("RtlStringFromGUID() Failed.\n");
      return STATUS_INVALID_HANDLE;
   }

   RtlInitUnicodeString(&AliasKeyName, BaseInterfaceString);
   RtlInitUnicodeString(&SymbolicLink, L"SymbolicLink");
   BaseKeyName.Length = wcslen(BaseKeyString) * sizeof(WCHAR);
   BaseKeyName.MaximumLength = BaseKeyName.Length + (38 * sizeof(WCHAR));
   BaseKeyName.Buffer = ExAllocatePool(
      NonPagedPool,
      BaseKeyName.MaximumLength);
   assert(BaseKeyName.Buffer != NULL);
   wcscpy(BaseKeyName.Buffer, BaseKeyString);
   RtlAppendUnicodeStringToString(&BaseKeyName, &GuidString);

   if (PhysicalDeviceObject)
   {
      WCHAR GuidBuffer[32];
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
      assert(fip != NULL);

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
            DPRINT("ZwEnumerateKey() Failed. (0x%X)\n", Status);
            ExFreePool(fip);
            if (SymLinkList != NULL)
               ExFreePool(SymLinkList);
            RtlFreeUnicodeString(&BaseKeyName);
            ZwClose(InterfaceKey);
            return Status;
         }

         bip = (PKEY_BASIC_INFORMATION)ExAllocatePool(NonPagedPool, Size);
         assert(bip != NULL);
    
         Status = ZwEnumerateKey(
            InterfaceKey,
            i,
            KeyBasicInformation,
            bip,
            Size,
            &Size);
      
         if (!NT_SUCCESS(Status))
         {
            DPRINT("ZwEnumerateKey() Failed. (0x%X)\n", Status);
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
         assert(SubKeyName.Buffer != NULL);
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

         Status = ZwEnumerateKey(
            SubKey,
            0,
            KeyBasicInformation,
            NULL,
            0,
            &Size);

         if (Status != STATUS_BUFFER_TOO_SMALL)
         {
            DPRINT("ZwEnumerateKey() Failed. (0x%X)\n", Status);
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
         assert(bip != NULL);

         Status = ZwEnumerateKey(
            SubKey,
            0,
            KeyBasicInformation,
            bip,
            Size,
            &Size);

         if (!NT_SUCCESS(Status))
         {
            DPRINT("ZwEnumerateKey() Failed. (0x%X)\n", Status);
            ExFreePool(fip);
            ExFreePool(bip);
            if (SymLinkList != NULL)
               ExFreePool(SymLinkList);
            RtlFreeUnicodeString(&SubKeyName);
            RtlFreeUnicodeString(&BaseKeyName);
            ZwClose(SubKey);
            ZwClose(InterfaceKey);
            return Status;
         }

         SymbolicLinkKeyName.Length = 0;
         SymbolicLinkKeyName.MaximumLength = SubKeyName.Length + bip->NameLength + sizeof(WCHAR);
         SymbolicLinkKeyName.Buffer = ExAllocatePool(NonPagedPool, SymbolicLinkKeyName.MaximumLength);
         assert(SymbolicLinkKeyName.Buffer != NULL);
         TempString.Length = TempString.MaximumLength = bip->NameLength;
         TempString.Buffer = bip->Name;
         RtlCopyUnicodeString(&SymbolicLinkKeyName, &SubKeyName);
         RtlAppendUnicodeToString(&SymbolicLinkKeyName, L"\\");
         RtlAppendUnicodeStringToString(&SymbolicLinkKeyName, &TempString);
      
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
      
         if (Status != STATUS_BUFFER_TOO_SMALL)
         {
            DPRINT("ZwQueryValueKey() Failed. (0x%X)\n", Status);
            ExFreePool(fip);
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
         assert(vpip != NULL);

         Status = ZwQueryValueKey(
            SymbolicLinkKey,
         		&SymbolicLink,
         		KeyValuePartialInformation,
         		vpip,
         		Size,
         		&Size);

         if (!NT_SUCCESS(Status))
         {
            ExFreePool(fip);
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

         /* Put the name in the string here */
         if (SymLinkList == NULL)
         {
            SymLinkListSize = vpip->DataLength;
            SymLinkList = ExAllocatePool(NonPagedPool, SymLinkListSize + sizeof(WCHAR));
            assert(SymLinkList != NULL);
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
            assert(SymLinkList != NULL);
            RtlCopyMemory(SymLinkList, OldSymLinkList, OldSymLinkListSize);
            ExFreePool(OldSymLinkList);
            SymLinkListPtr = SymLinkList + (OldSymLinkListSize / sizeof(WCHAR));
            RtlCopyMemory(SymLinkListPtr, vpip->Data, vpip->DataLength);
            SymLinkListPtr[vpip->DataLength / sizeof(WCHAR)] = 0;
            SymLinkListPtr[1] = '?';
         }

         RtlFreeUnicodeString(&SymbolicLinkKeyName);
         RtlFreeUnicodeString(&SubKeyName);
         ZwClose(SymbolicLinkKey);
         ZwClose(SubKey);
         ExFreePool(vpip);
      }

      if (SymLinkList != NULL)
         SymLinkList[SymLinkListSize / sizeof(WCHAR)] = 0;
      *SymbolicLinkList = SymLinkList;
    
      RtlFreeUnicodeString(&BaseKeyName);
      ZwClose(InterfaceKey);
      ExFreePool(fip);
   }

   return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */

NTSTATUS STDCALL
IoRegisterDeviceInterface(
   IN PDEVICE_OBJECT PhysicalDeviceObject,
   IN CONST GUID *InterfaceClassGuid,
   IN PUNICODE_STRING ReferenceString OPTIONAL,
   OUT PUNICODE_STRING SymbolicLinkName)
{
   PWCHAR KeyNameString = L"\\Device\\Serenum";

   DPRINT("IoRegisterDeviceInterface called (UNIMPLEMENTED)\n");
   if (IsEqualGUID(InterfaceClassGuid, (LPGUID)&GUID_SERENUM_BUS_ENUMERATOR))
   {
      RtlInitUnicodeString(SymbolicLinkName, KeyNameString);
      return STATUS_SUCCESS;
   }

   return STATUS_INVALID_DEVICE_REQUEST;
}

/*
 * @unimplemented
 */

NTSTATUS STDCALL
IoSetDeviceInterfaceState(
   IN PUNICODE_STRING SymbolicLinkName,
   IN BOOLEAN Enable)
{
   return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
