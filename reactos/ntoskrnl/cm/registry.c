/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cm/registry.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FILE STATICS *************************************************************/

POBJECT_TYPE CmKeyType = NULL;
PKEY_OBJECT RootKey = NULL;

/* FUNCTIONS *****************************************************************/

VOID
CmInitializeRegistry(VOID)
{
#if 0
  ANSI_STRING AnsiString;
  
  CmKeyType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  CmKeyType->TotalObjects = 0;
  CmKeyType->TotalHandles = 0;
  CmKeyType->MaxObjects = ULONG_MAX;
  CmKeyType->MaxHandles = ULONG_MAX;
  CmKeyType->PagedPoolCharge = 0;
  CmKeyType->NonpagedPoolCharge = sizeof(KEY_OBJECT);
  CmKeyType->Dump = NULL;
  CmKeyType->Open = NULL;
  CmKeyType->Close = NULL;
  CmKeyType->Delete = NULL;
  CmKeyType->Parse = CmpObjectParse;
  CmKeyType->Security = NULL;
  CmKeyType->QueryName = NULL;
  CmKeyType->OkayToClose = NULL;
   
  RtlInitAnsiString(&AnsiString, "Key");
  RtlAnsiStringToUnicodeString(&CmKeyType->TypeName, &AnsiString, TRUE);

  RtlInitAnsiString(&AnsiString,"\\Registry");
  RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
  InitializeObjectAttributes(&attr, &UnicodeString, 0, NULL, NULL);
  ZwCreateDirectoryObject(&handle, 0, &attr);
  RtlFreeUnicodeString(UnicodeString);

  /* FIXME: build initial registry skeleton */
  RootKey = ObGenericCreateObject(NULL, 
                                 KEY_ALL_ACCESS, 
                                 NULL, 
                                 CmKeyType);
  if (NewKey == NULL)
    {
      return STATUS_UNSUCCESSFUL;
    }
  RootKey->Flags = 0;
  KeQuerySystemTime(&RootKey->LastWriteTime);
  RootKey->TitleIndex = 0;
  RootKey->NumSubKeys = 0;
  RootKey->MaxSubNameLength = 0;
  RootKey->MaxSubClassLength = 0;
  RootKey->SubKeys = NULL;
  RootKey->NumValues = 0;
  RootKey->MaxValueNameLength = 0;
  RootKey->MaxValueDataLength = 0;
  RootKey->Values = NULL;
  RootKey->Name = ExAllocatePool(NonPagedPool, 2);
  wstrcpy(RootKey->Name, "\\");
  RootKey->Class = NULL;
  RootKey->NextKey = NULL;

  /* FIXME: Create initial predefined symbolic links */
  /* HKEY_LOCAL_MACHINE */
  /* HKEY_USERS */
#endif
}

NTSTATUS 
NtCreateKey(PHANDLE KeyHandle, 
            ACCESS_MASK DesiredAccess,
            POBJECT_ATTRIBUTES ObjectAttributes, 
            ULONG TitleIndex,
            PUNICODE_STRING Class, 
            ULONG CreateOptions, 
            PULONG Disposition)
{
  return ZwCreateKey(KeyHandle, 
                     DesiredAccess,
                     ObjectAttributes, 
                     TitleIndex,
                     Class, 
                     CreateOptions,
                     Disposition);
}

NTSTATUS 
ZwCreateKey(PHANDLE KeyHandle, 
            ACCESS_MASK DesiredAccess,
            POBJECT_ATTRIBUTES ObjectAttributes, 
            ULONG TitleIndex,
            PUNICODE_STRING Class, 
            ULONG CreateOptions,
            PULONG Disposition)
{
#if 0
  /* FIXME: Should CurLevel be alloced to handle arbitrary size components? */
  WCHAR *S, *T, CurLevel[255];
  PKEY_OBJECT ParentKey, CurSubKey, NewKey;
  
  assert(ObjectAttributes != NULL);

  /* FIXME: Verify ObjectAttributes is in \\Registry space */
  if (ObjectAttributes->RootDirectory == NULL)
    {
      S = ObjectAttributes->ObjectName;
      if (wstrncmp(S, "\\Registry", 9))
        {
          return STATUS_UNSUCCESSFUL;
        }
      ParentKey = RootKey;

      /*  Get remainder of full key path after removal of \\Registry */
      S += 9;
      if (S[0] != '\\')
        {
          return STATUS_UNSUCCESSFUL;
        }
      S++;

      /*  Walk through key path and fail if any component does not exist */
      while ((T = wstrchr(S, '\\')) != NULL)
        {
          /*  Move Key Object pointer to first child  */
          ParentKey = ParentKey->SubKeys;
          
          /*  Extract the next path component from requested path  */
          wstrncpy(CurLevel, S, T-S);
          CurLevel[T-S] = 0;
          DPRINT("CurLevel:[%w]", CurLevel);
          
          /*  Walk through children looking for path component  */
          while (ParentKey != NULL)
            {
              if (wstrcmp(CurLevel, ParentKey->Name) == 0)
                {
                  break;
                }
              ParentKey = ParentKey->NextKey;
            }
          
          /*  Fail if path component was not one of the children  */
          if (ParentKey == NULL)
            {
              return STATUS_UNSUCCESSFUL;
            }
          
          /*  Advance path string pointer to next component  */
          S = wstrchr(S, '\\') + 1;
        }

      /*  Check for existance of subkey , return if it exists */
      CurSubKey = ParentKey->SubKeys;
      while (CurSubKey != NULL && wstrcmp(S, CurSubKey->Name) != 0)
        {
          CurSubKey = CurSubKey->NextKey;
        }
      if (CurSubKey != NULL)
        {
          /* FIXME: Fail if key is marked for deletion  */
          *Disposition = REG_KEY_ALREADY_EXISTS;
          *KeyHandle = ObInsertHandle(KeGetCurrentProcess(),
                                      HEADER_TO_BODY(CurSubKey),
                                      DesiredAccess,
                                      FALSE);

          return STATUS_SUCCESS;
        }
      else
        {
          /*  If KeyHandle is not the parent key, or is not open with */
          /*  KEY_CREATE_SUB_KEY permission, then fail */
          KeyHandleRep = ObTranslateHandle(KeGetCurrentProcess(), KeyHandle);
          if (KeyHandleRep == NULL ||
              KeyHandleRep->ObjectBody != ParentKey ||
              (KeyHandleRep->GrantedAccess & KEY_CREATE_SUB_KEY) == 0)
            {
              return STATUS_UNSUCCESSFUL;
            }
          
          /*  Build new CmKeyType object */
          NewKey = ObGenericCreateObject(KeyHandle, 
                                         DesiredAccess, 
                                         NULL, 
                                         CmKeyType);
          if (NewKey == NULL)
            {
              return STATUS_UNSUCCESSFUL;
            }
          NewKey->Flags = 0;
          KeQuerySystemTime(&NewKey->LastWriteTime);
          NewKey->TitleIndex = 0;
          NewKey->NumSubKeys = 0;
          NewKey->MaxSubNameLength = 0;
          NewKey->MaxSubClassLength = 0;
          NewKey->SubKeys = NULL;
          NewKey->NumValues = 0;
          NewKey->MaxValueNameLength = 0;
          NewKey->MaxValueDataLength = 0;
          NewKey->Values = NULL;
          NewKey->Name = ExAllocatePool(NonPagedPool, 
                                        (wstrlen(S) + 1) * sizeof(WCHAR));
          wstrcpy(NewKey->Name, S);
          if (Class != NULL)
            {
              NewKey->Class = ExAllocatePool(NonPagedPool, 
                                             (wstrlen(Class) + 1) * 
                                               sizeof(WCHAR));
              wstrcpy(NewKey->Class, Class);
            }
          else
            {
              NewKey->Class = NULL;
            }
          NewKey->NextKey = NULL;
          
          /*  Add to end of parent key subkey list */
          if (ParentKey->SubKeys == NULL)
            {
              ParentKey->SubKeys = NewKey;
            }
          else
            {
              CurSubKey = ParentKey->SubKeys;
              while (CurSubKey->NextKey != NULL)
                {
                  CurSubKey = CurSubKey->NextKey;
                }
              NewKey->TitleIndex = CurSubKey->TitleIndex + 1;
              CurSubKey->NextKey = NewKey;
            }
          
          /*  Increment parent key subkey count and set parent subkey maxes */
          ParentKey->NumSubKeys++;
          if (ParentKey->MaxSubNameLength < wstrlen(NewKey->Name))
            {
              ParentKey->MaxSubNameLength = wstrlen(NewKey->Name);
            }
          if (NewKey->Class != NULL &&
              ParentKey->MaxSubClassLength < wstrlen(NewKey->Class))
            {
              ParentKey->MaxSubClassLength = wstrlen(NewKey->Class);
            }
          
          return STATUS_SUCCESS;
        }
    }
  else
    {
      return STATUS_UNSUCCESSFUL;
    }
#endif
  
  UNIMPLEMENTED;
}

NTSTATUS NtDeleteKey(HANDLE KeyHandle)
{
}

NTSTATUS ZwDeleteKey(HANDLE KeyHandle)
{
   UNIMPLEMENTED;
}

NTSTATUS NtEnumerateKey(HANDLE KeyHandle, ULONG Index, 
			KEY_INFORMATION_CLASS KeyInformationClass,
			PVOID KeyInformation,
			ULONG Length,
			PULONG ResultLength)
{
}

NTSTATUS ZwEnumerateKey(HANDLE KeyHandle, ULONG Index, 
			KEY_INFORMATION_CLASS KeyInformationClass,
			PVOID KeyInformation,
			ULONG Length,
			PULONG ResultLength)
{
   UNIMPLEMENTED;
}

NTSTATUS NtEnumerateValueKey(HANDLE KeyHandle, ULONG Index, 
			     KEY_VALUE_INFORMATION_CLASS KeyInformationClass,
			     PVOID KeyInformation,
			     ULONG Length,
			     PULONG ResultLength)
{
  return ZwEnumerateValueKey(KeyHandle, 
                             Index, 
			     KeyInformationClass,
			     KeyInformation,
			     Length,
			     ResultLength);
}

NTSTATUS ZwEnumerateValueKey(HANDLE KeyHandle, ULONG Index, 
			     KEY_VALUE_INFORMATION_CLASS KeyInformationClass,
			     PVOID KeyInformation,
			     ULONG Length,
			     PULONG ResultLength)
{
   UNIMPLEMENTED;
}

NTSTATUS NtFlushKey(HANDLE KeyHandle)
{
  return ZwFlushKey(KeyHandle);
}

NTSTATUS ZwFlushKey(HANDLE KeyHandle)
{
   UNIMPLEMENTED;
}

NTSTATUS NtOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess,
		   POBJECT_ATTRIBUTES ObjectAttributes)
{
  return ZwOpenKey(KeyHandle, 
                   DesiredAccess,
		   ObjectAttributes);
}

NTSTATUS ZwOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess,
		   POBJECT_ATTRIBUTES ObjectAttributes)
{
   UNIMPLEMENTED;
}

NTSTATUS NtQueryKey(HANDLE KeyHandle, 
		    KEY_INFORMATION_CLASS KeyInformationClass,
		    PVOID KeyInformation,
		    ULONG Length,
		    PULONG ResultLength)
{
  return ZwQueryKey(KeyHandle, 
		    KeyInformationClass,
		    KeyInformation,
		    Length,
		    ResultLength);
}

NTSTATUS ZwQueryKey(HANDLE KeyHandle, 
		    KEY_INFORMATION_CLASS KeyInformationClass,
		    PVOID KeyInformation,
		    ULONG Length,
		    PULONG ResultLength)
{
   UNIMPLEMENTED;
}

NTSTATUS NtQueryValueKey(HANDLE KeyHandle,
		    PUNICODE_STRING ValueName,
		    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
		    PVOID KeyValueInformation,
		    ULONG Length,
		    PULONG ResultLength)
{
  return ZwQueryValueKey(KeyHandle,
		         ValueName,
		         KeyValueInformationClass,
		         KeyValueInformation,
		         Length,
		         ResultLength);
}

NTSTATUS ZwQueryValueKey(HANDLE KeyHandle,
		    PUNICODE_STRING ValueName,
		    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
		    PVOID KeyValueInformation,
		    ULONG Length,
		    PULONG ResultLength)
{
   UNIMPLEMENTED;
}

NTSTATUS NtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName,
		       ULONG TitleIndex, ULONG Type, PVOID Data,
		       ULONG DataSize)
{
  return ZwSetValueKey(KeyHandle, 
                       ValueName,
		       TitleIndex, 
                       Type, 
                       Data,
		       DataSize);
}

NTSTATUS ZwSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName,
		       ULONG TitleIndex, ULONG Type, PVOID Data,
		       ULONG DataSize)
{
   UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtDeleteValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName
	)
{
  return ZwDeleteValueKey(KeyHandle,
                          ValueName);
}

NTSTATUS
STDCALL
ZwDeleteValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName	
		 )
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL 
NtLoadKey(
	PHANDLE KeyHandle,
	OBJECT_ATTRIBUTES ObjectAttributes
	)
{
  return ZwLoadKey(KeyHandle,
                   ObjectAttributes);
}

NTSTATUS
STDCALL 
ZwLoadKey(
	PHANDLE KeyHandle,
	OBJECT_ATTRIBUTES ObjectAttributes
	)
{
  UNIMPLEMENTED;
}

NTSTATUS STDCALL NtLoadKey2(VOID)
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtNotifyChangeKey(
	IN HANDLE KeyHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchroneous, 
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree
	)
{
  return ZwNotifyChangeKey(KeyHandle,
                           Event,
                           ApcRoutine, 
                           ApcContext, 
                           IoStatusBlock,
                           CompletionFilter,
                           Asynchroneous, 
                           ChangeBuffer,
                           Length,
                           WatchSubtree);
}

NTSTATUS
STDCALL
ZwNotifyChangeKey(
	IN HANDLE KeyHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchroneous, 
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree
	)
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtQueryMultipleValueKey(
   HANDLE KeyHandle,	
   PVALENT ListOfValuesToQuery,	
   ULONG NumberOfItems,	
   PVOID MultipleValueInformation,		
   ULONG Length,
   PULONG  ReturnLength
)
{
  return ZwQueryMultipleValueKey(KeyHandle,	
                                 ListOfValuesToQuery,	
                                 NumberOfItems,	
                                 MultipleValueInformation,		
                                 Length,
                                 ReturnLength);
}

NTSTATUS
STDCALL
ZwQueryMultipleValueKey(
   HANDLE KeyHandle,	
   PVALENT ListOfValuesToQuery,	
   ULONG NumberOfItems,	
   PVOID MultipleValueInformation,		
   ULONG Length,
   PULONG  ReturnLength
)
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtReplaceKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes, 
	IN HANDLE Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes 
	)
{
  return ZwReplaceKey(ObjectAttributes, 
                      Key,
                      ReplacedObjectAttributes);
}

NTSTATUS
STDCALL
ZwReplaceKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes, 
	IN HANDLE Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes 
	)
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags
	)
{
  return ZwRestoreKey(KeyHandle,
                      FileHandle,
                      RestoreFlags);
}

NTSTATUS
STDCALL
ZwRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags
	)
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtSaveKey(
	IN HANDLE KeyHandle,
	IN HANDLE FileHandle
	)
{
  return ZwSaveKey(KeyHandle,
                   FileHandle);
}

NTSTATUS
STDCALL
ZwSaveKey(
	IN HANDLE KeyHandle,
	IN HANDLE FileHandle
	)
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtSetInformationKey(
	IN HANDLE KeyHandle,
	IN CINT KeyInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength
	)
{
  return ZwSetInformationKey(KeyHandle,
                             KeyInformationClass,
                             KeyInformation,
                             KeyInformationLength);
}

NTSTATUS STDCALL ZwSetInformationKey(IN HANDLE KeyHandle,
				     IN CINT KeyInformationClass,
				     IN PVOID KeyInformation,
				     IN ULONG KeyInformationLength)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtUnloadKey(HANDLE KeyHandle)
{
  return ZwUnloadKey(KeyHandle);
}

NTSTATUS STDCALL ZwUnloadKey(HANDLE KeyHandle)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtInitializeRegistry(BOOLEAN SetUpBoot)
{
  return ZwInitializeRegistry(SetUpBoot);
}

NTSTATUS STDCALL ZwInitializeRegistry(BOOLEAN SetUpBoot)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlCheckRegistryKey(ULONG RelativeTo, PWSTR Path)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlCreateRegistryKey(ULONG RelativeTo, PWSTR Path)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlDeleteRegistryValue(ULONG RelativeTo, PWSTR Path,
				PWSTR ValueName)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlQueryRegistryValues(ULONG RelativeTo,
				PWSTR Path,
				PRTL_QUERY_REGISTRY_TABLE QueryTable,
				PVOID Context,
				PVOID Environment)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlWriteRegistryValue(ULONG RelativeTo,
			       PWSTR Path,
			       PWSTR ValueName,
			       ULONG ValueType,
			       PVOID ValueData,
			       ULONG ValueLength)
{
   UNIMPLEMENTED;
}

