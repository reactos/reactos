/*++

Copyright (c) 1991  Microsoft Corporation


Module Name:

    registry.c

Abstract:

    (This file has been copied from the temporary hack that BryanWi and
    ScottBi did in kernel mode.  I saw no need to have it be in kernel
    mode and it had many bugs caused as a result of being in kernel mode,
    so I made it caller mode.  Jim Kelly).



   This module represents a quick and dirty Nt level registry.  Each key
   in the Registry is implemented as a file directory within a directory
   tree whose root is the directory "\Registry" on the system disk.
   A key's data is stored within a file called  "Data.Reg" in the key's
   directory, and a key's attributes is stored as the file "Attr.Reg"
   within the directory.






Author:

    Bryan M. Willman (bryanwi) 30-Apr-1991
    Scott Birrell (ScottBi) 6-Jun-1991

Environment:

    callable from Kernel or user mode.

Revision History:

--*/

#include "ntrtlp.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlpNtOpenKey)
#pragma alloc_text(PAGE,RtlpNtCreateKey)
#pragma alloc_text(PAGE,RtlpNtQueryValueKey)
#pragma alloc_text(PAGE,RtlpNtSetValueKey)
#pragma alloc_text(PAGE,RtlpNtMakeTemporaryKey)
#pragma alloc_text(PAGE,RtlpNtEnumerateSubKey)
#endif

#define REG_INVALID_ATTRIBUTES (OBJ_EXCLUSIVE | OBJ_PERMANENT)



//
// Temporary Registry User APIs.
//
// NOTE:  These are temporary implementations.  Although there is no code
// within that requires these API to be implemented as system services, the
// eventual replacements for these routines will use the Object Manager and
// hence require to be system services.
//


NTSTATUS
RtlpNtOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG Options
    )

/*++

Routine Description:

    This function opens a key in the Registry.  The key must already exist.

Arguments:

    KeyHandle - Receives a value called a Handle which is used to access
        the specified key in the Registration Database.

    DesiredAccess - Specifies the Accesses desired

        REG_KEY_READ - Generic Read access to key
          REG_KEY_QUERY_VALUE - Query Key's value
        REG_KEY_WRITE - Generic Write access to key
          REG_KEY_SET_VALUE - Set Key's value

    ObjectAttributes - Specifies the attributes of the key being opened.
        Note that a key name must be specified.  If a Root Directory
        is specified, the name is relative to the root.  The name of the
        object must be within the name space allocated to the Registry, that
        is, all names beginning "\Registry".  RootHandle, if present, must
        be a handle to "\", or "\Registry", or a key under "\Registry".

    Options - REG_OPTION_READ_FUZZY - Allow Read access on handle even if
        it is open for Read/Write access.

Return Value:

    NTSTATUS - Result code from call.  The following are returned

        STATUS_SUCCESS - The open was successful.

        STATUS_INVALID_PARAMETER - A parameter other that object name was
            invalid.

        STATUS_OBJECT_NAME_INVALID - The key name has invalid syntax

        STATUS_OBJECT_NAME_NOT_FOUND - No key of the given name exists

        STATUS_ACCESS_DENIED - Caller does not have the requested access
            to the specified key.
--*/

{
    RTL_PAGED_CODE();

    if (ARGUMENT_PRESENT(ObjectAttributes)) {
        ObjectAttributes->Attributes &= ~(REG_INVALID_ATTRIBUTES);
    }

    return( NtOpenKey( KeyHandle,
                       DesiredAccess,
                       ObjectAttributes
                       ) );

    DBG_UNREFERENCED_PARAMETER( Options );
}


NTSTATUS
RtlpNtCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG Options,
    IN PUNICODE_STRING Provider,
    OUT OPTIONAL PULONG Disposition
    )

/*++

Routine Description:

    This function creates or opens the specified key in the Registry.  If
    the key does not exist, it is created.  If the key already exists, it
    is opened.

Arguments:

    KeyHandle - Receives a value called a Handle which is used to access
        the specified key in the Registration Database.

    DesiredAccess - Specifies the Accesses desired

        REG_KEY_READ - Generic Read access to key
          REG_KEY_QUERY_VALUE - Query Key's value
        REG_KEY_WRITE - Generic Write access to key
          REG_KEY_SET_VALUE - Set Key's value

    ObjectAttributes - Specifies the attributes of the key being opened.
        Note that a key name must be specified.  If a Root Directory
        is specified, the name is relative to the root.  The name of the
        object must be within the name space allocated to the Registry, that
        is, all names beginning "\Registry".  RootHandle, if present, must
        be a handle to "\", or "\Registry", or a key under "\Registry".


    Options - REG_OPTION_READ_FUZZY - Allow Read access on handle even if it is
                                      open for READ_WRITE access.

              REG_OPTION_VOLATILE - Object is not to be stored across boots.

    Provider - This parameter is reserved for future use and must currently
        be set to NULL.  It will be used in the future to specify the name of
        the provider to be used for operations on this node and its descendant
        nodes.

    Disposition - This optional parameter is a pointer to a variable that
        will receive a value indicating whether a new Registry key was
        created or an existing one opened.

        REG_CREATED_NEW_KEY - A new Registry Key was created
        REG_OPENED_EXISTING_KEY - An existing Registry Key was opened

Return Value:

    NTSTATUS - Result code from call.  The following are returned

        STATUS_SUCCESS - The open was successful.

        STATUS_INVALID_PARAMETER - A parameter other that object name was
--*/

{
    RTL_PAGED_CODE();

    if (ARGUMENT_PRESENT(ObjectAttributes)) {
        ObjectAttributes->Attributes &= ~(REG_INVALID_ATTRIBUTES);
    }


    return(NtCreateKey( KeyHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        0,                          //TitleIndex
                        NULL,                       //Class OPTIONAL,
                        REG_OPTION_NON_VOLATILE,    //CreateOptions,
                        Disposition
                        ) );

    DBG_UNREFERENCED_PARAMETER( Options );
    DBG_UNREFERENCED_PARAMETER( Provider );
}



NTSTATUS
RtlpNtQueryValueKey(
    IN HANDLE KeyHandle,
    OUT OPTIONAL PULONG KeyValueType,
    OUT OPTIONAL PVOID KeyValue,
    IN OUT OPTIONAL PULONG KeyValueLength,
    OUT OPTIONAL PLARGE_INTEGER LastWriteTime
    )

/*++

Routine Description:

    This function queries the value of a key.

Arguments:

    KeyHandle - Handle of a key opened for GENERIC_READ access via NtOpenKey.

    KeyValueType - Optional pointer to variable that will receive the
        client-defined type of the key value (if any).  If no value has been
        set for the key, 0 is returned.

    KeyValue - Optional pointer to buffer in which part or all of the key's
        value (as set on the most recent call to NtSetValueKey) will be
        returned.  If the key's value is too large to fit into the supplied
        buffer, as much of the value as will fit into the buffer will be
        returned and the warning STATUS_BUFFER_OVERFLOW is returned.  If no
        value has ever been set, nothing is returned.  If NULL is specified
        for this parameter, no Key Value is returned.

    KeyValueLength - On input, this optional parameter points to a variable
        that contains the length in bytes of the KeyValue buffer (if any).  If
        no KeyValue buffer is specified, the variable content on entry is
        ignored.  On return, the referenced variable (if any) receives the
        FULL length in bytes of the key value.  If the key's value is too
        large to fit into the supplied buffer, as much of the value as will
        fit into the buffer will be returned and the warning
        STATUS_BUFFER_OVERFLOW is returned.

        The returned length is intended for use by calling code in allocating
        a buffer of sufficient size to hold the key's value.  After receiving
        STATUS_BUFFER_OVERFLOW from NtQueryValueKey, calling code may make a
        subsequent call to NtQueryValueKey with a buffer of size equal to the
        length returned by the prior call.

        If no value has been set for the key, 0 is returned.

    LastWriteTime - Optional parameter to variable which receives a time stamp
        specifying the last time that the key was written.

Return Value:

    NTSTATUS - Result code

        STATUS_SUCCESS - Call was successful

        STATUS_INVALID_PARAMETER - Invalid parameter

        STATUS_ACCESS_DENIED - Caller does not have GENERIC_READ access to
            the specified key

        STATUS_BUFFER_OVERFLOW - This is a warning that the key's value
            is too large for the buffer specified by the KeyValue and
            KeyValueLength parameters.  Use the length returned to
            determine the size of buffer to allocate for a subsequent
            call of NtQueryValueKey.

--*/

{

    UNICODE_STRING NullName;
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInformation;
    ULONG ValueLength;

    RTL_PAGED_CODE();

    //
    // Compute the size of the buffer needed to hold the key value information.
    //

    ValueLength = 0;
    if (ARGUMENT_PRESENT(KeyValueLength)) {
        ValueLength = *KeyValueLength;
    }

    ValueLength += FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
    ValueInformation = RtlAllocateHeap(RtlProcessHeap(), 0, ValueLength);
    if (ValueInformation == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query the key value.
    //

    NullName.Length = 0;
    Status = NtQueryValueKey(KeyHandle,
                             &NullName,
                             KeyValuePartialInformation,
                             ValueInformation,
                             ValueLength,
                             &ValueLength);

    //
    // Temporary hack to allow query of "" attribute when it hasn't
    // yet been set.
    //

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = STATUS_SUCCESS;
        ValueInformation->DataLength = 0;
        ValueInformation->Type = 0;
    }

    //
    // If requested return the key value length and the key type.
    //

    if (NT_SUCCESS(Status) || (Status == STATUS_BUFFER_OVERFLOW)) {
        if (ARGUMENT_PRESENT(KeyValueLength)) {
            *KeyValueLength = ValueInformation->DataLength;
        }

        if (ARGUMENT_PRESENT(KeyValueType)) {
            *KeyValueType = ValueInformation->Type;
        }
    }

    //
    // If the query was successful and buffer overflow did not occur, then
    // return the key value information.
    //

    if (NT_SUCCESS(Status) && ARGUMENT_PRESENT(KeyValue)) {
        RtlMoveMemory(KeyValue,
                      &ValueInformation->Data[0],
                      ValueInformation->DataLength);
    }

    RtlFreeHeap(RtlProcessHeap(), 0, ValueInformation);
    return Status;
}

NTSTATUS
RtlpNtSetValueKey(
    IN HANDLE KeyHandle,
    IN ULONG KeyValueType,
    IN OPTIONAL PVOID KeyValue,
    IN ULONG KeyValueLength
    )

/*++

Routine Description:

    This function sets the type and value of a key.

Arguments:

    KeyHandle - Specifies a handle of the key whose type and value are to
        be set.  The key must have been opened with GENERIC_WRITE access.

    KeyValueType - This is a value that the client of the registry defines to
        distinguish different client-defined types of data value stored
        with the key.  When setting the value of a key that has previously
        had a Type and Value stored, the Type may be changed.

    KeyValue - Optional pointer to the data to be optionally stored as the
        value of the key.  If NULL is specified for this parameter, only
        the value type will be written.

    KeyValueLength - Specifies the length in bytes of the data to be stored as
        the key's value.  A zero value indicates that no data is being stored:
        if zero is specified, the Value parameter will be ignored.

Return Value:

    NTSTATUS - Result code.  The following values are returned

        STATUS_SUCCESS - The call was successful

        STATUS_INVALID_PARAMETER - Invalid Parameter(s)
--*/

{
    UNICODE_STRING NullName;
    NullName.Length = 0;

    RTL_PAGED_CODE();

    return( NtSetValueKey( KeyHandle,
                           &NullName,       // ValueName
                           0,               // TitleIndex
                           KeyValueType,
                           KeyValue,
                           KeyValueLength
                           ) );
}



NTSTATUS
RtlpNtMakeTemporaryKey(
    IN HANDLE KeyHandle
    )

/*++

Routine Description:

    This function makes a Registry key temporary.  The key will be deleted
    when the last handle to it is closed.

Arguments:

    KeyHandle - Specifies the handle of the Key.  This is also the handle
        of the key's directory.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_HANDLE - The specified handle is invalid.

        STATUS_ACCESS_DENIED - The specified handle does not specify delet
            access.

--*/

{
    RTL_PAGED_CODE();

    return( NtDeleteKey(KeyHandle) );
}


NTSTATUS
RtlpNtEnumerateSubKey(
    IN HANDLE KeyHandle,
    OUT PUNICODE_STRING SubKeyName,
    IN ULONG Index,
    OUT PLARGE_INTEGER LastWriteTime
    )

/*++

Routine Description:

    This function finds the name of the next sub key of a given key.  By
    making successive calls, all of the sub keys of a key can be determined.


Arguments:

    KeyHandle - Handle of the key whose sub keys are to be enumerated.

    SubKeyName - Pointer to a Unicode String in which the name of the sub
        key will be returned.

    Index - Specifies the (ZERO-based) number of the sub key to be returned.


    LastWriteTime - Receives the time stamp that specifies when the key
        was last written.

Return Value:

    NTSTATUS - Result code

        STATUS_SUCCESS - The call succeeded

        STATUS_INVALID_PARAMETER - Invalid parameter

        STATUS_NO_MORE_ENTRIES - There is no key having the specified index

        STATUS_BUFFER_OVERFLOW - The buffer of the output string was not
            large enough to hold the next sub-key name. SubKeyName->Length
            contains the number of bytes required.

        STATUS_NO_MEMORY - There was not sufficient heap to perform the
            requested operation.

--*/

{
    NTSTATUS Status;
    PKEY_BASIC_INFORMATION KeyInformation = NULL;
    ULONG LocalBufferLength, ResultLength;

    RTL_PAGED_CODE();

    LocalBufferLength = 0;
    if (SubKeyName->MaximumLength > 0) {

        LocalBufferLength = SubKeyName->MaximumLength +
                            FIELD_OFFSET(KEY_BASIC_INFORMATION, Name);
        KeyInformation = RtlAllocateHeap( RtlProcessHeap(), 0,
                                          LocalBufferLength
                                          );
        if (KeyInformation == NULL) {
            return(STATUS_NO_MEMORY);
        }
    }

    Status = NtEnumerateKey( KeyHandle,
                             Index,
                             KeyBasicInformation,    //KeyInformationClass
                             (PVOID)KeyInformation,
                             LocalBufferLength,
                             &ResultLength
                             );

    if (NT_SUCCESS(Status)) {

        if ( SubKeyName->MaximumLength >= KeyInformation->NameLength) {

            SubKeyName->Length = (USHORT)KeyInformation->NameLength;

            RtlMoveMemory( SubKeyName->Buffer,
                           &KeyInformation->Name[0],
                           SubKeyName->Length
                           );
        } else {
            Status = STATUS_BUFFER_OVERFLOW;
        }
    }

    //
    // Return the length required if we failed due to a small buffer
    //

    if (Status == STATUS_BUFFER_OVERFLOW) {
        SubKeyName->Length = (USHORT)(ResultLength -
                                      FIELD_OFFSET(KEY_BASIC_INFORMATION, Name));
    }


    //
    // Free up any memory we allocated
    //

    if (KeyInformation != NULL) {

        RtlFreeHeap( RtlProcessHeap(), 0,
                     KeyInformation
                     );
    }


    return(Status);

    DBG_UNREFERENCED_PARAMETER( LastWriteTime );

}
