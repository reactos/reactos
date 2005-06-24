#ifndef _INCLUDE_DDK_OBTYPES_H
#define _INCLUDE_DDK_OBTYPES_H
/* $Id$ */
struct _DIRECTORY_OBJECT;
struct _OBJECT_ATTRIBUTES;

#ifndef __USE_W32API

typedef struct _OBJECT_HANDLE_INFORMATION
{
  ULONG HandleAttributes;
  ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;


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


typedef struct _OBJECT_NAME_INFORMATION
{
  UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;


typedef struct _OBJECT_TYPE_INFORMATION {
   UNICODE_STRING TypeName;
   ULONG TotalNumberOfObjects;
   ULONG TotalNumberOfHandles;
   ULONG TotalPagedPoolUsage;
   ULONG TotalNonPagedPoolUsage;
   ULONG TotalNamePoolUsage;
   ULONG TotalHandleTableUsage;
   ULONG PeakNumberOfObjects;
   ULONG PeakNumberOfHandles;
   ULONG PeakPagedPoolUsage;
   ULONG PeakNonPagedPoolUsage;
   ULONG PeakNamePoolUsage;
   ULONG PeakHandleTableUsage;
   ULONG InvalidAttributes;
   GENERIC_MAPPING GenericMapping;
   ULONG ValidAccessMask;
   BOOLEAN SecurityRequired;
   BOOLEAN MaintainHandleCount;
   ULONG PoolType;
   ULONG DefaultPagedPoolCharge;
   ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;


typedef struct _OBJECT_ALL_TYPES_INFORMATION
{
  ULONG NumberOfTypes;
  OBJECT_TYPE_INFORMATION TypeInformation[1];
} OBJECT_ALL_TYPES_INFORMATION, *POBJECT_ALL_TYPES_INFORMATION;


typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFORMATION
{
  BOOLEAN Inherit;
  BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_ATTRIBUTE_INFORMATION, *POBJECT_HANDLE_ATTRIBUTE_INFORMATION;


typedef struct _OBJECT_TYPE *POBJECT_TYPE;


typedef struct _OBJECT_ATTRIBUTES
{
   ULONG Length;
   HANDLE RootDirectory;
   PUNICODE_STRING ObjectName;
   ULONG Attributes;
   PVOID SecurityDescriptor;
   PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

#endif

typedef struct _HANDLE_TABLE_ENTRY_INFO {
    ULONG AuditMask;
} HANDLE_TABLE_ENTRY_INFO, *PHANDLE_TABLE_ENTRY_INFO;

typedef struct _HANDLE_TABLE_ENTRY {
    union {
        PVOID Object;
        ULONG_PTR ObAttributes;
        PHANDLE_TABLE_ENTRY_INFO InfoTable;
        ULONG_PTR Value;
    } u1;
    union {
        ULONG GrantedAccess;
        USHORT GrantedAccessIndex;
        LONG NextFreeTableEntry;
    } u2;
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

typedef struct _HANDLE_TABLE
{
    ULONG Flags;
    LONG HandleCount;
    PHANDLE_TABLE_ENTRY **Table;
    PEPROCESS QuotaProcess;
    HANDLE UniqueProcessId;
    LONG FirstFreeTableEntry;
    LONG NextIndexNeedingPool;
    ERESOURCE HandleTableLock;
    LIST_ENTRY HandleTableList;
    KEVENT HandleContentionEvent;
} HANDLE_TABLE;

#ifndef __USE_W32API

typedef struct _HANDLE_TABLE *PHANDLE_TABLE;

/*
 * FIXME: These will eventually become centerfold in the compliant Ob Manager
 * For now, they are only here so Device Map is properly defined before the header
 * changes
 */
typedef struct _OBJECT_DIRECTORY_ENTRY {
    struct _OBJECT_DIRECTORY_ENTRY *ChainLink;
    PVOID Object;
    ULONG HashValue;
} OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;

#define NUMBER_HASH_BUCKETS 37
typedef struct _OBJECT_DIRECTORY {
    struct _OBJECT_DIRECTORY_ENTRY *HashBuckets[NUMBER_HASH_BUCKETS];
    struct _EX_PUSH_LOCK *Lock;
    struct _DEVICE_MAP *DeviceMap;
    ULONG SessionId;
} OBJECT_DIRECTORY, *POBJECT_DIRECTORY;

#endif /* __USE_W32API */

typedef struct _DEVICE_MAP {
    POBJECT_DIRECTORY   DosDevicesDirectory;
    POBJECT_DIRECTORY   GlobalDosDevicesDirectory;
    ULONG               ReferenceCount;
    ULONG               DriveMap;
    UCHAR               DriveType[32];
} DEVICE_MAP, *PDEVICE_MAP; 

extern POBJECT_TYPE ObDirectoryType;
extern PDEVICE_MAP ObSystemDeviceMap;

#endif /* ndef _INCLUDE_DDK_OBTYPES_H */
