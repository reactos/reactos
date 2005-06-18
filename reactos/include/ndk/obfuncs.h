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
STDCALL
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
STDCALL
ObGetObjectPointerCount (
    IN PVOID Object
);

NTSTATUS
STDCALL
ObInsertObject (
    IN PVOID            Object,
    IN PACCESS_STATE    PassedAccessState OPTIONAL,
    IN ACCESS_MASK      DesiredAccess,
    IN ULONG            AdditionalReferences,
    OUT PVOID           *ReferencedObject OPTIONAL,
    OUT PHANDLE         Handle
);

VOID
STDCALL
ObMakeTemporaryObject (
    IN PVOID Object
);

NTSTATUS
STDCALL
ObOpenObjectByPointer (
    IN PVOID            Object,
    IN ULONG            HandleAttributes,
    IN PACCESS_STATE    PassedAccessState OPTIONAL,
    IN ACCESS_MASK      DesiredAccess OPTIONAL,
    IN POBJECT_TYPE     ObjectType OPTIONAL,
    IN KPROCESSOR_MODE  AccessMode,
    OUT PHANDLE         Handle
);

NTSTATUS
STDCALL
ObQueryNameString (
    IN PVOID                        Object,
    OUT POBJECT_NAME_INFORMATION    ObjectNameInfo,
    IN ULONG                        Length,
    OUT PULONG                      ReturnLength
);

NTSTATUS
STDCALL
ObQueryObjectAuditingByHandle (
    IN HANDLE       Handle,
    OUT PBOOLEAN    GenerateOnClose
);

NTSTATUS
STDCALL
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

#endif
