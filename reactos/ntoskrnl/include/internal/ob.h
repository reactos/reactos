/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * FILE:              include/internal/objmgr.h
 * PURPOSE:           Object manager definitions
 * PROGRAMMER:        David Welch (welch@mcmail.com)
 */

#ifndef __INCLUDE_INTERNAL_OBJMGR_H
#define __INCLUDE_INTERNAL_OBJMGR_H

#define GENERIC_ACCESS (GENERIC_READ |      \
                        GENERIC_WRITE |     \
                        GENERIC_EXECUTE |   \
                        GENERIC_ALL)

#define KERNEL_HANDLE_FLAG (1 << ((sizeof(HANDLE) * 8) - 1))
#define ObIsKernelHandle(Handle, ProcessorMode)                                \
  (((ULONG_PTR)(Handle) & KERNEL_HANDLE_FLAG) &&                               \
   ((ProcessorMode) == KernelMode))
#define ObKernelHandleToHandle(Handle)                                         \
  (HANDLE)((ULONG_PTR)(Handle) & ~KERNEL_HANDLE_FLAG)
#define ObMarkHandleAsKernelHandle(Handle)                                     \
  (HANDLE)((ULONG_PTR)(Handle) | KERNEL_HANDLE_FLAG)

extern KEVENT ObpDefaultObject;
extern POBJECT_TYPE ObpTypeObjectType;
extern POBJECT_TYPE ObSymbolicLinkType;
extern POBJECT_TYPE ObTypeObjectType;
extern POBJECT_DIRECTORY NameSpaceRoot;
extern POBJECT_DIRECTORY ObpTypeDirectoryObject;
extern PHANDLE_TABLE ObpKernelHandleTable;
extern WORK_QUEUE_ITEM ObpReaperWorkItem;
extern volatile PVOID ObpReaperList;

BOOLEAN
NTAPI
ObpDeleteEntryDirectory(POBP_LOOKUP_CONTEXT Context);

BOOLEAN
NTAPI
ObpInsertEntryDirectory(IN POBJECT_DIRECTORY Parent,
                        IN POBP_LOOKUP_CONTEXT Context,
                        IN POBJECT_HEADER ObjectHeader);

PVOID
NTAPI
ObpLookupEntryDirectory(IN POBJECT_DIRECTORY Directory,
                        IN PUNICODE_STRING Name,
                        IN ULONG Attributes,
                        IN UCHAR SearchShadow,
                        IN POBP_LOOKUP_CONTEXT Context);

VOID
NTAPI
ObInitSymbolicLinkImplementation(VOID);

NTSTATUS
NTAPI
ObpCreateHandle(
    PVOID ObjectBody,
    ACCESS_MASK GrantedAccess,
    ULONG HandleAttributes,
    PHANDLE Handle
);

NTSTATUS
NTAPI
ObpParseDirectory(
    PVOID Object,
    PVOID * NextObject,
    PUNICODE_STRING FullPath,
    PWSTR * Path,
    ULONG Attributes,
    POBP_LOOKUP_CONTEXT Context
);

VOID
NTAPI
ObCreateHandleTable(
    struct _EPROCESS* Parent,
    BOOLEAN Inherit,
    struct _EPROCESS* Process
);

NTSTATUS
NTAPI
ObFindObject(
    IN HANDLE RootHandle,
    IN PUNICODE_STRING ObjectName,
    IN ULONG Attributes,
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID *ReturnedObject,
    IN POBJECT_TYPE ObjectType,
    IN POBP_LOOKUP_CONTEXT Context,
    IN PACCESS_STATE AccessState,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN PVOID ParseContext,
    IN PVOID Insert
);

NTSTATUS
NTAPI
ObpQueryHandleAttributes(
    HANDLE Handle,
    POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo
);

NTSTATUS
NTAPI
ObpSetHandleAttributes(
    HANDLE Handle,
    POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo
);

NTSTATUS
STDCALL
ObpCreateTypeObject(
    struct _OBJECT_TYPE_INITIALIZER *ObjectTypeInitializer,
    PUNICODE_STRING TypeName,
    POBJECT_TYPE *ObjectType
);

ULONG
NTAPI
ObGetObjectHandleCount(PVOID Object);

NTSTATUS
NTAPI
ObDuplicateObject(
    PEPROCESS SourceProcess,
    PEPROCESS TargetProcess,
    HANDLE SourceHandle,
    PHANDLE TargetHandle,
    ACCESS_MASK DesiredAccess,
    ULONG HandleAttributes,
    ULONG Options
);

ULONG
NTAPI
ObpGetHandleCountByHandleTable(PHANDLE_TABLE HandleTable);

VOID
STDCALL
ObQueryDeviceMapInformation(
    PEPROCESS Process,
    PPROCESS_DEVICEMAP_INFORMATION DeviceMapInfo
);

VOID
FASTCALL
ObpSetPermanentObject(
    IN PVOID ObjectBody,
    IN BOOLEAN Permanent
);

VOID
STDCALL
ObKillProcess(PEPROCESS Process);

VOID
FASTCALL
ObpDeleteObject(
    IN PVOID Object
);

VOID
NTAPI
ObpReapObject(
    IN PVOID Unused
);

/* Security descriptor cache functions */

NTSTATUS
NTAPI
ObpInitSdCache(VOID);

NTSTATUS
NTAPI
ObpAddSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SourceSD,
    OUT PSECURITY_DESCRIPTOR *DestinationSD
);

NTSTATUS
NTAPI
ObpRemoveSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor);

VOID
NTAPI
ObpReferenceCachedSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor);

VOID
NTAPI
ObpDereferenceCachedSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor);

VOID
FASTCALL
ObInitializeFastReference(
    IN PEX_FAST_REF FastRef,
    PVOID Object
);

PVOID
FASTCALL
ObFastReplaceObject(
    IN PEX_FAST_REF FastRef,
    PVOID Object
);

PVOID
FASTCALL
ObFastReferenceObject(IN PEX_FAST_REF FastRef);

VOID
FASTCALL
ObFastDereferenceObject(
    IN PEX_FAST_REF FastRef,
    PVOID Object
);

/* Secure object information functions */

NTSTATUS
STDCALL
ObpCaptureObjectName(
    IN PUNICODE_STRING CapturedName,
    IN PUNICODE_STRING ObjectName,
    IN KPROCESSOR_MODE AccessMode
);

NTSTATUS
STDCALL
ObpCaptureObjectAttributes(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN KPROCESSOR_MODE AccessMode,
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    OUT PUNICODE_STRING ObjectName
);

VOID
STDCALL
ObpReleaseCapturedAttributes(IN POBJECT_CREATE_INFORMATION ObjectCreateInfo);

/* object information classes */



#endif /* __INCLUDE_INTERNAL_OBJMGR_H */
