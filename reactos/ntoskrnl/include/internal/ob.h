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
  if(PrevMode != KernelMode)                                                     \
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
  if(PrevMode != KernelMode)                                                     \
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
