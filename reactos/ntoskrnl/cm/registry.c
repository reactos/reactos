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

// #define  PROTO_REG  1  /* Comment out to disable */

/* FILE STATICS *************************************************************/

#if PROTO_REG

typedef struct _KEY_OBJECT
/*
 * Type defining the Object Manager Key Object
 */
{
  CSHORT Type;
  CSHORT Size;
  
  ULONG Flags;
  WCHAR *Name;
  struct _KEY_OBJECT *NextKey;
} KEY_OBJECT, *PKEY_OBJECT;

#define  KO_MARKED_FOR_DELETE  0x00000001

static POBJECT_TYPE  CmiKeyType = NULL;
static PKEY_OBJECT  CmiKeyList = NULL;

static PVOID  CmiObjectParse(PVOID  ParsedObject, PWSTR  *Path);
#endif

/* FUNCTIONS *****************************************************************/

VOID
CmInitializeRegistry(VOID)
{
#if PROTO_REG
  UNICODE_STRING  RootKeyName;
  OBJECT_ATTRIBUTES  ObjectAttributes;
  
  /*  Initialize the Key object type  */
  CmiKeyType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  CmiKeyType->TotalObjects = 0;
  CmiKeyType->TotalHandles = 0;
  CmiKeyType->MaxObjects = ULONG_MAX;
  CmiKeyType->MaxHandles = ULONG_MAX;
  CmiKeyType->PagedPoolCharge = 0;
  CmiKeyType->NonpagedPoolCharge = sizeof(KEY_OBJECT);
  CmiKeyType->Dump = NULL;
  CmiKeyType->Open = NULL;
  CmiKeyType->Close = NULL;
  CmiKeyType->Delete = NULL;
  CmiKeyType->Parse = CmpObjectParse;
  CmiKeyType->Security = NULL;
  CmiKeyType->QueryName = NULL;
  CmiKeyType->OkayToClose = NULL;
  RtlInitUnicodeString(&CmiKeyType->TypeName, L"Key");

  /*  Build the Root Key Object  */
  /*  FIXME: This should be split into two objects, 1 system and 1 user  */
  RtlInitUnicodeString(&RootKeyName, L"\\Registry");
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  Status = ObCreateObject(&RootKeyHandle,
                          STANDARD_RIGHTS_REQUIRED,
                          &ObjectAttributes,
                          CmiKeyType);

  /* FIXME: map / build registry data  */
  /* FIXME: Create initial predefined symbolic links  */
  /* HKEY_LOCAL_MACHINE  */
  /* HKEY_USERS  */
  /* FIXME: load volatile registry data from ROSDTECT  */

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
#if PROTO_REG
  NTSTATUS  Status;
  PKEY_TYPE  CurKey;

  /* FIXME: Should CurLevel be alloced to handle arbitrary size components? */
  WCHAR *S, *T, CurLevel[255];
  PKEY_OBJECT ParentKey, CurSubKey, NewKey;
  
  assert(ObjectAttributes != NULL);

  Status = CmiBuildKeyPath(&KeyNameBuf, ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  /* FIXME: Scan the key list to see if key already open  */
  CurKey = CmiKeyList;
  while (CurKey != NULL && wcscmp(KeyPath, CurKey->Name) != 0)
    {
      CurKey = CurKey->Next;
    }
  if (CurKey != NULL)
    {
      /* FIXME: If so, return a reference to it  */
      /* FIXME: destroy KeyNameBuf before return  */
      /* FIXME:(?) we could check to see if the key still exists here...  */
    }

  /* FIXME: Call CmiCreateKey to create/open the key in the registry file  */
  Status = CmiCreateKey(KeyNameBuf, TitleIndex, Class, Disposition);

  /*  Create new key object and put into linked list  */
  NewKey = ObGenericCreateObject(KeyHandle, 
                                 DesiredAccess, 
                                 NULL, 
                                 CmiKeyType);
  if (NewKey == NULL)
    {
      return  STATUS_UNSUCCESSFUL;
    }
  NewKey->Flags = 0;
  NewKey->Name = KeyNameBuf;
  NewKey->NextKey = CmiKeyList;
  CmiKeyList = NewKey;

  return  STATUS_SUCCESS;
#endif
  
  UNIMPLEMENTED;
}

NTSTATUS NtDeleteKey(HANDLE KeyHandle)
{
   return(ZwDeleteKey(KeyHandle));
}

NTSTATUS ZwDeleteKey(HANDLE KeyHandle)
{
   UNIMPLEMENTED;
}

NTSTATUS NtEnumerateKey(HANDLE KeyHandle, 
			ULONG Index,
			KEY_INFORMATION_CLASS KeyInformationClass,
			PVOID KeyInformation,
			ULONG Length,
			PULONG ResultLength)
{
   return(ZwEnumerateKey(KeyHandle,
			 Index,
			 KeyInformationClass,
			 KeyInformation,
			 Length,
			 ResultLength));
}

NTSTATUS ZwEnumerateKey(HANDLE KeyHandle, 
			ULONG Index,
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

/*  ------------------------------------------  Private Implementation  */

#if PROTO_REG
static PVOID 
CmiObjectParse(PVOID ParsedObject, PWSTR* Path)
{
  PWSTR  S, SubKeyBuffer;
  PKEY_OBJECT  CurrentKey, ChildKey;

  UNIMPLEMENTED

  /*  If the path is an empty string, we're done  */
  if (Path == NULL || Path[0] == 0)
    {
/* FIXME: return the root key */
      return NULL;
    }

  /*  Extract subkey name from path  */
  S = *Path;
  while (*S != '\\')
    {
      S++;
    }
  SubKeyBuffer = ExAllocatePool(NonPagedPool, (S - *Path) * sizeof(WSTR));
  wstrncpy(SubKeyBuffer, *Path, (S - *Path));
  SubKeyBuffer[S - *Path] = 0;
  
  /* %%% Scan Key for matching SubKey  */
  CurrentKey = (PKEY_OBJECT) ParsedObject;
  ChildKey = CurrentKey->
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


  /* %%% If SubKey is not found return NULL  */
  /* %%% Adjust path to next level  */
  /* %%% Return object for SubKey  */

  ExFreePool(SubKeyBuffer);

}

static NTSTATUS
CmiBuildKeyPath(PWSTR  *KeyPath, POBJECT_ATTRIBUTES  ObjectAttributes)
{
  /* FIXME: Verify ObjectAttributes is in \\Registry space and comppute size for path */
  KeyNameSize = 0;
  if (ObjectAttributes->RootDirectory != NULL)
    {
      /* FIXME: determine type of object for RootDirectory  */
      /* FIXME: if object type is ObDirType  */
        /* FIXME: fail if RootDirectory != '\\'  */
        /* FIXME: check for 'Registry' in ObjectName, fail if missing  */
        /* FIXME: add size for remainder to KeyNameSize  */
      /* FIXME: else if object type is CmiKeyType  */
        /* FIXME: add size of Name from RootDirectory object to KeyNameSize  */
        /* FIXME: add 1 to KeyNamesize for '\\'  */
        /* FIXME: add size of ObjectName to KeyNameSize  */
      /* FIXME: else fail on incorrect type  */
    }
  else
    {
      /* FIXME: check for \\Registry and fail if missing  */
      /* FIXME: add size of remainder to KeyNameSize  */
    }

  KeyNameBuf = ExAllocatePool(NonPagedPool, KeyNameSize);

  /* FIXME: Construct relative pathname  */
  KeyNameBuf[0] = 0;
  if (ObjectAttributes->RootDirectory != NULL)
    {
      /* FIXME: determine type of object for RootDirectory  */
      /* FIXME: if object type is ObDirType  */
        /* FIXME: fail if RootDirectory != '\\'  */
        /* FIXME: check for 'Registry' in ObjectName, fail if missing  */
        /* FIXME: copy remainder into KeyNameBuf  */
      /* FIXME: else if object type is CmiKeyType  */
        /* FIXME: copy Name from RootDirectory object into KeyNameBuf  */
        /* FIXME: append '\\' into KeyNameBuf  */
        /* FIXME: append ObjectName into KeyNameBuf  */
    }
  else
    {
      /* FIXME: check for \\Registry\\ and fail if missing  */
      /* FIXME: copy remainder into KeyNameBuf  */
    }

  *KeyPath = KeyNameBuf;
  return  STATUS_SUCCESS;
}

static NTSTATUS
CmiCreateKey(PWSTR  KeyNameBuf,
             ULONG TitleIndex,
             PUNICODE_STRING Class, 
             PULONG Disposition)
{

}
#endif


