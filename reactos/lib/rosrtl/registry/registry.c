#define NTOS_MODE_USER
#include <ntos.h>
#include <rosrtl/string.h>

/*
 * Utility function to read a value from the registry more easily.
 *
 * IN  PUNICODE_STRING KeyName       -> Name of key to open
 * IN  PUNICODE_STRING ValueName     -> Name of value to open
 * OUT PUNICODE_STRING ReturnedValue -> String contained in registry
 *
 * Returns NTSTATUS
 */

NTSTATUS NTAPI RosReadRegistryValue( PUNICODE_STRING KeyName,
				     PUNICODE_STRING ValueName,
				     PUNICODE_STRING ReturnedValue ) {
    NTSTATUS Status;
    HANDLE KeyHandle;
    OBJECT_ATTRIBUTES KeyAttributes;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInfo;
    ULONG Length = 0;
    ULONG ResLength = 0;
    UNICODE_STRING Temp;
    
    InitializeObjectAttributes(&KeyAttributes, KeyName, OBJ_CASE_INSENSITIVE,
			       NULL, NULL);
    Status = ZwOpenKey(&KeyHandle, KEY_ALL_ACCESS, &KeyAttributes);
    if( !NT_SUCCESS(Status) ) {
	return Status;
    }
    
    Status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation,
			     0,
			     0,
			     &ResLength);
    
    if( Status != STATUS_BUFFER_TOO_SMALL ) {
	NtClose(KeyHandle);
	return Status;
    }
    
    ResLength += sizeof( *KeyValuePartialInfo );
    KeyValuePartialInfo = 
	RtlAllocateHeap(GetProcessHeap(), 0, ResLength);
    Length = ResLength;
    
    if( !KeyValuePartialInfo ) {
	NtClose(KeyHandle);
	return STATUS_NO_MEMORY;
    }
    
    Status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation,
			     (PVOID)KeyValuePartialInfo,
			     Length,
			     &ResLength);
    
    if( !NT_SUCCESS(Status) ) {
	NtClose(KeyHandle);
	RtlFreeHeap(GetProcessHeap(),0,KeyValuePartialInfo);
	return Status;
    }
    
    Temp.Length = Temp.MaximumLength = KeyValuePartialInfo->DataLength;
    Temp.Buffer = (PWCHAR)KeyValuePartialInfo->Data;
    
    /* At this point, KeyValuePartialInfo->Data contains the key data */
    RtlInitUnicodeString(ReturnedValue,L"");
    RosAppendUnicodeString(ReturnedValue,&Temp,FALSE);
    
    RtlFreeHeap(GetProcessHeap(),0,KeyValuePartialInfo);
    NtClose(KeyHandle);
    
    return Status;
}

