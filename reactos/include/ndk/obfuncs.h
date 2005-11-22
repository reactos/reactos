/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    obtypes.h

Abstract:

    Type definitions for the Object Manager

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _OBFUNCS_H
#define _OBFUNCS_H

//
// Dependencies
//

//
// Object Functions
//
NTSTATUS
NTAPI
ObCreateObject(
    IN KPROCESSOR_MODE ObjectAttributesAccessMode OPTIONAL,
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PVOID ParseContext OPTIONAL,
    IN ULONG ObjectSize,
    IN ULONG PagedPoolCharge OPTIONAL,
    IN ULONG NonPagedPoolCharge OPTIONAL,
    OUT PVOID *Object
);

ULONG
NTAPI
ObGetObjectPointerCount(
    IN PVOID Object
);

NTSTATUS
NTAPI
ObReferenceObjectByName(
    IN PUNICODE_STRING ObjectName,
    IN ULONG Attributes,
    IN PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PVOID *Object
);

NTSTATUS 
NTAPI
ObFindHandleForObject(
    IN PEPROCESS Process,
    IN PVOID Object,
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_HANDLE_INFORMATION HandleInformation,
    OUT PHANDLE Handle
);

#endif
