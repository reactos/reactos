/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    obtypes.h

Abstract:

    Type definitions for the Object Manager

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _OBFUNCS_H
#define _OBFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <pstypes.h>
#include <obtypes.h>

#ifndef NTOS_MODE_USER

//
// Object Functions
//
NTKERNELAPI
NTSTATUS
NTAPI
ObAssignSecurity(
    IN PACCESS_STATE AccessState,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PVOID Object,
    IN POBJECT_TYPE Type
);

NTKERNELAPI
NTSTATUS
NTAPI
ObCloseHandle(
    IN HANDLE Handle,
    IN KPROCESSOR_MODE AccessMode
);

NTKERNELAPI
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

NTKERNELAPI
NTSTATUS
NTAPI
ObCreateObjectType(
    IN PUNICODE_STRING TypeName,
    IN POBJECT_TYPE_INITIALIZER ObjectTypeInitializer,
    IN PVOID Reserved,
    OUT POBJECT_TYPE *ObjectType
);

NTKERNELAPI
ULONG
NTAPI
ObGetObjectPointerCount(
    IN PVOID Object
);

NTKERNELAPI
NTSTATUS
NTAPI
ObOpenObjectByName(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN PACCESS_STATE PassedAccessState,
    IN ACCESS_MASK DesiredAccess,
    IN OUT PVOID ParseContext,
    OUT PHANDLE Handle
);

NTKERNELAPI
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

NTKERNELAPI
BOOLEAN
NTAPI
ObFindHandleForObject(
    IN PEPROCESS Process,
    IN PVOID Object,
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_HANDLE_INFORMATION HandleInformation,
    OUT PHANDLE Handle
);

VOID
NTAPI
ObDereferenceObjectDeferDelete(
    IN PVOID Object
);

#endif

//
// Native Calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtClose(
    IN HANDLE Handle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCloseObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING Name
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDuplicateObject(
    IN HANDLE SourceProcessHandle,
    IN HANDLE SourceHandle,
    IN HANDLE TargetProcessHandle,
    OUT PHANDLE TargetHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMakePermanentObject(
    IN HANDLE Object
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMakeTemporaryObject(
    IN HANDLE Handle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenDirectoryObject(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDirectoryObject(
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG Length,
    OUT PULONG ResultLength OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySymbolicLinkObject(
    IN HANDLE SymLinkObjHandle,
    OUT PUNICODE_STRING LinkTarget,
    OUT PULONG DataWritten OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    IN PVOID ObjectInformation,
    IN ULONG Length
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSignalAndWaitForSingleObject(
    IN HANDLE SignalObject,
    IN HANDLE WaitObject,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForMultipleObjects(
    IN ULONG Count,
    IN HANDLE Object[],
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSTATUS
NTAPI
NtWaitForMultipleObjects32(
    IN ULONG ObjectCount,
    IN PLONG Handles,
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER TimeOut OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForSingleObject(
    IN HANDLE Object,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSYSAPI
NTSTATUS
NTAPI
ZwClose(
    IN HANDLE Handle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCloseObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING Name
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateObject(
    IN HANDLE SourceProcessHandle,
    IN HANDLE SourceHandle,
    IN HANDLE TargetProcessHandle,
    OUT PHANDLE TargetHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options
);

NTSYSAPI
NTSTATUS
NTAPI
ZwMakePermanentObject(
    IN HANDLE Object
);

NTSYSAPI
NTSTATUS
NTAPI
ZwMakeTemporaryObject(
    IN HANDLE Handle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenDirectoryObject(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject(
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG Length,
    OUT PULONG ResultLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySymbolicLinkObject(
    IN HANDLE SymLinkObjHandle,
    OUT PUNICODE_STRING LinkTarget,
    OUT PULONG DataWritten OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    IN PVOID ObjectInformation,
    IN ULONG Length
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSignalAndWaitForSingleObject(
    IN HANDLE SignalObject,
    IN HANDLE WaitObject,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForMultipleObjects(
    IN ULONG Count,
    IN HANDLE Object[],
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForSingleObject(
    IN HANDLE Object,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

#endif
