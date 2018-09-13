/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    regutil.c

Abstract:

    This file contains support routines for accessing the registry.

Author:

    Steve Wood (stevewo) 15-Apr-1992

Revision History:

--*/

#include "ntrtlp.h"
#include <ctype.h>

NTSTATUS
RtlpGetRegistryHandle(
    IN ULONG RelativeTo,
    IN PCWSTR KeyName,
    IN BOOLEAN WriteAccess,
    OUT PHANDLE Key
    );

NTSTATUS
RtlpQueryRegistryDirect(
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN OUT PVOID Destination
    );

NTSTATUS
RtlpCallQueryRegistryRoutine(
    IN PRTL_QUERY_REGISTRY_TABLE QueryTable,
    IN PKEY_VALUE_FULL_INFORMATION KeyValueInformation,
    IN OUT PULONG PKeyValueInfoLength,
    IN PVOID Context,
    IN PVOID Environment OPTIONAL
    );

PVOID
RtlpAllocDeallocQueryBuffer(
   IN OUT SIZE_T    *PAllocLength            OPTIONAL,
   IN     PVOID      OldKeyValueInformation  OPTIONAL,
   IN     SIZE_T     OldAllocLength          OPTIONAL,
      OUT NTSTATUS  *pStatus                 OPTIONAL
    );

NTSTATUS
RtlpInitCurrentUserString(
    OUT PUNICODE_STRING UserString
    );


NTSTATUS
RtlpGetTimeZoneInfoHandle(
    IN BOOLEAN WriteAccess,
    OUT PHANDLE Key
    );

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlpGetRegistryHandle)
#pragma alloc_text(PAGE,RtlpQueryRegistryDirect)
#pragma alloc_text(PAGE,RtlpCallQueryRegistryRoutine)
#pragma alloc_text(PAGE,RtlpAllocDeallocQueryBuffer)
#pragma alloc_text(PAGE,RtlQueryRegistryValues)
#pragma alloc_text(PAGE,RtlWriteRegistryValue)
#pragma alloc_text(PAGE,RtlCheckRegistryKey)
#pragma alloc_text(PAGE,RtlCreateRegistryKey)
#pragma alloc_text(PAGE,RtlDeleteRegistryValue)
#pragma alloc_text(PAGE,RtlExpandEnvironmentStrings_U)
#pragma alloc_text(PAGE,RtlGetNtGlobalFlags)
#pragma alloc_text(PAGE,RtlpInitCurrentUserString)
#pragma alloc_text(PAGE,RtlOpenCurrentUser)
#pragma alloc_text(PAGE,RtlpGetTimeZoneInfoHandle)
#pragma alloc_text(PAGE,RtlQueryTimeZoneInformation)
#pragma alloc_text(PAGE,RtlSetTimeZoneInformation)
#pragma alloc_text(PAGE,RtlSetActiveTimeBias)
#endif

extern  const PWSTR RtlpRegistryPaths[ RTL_REGISTRY_MAXIMUM ];

NTSTATUS
RtlpGetRegistryHandle(
    IN ULONG RelativeTo,
    IN PCWSTR KeyName,
    IN BOOLEAN WriteAccess,
    OUT PHANDLE Key
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR KeyPathBuffer[ MAXIMUM_FILENAME_LENGTH+6 ];
    UNICODE_STRING KeyPath;
    UNICODE_STRING CurrentUserKeyPath;
    BOOLEAN OptionalPath;

    if (RelativeTo & RTL_REGISTRY_HANDLE) {
        *Key = (HANDLE)KeyName;
        return STATUS_SUCCESS;
    }

    if (RelativeTo & RTL_REGISTRY_OPTIONAL) {
        RelativeTo &= ~RTL_REGISTRY_OPTIONAL;
        OptionalPath = TRUE;
    } else {
        OptionalPath = FALSE;
    }

    if (RelativeTo >= RTL_REGISTRY_MAXIMUM) {
        return STATUS_INVALID_PARAMETER;
    }

    KeyPath.Buffer = KeyPathBuffer;
    KeyPath.Length = 0;
    KeyPath.MaximumLength = sizeof( KeyPathBuffer );
    if (RelativeTo != RTL_REGISTRY_ABSOLUTE) {
        if (RelativeTo == RTL_REGISTRY_USER &&
            NT_SUCCESS( RtlFormatCurrentUserKeyPath( &CurrentUserKeyPath ) )
           ) {
            Status = RtlAppendUnicodeStringToString( &KeyPath, &CurrentUserKeyPath );
            RtlFreeUnicodeString( &CurrentUserKeyPath );
        } else {
            Status = RtlAppendUnicodeToString( &KeyPath, RtlpRegistryPaths[ RelativeTo ] );
        }

        if (!NT_SUCCESS( Status )) {
            return Status;
        }

        Status = RtlAppendUnicodeToString( &KeyPath, L"\\" );
        if (!NT_SUCCESS( Status )) {
            return Status;
        }
    }

    Status = RtlAppendUnicodeToString( &KeyPath, KeyName );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }


    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
#ifdef NTOS_KERNEL_RUNTIME
    //
    // Use a kernel-mode handle for the registry key to prevent
    // malicious apps from hijacking it.
    //
    ObjectAttributes.Attributes |= OBJ_KERNEL_HANDLE;
#endif
    if (WriteAccess) {
        Status = ZwCreateKey( Key,
                              GENERIC_WRITE,
                              &ObjectAttributes,
                              0,
                              (PUNICODE_STRING) NULL,
                              0,
                              NULL
                            );
    } else {
        Status = ZwOpenKey( Key,
                            MAXIMUM_ALLOWED | GENERIC_READ,
                            &ObjectAttributes
                          );
    }

    return Status;
}

//
// This is the maximum MaximumLength for a UNICODE_STRING.
//
#define MAX_USTRING ( sizeof(WCHAR) * (MAXUSHORT/sizeof(WCHAR)) )

//
// This is the maximum MaximumLength for a UNICODE_STRING that still leaves
// room for a UNICODE_NULL.
//
#define MAX_NONNULL_USTRING ( MAX_USTRING - sizeof(UNICODE_NULL) )

//
// Return a registry value for RTL_QUERY_REGISTRY_DIRECT.
// For string values, ValueLength includes the UNICODE_NULL.
// Truncate string values if they don't fit within a UNICODE_STRING.
//
NTSTATUS
RtlpQueryRegistryDirect(
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN OUT PVOID Destination
    )
{

    if (ValueType == REG_SZ ||
        ValueType == REG_EXPAND_SZ ||
        ValueType == REG_MULTI_SZ
       ) {
        PUNICODE_STRING DestinationString;
        USHORT TruncValueLength;

        //
        // Truncate ValueLength to be represented in a UNICODE_STRING
        //
        if ( ValueLength <= MAX_USTRING ) {
            TruncValueLength = (USHORT)ValueLength;
        } else {
            TruncValueLength = MAX_USTRING;

//davepr: move all this stuff to debug builds at some point.
//davepr: but for now, I'd like to identify whether there are components
//davepr: that are seeing silent failures.
//#if DBG
            DbgPrint("RtlpQueryRegistryDirect: truncating SZ Value length: %x -> %x\n",
                     ValueLength, TruncValueLength);
//#endif //DBG
        }

        DestinationString = (PUNICODE_STRING)Destination;
        if (DestinationString->Buffer == NULL) {

            DestinationString->Buffer = RtlAllocateStringRoutine( TruncValueLength );
            if (!DestinationString->Buffer) {
                return STATUS_NO_MEMORY;
            }
            DestinationString->MaximumLength = TruncValueLength;
        } else if (TruncValueLength > DestinationString->MaximumLength) {
                return STATUS_BUFFER_TOO_SMALL;
        }

        RtlCopyMemory( DestinationString->Buffer, ValueData, TruncValueLength );
        DestinationString->Length = (TruncValueLength - sizeof(UNICODE_NULL));

    } else if (ValueLength <= sizeof( ULONG )) {
        RtlCopyMemory( Destination, ValueData, ValueLength );

    } else {
        PULONG DestinationLength;

        DestinationLength = (PULONG)Destination;
        if ((LONG)*DestinationLength < 0) {
            ULONG n = -(LONG)*DestinationLength;

            if (n < ValueLength) {
                return STATUS_BUFFER_TOO_SMALL;
            }
            RtlCopyMemory( DestinationLength, ValueData, ValueLength );

        } else {
            if (*DestinationLength < (2 * sizeof(*DestinationLength) + ValueLength)) {
                return STATUS_BUFFER_TOO_SMALL;
            }

            *DestinationLength++ = ValueLength;
            *DestinationLength++ = ValueType;
            RtlCopyMemory( DestinationLength, ValueData, ValueLength );
        }
    }

    return STATUS_SUCCESS;
}

#define QuadAlignPtr(P) (             \
    (PVOID)((((ULONG_PTR)(P)) + 7) & (-8)) \
)

NTSTATUS
RtlpCallQueryRegistryRoutine(
    IN PRTL_QUERY_REGISTRY_TABLE QueryTable,
    IN PKEY_VALUE_FULL_INFORMATION KeyValueInformation,
    IN OUT PULONG PKeyValueInfoLength,
    IN PVOID Context,
    IN PVOID Environment OPTIONAL
    )

/*++

Routine Description:

    This function implements the caller out the a caller specified
    routine.  It is reponsible for capturing the arguments for the
    routine and then calling it.  If not specifically disabled, this
    routine will converted REG_EXPAND_SZ Registry values to REG_SZ by
    calling RtlExpandEnvironmentStrings_U prior to calling the routine.
    It will also converted REG_MULTI_SZ registry values into multiple
    REG_SZ calls to the specified routine.

    N.B. UNICODE_STRINGs cannot handle strings exceeding MAX_USTRING bytes. This creates
    issues both for expansion and for returning queries.  Whenever this limitation
    is a encountered, we punt as best we can -- often returning an unexpanded, or perhaps
    truncated stream -- since this seems to create fewer problems for our callers than
    if we unexpectedly fail.

Arguments:

    QueryTable - specifies the current query table entry.

    KeyValueInformation - points to a buffer that contains the information
        about the current registry value.

    PKeyValueInfoLength - pointer to the maximum length of the KeyValueInformation
        buffer.  This function will use the
        unused portion at the end of this buffer for storing null terminated
        value name strings and the expanded version of REG_EXPAND_SZ values.
        PKeyValueInfoLength returns an estimate of the space required if
        STATUS_BUFFER_TOO_SMALL is returned.  This estimate can be used to retry
        with a larger buffer. Two retries may be required if REG_EXPAND_SZ is specified.

    Context - specifies a 32-bit quantity that is passed uninterpreted to
        each QueryRoutine called.

    Environment - optional parameter, that if specified is the environment
        used when expanding variable values in REG_EXPAND_SZ registry
        values.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;
    ULONG ValueType;
    PWSTR ValueName;
    PVOID ValueData;
    ULONG ValueLength;
    PWSTR s;
    PCHAR FreeMem;
    PCHAR EndFreeMem;
    LONG  FreeMemSize;
    ULONG KeyValueInfoLength;
    int   retries;


    //
    // Return 0 length unless we return STATUS_BUFFER_TOO_SMALL.
    //
    KeyValueInfoLength = *PKeyValueInfoLength;
    *PKeyValueInfoLength = 0;

    //
    // Initially assume the entire KeyValueInformation buffer is unused.
    //

    FreeMem = (PCHAR)KeyValueInformation;
    FreeMemSize = KeyValueInfoLength;
    EndFreeMem = FreeMem + FreeMemSize;

    if (KeyValueInformation->Type == REG_NONE ||
        (KeyValueInformation->DataLength == 0 &&
         KeyValueInformation->Type == QueryTable->DefaultType)
       ) {

        //
        // If there is no registry value then see if they want to default
        // this value.
        //
        if (QueryTable->DefaultType == REG_NONE) {
            //
            // No default value specified.  Return success unless this is
            // a required value.
            //
            if ( QueryTable->Flags & RTL_QUERY_REGISTRY_REQUIRED ) {
               return STATUS_OBJECT_NAME_NOT_FOUND;
            } else {
               return STATUS_SUCCESS;
            }
        }

        //
        // Default requested.  Setup the value data pointers from the
        // information in the table entry.
        //

        ValueName = QueryTable->Name,
        ValueType = QueryTable->DefaultType;
        ValueData = QueryTable->DefaultData;
        ValueLength = QueryTable->DefaultLength;
        if (ValueLength == 0) {
            //
            // If the length of the value is zero, then calculate the
            // actual length for REG_SZ, REG_EXPAND_SZ and REG_MULTI_SZ
            // value types.
            //

            s = (PWSTR)ValueData;
            if (ValueType == REG_SZ || ValueType == REG_EXPAND_SZ) {
                while (*s++ != UNICODE_NULL) {
                }
                ValueLength = (ULONG)((PCHAR)s - (PCHAR)ValueData);

            } else if (ValueType == REG_MULTI_SZ) {
                while (*s != UNICODE_NULL) {
                    while (*s++ != UNICODE_NULL) {
                        }
                    }
                ValueLength = (ULONG)((PCHAR)s - (PCHAR)ValueData) + sizeof( UNICODE_NULL );
            }
        }

    } else {
        if (!(QueryTable->Flags & RTL_QUERY_REGISTRY_DIRECT)) {
            LONG ValueSpaceNeeded;

            //
            // There is a registry value.  Calculate a pointer to the
            // free memory at the end of the value information buffer,
            // and its size.
            //
            if (KeyValueInformation->DataLength) {
                FreeMem += KeyValueInformation->DataOffset +
                           KeyValueInformation->DataLength;
            } else {
                FreeMem += FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name) +
                           KeyValueInformation->NameLength;
            }
            FreeMem = (PCHAR)QuadAlignPtr(FreeMem);
            FreeMemSize = (ULONG) (EndFreeMem - FreeMem);

            //
            // See if there is room in the free memory area for a null
            // terminated copy of the value name string.  If not return
            // the length we require (so far) and an error.
            //
            ValueSpaceNeeded = KeyValueInformation->NameLength + sizeof(UNICODE_NULL);
            if ( FreeMemSize < ValueSpaceNeeded ) {

               *PKeyValueInfoLength = (ULONG)(((PCHAR)FreeMem - (PCHAR)KeyValueInformation) + ValueSpaceNeeded);
                return STATUS_BUFFER_TOO_SMALL;
            }

            //
            // There is room, so copy the string, and null terminate it.
            //

            ValueName = (PWSTR)FreeMem;
            RtlCopyMemory( ValueName,
                           KeyValueInformation->Name,
                           KeyValueInformation->NameLength
                         );
            *(PWSTR)((PCHAR)ValueName + KeyValueInformation->NameLength) = UNICODE_NULL;

            //
            // Update the free memory pointer and size to reflect the space we
            // just used for the null terminated value name.
            //
            FreeMem += ValueSpaceNeeded;
            FreeMem = (PCHAR)QuadAlignPtr(FreeMem);
            FreeMemSize = (LONG) (EndFreeMem - FreeMem);

        } else {
            ValueName = QueryTable->Name;
        }

        //
        // Get the remaining data for the registry value.
        //

        ValueType = KeyValueInformation->Type;
        ValueData = (PCHAR)KeyValueInformation + KeyValueInformation->DataOffset;
        ValueLength = KeyValueInformation->DataLength;
    }

    //
    // Unless specifically disabled for this table entry, preprocess
    // registry values of type REG_EXPAND_SZ and REG_MULTI_SZ
    //

    if (!(QueryTable->Flags & RTL_QUERY_REGISTRY_NOEXPAND)) {
        if (ValueType == REG_MULTI_SZ) {
            PWSTR ValueEnd;

            //
            // For REG_MULTI_SZ value type, call the query routine once
            // for each null terminated string in the registry value.  Fake
            // like this is multiple REG_SZ values with the same value name.
            //

            Status = STATUS_SUCCESS;
            ValueEnd = (PWSTR)((PCHAR)ValueData + ValueLength) - sizeof(UNICODE_NULL);
            s = (PWSTR)ValueData;
            while (s < ValueEnd) {
                while (*s++ != UNICODE_NULL) {
                }

                ValueLength = (ULONG)((PCHAR)s - (PCHAR)ValueData);
                if (QueryTable->Flags & RTL_QUERY_REGISTRY_DIRECT) {
                    Status = RtlpQueryRegistryDirect( REG_SZ,
                                                      ValueData,
                                                      ValueLength,
                                                      QueryTable->EntryContext
                                                    );
                    (PUNICODE_STRING)(QueryTable->EntryContext) += 1;

                } else {
                    Status = (QueryTable->QueryRoutine)( ValueName,
                                                         REG_SZ,
                                                         ValueData,
                                                         ValueLength,
                                                         Context,
                                                         QueryTable->EntryContext
                                                       );
                }

                //
                // We ignore failures where the buffer is too small.
                //
                if (Status == STATUS_BUFFER_TOO_SMALL) {
                   Status = STATUS_SUCCESS;
                }

                if (!NT_SUCCESS( Status )) {
                    break;
                }

                ValueData = (PVOID)s;
            }

            return Status;
        }

        //
        // If requested, expand the Value -- but only if the unexpanded value
        // can be represented with a UNICODE_STRING.
        //
        if ((ValueType == REG_EXPAND_SZ) &&
            (ValueLength >= sizeof(WCHAR)) &&
            (ValueLength <= MAX_NONNULL_USTRING)) {
            //
            // For REG_EXPAND_SZ value type, expand any environment variable
            // references in the registry value string using the Rtl function.
            //

            UNICODE_STRING Source;
            UNICODE_STRING Destination;
            PWCHAR  Src;
            ULONG   SrcLength;
            ULONG   RequiredLength;
            BOOLEAN PercentFound;

            //
            // Don't expand unless we have to since expansion doubles buffer usage.
            //

            PercentFound = FALSE;
            SrcLength = ValueLength - sizeof(WCHAR);
            Src = (PWSTR)ValueData;
            while (SrcLength) {
                if (*Src == L'%') {
                    PercentFound = TRUE;
                    break;
                }
                Src++;
                SrcLength -= sizeof(WCHAR);
            }

            if ( PercentFound ) {
                Source.Buffer = (PWSTR)ValueData;
                Source.MaximumLength = (USHORT)ValueLength;
                Source.Length = (USHORT)(Source.MaximumLength - sizeof(UNICODE_NULL));
                Destination.Buffer = (PWSTR)FreeMem;
                Destination.Length = 0;

                if (FreeMemSize <= 0) {
                    Destination.MaximumLength = 0;
                } else if (FreeMemSize <= MAX_USTRING) {
                    Destination.MaximumLength = (USHORT)FreeMemSize;
                    Destination.Buffer[FreeMemSize/sizeof(WCHAR) - 1] = UNICODE_NULL;
                } else {
                    Destination.MaximumLength = MAX_USTRING;
                    Destination.Buffer[MAX_USTRING/sizeof(WCHAR) - 1] = UNICODE_NULL;
                }

                Status = RtlExpandEnvironmentStrings_U( Environment,
                                                        &Source,
                                                        &Destination,
                                                        &RequiredLength
                                                      );
                ValueType = REG_SZ;

                if ( NT_SUCCESS(Status) ) {
                    ValueData = Destination.Buffer;
                    ValueLength = Destination.Length + sizeof( UNICODE_NULL );
                } else {
                    if (Status == STATUS_BUFFER_TOO_SMALL) {
                       *PKeyValueInfoLength = (ULONG)((PCHAR)FreeMem - (PCHAR)KeyValueInformation) + RequiredLength;
                    }
//#if DBG
                    if (Status == STATUS_BUFFER_TOO_SMALL) {
                       DbgPrint( "RTL: Expand variables for %wZ failed - Status == %lx Size %x > %x <%x>\n",
                                     &Source, Status, *PKeyValueInfoLength, KeyValueInfoLength,
                                     Destination.MaximumLength );
                    } else {
                       DbgPrint( "RTL: Expand variables for %wZ failed - Status == %lx\n", &Source, Status );
                    }
//#endif  // DBG
                    if ( Status == STATUS_BUFFER_OVERFLOW ||
                         Status == STATUS_BUFFER_TOO_SMALL &&
                        ( Destination.MaximumLength == MAX_USTRING
                         || RequiredLength > MAX_NONNULL_USTRING ) ) {

                       // We can't do variable expansion because the required buffer can't be described
                       // by a UNICODE_STRING, so we silently ignore expansion.
//#if DBG
                       DbgPrint("RtlpCallQueryRegistryRoutine: skipping expansion.  Status=%x RequiredLength=%x\n",
                         Status, RequiredLength);
//#endif //DBG
                   } else {
                        return Status;
                   }
                }
            }
        }
//#if DBG
        else if (ValueType == REG_EXPAND_SZ  &&  ValueLength > MAX_NONNULL_USTRING) {
            DbgPrint("RtlpCallQueryRegistryRoutine: skipping environment expansion.  ValueLength=%x\n",
                     ValueLength);
        }
//#endif //DBG
    }

    //
    // No special process of the registry value required so just call
    // the query routine.
    //
    if (QueryTable->Flags & RTL_QUERY_REGISTRY_DIRECT) {
        Status = RtlpQueryRegistryDirect( ValueType,
                                          ValueData,
                                          ValueLength,
                                          QueryTable->EntryContext
                                        );
    } else {
        Status = (QueryTable->QueryRoutine)( ValueName,
                                             ValueType,
                                             ValueData,
                                             ValueLength,
                                             Context,
                                             QueryTable->EntryContext
                                           );

    }

    //
    // At this point we fail silently if the buffer is too small.
    //
    if (Status == STATUS_BUFFER_TOO_SMALL) {
        Status = STATUS_SUCCESS;
    }
    return Status;
}

//
// Most of the registry queries in the kernel are small (40-50 bytes).
// User queries use ZwAllocateVirtualMemory, so nothing less than a page will do.
//
#ifdef NTOS_KERNEL_RUNTIME
SIZE_T RtlpRegistryQueryInitialBuffersize = 0x80 + sizeof(PVOID);
#else
SIZE_T RtlpRegistryQueryInitialBuffersize = PAGE_SIZE;
#endif

//
// Allocate, Free, or Free/Allocate space for registry queries.
//
PVOID
RtlpAllocDeallocQueryBuffer(
   IN OUT SIZE_T    *PAllocLength            OPTIONAL,
   IN     PVOID      OldKeyValueInformation  OPTIONAL,
   IN     SIZE_T     OldAllocLength          OPTIONAL,
      OUT NTSTATUS  *pStatus                 OPTIONAL
   )
{
   PVOID    Ptr     = NULL;
   NTSTATUS Status  = STATUS_SUCCESS;

#ifdef NTOS_KERNEL_RUNTIME

   //
   // Kernel version
   //

   UNREFERENCED_PARAMETER( OldAllocLength );

   if ( ARGUMENT_PRESENT(OldKeyValueInformation) ) {
      ExFreePool( OldKeyValueInformation );
   }

   if ( ARGUMENT_PRESENT(PAllocLength) ) {
      Ptr = ExAllocatePoolWithTag( PagedPool, *PAllocLength, 'vrqR' );
      if (Ptr == NULL) {
         Status = STATUS_NO_MEMORY;
      }
   }

#else

   //
   // User version
   //

   if ( ARGUMENT_PRESENT(OldKeyValueInformation) ) {
       Status = ZwFreeVirtualMemory( NtCurrentProcess(),
                                     &OldKeyValueInformation,
                                     &OldAllocLength,
                                     MEM_RELEASE );
   }

   if ( ARGUMENT_PRESENT(PAllocLength) ) {

       Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                     &Ptr,
                                     0,
                                     PAllocLength,
                                     MEM_COMMIT,
                                     PAGE_READWRITE );
       if (!NT_SUCCESS(Status)) {
          Ptr = NULL;
       }
   }

#endif

   if ( ARGUMENT_PRESENT(pStatus) ) {
      *pStatus = Status;
   }

   return Ptr;
}

NTSTATUS
RtlQueryRegistryValues(
    IN ULONG RelativeTo,
    IN PCWSTR Path,
    IN PRTL_QUERY_REGISTRY_TABLE QueryTable,
    IN PVOID Context,
    IN PVOID Environment OPTIONAL
    )

/*++

Routine Description:

    This function allows the caller to query multiple values from the registry
    sub-tree with a single call.  The caller specifies an initial key path,
    and a table.  The table contains one or more entries that describe the
    key values and subkey names the caller is interested in.  This function
    starts at the initial key and enumerates the entries in the table.  For
    each entry that specifies a value name or subkey name that exists in
    the registry, this function calls the caller's query routine associated
    with each table entry.  The caller's query routine is passed the value
    name, type, data and data length, to do with what they wish.

Arguments:

    RelativeTo - specifies that the Path parameter is either an absolute
        registry path, or a path relative to a predefined key path.  The
        following values are defined:

        RTL_REGISTRY_ABSOLUTE   - Path is an absolute registry path
        RTL_REGISTRY_SERVICES   - Path is relative to \Registry\Machine\System\CurrentControlSet\Services
        RTL_REGISTRY_CONTROL    - Path is relative to \Registry\Machine\System\CurrentControlSet\Control
        RTL_REGISTRY_WINDOWS_NT - Path is relative to \Registry\Machine\Software\Microsoft\Windows NT\CurrentVersion
        RTL_REGISTRY_DEVICEMAP  - Path is relative to \Registry\Machine\Hardware\DeviceMap
        RTL_REGISTRY_USER       - Path is relative to \Registry\User\CurrentUser

        RTL_REGISTRY_OPTIONAL   - Bit that specifies the key referenced by
                                  this parameter and the Path parameter is
                                  optional.

        RTL_REGISTRY_HANDLE     - Bit that specifies that the Path parameter
                                  is actually a registry handle to use.
                                  optional.

    Path - specifies either an absolute registry path, or a path relative to the
        known location specified by the RelativeTo parameter.  If the the
        RTL_REGISTRY_HANDLE flag is specified, then this parameter is a
        registry handle to use directly.

    QueryTable - specifies a table of one or more value names and subkey names
        that the caller is interested.  Each table entry contains a query routine
        that will be called for each value name that exists in the registry.
        The table is terminated when a NULL table entry is reached.  A NULL
        table entry is defined as a table entry with a NULL QueryRoutine
        and a NULL Name field.

        QueryTable entry fields:

        PRTL_QUERY_REGISTRY_ROUTINE QueryRoutine - This routine is
            called with the name, type, data and data length of a
            registry value.  If this field is NULL, then it marks the
            end of the table.

        ULONG Flags - These flags control how the following fields are
            interpreted.  The following flags are defined:

            RTL_QUERY_REGISTRY_SUBKEY - says the Name field of this
                table entry is another path to a registry key and all
                following table entries are for that key rather than the
                key specified by the Path parameter.  This change in
                focus lasts until the end of the table or another
                RTL_QUERY_REGISTRY_SUBKEY entry is seen or
                RTL_QUERY_REGISTRY_TOPKEY entry is seen.  Each such
                entry must specify a path that is relative to the Path
                specified on the call to this function.

            RTL_QUERY_REGISTRY_TOPKEY - resets the current registry key
                handle to the original one specified by the RelativeTo
                and Path parameters.  Useful for getting back to the
                original node after descending into subkeys with the
                RTL_QUERY_REGISTRY_SUBKEY flag.

            RTL_QUERY_REGISTRY_REQUIRED - specifies that this value is
                required and if not found then STATUS_OBJECT_NAME_NOT_FOUND
                is returned.  For a table entry that specifies a NULL
                name so that this function will enumerate all of the
                value names under a key, STATUS_OBJECT_NAME_NOT_FOUND
                will be returned only if there are no value keys under
                the current key.

            RTL_QUERY_REGISTRY_NOVALUE - specifies that even though
                there is no Name field for this table entry, all the
                caller wants is a call back, it does NOT want to
                enumerate all the values under the current key.  The
                query routine is called with NULL for ValueData,
                REG_NONE for ValueType and zero for ValueLength.

            RTL_QUERY_REGISTRY_NOEXPAND - specifies that if the value
                type of this registry value is REG_EXPAND_SZ or
                REG_MULTI_SZ, then this function is NOT to do any
                preprocessing of the registry values prior to calling
                the query routine.  Default behavior is to expand
                environment variable references in REG_EXPAND_SZ
                values and to enumerate the NULL terminated strings
                in a REG_MULTI_SZ value and call the query routine
                once for each, making it look like multiple REG_SZ
                values with the same ValueName.

            RTL_QUERY_REGISTRY_DIRECT QueryRoutine field ignored.
                EntryContext field points to location to store value.
                For null terminated strings, EntryContext points to
                UNICODE_STRING structure that that describes maximum
                size of buffer.  If .Buffer field is NULL then a buffer
                is allocated.

            RTL_QUERY_REGISTRY_DELETE Used to delete value keys after
                they are queried.

        PWSTR Name - This field gives the name of a Value the caller
            wants to query the value of.  If this field is NULL, then
            the QueryRoutine specified for this table entry is called
            for all values associated with the current registry key.

        PVOID EntryContext - This field is an arbitrary 32-bit field
            that is passed uninterpreted to each QueryRoutine called.

        ULONG DefaultType
        PVOID DefaultData
        ULONG DefaultLength If there is no value name that matches the
            name given by the Name field, and the DefaultType field is
            not REG_NONE, then the QueryRoutine for this table entry is
            called with the contents of the following fields as if the
            value had been found in the registry.  If the DefaultType is
            REG_SZ, REG_EXPANDSZ or REG_MULTI_SZ and the DefaultLength
            is 0 then the value of DefaultLength will be computed based
            on the length of unicode string pointed to by DefaultData

    Context - specifies a 32-bit quantity that is passed uninterpreted to
        each QueryRoutine called.

    Environment - optional parameter, that if specified is the environment
        used when expanding variable values in REG_EXPAND_SZ registry
        values.

Return Value:

    Status of the operation.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    KeyPath, KeyValueName;
    HANDLE  Key, Key1;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    SIZE_T  KeyValueInfoLength;
    ULONG   ValueIndex;
    SIZE_T  AllocLength;
    ULONG   KeyResultLength;
    int     retries;

    RTL_PAGED_CODE();

    KeyValueInformation = NULL;

    Status = RtlpGetRegistryHandle( RelativeTo, Path, FALSE, &Key );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    if ((RelativeTo & RTL_REGISTRY_HANDLE) == 0) {
        RtlInitUnicodeString(&KeyPath, Path);
    } else {
        RtlInitUnicodeString(&KeyPath, NULL);
    }

    AllocLength = RtlpRegistryQueryInitialBuffersize;

    KeyValueInformation = RtlpAllocDeallocQueryBuffer( &AllocLength, NULL, 0, &Status );
    if ( KeyValueInformation == NULL ) {
        if (!(RelativeTo & RTL_REGISTRY_HANDLE)) {
            ZwClose( Key );
        }
        return Status;
    }

    KeyValueInfoLength = AllocLength - sizeof(UNICODE_NULL);
    Key1 = Key;
    while (QueryTable->QueryRoutine != NULL ||
           (QueryTable->Flags & (RTL_QUERY_REGISTRY_SUBKEY | RTL_QUERY_REGISTRY_DIRECT))
          ) {

        if ((QueryTable->Flags & RTL_QUERY_REGISTRY_DIRECT) &&
            (QueryTable->Name == NULL ||
             (QueryTable->Flags & RTL_QUERY_REGISTRY_SUBKEY) ||
             QueryTable->QueryRoutine != NULL)
           ) {

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (QueryTable->Flags & (RTL_QUERY_REGISTRY_TOPKEY | RTL_QUERY_REGISTRY_SUBKEY)) {
            if (Key1 != Key) {
                NtClose( Key1 );
                Key1 = Key;
            }
        }

        if (QueryTable->Flags & RTL_QUERY_REGISTRY_SUBKEY) {
            if (QueryTable->Name == NULL) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
                RtlInitUnicodeString( &KeyPath, QueryTable->Name );
                InitializeObjectAttributes( &ObjectAttributes,
                                            &KeyPath,
                                            OBJ_CASE_INSENSITIVE,
                                            Key,
                                            NULL
                                            );
#ifdef NTOS_KERNEL_RUNTIME
                //
                // Use a kernel-mode handle for the registry key to prevent
                // malicious apps from hijacking it.
                //
                ObjectAttributes.Attributes |= OBJ_KERNEL_HANDLE;
#endif
                Status = ZwOpenKey( &Key1,
                                    MAXIMUM_ALLOWED,
                                    &ObjectAttributes
                                  );
                if (NT_SUCCESS( Status )) {
                    if (QueryTable->QueryRoutine != NULL) {
                        goto enumvalues;
                    }
                }
            }

        } else if (QueryTable->Name != NULL) {
                RtlInitUnicodeString( &KeyValueName, QueryTable->Name );
                retries = 0;
    retryqueryvalue:
                //
                // A maximum of two retries is expected. If we see more we must
                // have miscomputed how much is required for the query buffer.
                //
                if (retries++ > 4) {
//#if DBG
                   DbgPrint("RtlQueryRegistryValues: Miscomputed buffer size at line %d\n", __LINE__);
//#endif
                   break;
                }

                Status = ZwQueryValueKey( Key1,
                                          &KeyValueName,
                                          KeyValueFullInformation,
                                          KeyValueInformation,
                                          (ULONG) KeyValueInfoLength,
                                          &KeyResultLength
                                        );
                //
                // ZwQueryValueKey returns overflow even though the problem is that
                // the specified buffer was too small, so we fix that up here so we
                // can decide correctly whether to retry or not below.
                //
                if (Status == STATUS_BUFFER_OVERFLOW) {
                   Status = STATUS_BUFFER_TOO_SMALL;
                }

                if (!NT_SUCCESS( Status )) {
                    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

                        KeyValueInformation->Type = REG_NONE;
                        KeyValueInformation->DataLength = 0;
                        KeyResultLength = (ULONG)KeyValueInfoLength;
                        Status = RtlpCallQueryRegistryRoutine( QueryTable,
                                                               KeyValueInformation,
                                                               &KeyResultLength,
                                                               Context,
                                                               Environment
                                                             );
                    }

                   if (Status == STATUS_BUFFER_TOO_SMALL) {
                        //
                        // Try to allocate a larger buffer as this is one humongous
                        // value.
                        //
                        AllocLength = KeyResultLength + sizeof(PVOID) + sizeof(UNICODE_NULL);
                        KeyValueInformation = RtlpAllocDeallocQueryBuffer( &AllocLength,
                                                                           KeyValueInformation,
                                                                           AllocLength,
                                                                           &Status
                                                                         );
                        if ( KeyValueInformation == NULL) {
                           break;
                        }
                        KeyValueInfoLength = AllocLength - sizeof(UNICODE_NULL);
                        goto retryqueryvalue;
                    }

                } else {
                    //
                    // KeyResultLength holds the length of the data returned by ZwQueryKeyValue.
                    // If this is a MULTI_SZ value, catenate a NUL.
                    //
                    if ( KeyValueInformation->Type == REG_MULTI_SZ ) {
                            *(PWCHAR) ((PUCHAR)KeyValueInformation + KeyResultLength) = UNICODE_NULL;
                            KeyValueInformation->DataLength += sizeof(UNICODE_NULL);
                    }

                    KeyResultLength = (ULONG)KeyValueInfoLength;
                    Status = RtlpCallQueryRegistryRoutine( QueryTable,
                                                           KeyValueInformation,
                                                           &KeyResultLength,
                                                           Context,
                                                           Environment
                                                         );

                    if ( Status == STATUS_BUFFER_TOO_SMALL ) {
                         //
                         // Try to allocate a larger buffer as this is one humongous
                         // value.
                         //
                         AllocLength = KeyResultLength + sizeof(PVOID) + sizeof(UNICODE_NULL);
                         KeyValueInformation = RtlpAllocDeallocQueryBuffer( &AllocLength,
                                                                            KeyValueInformation,
                                                                            AllocLength,
                                                                            &Status
                                                                          );
                         if ( KeyValueInformation == NULL) {
                            break;
                         }
                         KeyValueInfoLength = AllocLength - sizeof(UNICODE_NULL);
                         goto retryqueryvalue;
                     }

                    //
                    // If requested, delete the value key after it has been successfully queried.
                    //

                    if (NT_SUCCESS( Status ) && QueryTable->Flags & RTL_QUERY_REGISTRY_DELETE) {
                        ZwDeleteValueKey (Key1, &KeyValueName);
                    }
                }

        } else if (QueryTable->Flags & RTL_QUERY_REGISTRY_NOVALUE) {
            Status = (QueryTable->QueryRoutine)( NULL,
                                                 REG_NONE,
                                                 NULL,
                                                 0,
                                                 Context,
                                                 QueryTable->EntryContext
                                               );
        } else {

        enumvalues:
            retries = 0;
            for (ValueIndex = 0; TRUE; ValueIndex++) {
                Status = ZwEnumerateValueKey( Key1,
                                              ValueIndex,
                                              KeyValueFullInformation,
                                              KeyValueInformation,
                                              (ULONG) KeyValueInfoLength,
                                              &KeyResultLength
                                            );
                //
                // ZwEnumerateValueKey returns overflow even though the problem is that
                // the specified buffer was too small, so we fix that up here so we
                // can decide correctly whether to retry or not below.
                //
                if (Status == STATUS_BUFFER_OVERFLOW) {
                   Status = STATUS_BUFFER_TOO_SMALL;
                }

                if (Status == STATUS_NO_MORE_ENTRIES) {
                    if (ValueIndex == 0 && (QueryTable->Flags & RTL_QUERY_REGISTRY_REQUIRED)) {
                       Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    } else {
                        Status = STATUS_SUCCESS;
                    }
                    break;
                }

                if ( NT_SUCCESS( Status ) ) {

                    KeyResultLength = (ULONG)KeyValueInfoLength;
                    Status = RtlpCallQueryRegistryRoutine( QueryTable,
                                                           KeyValueInformation,
                                                           &KeyResultLength,
                                                           Context,
                                                           Environment
                                                         );
                }

                if (Status == STATUS_BUFFER_TOO_SMALL) {
                    //
                    // Allocate a larger buffer and try again.
                    //
                    AllocLength = KeyResultLength + sizeof(PVOID) + sizeof(UNICODE_NULL);
                    KeyValueInformation = RtlpAllocDeallocQueryBuffer( &AllocLength,
                                                                       KeyValueInformation,
                                                                       AllocLength,
                                                                       &Status
                                                                     );
                    if (KeyValueInformation == NULL) {
                       break;
                    }
                    KeyValueInfoLength = AllocLength - sizeof(UNICODE_NULL);
                    ValueIndex -= 1;

                    //
                    // A maximum of two retries is expected per loop iteration.
                    // If we see more we must have miscomputed
                    // how much is required for the query buffer.
                    //
                    if (retries++ <= 4) {
                        continue;
                    }
//#if DBG
                    DbgPrint("RtlQueryRegistryValues: Miscomputed buffer size at line %d\n", __LINE__);
//#endif
                    break;
                }

                if (!NT_SUCCESS( Status )) {
                    break;
                }

                retries = 0;

                //
                // If requested, delete the value key after it has been successfully queried.
                // After deletion the current ValueIndex is for the next sub-key, so adjust it.
                // KeyValueInformation->NameLength should fit in a USHORT, but we don't check since
                // it only harms our caller.
                //

                if (QueryTable->Flags & RTL_QUERY_REGISTRY_DELETE) {
                    KeyValueName.Buffer = KeyValueInformation->Name;
                    KeyValueName.Length = (USHORT)KeyValueInformation->NameLength;
                    KeyValueName.MaximumLength = (USHORT)KeyValueInformation->NameLength;
                    Status = ZwDeleteValueKey( Key1,
                                               &KeyValueName
                                             );
                    if (NT_SUCCESS( Status )) {
                        ValueIndex -= 1;
                    }
                }
            }
        }

        if (!NT_SUCCESS( Status )) {
            break;
        }

        QueryTable++;
    }

    if (Key != NULL && !(RelativeTo & RTL_REGISTRY_HANDLE)) {
        ZwClose( Key );
    }

    if (Key1 != NULL && Key1 != Key) {
        ZwClose( Key1 );
    }

    //
    // Free any query buffer we allocated.
    //
    (void) RtlpAllocDeallocQueryBuffer( NULL, KeyValueInformation, AllocLength, NULL );
    return Status;
}


NTSTATUS
RtlWriteRegistryValue(
    IN ULONG RelativeTo,
    IN PCWSTR Path,
    IN PCWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength
    )
{
    NTSTATUS Status;
    UNICODE_STRING KeyValueName;
    HANDLE Key;

    RTL_PAGED_CODE();

    Status = RtlpGetRegistryHandle( RelativeTo, Path, TRUE, &Key );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    RtlInitUnicodeString( &KeyValueName, ValueName );
    Status = ZwSetValueKey( Key,
                            &KeyValueName,
                            0,
                            ValueType,
                            ValueData,
                            ValueLength
                          );
    if (!(RelativeTo & RTL_REGISTRY_HANDLE)) {
        ZwClose( Key );
    }

    return Status;
}


NTSTATUS
RtlCheckRegistryKey(
    IN ULONG RelativeTo,
    IN PWSTR Path
    )
{
    NTSTATUS Status;
    HANDLE Key;

    RTL_PAGED_CODE();

    Status = RtlpGetRegistryHandle( RelativeTo, Path, FALSE, &Key );
    if (!NT_SUCCESS( Status )) {
        return Status;
        }

    ZwClose( Key );
    return STATUS_SUCCESS;
}


NTSTATUS
RtlCreateRegistryKey(
    IN ULONG RelativeTo,
    IN PWSTR Path
    )
{
    NTSTATUS Status;
    HANDLE Key;

    RTL_PAGED_CODE();

    Status = RtlpGetRegistryHandle( RelativeTo, Path, TRUE, &Key );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    ZwClose( Key );
    return STATUS_SUCCESS;
}


NTSTATUS
RtlDeleteRegistryValue(
    IN ULONG RelativeTo,
    IN PCWSTR Path,
    IN PCWSTR ValueName
    )
{
    NTSTATUS Status;
    UNICODE_STRING KeyValueName;
    HANDLE Key;

    RTL_PAGED_CODE();

    Status = RtlpGetRegistryHandle( RelativeTo, Path, TRUE, &Key );
    if (!NT_SUCCESS( Status )) {
        return Status;
        }

    RtlInitUnicodeString( &KeyValueName, ValueName );
    Status = ZwDeleteValueKey( Key, &KeyValueName );

    ZwClose( Key );
    return Status;
}


NTSTATUS
RtlExpandEnvironmentStrings_U(
    IN PVOID Environment OPTIONAL,
    IN PUNICODE_STRING Source,
    OUT PUNICODE_STRING Destination,
    OUT PULONG ReturnedLength OPTIONAL
    )
{
    NTSTATUS Status, Status1;
    PWCHAR Src, Src1, Dst;
    UNICODE_STRING VariableName, VariableValue;
    ULONG SrcLength, DstLength, VarLength, RequiredLength;

    RTL_PAGED_CODE();

    Src = Source->Buffer;
    SrcLength = Source->Length;
    Dst = Destination->Buffer;
    DstLength = Destination->MaximumLength;
    Status = STATUS_SUCCESS;
    RequiredLength = 0;
    while (SrcLength >= sizeof(WCHAR)) {
        if (*Src == L'%') {
            Src1 = Src + 1;
            VarLength = 0;
            VariableName.Length = 0;
            VariableName.Buffer = Src1;

            while (VarLength < (SrcLength - sizeof(WCHAR))) {
                if (*Src1 == L'%') {
                    if (VarLength) {
                        VariableName.Length = (USHORT)VarLength;
                        VariableName.MaximumLength = (USHORT)VarLength;
                    }
                    break;

                }

                Src1++;
                VarLength += sizeof(WCHAR);
            }

            if (VariableName.Length) {
                VariableValue.Buffer = Dst;
                VariableValue.Length = 0;
                VariableValue.MaximumLength = (USHORT)DstLength;
                Status1 = RtlQueryEnvironmentVariable_U( Environment,
                                                         &VariableName,
                                                         &VariableValue
                                                       );
                if (NT_SUCCESS( Status1 ) || Status1 == STATUS_BUFFER_TOO_SMALL) {
                    RequiredLength += VariableValue.Length;
                    Src = Src1 + 1;
                    SrcLength -= (VarLength + 2*sizeof(WCHAR));

                    if (NT_SUCCESS( Status1 )) {
                        DstLength -= VariableValue.Length;
                        Dst += VariableValue.Length / sizeof(WCHAR);

                    } else {
                        Status = Status1;
                    }

                    continue;
                }
            }
        }

        if (NT_SUCCESS( Status )) {
            if (DstLength > sizeof(WCHAR)) {
                DstLength -= sizeof(WCHAR);
                *Dst++ = *Src;

            } else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }

        RequiredLength += sizeof(WCHAR);
        SrcLength -= sizeof(WCHAR);
        Src++;
    }

    if (NT_SUCCESS( Status )) {
        if (DstLength) {
            DstLength -= sizeof(WCHAR);
            *Dst = UNICODE_NULL;

        } else {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    RequiredLength += sizeof(WCHAR);

    if (ARGUMENT_PRESENT( ReturnedLength )) {
        *ReturnedLength = RequiredLength;
    }

    if (NT_SUCCESS( Status )) {
        Destination->Length = (USHORT)(RequiredLength - sizeof(WCHAR));
    }

    return Status;
}


ULONG
RtlGetNtGlobalFlags( VOID )
{
#ifdef NTOS_KERNEL_RUNTIME
    return NtGlobalFlag;
#else
    return NtCurrentPeb()->NtGlobalFlag;
#endif
}


//
// Maximum size of TOKEN_USER information.
//

#define SIZE_OF_TOKEN_INFORMATION                   \
    sizeof( TOKEN_USER )                            \
    + sizeof( SID )                                 \
    + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES


NTSTATUS
RtlFormatCurrentUserKeyPath(
    OUT PUNICODE_STRING CurrentUserKeyPath
    )

/*++

Routine Description:

    Initialize the supplied buffer with a string representation
    of the current user's SID.

Arguments:

    CurrentUserKeyPath - Returns a string that represents the current
        user's root key in the Registry.  Caller must call
        RtlFreeUnicodeString to free the buffer when done with it.

Return Value:

    NTSTATUS - Returns STATUS_SUCCESS if the user string was
        succesfully initialized.

--*/

{
    HANDLE TokenHandle;
    UCHAR TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
    ULONG ReturnLength;
    ULONG SidStringLength ;
    UNICODE_STRING SidString ;
    NTSTATUS Status;

    Status = ZwOpenThreadToken( NtCurrentThread(),
                                 TOKEN_READ,
                                 TRUE,
                                 &TokenHandle
                               );

    if ( !NT_SUCCESS( Status ) && ( Status != STATUS_NO_TOKEN ) ) {
        return Status;
    }

    if ( !NT_SUCCESS( Status ) ) {

        Status = ZwOpenProcessToken( NtCurrentProcess(),
                                     TOKEN_READ,
                                     &TokenHandle
                                   );
        if ( !NT_SUCCESS( Status )) {
            return Status;
        }
    }

    Status = ZwQueryInformationToken( TokenHandle,
                                      TokenUser,
                                      TokenInformation,
                                      sizeof( TokenInformation ),
                                      &ReturnLength
                                    );

    ZwClose( TokenHandle );

    if ( !NT_SUCCESS( Status )) {
        return Status;
    }

    Status = RtlLengthSidAsUnicodeString(
                        ((PTOKEN_USER)TokenInformation)->User.Sid,
                        &SidStringLength
                        );

    if ( !NT_SUCCESS( Status ) ) {
        return Status ;
    }

    CurrentUserKeyPath->Length = 0;
    CurrentUserKeyPath->MaximumLength = (USHORT)(SidStringLength +
                                        sizeof( L"\\REGISTRY\\USER\\" ) +
                                        sizeof( UNICODE_NULL ));
    CurrentUserKeyPath->Buffer = (RtlAllocateStringRoutine)( CurrentUserKeyPath->MaximumLength );
    if (CurrentUserKeyPath->Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // Copy "\REGISTRY\USER" to the current user string.
    //

    RtlAppendUnicodeToString( CurrentUserKeyPath, L"\\REGISTRY\\USER\\" );

    SidString.MaximumLength = (USHORT)SidStringLength ;
    SidString.Length = 0 ;
    SidString.Buffer = CurrentUserKeyPath->Buffer +
            (CurrentUserKeyPath->Length / sizeof(WCHAR) );

    Status = RtlConvertSidToUnicodeString( &SidString,
                                           ((PTOKEN_USER)TokenInformation)->User.Sid,
                                           FALSE
                                         );
    if ( !NT_SUCCESS( Status )) {
        RtlFreeUnicodeString( CurrentUserKeyPath );

    } else {
        CurrentUserKeyPath->Length += SidString.Length ;
    }

    return Status;
}


NTSTATUS
RtlOpenCurrentUser(
    IN ULONG DesiredAccess,
    OUT PHANDLE CurrentUserKey
    )

/*++

Routine Description:

    Attempts to open the the HKEY_CURRENT_USER predefined handle.

Arguments:

    DesiredAccess - Specifies the access to open the key for.

    CurrentUserKey - Returns a handle to the key \REGISTRY\USER\*.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    UNICODE_STRING      CurrentUserKeyPath;
    OBJECT_ATTRIBUTES   Obja;
    NTSTATUS            Status;

    RTL_PAGED_CODE();

    //
    // Format the registry path for the current user.
    //

    Status = RtlFormatCurrentUserKeyPath( &CurrentUserKeyPath );
    if ( NT_SUCCESS(Status) ) {

        InitializeObjectAttributes( &Obja,
                                    &CurrentUserKeyPath,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                  );
        Status = ZwOpenKey( CurrentUserKey,
                            DesiredAccess,
                            &Obja
                          );
        RtlFreeUnicodeString( &CurrentUserKeyPath );
    }

    if ( !NT_SUCCESS(Status) ) {
        //
        // Opening \REGISTRY\USER\<SID> failed, try \REGISTRY\USER\.DEFAULT
        //
        RtlInitUnicodeString( &CurrentUserKeyPath, RtlpRegistryPaths[ RTL_REGISTRY_USER ] );
        InitializeObjectAttributes( &Obja,
                                    &CurrentUserKeyPath,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                  );

        Status = ZwOpenKey( CurrentUserKey,
                            DesiredAccess,
                            &Obja
                          );
    }

    return Status;
}


NTSTATUS
RtlpGetTimeZoneInfoHandle(
    IN BOOLEAN WriteAccess,
    OUT PHANDLE Key
    )
{
    return RtlpGetRegistryHandle( RTL_REGISTRY_CONTROL, L"TimeZoneInformation", WriteAccess, Key );
}



extern  const WCHAR szBias[];
extern  const WCHAR szStandardName[];
extern  const WCHAR szStandardBias[];
extern  const WCHAR szStandardStart[];
extern  const WCHAR szDaylightName[];
extern  const WCHAR szDaylightBias[];
extern  const WCHAR szDaylightStart[];

NTSTATUS
RtlQueryTimeZoneInformation(
    OUT PRTL_TIME_ZONE_INFORMATION TimeZoneInformation
    )
{
    NTSTATUS Status;
    HANDLE Key;
    UNICODE_STRING StandardName, DaylightName;
    RTL_QUERY_REGISTRY_TABLE RegistryConfigurationTable[ 8 ];

    RTL_PAGED_CODE();

    Status = RtlpGetTimeZoneInfoHandle( FALSE, &Key );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    RtlZeroMemory( TimeZoneInformation, sizeof( *TimeZoneInformation ) );
    RtlZeroMemory( RegistryConfigurationTable, sizeof( RegistryConfigurationTable ) );

    RegistryConfigurationTable[ 0 ].Flags = RTL_QUERY_REGISTRY_DIRECT;
    RegistryConfigurationTable[ 0 ].Name = (PWSTR)szBias;
    RegistryConfigurationTable[ 0 ].EntryContext = &TimeZoneInformation->Bias;


    StandardName.Buffer = TimeZoneInformation->StandardName;
    StandardName.Length = 0;
    StandardName.MaximumLength = sizeof( TimeZoneInformation->StandardName );
    RegistryConfigurationTable[ 1 ].Flags = RTL_QUERY_REGISTRY_DIRECT;
    RegistryConfigurationTable[ 1 ].Name = (PWSTR)szStandardName;
    RegistryConfigurationTable[ 1 ].EntryContext = &StandardName;

    RegistryConfigurationTable[ 2 ].Flags = RTL_QUERY_REGISTRY_DIRECT;
    RegistryConfigurationTable[ 2 ].Name = (PWSTR)szStandardBias;
    RegistryConfigurationTable[ 2 ].EntryContext = &TimeZoneInformation->StandardBias;

    RegistryConfigurationTable[ 3 ].Flags = RTL_QUERY_REGISTRY_DIRECT;
    RegistryConfigurationTable[ 3 ].Name = (PWSTR)szStandardStart;
    RegistryConfigurationTable[ 3 ].EntryContext = &TimeZoneInformation->StandardStart;
    *(PLONG)(RegistryConfigurationTable[ 3 ].EntryContext) = -(LONG)sizeof( TIME_FIELDS );

    DaylightName.Buffer = TimeZoneInformation->DaylightName;
    DaylightName.Length = 0;
    DaylightName.MaximumLength = sizeof( TimeZoneInformation->DaylightName );
    RegistryConfigurationTable[ 4 ].Flags = RTL_QUERY_REGISTRY_DIRECT;
    RegistryConfigurationTable[ 4 ].Name = (PWSTR)szDaylightName;
    RegistryConfigurationTable[ 4 ].EntryContext = &DaylightName;

    RegistryConfigurationTable[ 5 ].Flags = RTL_QUERY_REGISTRY_DIRECT;
    RegistryConfigurationTable[ 5 ].Name = (PWSTR)szDaylightBias;
    RegistryConfigurationTable[ 5 ].EntryContext = &TimeZoneInformation->DaylightBias;

    RegistryConfigurationTable[ 6 ].Flags = RTL_QUERY_REGISTRY_DIRECT;
    RegistryConfigurationTable[ 6 ].Name = (PWSTR)szDaylightStart;
    RegistryConfigurationTable[ 6 ].EntryContext = &TimeZoneInformation->DaylightStart;
    *(PLONG)(RegistryConfigurationTable[ 6 ].EntryContext) = -(LONG)sizeof( TIME_FIELDS );

    Status = RtlQueryRegistryValues( RTL_REGISTRY_HANDLE,
                                     (PWSTR)Key,
                                     RegistryConfigurationTable,
                                     NULL,
                                     NULL
                                   );
    ZwClose( Key );
    return Status;
}


NTSTATUS
RtlSetTimeZoneInformation(
    IN PRTL_TIME_ZONE_INFORMATION TimeZoneInformation
    )
{
    NTSTATUS Status;
    HANDLE Key;

    RTL_PAGED_CODE();

    Status = RtlpGetTimeZoneInfoHandle( TRUE, &Key );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    Status = RtlWriteRegistryValue( RTL_REGISTRY_HANDLE,
                                    (PWSTR)Key,
                                    szBias,
                                    REG_DWORD,
                                    &TimeZoneInformation->Bias,
                                    sizeof( TimeZoneInformation->Bias )
                                  );
    if (NT_SUCCESS( Status )) {
        Status = RtlWriteRegistryValue( RTL_REGISTRY_HANDLE,
                                        (PWSTR)Key,
                                        szStandardName,
                                        REG_SZ,
                                        TimeZoneInformation->StandardName,
                                        (wcslen( TimeZoneInformation->StandardName ) + 1) * sizeof( WCHAR )
                                      );
    }

    if (NT_SUCCESS( Status )) {
        Status = RtlWriteRegistryValue( RTL_REGISTRY_HANDLE,
                                        (PWSTR)Key,
                                        szStandardBias,
                                        REG_DWORD,
                                        &TimeZoneInformation->StandardBias,
                                        sizeof( TimeZoneInformation->StandardBias )
                                      );
    }

    if (NT_SUCCESS( Status )) {
        Status = RtlWriteRegistryValue( RTL_REGISTRY_HANDLE,
                                        (PWSTR)Key,
                                        szStandardStart,
                                        REG_BINARY,
                                        &TimeZoneInformation->StandardStart,
                                        sizeof( TimeZoneInformation->StandardStart )
                                      );
    }

    if (NT_SUCCESS( Status )) {
        Status = RtlWriteRegistryValue( RTL_REGISTRY_HANDLE,
                                        (PWSTR)Key,
                                        szDaylightName,
                                        REG_SZ,
                                        TimeZoneInformation->DaylightName,
                                        (wcslen( TimeZoneInformation->DaylightName ) + 1) * sizeof( WCHAR )
                                      );
    }

    if (NT_SUCCESS( Status )) {
        Status = RtlWriteRegistryValue( RTL_REGISTRY_HANDLE,
                                        (PWSTR)Key,
                                        szDaylightBias,
                                        REG_DWORD,
                                        &TimeZoneInformation->DaylightBias,
                                        sizeof( TimeZoneInformation->DaylightBias )
                                      );
    }

    if (NT_SUCCESS( Status )) {
        Status = RtlWriteRegistryValue( RTL_REGISTRY_HANDLE,
                                        (PWSTR)Key,
                                        szDaylightStart,
                                        REG_BINARY,
                                        &TimeZoneInformation->DaylightStart,
                                        sizeof( TimeZoneInformation->DaylightStart )
                                      );
    }

    ZwClose( Key );
    return Status;
}


NTSTATUS
RtlSetActiveTimeBias(
    IN LONG ActiveBias
    )
{
    NTSTATUS Status;
    HANDLE Key;
    RTL_QUERY_REGISTRY_TABLE RegistryConfigurationTable[ 2 ];
    LONG CurrentActiveBias;

    RTL_PAGED_CODE();

    Status = RtlpGetTimeZoneInfoHandle( TRUE, &Key );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    RtlZeroMemory( RegistryConfigurationTable, sizeof( RegistryConfigurationTable ) );
    RegistryConfigurationTable[ 0 ].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    RegistryConfigurationTable[ 0 ].Name = L"ActiveTimeBias";
    RegistryConfigurationTable[ 0 ].EntryContext = &CurrentActiveBias;

    Status = RtlQueryRegistryValues( RTL_REGISTRY_HANDLE,
                                     (PWSTR)Key,
                                     RegistryConfigurationTable,
                                     NULL,
                                     NULL
                                   );

    if ( !NT_SUCCESS(Status) || CurrentActiveBias != ActiveBias ) {

        Status = RtlWriteRegistryValue( RTL_REGISTRY_HANDLE,
                                        (PWSTR)Key,
                                        L"ActiveTimeBias",
                                        REG_DWORD,
                                        &ActiveBias,
                                        sizeof( ActiveBias )
                                      );
    }

    ZwClose( Key );
    return Status;
}
