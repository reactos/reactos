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


typedef struct _OBJECT_TYPE
{
  /*
   * PURPOSE: Tag to be used when allocating objects of this type
   */
  ULONG Tag;

  /*
   * PURPOSE: Name of the type
   */
  UNICODE_STRING TypeName;
  
  /*
   * PURPOSE: Total number of objects of this type
   */
  ULONG TotalObjects;
  
  /*
   * PURPOSE: Total number of handles of this type
   */
  ULONG TotalHandles;
  
  /*
   * PURPOSE: Peak objects of this type
   */
  ULONG PeakObjects;
  
   /*
    * PURPOSE: Peak handles of this type
    */
  ULONG PeakHandles;
  
  /*
   * PURPOSE: Paged pool charge
   */
   ULONG PagedPoolCharge;
  
  /*
   * PURPOSE: Nonpaged pool charge
   */
  ULONG NonpagedPoolCharge;
  
  /*
   * PURPOSE: Mapping of generic access rights
   */
  PGENERIC_MAPPING Mapping;
  
  /*
   * PURPOSE: Dumps the object
   * NOTE: To be defined
   */
  VOID STDCALL_FUNC (*Dump)(VOID);
  
  /*
   * PURPOSE: Opens the object
   * NOTE: To be defined
   */
  VOID STDCALL_FUNC (*Open)(VOID);
  
   /*
    * PURPOSE: Called to close an object if OkayToClose returns true
    */
  VOID STDCALL_FUNC (*Close)(PVOID ObjectBody,
         ULONG HandleCount);
  
  /*
   * PURPOSE: Called to delete an object when the last reference is removed
   */
  VOID STDCALL_FUNC (*Delete)(PVOID ObjectBody);
  
  /*
   * PURPOSE: Called when an open attempts to open a file apparently
   * residing within the object
   * RETURNS
   *     STATUS_SUCCESS       NextObject was found
   *     STATUS_UNSUCCESSFUL  NextObject not found
   *     STATUS_REPARSE       Path changed, restart parsing the path
   */
   NTSTATUS STDCALL_FUNC (*Parse)(PVOID ParsedObject,
              PVOID *NextObject,
              PUNICODE_STRING FullPath,
              PWSTR *Path,
              ULONG Attributes);

  /*
   * PURPOSE: Called to set, query, delete or assign a security-descriptor
   * to the object
   * RETURNS
   *     STATUS_SUCCESS       NextObject was found
   */
  NTSTATUS STDCALL_FUNC (*Security)(PVOID ObjectBody,
                SECURITY_OPERATION_CODE OperationCode,
                SECURITY_INFORMATION SecurityInformation,
                PSECURITY_DESCRIPTOR SecurityDescriptor,
                PULONG BufferLength);

  /*
   * PURPOSE: Called to query the name of the object
   * RETURNS
   *     STATUS_SUCCESS       NextObject was found
   */
  NTSTATUS STDCALL_FUNC (*QueryName)(PVOID ObjectBody,
                 POBJECT_NAME_INFORMATION ObjectNameInfo,
                 ULONG Length,
                 PULONG ReturnLength);

  /*
   * PURPOSE: Called when a process asks to close the object
   */
  VOID STDCALL_FUNC (*OkayToClose)(VOID);

  NTSTATUS STDCALL_FUNC (*Create)(PVOID ObjectBody,
              PVOID Parent,
              PWSTR RemainingPath,
              struct _OBJECT_ATTRIBUTES* ObjectAttributes);

  VOID STDCALL_FUNC (*DuplicationNotify)(PEPROCESS DuplicateTo,
                PEPROCESS DuplicateFrom,
                PVOID Object);
} OBJECT_TYPE;



typedef struct _OBJECT_HEADER
/*
 * PURPOSE: Header for every object managed by the object manager
 */
{
   UNICODE_STRING Name;
   LIST_ENTRY Entry;
   LONG RefCount;
   LONG HandleCount;
   BOOLEAN CloseInProcess;
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


typedef struct _TYPE_OBJECT
{
  CSHORT Type;
  CSHORT Size;
  
  /* pointer to object type data */
  POBJECT_TYPE ObjectType;
} TYPE_OBJECT, *PTYPE_OBJECT;


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


#define OBJECT_ALLOC_SIZE(ObjectSize) ((ObjectSize)+sizeof(OBJECT_HEADER)-sizeof(COMMON_BODY_HEADER))


extern PDIRECTORY_OBJECT NameSpaceRoot;
extern POBJECT_TYPE ObSymbolicLinkType;


POBJECT_HEADER BODY_TO_HEADER(PVOID body);
PVOID HEADER_TO_BODY(POBJECT_HEADER obj);

VOID ObpAddEntryDirectory(PDIRECTORY_OBJECT Parent,
			  POBJECT_HEADER Header,
			  PWSTR Name);
VOID ObpRemoveEntryDirectory(POBJECT_HEADER Header);

VOID
ObInitSymbolicLinkImplementation(VOID);


NTSTATUS ObCreateHandle(struct _EPROCESS* Process,
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
VOID ObCloseAllHandles(struct _EPROCESS* Process);
VOID ObDeleteHandleTable(struct _EPROCESS* Process);

NTSTATUS
ObDeleteHandle(PEPROCESS Process,
	       HANDLE Handle,
	       PVOID *ObjectBody);

NTSTATUS
ObpQueryHandleAttributes(HANDLE Handle,
			 POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo);

NTSTATUS
ObpSetHandleAttributes(HANDLE Handle,
		       POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo);

NTSTATUS
ObpCreateTypeObject(POBJECT_TYPE ObjectType);

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

/* Secure object information functions */

typedef struct _CAPTURED_OBJECT_ATTRIBUTES
{
  HANDLE RootDirectory;
  ULONG Attributes;
  PSECURITY_DESCRIPTOR SecurityDescriptor;
  /* PVOID SecurityQualityOfService; */
} CAPTURED_OBJECT_ATTRIBUTES, *PCAPTURED_OBJECT_ATTRIBUTES;

NTSTATUS
ObpCaptureObjectAttributes(IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                           IN POBJECT_TYPE ObjectType,
                           IN KPROCESSOR_MODE AccessMode,
                           IN BOOLEAN CaptureIfKernel,
                           OUT PCAPTURED_OBJECT_ATTRIBUTES CapturedObjectAttributes  OPTIONAL,
                           OUT PUNICODE_STRING ObjectName  OPTIONAL);

VOID
ObpReleaseObjectAttributes(IN PCAPTURED_OBJECT_ATTRIBUTES CapturedObjectAttributes  OPTIONAL,
                           IN PUNICODE_STRING ObjectName  OPTIONAL,
                           IN KPROCESSOR_MODE AccessMode,
                           IN BOOLEAN CaptureIfKernel);


#endif /* __INCLUDE_INTERNAL_OBJMGR_H */
