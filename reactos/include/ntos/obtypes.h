#ifndef _INCLUDE_DDK_OBTYPES_H
#define _INCLUDE_DDK_OBTYPES_H
/* $Id: obtypes.h,v 1.3 2003/06/02 10:02:16 ekohl Exp $ */
struct _DIRECTORY_OBJECT;
struct _OBJECT_ATTRIBUTES;

#ifndef __USE_W32API

typedef ULONG ACCESS_STATE, *PACCESS_STATE;

typedef struct _OBJECT_NAME_INFORMATION
{
  UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

typedef struct _OBJECT_HANDLE_INFORMATION
{
  ULONG HandleAttributes;
  ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;

#endif /* __USE_W32API */

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
   * PURPOSE: Maximum objects of this type
   */
  ULONG MaxObjects;
  
   /*
    * PURPOSE: Maximum handles of this type
    */
  ULONG MaxHandles;
  
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

#ifndef __USE_W32API

typedef struct _OBJECT_TYPE *POBJECT_TYPE;

#endif /* __USE_W32API */


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
   struct _DIRECTORY_OBJECT* Parent;
   POBJECT_TYPE ObjectType;
   
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

#ifndef __USE_W32API

typedef struct _OBJECT_ATTRIBUTES
{
   ULONG Length;
   HANDLE RootDirectory;
   PUNICODE_STRING ObjectName;
   ULONG Attributes;
   SECURITY_DESCRIPTOR *SecurityDescriptor;
   SECURITY_QUALITY_OF_SERVICE *SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

#endif /* __USE_W32API */

typedef struct _HANDLE_TABLE
{
   LIST_ENTRY ListHead;
   KSPIN_LOCK ListLock;
} HANDLE_TABLE;

#ifndef __USE_W32API

typedef struct _HANDLE_TABLE *PHANDLE_TABLE;

#endif /* __USE_W32API */

extern POBJECT_TYPE ObDirectoryType;

#endif /* ndef _INCLUDE_DDK_OBTYPES_H */
