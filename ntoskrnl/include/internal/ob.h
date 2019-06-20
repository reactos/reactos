/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/internal/ob.h
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
#define OBTRACE(x, fmt, ...) DPRINT(fmt, ##__VA_ARGS__)
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
// Handle Bit Flags
//
#define OBJ_PROTECT_CLOSE                               0x01
//#define OBJ_INHERIT                                   0x02
#define OBJ_AUDIT_OBJECT_CLOSE                          0x04
#define OBJ_HANDLE_ATTRIBUTES                           (OBJ_PROTECT_CLOSE |\
                                                         OBJ_INHERIT |      \
                                                         OBJ_AUDIT_OBJECT_CLOSE)

//
// Identifies a Kernel Handle
//
#ifdef _WIN64
#define KERNEL_HANDLE_FLAG 0xFFFFFFFF80000000ULL
#else
#define KERNEL_HANDLE_FLAG 0x80000000
#endif
#define ObpIsKernelHandle(Handle, ProcessorMode)        \
    ((((ULONG_PTR)(Handle) & KERNEL_HANDLE_FLAG) == KERNEL_HANDLE_FLAG) && \
     ((ProcessorMode) == KernelMode) && \
     ((Handle) != NtCurrentProcess()) && \
     ((Handle) != NtCurrentThread()))

//
// Converts to and from a Kernel Handle to a normal handle
//
#define ObKernelHandleToHandle(Handle)                  \
    (HANDLE)((ULONG_PTR)(Handle) & ~KERNEL_HANDLE_FLAG)
#define ObMarkHandleAsKernelHandle(Handle)              \
    (HANDLE)((ULONG_PTR)(Handle) | KERNEL_HANDLE_FLAG)

//
// Converts from an EXHANDLE object to a POBJECT_HEADER
//
#define ObpGetHandleObject(x)                           \
    ((POBJECT_HEADER)((ULONG_PTR)x->Object & ~OBJ_HANDLE_ATTRIBUTES))

//
// Recovers the security descriptor from a cached security descriptor header
//
#define ObpGetHeaderForSd(x) \
    CONTAINING_RECORD((x), SECURITY_DESCRIPTOR_HEADER, SecurityDescriptor)

//
// Recovers the security descriptor from a cached security descriptor list entry
//
#define ObpGetHeaderForEntry(x) \
    CONTAINING_RECORD((x), SECURITY_DESCRIPTOR_HEADER, Link)

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

typedef struct _OBP_FIND_HANDLE_DATA
{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    POBJECT_HANDLE_INFORMATION HandleInformation;
} OBP_FIND_HANDLE_DATA, *POBP_FIND_HANDLE_DATA;

//
// Cached Security Descriptor Header
//
typedef struct _SECURITY_DESCRIPTOR_HEADER
{
    LIST_ENTRY Link;
    ULONG RefCount;
    ULONG FullHash;
    QUAD SecurityDescriptor;
} SECURITY_DESCRIPTOR_HEADER, *PSECURITY_DESCRIPTOR_HEADER;

//
// Cached Security Descriptor List
//
typedef struct _OB_SD_CACHE_LIST
{
    EX_PUSH_LOCK PushLock;
    LIST_ENTRY Head;
} OB_SD_CACHE_LIST, *POB_SD_CACHE_LIST;

//
// Structure for quick-compare of a DOS Device path
//
typedef union
{
    WCHAR Name[sizeof(ULARGE_INTEGER) / sizeof(WCHAR)];
    ULARGE_INTEGER Alignment;
} ALIGNEDNAME;

//
// Private Temporary Buffer for Lookup Routines
//
#define TAG_OB_TEMP_STORAGE 'tSbO'
typedef struct _OB_TEMP_BUFFER
{
    ACCESS_STATE LocalAccessState;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;
    OBP_LOOKUP_CONTEXT LookupContext;
    AUX_ACCESS_DATA AuxData;
} OB_TEMP_BUFFER, *POB_TEMP_BUFFER;

//
// Startup and Shutdown Functions
//
INIT_FUNCTION
BOOLEAN
NTAPI
ObInitSystem(
    VOID
);

VOID
NTAPI
ObShutdownSystem(
    VOID
);

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

VOID
NTAPI
ObpCreateSymbolicLinkName(
    IN POBJECT_SYMBOLIC_LINK SymbolicLink
);

VOID
NTAPI
ObpDeleteSymbolicLinkName(
    IN POBJECT_SYMBOLIC_LINK SymbolicLink
);

//
// Process/Handle Table Init/Rundown
//
NTSTATUS
NTAPI
ObInitProcess(
    IN PEPROCESS Parent OPTIONAL,
    IN PEPROCESS Process
);

PHANDLE_TABLE
NTAPI
ObReferenceProcessHandleTable(
    IN PEPROCESS Process
);

VOID
NTAPI
ObDereferenceProcessHandleTable(
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
ObpLookupObjectName(
    IN HANDLE RootHandle OPTIONAL,
    IN OUT PUNICODE_STRING ObjectName,
    IN ULONG Attributes,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PVOID ParseContext,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    IN PVOID InsertObject OPTIONAL,
    IN OUT PACCESS_STATE AccessState,
    OUT POBP_LOOKUP_CONTEXT LookupContext,
    OUT PVOID *FoundObject
);

//
// Object Attribute Functions
//
BOOLEAN
NTAPI
ObpSetHandleAttributes(
    IN OUT PHANDLE_TABLE_ENTRY HandleTableEntry,
    IN ULONG_PTR Context
);

NTSTATUS
NTAPI
ObQueryDeviceMapInformation(
    IN PEPROCESS Process,
    OUT PPROCESS_DEVICEMAP_INFORMATION DeviceMapInfo,
    IN ULONG Flags
);

//
// Object Lifetime Functions
//
VOID
NTAPI
ObpDeleteObject(
    IN PVOID Object,
    IN BOOLEAN CalledFromWorkerThread
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

VOID
NTAPI
ObFreeObjectCreateInfoBuffer(
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo
);

VOID
NTAPI
ObpFreeObjectNameBuffer(
    IN PUNICODE_STRING Name
);

VOID
NTAPI
ObpDeleteObjectType(
    IN PVOID Object
);

NTSTATUS
NTAPI
ObReferenceFileObjectForWrite(
    IN HANDLE Handle,
    IN KPROCESSOR_MODE AccessMode,
    OUT PFILE_OBJECT *FileObject,
    OUT POBJECT_HANDLE_INFORMATION HandleInformation
);

//
// DOS Devices Functions
//
NTSTATUS
NTAPI
ObSetDeviceMap(
    IN PEPROCESS Process,
    IN HANDLE DirectoryHandle
);

NTSTATUS
NTAPI
ObSetDirectoryDeviceMap(OUT PDEVICE_MAP * DeviceMap,
                        IN HANDLE DirectoryHandle
);

VOID
NTAPI
ObDereferenceDeviceMap(
    IN PEPROCESS Process
);

VOID
FASTCALL
ObfDereferenceDeviceMap(
    IN PDEVICE_MAP DeviceMap
);

VOID
NTAPI
ObInheritDeviceMap(
    IN PEPROCESS Parent,
    IN PEPROCESS Process
);

INIT_FUNCTION
NTSTATUS
NTAPI
ObpCreateDosDevicesDirectory(
    VOID
);

ULONG
NTAPI
ObIsLUIDDeviceMapsEnabled(
    VOID
);

PDEVICE_MAP
NTAPI
ObpReferenceDeviceMap(
    VOID
);

//
// Security descriptor cache functions
//
INIT_FUNCTION
NTSTATUS
NTAPI
ObpInitSdCache(
    VOID
);

PSECURITY_DESCRIPTOR
NTAPI
ObpReferenceSecurityDescriptor(
    IN POBJECT_HEADER ObjectHeader
);

//
// Object Security Routines
//
BOOLEAN
NTAPI
ObCheckObjectAccess(
    IN PVOID Object,
    IN OUT PACCESS_STATE AccessState,
    IN BOOLEAN LockHeld,
    IN KPROCESSOR_MODE AccessMode,
    OUT PNTSTATUS ReturnedStatus
);

BOOLEAN
NTAPI
ObCheckCreateObjectAccess(
    IN PVOID Object,
    IN ACCESS_MASK CreateAccess,
    IN PACCESS_STATE AccessState,
    IN PUNICODE_STRING ComponentName,
    IN BOOLEAN LockHeld,
    IN KPROCESSOR_MODE AccessMode,
    OUT PNTSTATUS AccessStatus
);

BOOLEAN
NTAPI
ObpCheckTraverseAccess(
    IN PVOID Object,
    IN ACCESS_MASK TraverseAccess,
    IN PACCESS_STATE AccessState OPTIONAL,
    IN BOOLEAN LockHeld,
    IN KPROCESSOR_MODE AccessMode,
    OUT PNTSTATUS AccessStatus
);

BOOLEAN
NTAPI
ObpCheckObjectReference(
    IN PVOID Object,
    IN OUT PACCESS_STATE AccessState,
    IN BOOLEAN LockHeld,
    IN KPROCESSOR_MODE AccessMode,
    OUT PNTSTATUS AccessStatus
);

//
// Default Object Security Callback Routines
//
NTSTATUS
NTAPI
ObAssignObjectSecurityDescriptor(
    IN PVOID Object,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    IN POOL_TYPE PoolType
);

NTSTATUS
NTAPI
ObDeassignSecurity(
    IN OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
);

NTSTATUS
NTAPI
ObQuerySecurityDescriptorInfo(
    IN PVOID Object,
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG Length,
    IN PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
);

NTSTATUS
NTAPI
ObSetSecurityDescriptorInfo(
    IN PVOID Object,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *OutputSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
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
ObpCaptureObjectCreateInformation(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN KPROCESSOR_MODE AccessMode,
    IN KPROCESSOR_MODE CreatorMode,
    IN BOOLEAN AllocateFromLookaside,
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    OUT PUNICODE_STRING ObjectName
);

//
// Miscellanea
//
ULONG
NTAPI
ObGetProcessHandleCount(
    IN PEPROCESS Process
);

//
// Global data inside the Object Manager
//
extern ULONG ObpTraceLevel;
extern KEVENT ObpDefaultObject;
extern KGUARDED_MUTEX ObpDeviceMapLock;
extern POBJECT_TYPE ObpTypeObjectType;
extern POBJECT_TYPE ObpDirectoryObjectType;
extern POBJECT_TYPE ObpSymbolicLinkObjectType;
extern POBJECT_DIRECTORY ObpRootDirectoryObject;
extern POBJECT_DIRECTORY ObpTypeDirectoryObject;
extern PHANDLE_TABLE ObpKernelHandleTable;
extern WORK_QUEUE_ITEM ObpReaperWorkItem;
extern volatile PVOID ObpReaperList;
extern GENERAL_LOOKASIDE ObpNameBufferLookasideList, ObpCreateInfoLookasideList;
extern BOOLEAN IoCountOperations;
extern ALIGNEDNAME ObpDosDevicesShortNamePrefix;
extern ALIGNEDNAME ObpDosDevicesShortNameRoot;
extern UNICODE_STRING ObpDosDevicesShortName;
extern WCHAR ObpUnsecureGlobalNamesBuffer[128];
extern ULONG ObpUnsecureGlobalNamesLength;
extern ULONG ObpObjectSecurityMode;
extern ULONG ObpProtectionMode;
extern ULONG ObpLUIDDeviceMapsDisabled;
extern ULONG ObpLUIDDeviceMapsEnabled;

//
// Inlined Functions
//
#include "ob_x.h"
