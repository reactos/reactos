/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * FILE:              include/internal/objmgr.h
 * PURPOSE:           Object manager definitions
 * PROGRAMMER:        David Welch (welch@mcmail.com)
 */

#ifndef __INCLUDE_INTERNAL_OBJMGR_H
#define __INCLUDE_INTERNAL_OBJMGR_H

#define NTOS_MODE_KERNEL
#include <ntos.h>

#define TAG_OBJECT_TYPE TAG('O', 'b', 'j', 'T')

struct _EPROCESS;

typedef enum _OB_OPEN_REASON
{    
    ObCreateHandle,
    ObOpenHandle,
    ObDuplicateHandle,
    ObInheritHandle,
    ObMaxOpenReason
} OB_OPEN_REASON;
    
/* TEMPORARY HACK */
typedef NTSTATUS
(STDCALL *OB_CREATE_METHOD)(
  PVOID  ObjectBody,
  PVOID  Parent,
  PWSTR  RemainingPath,
  struct _OBJECT_ATTRIBUTES*  ObjectAttributes);
                         
/* Object Callbacks */
typedef NTSTATUS
(STDCALL *OB_OPEN_METHOD)(
  OB_OPEN_REASON  Reason,
  PVOID  ObjectBody,
  PEPROCESS  Process,
  ULONG  HandleCount,
  ACCESS_MASK  GrantedAccess);

typedef NTSTATUS 
(STDCALL *OB_PARSE_METHOD)(
  PVOID  Object,
  PVOID  *NextObject,
  PUNICODE_STRING  FullPath,
  PWSTR  *Path,
  ULONG  Attributes);
                        
typedef VOID 
(STDCALL *OB_DELETE_METHOD)(
  PVOID  DeletedObject);

typedef VOID 
(STDCALL *OB_CLOSE_METHOD)(
  PVOID  ClosedObject,
  ULONG  HandleCount);

typedef VOID
(STDCALL *OB_DUMP_METHOD)(
  VOID);

typedef NTSTATUS 
(STDCALL *OB_OKAYTOCLOSE_METHOD)(
  VOID);

typedef NTSTATUS 
(STDCALL *OB_QUERYNAME_METHOD)(
  PVOID  ObjectBody,
  POBJECT_NAME_INFORMATION  ObjectNameInfo,
  ULONG  Length,
  PULONG  ReturnLength);

typedef PVOID 
(STDCALL *OB_FIND_METHOD)(
  PVOID  WinStaObject,
  PWSTR  Name,
  ULONG  Attributes);

typedef NTSTATUS 
(STDCALL *OB_SECURITY_METHOD)(
  PVOID  ObjectBody,
  SECURITY_OPERATION_CODE  OperationCode,
  SECURITY_INFORMATION  SecurityInformation,
  PSECURITY_DESCRIPTOR  SecurityDescriptor,
  PULONG  BufferLength);

typedef struct _OBJECT_HEADER_NAME_INFO
{
  struct _DIRECTORY_OBJECT  *Directory;
  UNICODE_STRING  Name;
  ULONG  QueryReferences;
  ULONG  Reserved2;
  ULONG  DbgReferenceCount;
} OBJECT_HEADER_NAME_INFO, *POBJECT_HEADER_NAME_INFO;

typedef struct _OBJECT_CREATE_INFORMATION 
{
  ULONG  Attributes;
  HANDLE  RootDirectory;
  PVOID  ParseContext;
  KPROCESSOR_MODE  ProbeMode;
  ULONG  PagedPoolCharge;
  ULONG  NonPagedPoolCharge;
  ULONG  SecurityDescriptorCharge;
  PSECURITY_DESCRIPTOR  SecurityDescriptor;
  PSECURITY_QUALITY_OF_SERVICE  SecurityQos;
  SECURITY_QUALITY_OF_SERVICE  SecurityQualityOfService;
} OBJECT_CREATE_INFORMATION, *POBJECT_CREATE_INFORMATION;

typedef struct _OBJECT_TYPE_INITIALIZER
{
  WORD  Length;
  UCHAR  UseDefaultObject;
  UCHAR  CaseInsensitive;
  ULONG  InvalidAttributes;
  GENERIC_MAPPING  GenericMapping;
  ULONG  ValidAccessMask;
  UCHAR  SecurityRequired;
  UCHAR  MaintainHandleCount;
  UCHAR  MaintainTypeList;
  POOL_TYPE  PoolType;
  ULONG  DefaultPagedPoolCharge;
  ULONG  DefaultNonPagedPoolCharge;
  OB_DUMP_METHOD  DumpProcedure;
  OB_OPEN_METHOD  OpenProcedure;
  OB_CLOSE_METHOD  CloseProcedure;
  OB_DELETE_METHOD  DeleteProcedure;
  OB_PARSE_METHOD  ParseProcedure;
  OB_SECURITY_METHOD  SecurityProcedure;
  OB_QUERYNAME_METHOD  QueryNameProcedure;
  OB_OKAYTOCLOSE_METHOD  OkayToCloseProcedure;
} OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;

typedef struct _OBJECT_TYPE
{
  ERESOURCE  Mutex;                    /* Used to lock the Object Type */
  LIST_ENTRY  TypeList;                /* Links all the Types Together for Debugging */
  UNICODE_STRING  Name;                /* Name of the Type */
  PVOID  DefaultObject;                /* What Object to use during a Wait (ie, FileObjects wait on FileObject->Event) */
  ULONG  Index;                        /* Index of this Type in the Object Directory */
  ULONG  TotalNumberOfObjects;         /* Total number of objects of this type */
  ULONG  TotalNumberOfHandles;         /* Total number of handles of this type */
  ULONG  HighWaterNumberOfObjects;     /* Peak number of objects of this type */
  ULONG  HighWaterNumberOfHandles;     /* Peak number of handles of this type */
  OBJECT_TYPE_INITIALIZER  TypeInfo;   /* Information captured during type creation */
  ULONG  Key;                          /* Key to use when allocating objects of this type */
  ERESOURCE  ObjectLocks[4];           /* Locks for locking the Objects */
} OBJECT_TYPE;

typedef struct _OBJECT_HANDLE_COUNT_ENTRY
{
    struct _EPROCESS *Process;
    ULONG HandleCount;
} OBJECT_HANDLE_COUNT_ENTRY, *POBJECT_HANDLE_COUNT_ENTRY;
                        
typedef struct _OBJECT_HANDLE_COUNT_DATABASE
{
    ULONG CountEntries;
    POBJECT_HANDLE_COUNT_ENTRY HandleCountEntries[1];
} OBJECT_HANDLE_COUNT_DATABASE, *POBJECT_HANDLE_COUNT_DATABASE;
                        
typedef struct _OBJECT_HEADER_HANDLE_INFO
{
    union {
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

typedef PVOID POBJECT;

typedef struct _QUAD
{
    union {
        LONGLONG UseThisFieldToCopy;
        float DoNotUseThisField;
    };
} QUAD, *PQUAD;

#define OB_FLAG_CREATE_INFO    0x01 // has OBJECT_CREATE_INFO
#define OB_FLAG_KERNEL_MODE    0x02 // created by kernel
#define OB_FLAG_CREATOR_INFO   0x04 // has OBJECT_CREATOR_INFO
#define OB_FLAG_EXCLUSIVE      0x08 // OBJ_EXCLUSIVE
#define OB_FLAG_PERMANENT      0x10 // OBJ_PERMANENT
#define OB_FLAG_SECURITY       0x20 // has security descriptor
#define OB_FLAG_SINGLE_PROCESS 0x40 // no HandleDBList

/* Will be moved to public headers once "Entry" is gone */
typedef struct _OBJECT_HEADER
{
    LIST_ENTRY Entry;
    LONG PointerCount;
    union {
        LONG HandleCount;
        PVOID NextToFree;
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

typedef struct _DIRECTORY_OBJECT
{
   CSHORT Type;
   CSHORT Size;

   /*
    * PURPOSE: Head of the list of our subdirectories
    */
   LIST_ENTRY head;
   KSPIN_LOCK Lock;
} DIRECTORY_OBJECT, *PDIRECTORY_OBJECT;

typedef struct _SYMLINK_OBJECT
{
  CSHORT Type;
  CSHORT Size;
  UNICODE_STRING TargetName;
  LARGE_INTEGER CreateTime;
} SYMLINK_OBJECT, *PSYMLINK_OBJECT;

/*
 * Enumeration of object types
 */
enum
{
   OBJTYP_INVALID,
   OBJTYP_TYPE,
   OBJTYP_DIRECTORY,
   OBJTYP_SYMLNK,
   OBJTYP_DEVICE,
   OBJTYP_THREAD,
   OBJTYP_FILE,
   OBJTYP_PROCESS,
   OBJTYP_SECTION,
   OBJTYP_MAX,
};

#define BODY_TO_HEADER(objbdy)                                                 \
  CONTAINING_RECORD((objbdy), OBJECT_HEADER, Body)
  
#define HEADER_TO_OBJECT_NAME(objhdr) ((POBJECT_HEADER_NAME_INFO)              \
  (!(objhdr)->NameInfoOffset ? NULL: ((PCHAR)(objhdr) - (objhdr)->NameInfoOffset)))
  
#define HEADER_TO_HANDLE_INFO(objhdr) ((POBJECT_HEADER_HANDLE_INFO)            \
  (!(objhdr)->HandleInfoOffset ? NULL: ((PCHAR)(objhdr) - (objhdr)->HandleInfoOffset)))
  
#define HEADER_TO_CREATOR_INFO(objhdr) ((POBJECT_HEADER_CREATOR_INFO)          \
  (!((objhdr)->Flags & OB_FLAG_CREATOR_INFO) ? NULL: ((PCHAR)(objhdr) - sizeof(OBJECT_HEADER_CREATOR_INFO))))

#define OBJECT_ALLOC_SIZE(ObjectSize) ((ObjectSize)+sizeof(OBJECT_HEADER))

#define HANDLE_TO_EX_HANDLE(handle)                                            \
  (LONG)(((LONG)(handle) >> 2) - 1)
#define EX_HANDLE_TO_HANDLE(exhandle)                                          \
  (HANDLE)(((exhandle) + 1) << 2)

extern PDIRECTORY_OBJECT NameSpaceRoot;
extern POBJECT_TYPE ObSymbolicLinkType;
extern PHANDLE_TABLE ObpKernelHandleTable;

#define KERNEL_HANDLE_FLAG (1 << ((sizeof(HANDLE) * 8) - 1))
#define ObIsKernelHandle(Handle, ProcessorMode)                                \
  (((ULONG_PTR)(Handle) & KERNEL_HANDLE_FLAG) &&                               \
   ((ProcessorMode) == KernelMode))
#define ObKernelHandleToHandle(Handle)                                         \
  (HANDLE)((ULONG_PTR)(Handle) & ~KERNEL_HANDLE_FLAG)

VOID ObpAddEntryDirectory(PDIRECTORY_OBJECT Parent,
			  POBJECT_HEADER Header,
			  PWSTR Name);
VOID ObpRemoveEntryDirectory(POBJECT_HEADER Header);

VOID
ObInitSymbolicLinkImplementation(VOID);


NTSTATUS ObpCreateHandle(struct _EPROCESS* Process,
			PVOID ObjectBody,
			ACCESS_MASK GrantedAccess,
			BOOLEAN Inherit,
			PHANDLE Handle);
VOID ObCreateHandleTable(struct _EPROCESS* Parent,
			 BOOLEAN Inherit,
			 struct _EPROCESS* Process);
NTSTATUS ObFindObject(POBJECT_CREATE_INFORMATION ObjectCreateInfo,
            PUNICODE_STRING ObjectName,
		      PVOID* ReturnedObject,
		      PUNICODE_STRING RemainingPath,
		      POBJECT_TYPE ObjectType);

NTSTATUS
ObpQueryHandleAttributes(HANDLE Handle,
			 POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo);

NTSTATUS
ObpSetHandleAttributes(HANDLE Handle,
		       POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo);

NTSTATUS
STDCALL
ObpCreateTypeObject(struct _OBJECT_TYPE_INITIALIZER *ObjectTypeInitializer, 
                    PUNICODE_STRING TypeName, 
                    POBJECT_TYPE *ObjectType);

ULONG
ObGetObjectHandleCount(PVOID Object);
NTSTATUS
ObDuplicateObject(PEPROCESS SourceProcess,
		  PEPROCESS TargetProcess,
		  HANDLE SourceHandle,
		  PHANDLE TargetHandle,
		  ACCESS_MASK DesiredAccess,
		  BOOLEAN InheritHandle,
		  ULONG	Options);

ULONG
ObpGetHandleCountByHandleTable(PHANDLE_TABLE HandleTable);

VOID
STDCALL
ObQueryDeviceMapInformation(PEPROCESS Process, PPROCESS_DEVICEMAP_INFORMATION DeviceMapInfo);

VOID FASTCALL
ObpSetPermanentObject (IN PVOID ObjectBody, IN BOOLEAN Permanent);

VOID
STDCALL
ObKillProcess(PEPROCESS Process);
/* Security descriptor cache functions */

NTSTATUS
ObpInitSdCache(VOID);

NTSTATUS
ObpAddSecurityDescriptor(IN PSECURITY_DESCRIPTOR SourceSD,
			 OUT PSECURITY_DESCRIPTOR *DestinationSD);

NTSTATUS
ObpRemoveSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor);

VOID
ObpReferenceCachedSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor);

VOID
ObpDereferenceCachedSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor);

VOID
FASTCALL
ObInitializeFastReference(IN PEX_FAST_REF FastRef,
                          PVOID Object);

PVOID
FASTCALL
ObFastReplaceObject(IN PEX_FAST_REF FastRef,
                    PVOID Object);

PVOID
FASTCALL
ObFastReferenceObject(IN PEX_FAST_REF FastRef);

VOID
FASTCALL
ObFastDereferenceObject(IN PEX_FAST_REF FastRef,
                        PVOID Object);

/* Secure object information functions */

typedef struct _CAPTURED_OBJECT_ATTRIBUTES
{
  HANDLE RootDirectory;
  ULONG Attributes;
  PSECURITY_DESCRIPTOR SecurityDescriptor;
  PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
} CAPTURED_OBJECT_ATTRIBUTES, *PCAPTURED_OBJECT_ATTRIBUTES;

NTSTATUS
STDCALL
ObpCaptureObjectName(IN PUNICODE_STRING CapturedName,
                     IN PUNICODE_STRING ObjectName,
                     IN KPROCESSOR_MODE AccessMode);
                     
NTSTATUS
STDCALL
ObpCaptureObjectAttributes(IN POBJECT_ATTRIBUTES ObjectAttributes,
                           IN KPROCESSOR_MODE AccessMode,
                           IN POBJECT_TYPE ObjectType,
                           IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
                           OUT PUNICODE_STRING ObjectName);

VOID
STDCALL
ObpReleaseCapturedAttributes(IN POBJECT_CREATE_INFORMATION ObjectCreateInfo);

/* object information classes */

#define ICIF_QUERY               0x1
#define ICIF_SET                 0x2
#define ICIF_QUERY_SIZE_VARIABLE 0x4
#define ICIF_SET_SIZE_VARIABLE   0x8
#define ICIF_SIZE_VARIABLE (ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE)

typedef struct _INFORMATION_CLASS_INFO
{
  ULONG RequiredSizeQUERY;
  ULONG RequiredSizeSET;
  ULONG AlignmentSET;
  ULONG AlignmentQUERY;
  ULONG Flags;
} INFORMATION_CLASS_INFO, *PINFORMATION_CLASS_INFO;

#define ICI_SQ_SAME(Size, Alignment, Flags)                                    \
  { Size, Size, Alignment, Alignment, Flags }

#define ICI_SQ(SizeQuery, SizeSet, AlignmentQuery, AlignmentSet, Flags)        \
  { SizeQuery, SizeSet, AlignmentQuery, AlignmentSet, Flags }

#define CheckInfoClass(Class, BufferLen, ClassList, StatusVar, Mode)           \
  do {                                                                         \
  if((Class) >= 0 && (Class) < sizeof(ClassList) / sizeof(ClassList[0]))       \
  {                                                                            \
    if(!(ClassList[Class].Flags & ICIF_##Mode))                                \
    {                                                                          \
      *(StatusVar) = STATUS_INVALID_INFO_CLASS;                                \
    }                                                                          \
    else if(ClassList[Class].RequiredSize##Mode > 0 &&                         \
            (BufferLen) != ClassList[Class].RequiredSize##Mode)                \
    {                                                                          \
      if(!(ClassList[Class].Flags & ICIF_##Mode##_SIZE_VARIABLE) &&            \
           (BufferLen) != ClassList[Class].RequiredSize##Mode)                 \
      {                                                                        \
        *(StatusVar) = STATUS_INFO_LENGTH_MISMATCH;                            \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  else                                                                         \
  {                                                                            \
    *(StatusVar) = STATUS_INVALID_INFO_CLASS;                                  \
  }                                                                            \
  } while(0)


#define GetInfoClassAlignment(Class, ClassList, AlignmentVar, Mode)            \
  do {                                                                         \
  if((Class) >= 0 && (Class) < sizeof(ClassList) / sizeof(ClassList[0]))       \
  {                                                                            \
    *(AlignmentVar) = ClassList[Class].Alignment##Mode;                        \
  }                                                                            \
  else                                                                         \
  {                                                                            \
    *(AlignmentVar) = sizeof(ULONG);                                           \
  }                                                                            \
  } while(0)

#define ProbeQueryInfoBuffer(Buffer, BufferLen, Alignment, RetLen, PrevMode, StatusVar) \
  do {                                                                         \
  if(PrevMode == UserMode)                                                     \
  {                                                                            \
    _SEH_TRY                                                                   \
    {                                                                          \
      ProbeForWrite(Buffer,                                                    \
                    BufferLen,                                                 \
                    Alignment);                                                \
      if(RetLen != NULL)                                                       \
      {                                                                        \
        ProbeForWrite(RetLen,                                                  \
                      sizeof(ULONG),                                           \
                      1);                                                      \
      }                                                                        \
    }                                                                          \
    _SEH_HANDLE                                                                \
    {                                                                          \
      *(StatusVar) = _SEH_GetExceptionCode();                                  \
    }                                                                          \
    _SEH_END;                                                                  \
                                                                               \
    if(!NT_SUCCESS(*(StatusVar)))                                              \
    {                                                                          \
      DPRINT1("ProbeQueryInfoBuffer failed: 0x%x\n", *(StatusVar));            \
      return *(StatusVar);                                                     \
    }                                                                          \
  }                                                                            \
  } while(0)

#define ProbeSetInfoBuffer(Buffer, BufferLen, Alignment, PrevMode, StatusVar) \
  do {                                                                         \
  if(PrevMode == UserMode)                                                     \
  {                                                                            \
    _SEH_TRY                                                                   \
    {                                                                          \
      ProbeForRead(Buffer,                                                     \
                   BufferLen,                                                  \
                   Alignment);                                                 \
    }                                                                          \
    _SEH_HANDLE                                                                \
    {                                                                          \
      *(StatusVar) = _SEH_GetExceptionCode();                                  \
    }                                                                          \
    _SEH_END;                                                                  \
                                                                               \
    if(!NT_SUCCESS(*(StatusVar)))                                              \
    {                                                                          \
      DPRINT1("ProbeAllInfoBuffer failed: 0x%x\n", *(StatusVar));              \
      return *(StatusVar);                                                     \
    }                                                                          \
  }                                                                            \
  } while(0)

#define DefaultSetInfoBufferCheck(Class, ClassList, Buffer, BufferLen, PrevMode, StatusVar) \
  do {                                                                         \
  ULONG _Alignment;                                                            \
  /* get the preferred alignment for the information class or return */        \
  /* default alignment in case the class doesn't exist */                      \
  GetInfoClassAlignment(Class,                                                 \
                        ClassList,                                             \
                        &_Alignment,                                           \
                        SET);                                                  \
                                                                               \
  /* probe the ENTIRE buffers and return on failure */                         \
  ProbeSetInfoBuffer(Buffer,                                                   \
                     BufferLen,                                                \
                     _Alignment,                                               \
                     PrevMode,                                                 \
                     StatusVar);                                               \
                                                                               \
  /* validate information class index and check buffer size */                 \
  CheckInfoClass(Class,                                                        \
                 BufferLen,                                                    \
                 ClassList,                                                    \
                 StatusVar,                                                    \
                 SET);                                                         \
  } while(0)

#define DefaultQueryInfoBufferCheck(Class, ClassList, Buffer, BufferLen, RetLen, PrevMode, StatusVar) \
  do {                                                                         \
    ULONG _Alignment;                                                          \
   /* get the preferred alignment for the information class or return */       \
   /* alignment in case the class doesn't exist */                             \
   GetInfoClassAlignment(Class,                                                \
                         ClassList,                                            \
                         &_Alignment,                                          \
                         QUERY);                                               \
                                                                               \
   /* probe the ENTIRE buffers and return on failure */                        \
   ProbeQueryInfoBuffer(Buffer,                                                \
                        BufferLen,                                             \
                        _Alignment,                                            \
                        RetLen,                                                \
                        PrevMode,                                              \
                        StatusVar);                                            \
                                                                               \
   /* validate information class index and check buffer size */                \
   CheckInfoClass(Class,                                                       \
                  BufferLen,                                                   \
                  ClassList,                                                   \
                  StatusVar,                                                   \
                  QUERY);                                                      \
  } while(0)

#endif /* __INCLUDE_INTERNAL_OBJMGR_H */
