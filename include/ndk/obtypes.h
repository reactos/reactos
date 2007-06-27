/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    obtypes.h

Abstract:

    Type definitions for the Object Manager

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _OBTYPES_H
#define _OBTYPES_H

#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WS03SP1

//
// Dependencies
//
#include <umtypes.h>
#ifndef NTOS_MODE_USER
#include <extypes.h>
#endif

#ifdef NTOS_MODE_USER
//
// Definitions for Object Creation
//
#define OBJ_INHERIT                             0x00000002L
#define OBJ_PERMANENT                           0x00000010L
#define OBJ_EXCLUSIVE                           0x00000020L
#define OBJ_CASE_INSENSITIVE                    0x00000040L
#define OBJ_OPENIF                              0x00000080L
#define OBJ_OPENLINK                            0x00000100L
#define OBJ_KERNEL_HANDLE                       0x00000200L
#define OBJ_FORCE_ACCESS_CHECK                  0x00000400L
#define OBJ_VALID_ATTRIBUTES                    0x000007F2L

#define InitializeObjectAttributes(p,n,a,r,s) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES);    \
    (p)->RootDirectory = (r);                   \
    (p)->Attributes = (a);                      \
    (p)->ObjectName = (n);                      \
    (p)->SecurityDescriptor = (s);              \
    (p)->SecurityQualityOfService = NULL;       \
}

//
// Number of custom-defined bits that can be attached to a handle
//
#define OBJ_HANDLE_TAGBITS                      0x3

//
// Directory Object Access Rights
//
#define DIRECTORY_QUERY                         0x0001
#define DIRECTORY_TRAVERSE                      0x0002
#define DIRECTORY_CREATE_OBJECT                 0x0004
#define DIRECTORY_CREATE_SUBDIRECTORY           0x0008
#define DIRECTORY_ALL_ACCESS                    (STANDARD_RIGHTS_REQUIRED | 0xF)

//
// Slash separator used in the OB Namespace (and Registry)
//
#define OBJ_NAME_PATH_SEPARATOR                 L'\\'

#else

//
// Object Flags
//
#define OB_FLAG_CREATE_INFO                     0x01
#define OB_FLAG_KERNEL_MODE                     0x02
#define OB_FLAG_CREATOR_INFO                    0x04
#define OB_FLAG_EXCLUSIVE                       0x08
#define OB_FLAG_PERMANENT                       0x10
#define OB_FLAG_SECURITY                        0x20
#define OB_FLAG_SINGLE_PROCESS                  0x40
#define OB_FLAG_DEFER_DELETE                    0x80

#define OBJECT_TO_OBJECT_HEADER(o)                          \
    CONTAINING_RECORD((o), OBJECT_HEADER, Body)

#define OBJECT_HEADER_TO_NAME_INFO(h)                       \
    ((POBJECT_HEADER_NAME_INFO)(!(h)->NameInfoOffset ?      \
        NULL: ((PCHAR)(h) - (h)->NameInfoOffset)))

#define OBJECT_HEADER_TO_HANDLE_INFO(h)                     \
    ((POBJECT_HEADER_HANDLE_INFO)(!(h)->HandleInfoOffset ?  \
        NULL: ((PCHAR)(h) - (h)->HandleInfoOffset)))

#define OBJECT_HEADER_TO_QUOTA_INFO(h)                      \
    ((POBJECT_HEADER_QUOTA_INFO)(!(h)->QuotaInfoOffset ?    \
        NULL: ((PCHAR)(h) - (h)->QuotaInfoOffset)))

#define OBJECT_HEADER_TO_CREATOR_INFO(h)                    \
    ((POBJECT_HEADER_CREATOR_INFO)(!((h)->Flags &           \
        OB_FLAG_CREATOR_INFO) ? NULL: ((PCHAR)(h) -         \
        sizeof(OBJECT_HEADER_CREATOR_INFO))))

#define OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(h)               \
    ((!((h)->Flags & OB_FLAG_EXCLUSIVE)) ?                  \
        NULL: (((POBJECT_HEADER_QUOTA_INFO)((PCHAR)(h) -    \
        (h)->QuotaInfoOffset))->ExclusiveProcess))

//
// Reasons for Open Callback
//
typedef enum _OB_OPEN_REASON
{
    ObCreateHandle,
    ObOpenHandle,
    ObDuplicateHandle,
    ObInheritHandle,
    ObMaxOpenReason
} OB_OPEN_REASON;

#endif

//
// Object Duplication Flags
//
#define DUPLICATE_SAME_ATTRIBUTES               0x00000004

//
// Number of hash entries in an Object Directory
//
#define NUMBER_HASH_BUCKETS                     37

//
// Types for DosDeviceDriveType
//
#define DOSDEVICE_DRIVE_UNKNOWN                 0
#define DOSDEVICE_DRIVE_CALCULATE               1
#define DOSDEVICE_DRIVE_REMOVABLE               2
#define DOSDEVICE_DRIVE_FIXED                   3
#define DOSDEVICE_DRIVE_REMOTE                  4
#define DOSDEVICE_DRIVE_CDROM                   5
#define DOSDEVICE_DRIVE_RAMDISK                 6

//
// Object Information Classes for NtQueryInformationObject
//
typedef enum _OBJECT_INFORMATION_CLASS
{
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllTypesInformation,
    ObjectHandleInformation
} OBJECT_INFORMATION_CLASS;

//
// Dump Control Structure for Object Debugging
//
typedef struct _OB_DUMP_CONTROL
{
    PVOID Stream;
    ULONG Detail;
} OB_DUMP_CONTROL, *POB_DUMP_CONTROL;

#ifndef NTOS_MODE_USER

//
// Object Type Callbacks
//
typedef VOID
(NTAPI *OB_DUMP_METHOD)(
    IN PVOID Object,
    IN POB_DUMP_CONTROL Control OPTIONAL
);

typedef NTSTATUS
(NTAPI *OB_OPEN_METHOD)(
    IN OB_OPEN_REASON Reason,
    IN PEPROCESS Process OPTIONAL,
    IN PVOID ObjectBody,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG HandleCount
);

typedef VOID
(NTAPI *OB_CLOSE_METHOD)(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
);

typedef VOID
(NTAPI *OB_DELETE_METHOD)(
    IN PVOID Object
);

typedef NTSTATUS
(NTAPI *OB_PARSE_METHOD)(
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

typedef NTSTATUS
(NTAPI *OB_SECURITY_METHOD)(
    IN PVOID Object,
    IN SECURITY_OPERATION_CODE OperationType,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG CapturedLength,
    IN OUT PSECURITY_DESCRIPTOR *ObjectSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
);

typedef NTSTATUS
(NTAPI *OB_QUERYNAME_METHOD)(
    IN PVOID Object,
    IN BOOLEAN HasObjectName,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength,
    IN KPROCESSOR_MODE AccessMode
);

typedef NTSTATUS
(NTAPI *OB_OKAYTOCLOSE_METHOD)(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN HANDLE Handle,
    IN KPROCESSOR_MODE AccessMode
);

#else

//
// Object Information Types for NtQueryInformationObject
//
typedef struct _OBJECT_NAME_INFORMATION
{
    UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

#endif

typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFORMATION
{
    BOOLEAN Inherit;
    BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_ATTRIBUTE_INFORMATION, *POBJECT_HANDLE_ATTRIBUTE_INFORMATION;

typedef struct _OBJECT_DIRECTORY_INFORMATION
{
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

//
// Object Type Information
//
typedef struct _OBJECT_TYPE_INFORMATION
{
    UNICODE_STRING TypeName;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage;
    ULONG TotalNonPagedPoolUsage;
    ULONG TotalNamePoolUsage;
    ULONG TotalHandleTableUsage;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage;
    ULONG HighWaterNonPagedPoolUsage;
    ULONG HighWaterNamePoolUsage;
    ULONG HighWaterHandleTableUsage;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    ULONG PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

#ifndef NTOS_MODE_USER

typedef struct _OBJECT_BASIC_INFORMATION
{
    ULONG Attributes;
    ACCESS_MASK GrantedAccess;
    ULONG HandleCount;
    ULONG PointerCount;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
    ULONG Reserved[3];
    ULONG NameInformationLength;
    ULONG TypeInformationLength;
    ULONG SecurityDescriptorLength;
    LARGE_INTEGER CreateTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

typedef struct _OBJECT_CREATE_INFORMATION
{
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

//
// Object Type Initialize for ObCreateObjectType
//
typedef struct _OBJECT_TYPE_INITIALIZER
{
    USHORT Length;
    BOOLEAN UseDefaultObject;
    BOOLEAN CaseInsensitive;
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

//
// Object Type Object
//
typedef struct _OBJECT_TYPE
{
    ERESOURCE Mutex;
    LIST_ENTRY TypeList;
    UNICODE_STRING Name;
    PVOID DefaultObject;
    ULONG Index;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    OBJECT_TYPE_INITIALIZER TypeInfo;
    ULONG Key;
    ERESOURCE ObjectLocks[4];
} OBJECT_TYPE;

//
// Object Directory Structures
//
typedef struct _OBJECT_DIRECTORY_ENTRY
{
    struct _OBJECT_DIRECTORY_ENTRY *ChainLink;
    PVOID Object;
#if (NTDDI_VERSION >= NTDDI_WS03)
    ULONG HashValue;
#endif
} OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;

typedef struct _OBJECT_DIRECTORY
{
    struct _OBJECT_DIRECTORY_ENTRY *HashBuckets[NUMBER_HASH_BUCKETS];
#if (NTDDI_VERSION < NTDDI_WINXP)
    ERESOURCE Lock;
#else
    EX_PUSH_LOCK Lock;
#endif
#if (NTDDI_VERSION < NTDDI_WINXP)
    BOOLEAN CurrentEntryValid;
#else
    struct _DEVICE_MAP *DeviceMap;
#endif
    ULONG SessionId;
#if (NTDDI_VERSION == NTDDI_WINXP)
    USHORT Reserved;
    USHORT SymbolicLinkUsageCount;
#endif
} OBJECT_DIRECTORY, *POBJECT_DIRECTORY;

//
// Object Header Addon Information
//
typedef struct _OBJECT_HEADER_NAME_INFO
{
    POBJECT_DIRECTORY Directory;
    UNICODE_STRING Name;
    ULONG QueryReferences;
    ULONG Reserved2;
    ULONG DbgReferenceCount;
} OBJECT_HEADER_NAME_INFO, *POBJECT_HEADER_NAME_INFO;

typedef struct _OBJECT_HANDLE_COUNT_ENTRY
{
    struct _EPROCESS *Process;
    ULONG HandleCount;
} OBJECT_HANDLE_COUNT_ENTRY, *POBJECT_HANDLE_COUNT_ENTRY;

typedef struct _OBJECT_HANDLE_COUNT_DATABASE
{
    ULONG CountEntries;
    OBJECT_HANDLE_COUNT_ENTRY HandleCountEntries[1];
} OBJECT_HANDLE_COUNT_DATABASE, *POBJECT_HANDLE_COUNT_DATABASE;

typedef struct _OBJECT_HEADER_HANDLE_INFO
{
    union
    {
        POBJECT_HANDLE_COUNT_DATABASE HandleCountDatabase;
        OBJECT_HANDLE_COUNT_ENTRY SingleEntry;
    };
} OBJECT_HEADER_HANDLE_INFO, *POBJECT_HEADER_HANDLE_INFO;

typedef struct _OBJECT_HEADER_CREATOR_INFO
{
    LIST_ENTRY TypeList;
    PVOID CreatorUniqueProcess;
    USHORT CreatorBackTraceIndex;
    USHORT Reserved;
} OBJECT_HEADER_CREATOR_INFO, *POBJECT_HEADER_CREATOR_INFO;

typedef struct _OBJECT_HEADER_QUOTA_INFO
{
    ULONG PagedPoolCharge;
    ULONG NonPagedPoolCharge;
    ULONG SecurityDescriptorCharge;
    PEPROCESS ExclusiveProcess;
} OBJECT_HEADER_QUOTA_INFO, *POBJECT_HEADER_QUOTA_INFO;

//
// Object Header
//
typedef struct _OBJECT_HEADER
{
    LONG PointerCount;
    union
    {
        LONG HandleCount;
        volatile PVOID NextToFree;
    };
    POBJECT_TYPE Type;
    UCHAR NameInfoOffset;
    UCHAR HandleInfoOffset;
    UCHAR QuotaInfoOffset;
    UCHAR Flags;
    union
    {
        POBJECT_CREATE_INFORMATION ObjectCreateInfo;
        PVOID QuotaBlockCharged;
    };
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    QUAD Body;
} OBJECT_HEADER, *POBJECT_HEADER;

//
// Object Lookup Context
//
typedef struct _OBP_LOOKUP_CONTEXT
{
    POBJECT_DIRECTORY Directory;
    PVOID Object;
    ULONG HashValue;
    USHORT HashIndex;
    BOOLEAN DirectoryLocked;
    ULONG LockStateSignature;
} OBP_LOOKUP_CONTEXT, *POBP_LOOKUP_CONTEXT;

//
// Device Map
//
typedef struct _DEVICE_MAP
{
    POBJECT_DIRECTORY DosDevicesDirectory;
    POBJECT_DIRECTORY GlobalDosDevicesDirectory;
    ULONG ReferenceCount;
    ULONG DriveMap;
    UCHAR DriveType[32];
} DEVICE_MAP, *PDEVICE_MAP;

//
// Symbolic Link Object
//
typedef struct _OBJECT_SYMBOLIC_LINK
{
    LARGE_INTEGER CreationTime;
    UNICODE_STRING LinkTarget;
    UNICODE_STRING LinkTargetRemaining;
    PVOID LinkTargetObject;
    ULONG DosDeviceDriveIndex;
} OBJECT_SYMBOLIC_LINK, *POBJECT_SYMBOLIC_LINK;

//
// Kernel Exports
//
extern POBJECT_TYPE NTSYSAPI ObDirectoryType;
extern PDEVICE_MAP NTSYSAPI ObSystemDeviceMap;

#endif // !NTOS_MODE_USER

#endif // _OBTYPES_H
