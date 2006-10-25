/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/ob.h
* PURPOSE:         Internal header for the Object Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

//
// Define this if you want debugging support
//
#define _OB_DEBUG_                                      0x00

//
// These define the Debug Masks Supported
//
#define OB_HANDLE_DEBUG                                 0x01
#define OB_NAMESPACE_DEBUG                              0x02
#define OB_SECURITY_DEBUG                               0x04
#define OB_REFERENCE_DEBUG                              0x08
#define OB_CALLBACK_DEBUG                               0x10

//
// Debug/Tracing support
//
#if _OB_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define OBTRACE DbgPrintEx
#else
#define OBTRACE(x, ...)                                 \
    if (x & ObpTraceLevel) DbgPrint(__VA_ARGS__)
#endif
#else
#define OBTRACE(x, ...) DPRINT(__VA_ARGS__)
#endif

//
// Mask to detect GENERIC_XXX access masks being used
//
#define GENERIC_ACCESS                                  \
    (GENERIC_READ    |                                  \
     GENERIC_WRITE   |                                  \
     GENERIC_EXECUTE |                                  \
     GENERIC_ALL)

//
// Identifies a Kernel Handle
//
#define KERNEL_HANDLE_FLAG                              \
    (1 << ((sizeof(HANDLE) * 8) - 1))
#define ObIsKernelHandle(Handle, ProcessorMode)         \
    (((ULONG_PTR)(Handle) & KERNEL_HANDLE_FLAG) &&      \
    ((ProcessorMode) == KernelMode))

//
// Converts to and from a Kernel Handle to a normal handle
//
#define ObKernelHandleToHandle(Handle)                  \
    (HANDLE)((ULONG_PTR)(Handle) & ~KERNEL_HANDLE_FLAG)
#define ObMarkHandleAsKernelHandle(Handle)              \
    (HANDLE)((ULONG_PTR)(Handle) | KERNEL_HANDLE_FLAG)

//
// Returns the number of handles in a handle table
//
#define ObpGetHandleCountByHandleTable(HandleTable)     \
    ((PHANDLE_TABLE)HandleTable)->HandleCount

//
// Context Structures for Ex*Handle Callbacks
//
typedef struct _OBP_SET_HANDLE_ATTRIBUTES_CONTEXT
{
    KPROCESSOR_MODE PreviousMode;
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION Information;
} OBP_SET_HANDLE_ATTRIBUTES_CONTEXT, *POBP_SET_HANDLE_ATTRIBUTES_CONTEXT;
typedef struct _OBP_CLOSE_HANDLE_CONTEXT
{
    PHANDLE_TABLE HandleTable;
    KPROCESSOR_MODE AccessMode;
} OBP_CLOSE_HANDLE_CONTEXT, *POBP_CLOSE_HANDLE_CONTEXT;

//
// Directory Namespace Functions
//
BOOLEAN
NTAPI
ObpDeleteEntryDirectory(
    IN POBP_LOOKUP_CONTEXT Context
);

BOOLEAN
NTAPI
ObpInsertEntryDirectory(
    IN POBJECT_DIRECTORY Parent,
    IN POBP_LOOKUP_CONTEXT Context,
    IN POBJECT_HEADER ObjectHeader
);

PVOID
NTAPI
ObpLookupEntryDirectory(
    IN POBJECT_DIRECTORY Directory,
    IN PUNICODE_STRING Name,
    IN ULONG Attributes,
    IN UCHAR SearchShadow,
    IN POBP_LOOKUP_CONTEXT Context
);

//
// Symbolic Link Functions
//
VOID
NTAPI
ObpDeleteSymbolicLink(
    IN PVOID ObjectBody
);

NTSTATUS
NTAPI
ObpParseSymbolicLink(
    IN PVOID ParsedObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING FullPath,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *NextObject
);

//
// Process/Handle Table Init/Rundown
//
NTSTATUS
NTAPI
ObpCreateHandleTable(
    IN PEPROCESS Parent,
    IN PEPROCESS Process
);

VOID
NTAPI
ObKillProcess(
    IN PEPROCESS Process
);

//
// Object Lookup Functions
//
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

//
// Object Attribute Functions
//
BOOLEAN
NTAPI
ObpSetHandleAttributes(
    IN PHANDLE_TABLE HandleTable,
    IN OUT PHANDLE_TABLE_ENTRY HandleTableEntry,
    IN PVOID Context
);

VOID
NTAPI
ObQueryDeviceMapInformation(
    IN PEPROCESS Process,
    OUT PPROCESS_DEVICEMAP_INFORMATION DeviceMapInfo
);

//
// Object Lifetime Functions
//
VOID
FASTCALL
ObpDeleteObject(
    IN PVOID Object
);

LONG
FASTCALL
ObDereferenceObjectEx(
    IN PVOID Object,
    IN LONG Count
);

LONG
FASTCALL
ObReferenceObjectEx(
    IN PVOID Object,
    IN LONG Count
);

BOOLEAN
FASTCALL
ObReferenceObjectSafe(
    IN PVOID Object
);

VOID
NTAPI
ObpReapObject(
    IN PVOID Unused
);

VOID
FASTCALL
ObpSetPermanentObject(
    IN PVOID ObjectBody,
    IN BOOLEAN Permanent
);

VOID
NTAPI
ObpDeleteNameCheck(
    IN PVOID Object
);

VOID
NTAPI
ObClearProcessHandleTable(
    IN PEPROCESS Process
);

NTSTATUS
NTAPI
ObDuplicateObject(
    IN PEPROCESS SourceProcess,
    IN HANDLE SourceHandle,
    IN PEPROCESS TargetProcess OPTIONAL,
    IN PHANDLE TargetHandle OPTIONAL,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options,
    IN KPROCESSOR_MODE PreviousMode
);

//
// DOS Devices Functions
//
VOID
NTAPI
ObDereferenceDeviceMap(
    IN PEPROCESS Process
);

VOID
NTAPI
ObInheritDeviceMap(
    IN PEPROCESS Parent,
    IN PEPROCESS Process
);

NTSTATUS
NTAPI
ObpCreateDosDevicesDirectory(
    VOID
);

//
// Security descriptor cache functions
//
NTSTATUS
NTAPI
ObpInitSdCache(
    VOID
);

NTSTATUS
NTAPI
ObpAddSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SourceSD,
    OUT PSECURITY_DESCRIPTOR *DestinationSD
);

NTSTATUS
NTAPI
ObpRemoveSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

PSECURITY_DESCRIPTOR
NTAPI
ObpReferenceCachedSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

VOID
NTAPI
ObpDereferenceCachedSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

BOOLEAN
NTAPI
ObCheckObjectAccess(
    IN PVOID Object,
    IN OUT PACCESS_STATE AccessState,
    IN BOOLEAN Unknown,
    IN KPROCESSOR_MODE AccessMode,
    OUT PNTSTATUS ReturnedStatus
);

//
// Executive Fast Referencing Functions
//
VOID
FASTCALL
ObInitializeFastReference(
    IN PEX_FAST_REF FastRef,
    IN PVOID Object
);

PVOID
FASTCALL
ObFastReplaceObject(
    IN PEX_FAST_REF FastRef,
    IN PVOID Object
);

PVOID
FASTCALL
ObFastReferenceObject(
    IN PEX_FAST_REF FastRef
);

PVOID
FASTCALL
ObFastReferenceObjectLocked(
    IN PEX_FAST_REF FastRef
);

VOID
FASTCALL
ObFastDereferenceObject(
    IN PEX_FAST_REF FastRef,
    IN PVOID Object
);

//
// Object Create and Object Name Capture Functions
//
NTSTATUS
NTAPI
ObpCaptureObjectName(
    IN PUNICODE_STRING CapturedName,
    IN PUNICODE_STRING ObjectName,
    IN KPROCESSOR_MODE AccessMode,
    IN BOOLEAN AllocateFromLookaside
);

NTSTATUS
NTAPI
ObpCaptureObjectAttributes(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN KPROCESSOR_MODE AccessMode,
    IN BOOLEAN AllocateFromLookaside,
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    OUT PUNICODE_STRING ObjectName
);

//
// Global data inside the Object Manager
//
extern ULONG ObpTraceLevel;
extern KEVENT ObpDefaultObject;
extern POBJECT_TYPE ObpTypeObjectType;
extern POBJECT_TYPE ObSymbolicLinkType;
extern POBJECT_TYPE ObTypeObjectType;
extern POBJECT_DIRECTORY NameSpaceRoot;
extern POBJECT_DIRECTORY ObpTypeDirectoryObject;
extern PHANDLE_TABLE ObpKernelHandleTable;
extern WORK_QUEUE_ITEM ObpReaperWorkItem;
extern volatile PVOID ObpReaperList;
extern NPAGED_LOOKASIDE_LIST ObpNmLookasideList, ObpCiLookasideList;
extern BOOLEAN IoCountOperations;

//
// Inlined Functions
//
#include "ob_x.h"
