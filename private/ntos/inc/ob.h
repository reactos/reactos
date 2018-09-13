/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ob.h

Abstract:

    This module contains the object manager structure public data
    structures and procedure prototypes to be used within the NT
    system.

Author:

    Steve Wood (stevewo) 28-Mar-1989

Revision History:

--*/

#ifndef _OB_
#define _OB_

//
// System Initialization procedure for OB subcomponent of NTOS
//
BOOLEAN
ObInitSystem( VOID );


NTSTATUS
ObInitProcess(
    PEPROCESS ParentProcess OPTIONAL,
    PEPROCESS NewProcess
    );

VOID
ObInitProcess2(
    PEPROCESS NewProcess
    );

VOID
ObKillProcess(
    BOOLEAN AcquireLock,
    PEPROCESS Process
    );

// begin_ntddk begin_wdm begin_nthal begin_ntifs
//
// Object Manager types
//

typedef struct _OBJECT_HANDLE_INFORMATION {
    ULONG HandleAttributes;
    ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;

// end_ntddk end_wdm end_nthal end_ntifs

typedef struct _OBJECT_DUMP_CONTROL {
    PVOID Stream;
    ULONG Detail;
} OB_DUMP_CONTROL, *POB_DUMP_CONTROL;

typedef VOID (*OB_DUMP_METHOD)(
    IN PVOID Object,
    IN POB_DUMP_CONTROL Control OPTIONAL
    );

typedef enum _OB_OPEN_REASON {
    ObCreateHandle,
    ObOpenHandle,
    ObDuplicateHandle,
    ObInheritHandle,
    ObMaxOpenReason
} OB_OPEN_REASON;


typedef VOID (*OB_OPEN_METHOD)(
    IN OB_OPEN_REASON OpenReason,
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG HandleCount
    );

typedef BOOLEAN (*OB_OKAYTOCLOSE_METHOD)(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN HANDLE Handle
    );

typedef VOID (*OB_CLOSE_METHOD)(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    );

typedef VOID (*OB_DELETE_METHOD)(
    IN PVOID Object
    );

typedef NTSTATUS (*OB_PARSE_METHOD)(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    );

typedef NTSTATUS (*OB_SECURITY_METHOD)(
    IN PVOID Object,
    IN SECURITY_OPERATION_CODE OperationCode,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG CapturedLength,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    );

typedef NTSTATUS (*OB_QUERYNAME_METHOD)(
    IN PVOID Object,
    IN BOOLEAN HasObjectName,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength
    );

/*

    A security method and its caller must obey the following w.r.t.
    capturing and probing parameters:

    For a query operation, the caller must pass a kernel space address for
    CapturedLength.  The caller should be able to assume that it points to
    valid data that will not change.  In addition, the SecurityDescriptor
    parameter (which will receive the result of the query operation) must
    be probed for write up to the length given in CapturedLength.  The
    security method itself must always write to the SecurityDescriptor
    buffer in a try clause in case the caller de-allocates it.

    For a set operation, the SecurityDescriptor parameter must have
    been captured via SeCaptureSecurityDescriptor.  This parameter is
    not optional, and therefore may not be NULL.

*/


//
// Object Type Structure
//

typedef struct _OBJECT_TYPE_INITIALIZER {
    USHORT Length;
    BOOLEAN UseDefaultObject;
    BOOLEAN Reserved;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    BOOLEAN MaintainTypeList;
    POOL_TYPE PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
    OB_DUMP_METHOD DumpProcedure;
    OB_OPEN_METHOD OpenProcedure;
    OB_CLOSE_METHOD CloseProcedure;
    OB_DELETE_METHOD DeleteProcedure;
    OB_PARSE_METHOD ParseProcedure;
    OB_SECURITY_METHOD SecurityProcedure;
    OB_QUERYNAME_METHOD QueryNameProcedure;
    OB_OKAYTOCLOSE_METHOD OkayToCloseProcedure;
} OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;

typedef struct _OBJECT_TYPE {
    ERESOURCE Mutex;
    LIST_ENTRY TypeList;
    UNICODE_STRING Name;            // Copy from object header for convenience
    PVOID DefaultObject;
    ULONG Index;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    OBJECT_TYPE_INITIALIZER TypeInfo;
#ifdef POOL_TAGGING
    ULONG Key;
#endif //POOL_TAGGING
} OBJECT_TYPE, *POBJECT_TYPE;

//
// Object Directory Structure
//

#define NUMBER_HASH_BUCKETS 37

typedef struct _OBJECT_DIRECTORY {
    struct _OBJECT_DIRECTORY_ENTRY *HashBuckets[ NUMBER_HASH_BUCKETS ];
    struct _OBJECT_DIRECTORY_ENTRY **LookupBucket;
    BOOLEAN LookupFound;
    USHORT SymbolicLinkUsageCount;
    struct _DEVICE_MAP *DeviceMap;
} OBJECT_DIRECTORY, *POBJECT_DIRECTORY;

//
// Object Directory Entry Structure
//
typedef struct _OBJECT_DIRECTORY_ENTRY {
    struct _OBJECT_DIRECTORY_ENTRY *ChainLink;
    PVOID Object;
} OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;


//
// Symbolic Link Object Structure
//

typedef struct _OBJECT_SYMBOLIC_LINK {
    LARGE_INTEGER CreationTime;
    UNICODE_STRING LinkTarget;
    UNICODE_STRING LinkTargetRemaining;
    PVOID LinkTargetObject;
    ULONG DosDeviceDriveIndex;  // 1-based index into KUSER_SHARED_DATA.DosDeviceDriveType
} OBJECT_SYMBOLIC_LINK, *POBJECT_SYMBOLIC_LINK;


//
// Device Map Structure
//

typedef struct _DEVICE_MAP {
    ULONG ReferenceCount;
    POBJECT_DIRECTORY DosDevicesDirectory;
    ULONG DriveMap;
    UCHAR DriveType[ 32 ];
} DEVICE_MAP, *PDEVICE_MAP;

extern PDEVICE_MAP ObSystemDeviceMap;

//
// Object Handle Count Database
//

typedef struct _OBJECT_HANDLE_COUNT_ENTRY {
    PEPROCESS Process;
    ULONG HandleCount;
} OBJECT_HANDLE_COUNT_ENTRY, *POBJECT_HANDLE_COUNT_ENTRY;

typedef struct _OBJECT_HANDLE_COUNT_DATABASE {
    ULONG CountEntries;
    OBJECT_HANDLE_COUNT_ENTRY HandleCountEntries[ 1 ];
} OBJECT_HANDLE_COUNT_DATABASE, *POBJECT_HANDLE_COUNT_DATABASE;

//
// Object Header Structure
//
// The SecurityQuotaCharged is the amount of quota charged to cover
// the GROUP and DISCRETIONARY ACL fields of the security descriptor
// only.  The OWNER and SYSTEM ACL fields get charged for at a fixed
// rate that may be less than or greater than the amount actually used.
//
// If the object has no security, then the SecurityQuotaCharged and the
// SecurityQuotaInUse fields are set to zero.
//
// Modification of the OWNER and SYSTEM ACL fields should never fail
// due to quota exceeded problems.  Modifications to the GROUP and
// DISCRETIONARY ACL fields may fail due to quota exceeded problems.
//
//


typedef struct _OBJECT_CREATE_INFORMATION {
    ULONG Attributes;
    HANDLE RootDirectory;
    PVOID ParseContext;
    KPROCESSOR_MODE ProbeMode;
    ULONG PagedPoolCharge;
    ULONG NonPagedPoolCharge;
    ULONG SecurityDescriptorCharge;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_QUALITY_OF_SERVICE SecurityQos;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
} OBJECT_CREATE_INFORMATION, *POBJECT_CREATE_INFORMATION;

typedef struct _OBJECT_HEADER {
    LONG PointerCount;
    union {
        LONG HandleCount;
        PSINGLE_LIST_ENTRY SEntry;
    };
    POBJECT_TYPE Type;
    UCHAR NameInfoOffset;
    UCHAR HandleInfoOffset;
    UCHAR QuotaInfoOffset;
    UCHAR Flags;
    union {
        POBJECT_CREATE_INFORMATION ObjectCreateInfo;
        PVOID QuotaBlockCharged;
    };

    PSECURITY_DESCRIPTOR SecurityDescriptor;
    QUAD Body;
} OBJECT_HEADER, *POBJECT_HEADER;

typedef struct _OBJECT_HEADER_QUOTA_INFO {
    ULONG PagedPoolCharge;
    ULONG NonPagedPoolCharge;
    ULONG SecurityDescriptorCharge;
    PEPROCESS ExclusiveProcess;
} OBJECT_HEADER_QUOTA_INFO, *POBJECT_HEADER_QUOTA_INFO;

typedef struct _OBJECT_HEADER_HANDLE_INFO {
    union {
        POBJECT_HANDLE_COUNT_DATABASE HandleCountDataBase;
        OBJECT_HANDLE_COUNT_ENTRY SingleEntry;
    };
} OBJECT_HEADER_HANDLE_INFO, *POBJECT_HEADER_HANDLE_INFO;

typedef struct _OBJECT_HEADER_NAME_INFO {
    POBJECT_DIRECTORY Directory;
    UNICODE_STRING Name;
    ULONG Reserved;
#if DBG
    ULONG Reserved2 ;
    LONG DbgDereferenceCount ;
#endif
} OBJECT_HEADER_NAME_INFO, *POBJECT_HEADER_NAME_INFO;

typedef struct _OBJECT_HEADER_CREATOR_INFO {
    LIST_ENTRY TypeList;
    HANDLE CreatorUniqueProcess;
    USHORT CreatorBackTraceIndex;
    USHORT Reserved;
} OBJECT_HEADER_CREATOR_INFO, *POBJECT_HEADER_CREATOR_INFO;

#define OB_FLAG_NEW_OBJECT              0x01
#define OB_FLAG_KERNEL_OBJECT           0x02
#define OB_FLAG_CREATOR_INFO            0x04
#define OB_FLAG_EXCLUSIVE_OBJECT        0x08
#define OB_FLAG_PERMANENT_OBJECT        0x10
#define OB_FLAG_DEFAULT_SECURITY_QUOTA  0x20
#define OB_FLAG_SINGLE_HANDLE_ENTRY     0x40

#define OBJECT_TO_OBJECT_HEADER( o ) \
    CONTAINING_RECORD( (o), OBJECT_HEADER, Body )

#define OBJECT_HEADER_TO_EXCLUSIVE_PROCESS( oh ) ((oh->Flags & OB_FLAG_EXCLUSIVE_OBJECT) == 0 ? \
    NULL : (((POBJECT_HEADER_QUOTA_INFO)((PCHAR)(oh) - (oh)->QuotaInfoOffset))->ExclusiveProcess))


#define OBJECT_HEADER_TO_QUOTA_INFO( oh ) ((POBJECT_HEADER_QUOTA_INFO) \
    ((oh)->QuotaInfoOffset == 0 ? NULL : ((PCHAR)(oh) - (oh)->QuotaInfoOffset)))

#define OBJECT_HEADER_TO_HANDLE_INFO( oh ) ((POBJECT_HEADER_HANDLE_INFO) \
    ((oh)->HandleInfoOffset == 0 ? NULL : ((PCHAR)(oh) - (oh)->HandleInfoOffset)))

#define OBJECT_HEADER_TO_NAME_INFO( oh ) ((POBJECT_HEADER_NAME_INFO) \
    ((oh)->NameInfoOffset == 0 ? NULL : ((PCHAR)(oh) - (oh)->NameInfoOffset)))

#define OBJECT_HEADER_TO_CREATOR_INFO( oh ) ((POBJECT_HEADER_CREATOR_INFO) \
    (((oh)->Flags & OB_FLAG_CREATOR_INFO) == 0 ? NULL : ((PCHAR)(oh) - sizeof(OBJECT_HEADER_CREATOR_INFO))))


NTKERNELAPI
NTSTATUS
ObCreateObjectType(
    IN PUNICODE_STRING TypeName,
    IN POBJECT_TYPE_INITIALIZER ObjectTypeInitializer,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    OUT POBJECT_TYPE *ObjectType
    );

VOID
FASTCALL
ObFreeObjectCreateInfoBuffer(
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo
    );

// begin_nthal

NTKERNELAPI
VOID
ObDeleteCapturedInsertInfo(
    IN PVOID Object
    );

NTKERNELAPI
NTSTATUS
ObCreateObject(
    IN KPROCESSOR_MODE ProbeMode,
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE OwnershipMode,
    IN OUT PVOID ParseContext OPTIONAL,
    IN ULONG ObjectBodySize,
    IN ULONG PagedPoolCharge,
    IN ULONG NonPagedPoolCharge,
    OUT PVOID *Object
    );


NTKERNELAPI
NTSTATUS
ObInsertObject(
    IN PVOID Object,
    IN PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN ULONG ObjectPointerBias,
    OUT PVOID *NewObject OPTIONAL,
    OUT PHANDLE Handle
    );

// end_nthal

NTKERNELAPI                                                     // ntddk wdm nthal ntifs
NTSTATUS                                                        // ntddk wdm nthal ntifs
ObReferenceObjectByHandle(                                      // ntddk wdm nthal ntifs
    IN HANDLE Handle,                                           // ntddk wdm nthal ntifs
    IN ACCESS_MASK DesiredAccess,                               // ntddk wdm nthal ntifs
    IN POBJECT_TYPE ObjectType OPTIONAL,                        // ntddk wdm nthal ntifs
    IN KPROCESSOR_MODE AccessMode,                              // ntddk wdm nthal ntifs
    OUT PVOID *Object,                                          // ntddk wdm nthal ntifs
    OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL   // ntddk wdm nthal ntifs
    );                                                          // ntddk wdm nthal ntifs


NTKERNELAPI
NTSTATUS
ObOpenObjectByName(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PHANDLE Handle
    );


NTKERNELAPI                                                     // ntifs
NTSTATUS                                                        // ntifs
ObOpenObjectByPointer(                                          // ntifs
    IN PVOID Object,                                            // ntifs
    IN ULONG HandleAttributes,                                  // ntifs
    IN PACCESS_STATE PassedAccessState OPTIONAL,                // ntifs
    IN ACCESS_MASK DesiredAccess OPTIONAL,                      // ntifs
    IN POBJECT_TYPE ObjectType OPTIONAL,                        // ntifs
    IN KPROCESSOR_MODE AccessMode,                              // ntifs
    OUT PHANDLE Handle                                          // ntifs
    );                                                          // ntifs

NTSTATUS
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

NTKERNELAPI                                                     // ntifs
VOID                                                            // ntifs
ObMakeTemporaryObject(                                          // ntifs
    IN PVOID Object                                             // ntifs
    );                                                          // ntifs


NTKERNELAPI
BOOLEAN
ObFindHandleForObject(
    IN PEPROCESS Process,
    IN PVOID Object,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN POBJECT_HANDLE_INFORMATION MatchCriteria OPTIONAL,
    OUT PHANDLE Handle
    );

// begin_ntddk begin_wdm begin_nthal begin_ntifs

#define ObDereferenceObject(a)                                     \
        ObfDereferenceObject(a)

//  end_wdm end_ntddk end_nthal end_ntifs

#if defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTSRV_) || defined(_NTHAL_)

//  begin_wdm begin_ntddk begin_nthal begin_ntifs
#define ObReferenceObject(Object) ObfReferenceObject(Object)

NTKERNELAPI
VOID
FASTCALL
ObfReferenceObject(
    IN PVOID Object
    );

//  end_wdm end_ntddk end_nthal end_ntifs
#else

#define ObReferenceObject(Object) {                                \
    POBJECT_HEADER ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object); \
    InterlockedIncrement(&ObjectHeader->PointerCount);             \
}

#endif

//  begin_wdm begin_ntddk begin_nthal begin_ntifs

NTKERNELAPI
NTSTATUS
ObReferenceObjectByPointer(
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode
    );

NTKERNELAPI
VOID
FASTCALL
ObfDereferenceObject(
    IN PVOID Object
    );

// end_ntddk end_wdm end_nthal end_ntifs

NTSTATUS
ObWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

NTKERNELAPI                                                     // ntifs
NTSTATUS                                                        // ntifs
ObQueryNameString(                                              // ntifs
    IN PVOID Object,                                            // ntifs
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,                // ntifs
    IN ULONG Length,                                            // ntifs
    OUT PULONG ReturnLength                                     // ntifs
    );                                                          // ntifs
                                                                // ntifs
NTKERNELAPI                                                     // ntifs
ULONG                                                           // ntifs
ObGetObjectPointerCount(                                        // ntifs
    IN PVOID Object                                             // ntifs
    );                                                          // ntifs

#if DBG
PUNICODE_STRING
ObGetObjectName(
    IN PVOID Object
    );
#endif // DBG

NTSTATUS
ObQueryTypeName(
    IN PVOID Object,
    PUNICODE_STRING ObjectTypeName,
    IN ULONG Length,
    OUT PULONG ReturnLength
    );

NTSTATUS
ObQueryTypeInfo(
    IN POBJECT_TYPE ObjectType,
    OUT POBJECT_TYPE_INFORMATION ObjectTypeInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength
    );

NTSTATUS
ObDumpObjectByHandle(
    IN HANDLE Handle,
    IN POB_DUMP_CONTROL Control OPTIONAL
    );


NTSTATUS
ObDumpObjectByPointer(
    IN PVOID Object,
    IN POB_DUMP_CONTROL Control OPTIONAL
    );

NTSTATUS
ObSetDeviceMap(
    IN PEPROCESS TargetProcess,
    IN HANDLE DirectoryHandle
    );

NTSTATUS
ObQueryDeviceMapInformation(
    IN PEPROCESS TargetProcess,
    OUT PPROCESS_DEVICEMAP_INFORMATION DeviceMapInformation
    );

VOID
ObInheritDeviceMap(
    IN PEPROCESS NewProcess,
    IN PEPROCESS ParentProcess
    );

VOID
ObDereferenceDeviceMap(
    IN PEPROCESS Process
    );

// begin_ntsrv begin_ntddk
NTSTATUS
ObGetObjectSecurity(
    IN PVOID Object,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
    OUT PBOOLEAN MemoryAllocated
    );

VOID
ObReleaseObjectSecurity(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN MemoryAllocated
    );
// end_ntsrv end_ntddk

NTSTATUS
ObAssignObjectSecurityDescriptor(
    IN PVOID Object,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    IN POOL_TYPE PoolType
    );

NTSTATUS
ObValidateSecurityQuota(
    IN PVOID Object,
    IN ULONG NewSize
    );

NTKERNELAPI
BOOLEAN
ObCheckCreateObjectAccess(
    IN PVOID DirectoryObject,
    IN ACCESS_MASK CreateAccess,
    IN PACCESS_STATE AccessState OPTIONAL,
    IN PUNICODE_STRING ComponentName,
    IN BOOLEAN TypeMutexLocked,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PNTSTATUS AccessStatus
   );

NTKERNELAPI
BOOLEAN
ObCheckObjectAccess(
    IN PVOID Object,
    IN PACCESS_STATE AccessState,
    IN BOOLEAN TypeMutexLocked,
    IN KPROCESSOR_MODE AccessMode,
    OUT PNTSTATUS AccessStatus
    );


NTKERNELAPI
NTSTATUS
ObAssignSecurity(
    IN PACCESS_STATE AccessState,
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PVOID Object,
    IN POBJECT_TYPE ObjectType
    );

NTSTATUS                                                        // ntifs
ObQueryObjectAuditingByHandle(                                  // ntifs
    IN HANDLE Handle,                                           // ntifs
    OUT PBOOLEAN GenerateOnClose                                // ntifs
    );                                                          // ntifs

NTSTATUS
ObSetSecurityObjectByPointer (
    IN PVOID Object,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

#if DEVL

typedef BOOLEAN (*OB_ENUM_OBJECT_TYPE_ROUTINE)(
    IN PVOID Object,
    IN PUNICODE_STRING ObjectName,
    IN ULONG HandleCount,
    IN ULONG PointerCount,
    IN PVOID Parameter
    );

NTSTATUS
ObEnumerateObjectsByType(
    IN POBJECT_TYPE ObjectType,
    IN OB_ENUM_OBJECT_TYPE_ROUTINE EnumerationRoutine,
    IN PVOID Parameter
    );

NTSTATUS
ObGetHandleInformation(
    OUT PSYSTEM_HANDLE_INFORMATION HandleInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSTATUS
ObGetObjectInformation(
    IN PCHAR UserModeBufferAddress,
    OUT PSYSTEM_OBJECTTYPE_INFORMATION ObjectInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    );

NTKERNELAPI
NTSTATUS
ObSetSecurityDescriptorInfo(
    IN PVOID Object,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    );

NTKERNELAPI
NTSTATUS
ObQuerySecurityDescriptorInfo(
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG Length,
    IN PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor
    );

NTSTATUS
ObDeassignSecurity (
    IN OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

#endif // DEVL

#endif // _OB_
