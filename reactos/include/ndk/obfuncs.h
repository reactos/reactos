/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/obfuncs.h
 * PURPOSE:         Protoypes for OBject Manager Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _OBFUNCS_H
#define _OBFUNCS_H

/* DEPENDENCIES **************************************************************/

/* FUNCTION TYPES ************************************************************/

/* PROTOTYPES ****************************************************************/

NTSTATUS
NTAPI
ObCreateObject (
    IN KPROCESSOR_MODE      ObjectAttributesAccessMode OPTIONAL,
    IN POBJECT_TYPE         ObjectType,
    IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE      AccessMode,
    IN OUT PVOID            ParseContext OPTIONAL,
    IN ULONG                ObjectSize,
    IN ULONG                PagedPoolCharge OPTIONAL,
    IN ULONG                NonPagedPoolCharge OPTIONAL,
    OUT PVOID               *Object
);

ULONG
NTAPI
ObGetObjectPointerCount (
    IN PVOID Object
);

NTSTATUS
NTAPI
ObReferenceObjectByName (
    IN PUNICODE_STRING  ObjectName,
    IN ULONG            Attributes,
    IN PACCESS_STATE    PassedAccessState OPTIONAL,
    IN ACCESS_MASK      DesiredAccess OPTIONAL,
    IN POBJECT_TYPE     ObjectType,
    IN KPROCESSOR_MODE  AccessMode,
    IN OUT PVOID        ParseContext OPTIONAL,
    OUT PVOID           *Object
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
