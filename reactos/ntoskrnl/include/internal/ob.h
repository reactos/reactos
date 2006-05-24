/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * FILE:              include/internal/objmgr.h
 * PURPOSE:           Object manager definitions
 * PROGRAMMER:        David Welch (welch@mcmail.com)
 */

#ifndef __INCLUDE_INTERNAL_OBJMGR_H
#define __INCLUDE_INTERNAL_OBJMGR_H

struct _EPROCESS;

#define BODY_TO_HEADER(objbdy)                                                 \
  CONTAINING_RECORD((objbdy), OBJECT_HEADER, Body)
  
#define HEADER_TO_OBJECT_NAME(objhdr) ((POBJECT_HEADER_NAME_INFO)              \
  (!(objhdr)->NameInfoOffset ? NULL: ((PCHAR)(objhdr) - (objhdr)->NameInfoOffset)))
  
#define HEADER_TO_HANDLE_INFO(objhdr) ((POBJECT_HEADER_HANDLE_INFO)            \
  (!(objhdr)->HandleInfoOffset ? NULL: ((PCHAR)(objhdr) - (objhdr)->HandleInfoOffset)))
  
#define HEADER_TO_CREATOR_INFO(objhdr) ((POBJECT_HEADER_CREATOR_INFO)          \
  (!((objhdr)->Flags & OB_FLAG_CREATOR_INFO) ? NULL: ((PCHAR)(objhdr) - sizeof(OBJECT_HEADER_CREATOR_INFO))))

#define OBJECT_ALLOC_SIZE(ObjectSize) ((ObjectSize)+sizeof(OBJECT_HEADER))

#define KERNEL_HANDLE_FLAG (1 << ((sizeof(HANDLE) * 8) - 1))
#define ObIsKernelHandle(Handle, ProcessorMode)                                \
  (((ULONG_PTR)(Handle) & KERNEL_HANDLE_FLAG) &&                               \
   ((ProcessorMode) == KernelMode))
#define ObKernelHandleToHandle(Handle)                                         \
  (HANDLE)((ULONG_PTR)(Handle) & ~KERNEL_HANDLE_FLAG)
#define ObMarkHandleAsKernelHandle(Handle)                                     \
  (HANDLE)((ULONG_PTR)(Handle) | KERNEL_HANDLE_FLAG)

extern POBJECT_DIRECTORY NameSpaceRoot;
extern POBJECT_TYPE ObSymbolicLinkType;
extern PHANDLE_TABLE ObpKernelHandleTable;

typedef NTSTATUS
(NTAPI *OB_ROS_CREATE_METHOD)(
    PVOID ObjectBody,
    PVOID Parent,
    PWSTR RemainingPath,
    struct _OBJECT_ATTRIBUTES* ObjectAttributes
);

typedef PVOID
(NTAPI *OB_ROS_FIND_METHOD)(
    PVOID  WinStaObject,
    PWSTR  Name,
    ULONG  Attributes
);

typedef NTSTATUS
(NTAPI *OB_ROS_PARSE_METHOD)(
    PVOID Object,
    PVOID *NextObject,
    PUNICODE_STRING FullPath,
    PWSTR *Path,
    ULONG Attributes,
    POBP_LOOKUP_CONTEXT Context
);

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
    POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    PUNICODE_STRING ObjectName,
    PVOID* ReturnedObject,
    PUNICODE_STRING RemainingPath,
    POBJECT_TYPE ObjectType,
    POBP_LOOKUP_CONTEXT Context
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
