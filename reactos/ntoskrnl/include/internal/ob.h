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

typedef struct
{
   CSHORT Type;
   CSHORT Size;
} COMMON_BODY_HEADER, *PCOMMON_BODY_HEADER;

typedef PVOID POBJECT;

typedef struct _OBJECT_HEADER
/*
 * PURPOSE: Header for every object managed by the object manager
 */
{
   UNICODE_STRING Name;
   LIST_ENTRY Entry;
   LONG RefCount;
   LONG HandleCount;
   BOOLEAN Permanent;
   BOOLEAN Inherit;
   struct _DIRECTORY_OBJECT* Parent;
   POBJECT_TYPE ObjectType;
   PSECURITY_DESCRIPTOR SecurityDescriptor;

   /*
    * PURPOSE: Object type
    * NOTE: This overlaps the first member of the object body
    */
   CSHORT Type;

   /*
    * PURPOSE: Object size
    * NOTE: This overlaps the second member of the object body
    */
   CSHORT Size;


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

#define HEADER_TO_BODY(objhdr)                                                 \
  (PVOID)((ULONG_PTR)objhdr + sizeof(OBJECT_HEADER) - sizeof(COMMON_BODY_HEADER))

#define BODY_TO_HEADER(objbdy)                                                 \
  CONTAINING_RECORD(&(((PCOMMON_BODY_HEADER)objbdy)->Type), OBJECT_HEADER, Type)

#define OBJECT_ALLOC_SIZE(ObjectSize) ((ObjectSize)+sizeof(OBJECT_HEADER)-sizeof(COMMON_BODY_HEADER))

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
NTSTATUS ObFindObject(POBJECT_ATTRIBUTES ObjectAttributes,
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
ObpCreateTypeObject(POBJECT_TYPE_INITIALIZER ObjectTypeInitializer, 
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
ObpCaptureObjectAttributes(IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                           IN KPROCESSOR_MODE AccessMode,
                           IN POOL_TYPE PoolType,
                           IN BOOLEAN CaptureIfKernel,
                           OUT PCAPTURED_OBJECT_ATTRIBUTES CapturedObjectAttributes  OPTIONAL,
                           OUT PUNICODE_STRING ObjectName  OPTIONAL);

VOID
ObpReleaseObjectAttributes(IN PCAPTURED_OBJECT_ATTRIBUTES CapturedObjectAttributes  OPTIONAL,
                           IN PUNICODE_STRING ObjectName  OPTIONAL,
                           IN KPROCESSOR_MODE AccessMode,
                           IN BOOLEAN CaptureIfKernel);

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
