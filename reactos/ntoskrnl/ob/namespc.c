/* $Id: namespc.c,v 1.35 2002/11/10 13:36:15 robd Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/ob/namespc.c
 * PURPOSE:        Manages the system namespace
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 22/05/98: Created
 */

/* INCLUDES ***************************************************************/

#ifdef WIN32_REGDBG
#include "cm_win32.h"
#else

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

#endif

/* GLOBALS ****************************************************************/

POBJECT_TYPE ObDirectoryType = NULL;
POBJECT_TYPE ObTypeObjectType = NULL;

PDIRECTORY_OBJECT NameSpaceRoot = NULL;

static GENERIC_MAPPING ObpDirectoryMapping = {
	STANDARD_RIGHTS_READ|DIRECTORY_QUERY|DIRECTORY_TRAVERSE,
	STANDARD_RIGHTS_WRITE|DIRECTORY_CREATE_OBJECT|DIRECTORY_CREATE_SUBDIRECTORY,
	STANDARD_RIGHTS_EXECUTE|DIRECTORY_QUERY|DIRECTORY_TRAVERSE,
	DIRECTORY_ALL_ACCESS};

static GENERIC_MAPPING ObpTypeMapping = {
	STANDARD_RIGHTS_READ,
	STANDARD_RIGHTS_WRITE,
	STANDARD_RIGHTS_EXECUTE,
	0x000F0001};

/* FUNCTIONS **************************************************************/
#ifndef WIN32_REGDBG
NTSTATUS STDCALL
ObReferenceObjectByName(PUNICODE_STRING ObjectPath,
			ULONG Attributes,
			PACCESS_STATE PassedAccessState,
			ACCESS_MASK DesiredAccess,
			POBJECT_TYPE ObjectType,
			KPROCESSOR_MODE AccessMode,
			PVOID ParseContext,
			PVOID* ObjectPtr)
{
   PVOID Object = NULL;
   UNICODE_STRING RemainingPath;
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;

   InitializeObjectAttributes(&ObjectAttributes,
			      ObjectPath,
			      Attributes,
			      NULL,
			      NULL);
   Status = ObFindObject(&ObjectAttributes,
			 &Object,
			 &RemainingPath,
			 ObjectType);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
CHECKPOINT;
DPRINT("RemainingPath.Buffer '%S' Object %p\n", RemainingPath.Buffer, Object);

   if (RemainingPath.Buffer != NULL || Object == NULL)
     {
CHECKPOINT;
DPRINT("Object %p\n", Object);
	*ObjectPtr = NULL;
	RtlFreeUnicodeString (&RemainingPath);
	return(STATUS_UNSUCCESSFUL);
     }
   *ObjectPtr = Object;
   RtlFreeUnicodeString (&RemainingPath);
   return(STATUS_SUCCESS);
}
#endif // WIN32_REGDBG

/**********************************************************************
 * NAME							EXPORTED
 *	ObOpenObjectByName
 *	
 * DESCRIPTION
 *	Obtain a handle to an existing object.
 *	
 * ARGUMENTS
 *	ObjectAttributes
 *		...
 *	ObjectType
 *		...
 *	ParseContext
 *		...
 *	AccessMode
 *		...
 *	DesiredAccess
 *		...
 *	PassedAccessState
 *		...
 *	Handle
 *		Handle to close.
 *
 * RETURN VALUE
 * 	Status.
 */
NTSTATUS STDCALL
ObOpenObjectByName(IN POBJECT_ATTRIBUTES ObjectAttributes,
		   IN POBJECT_TYPE ObjectType,
		   IN OUT PVOID ParseContext,
		   IN KPROCESSOR_MODE AccessMode,
		   IN ACCESS_MASK DesiredAccess,
		   IN PACCESS_STATE PassedAccessState,
		   OUT PHANDLE Handle)
{
   UNICODE_STRING RemainingPath;
   PVOID Object = NULL;
   NTSTATUS Status;

   DPRINT("ObOpenObjectByName(...)\n");

   Status = ObFindObject(ObjectAttributes,
			 &Object,
			 &RemainingPath,
			 ObjectType);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

   if (RemainingPath.Buffer != NULL ||
       Object == NULL)
     {
	RtlFreeUnicodeString(&RemainingPath);
	return STATUS_UNSUCCESSFUL;
     }

   Status = ObCreateHandle(PsGetCurrentProcess(),
			   Object,
			   DesiredAccess,
			   FALSE,
			   Handle);

   ObDereferenceObject(Object);
   RtlFreeUnicodeString(&RemainingPath);

   return Status;
}


VOID
ObpAddEntryDirectory(PDIRECTORY_OBJECT Parent,
		     POBJECT_HEADER Header,
		     PWSTR Name)
/*
 * FUNCTION: Add an entry to a namespace directory
 * ARGUMENTS:
 *         Parent = directory to add in
 *         Header = Header of the object to add the entry for
 *         Name = Name to give the entry
 */
{
  KIRQL oldlvl;

  RtlCreateUnicodeString(&Header->Name, Name);
  Header->Parent = Parent;

  KeAcquireSpinLock(&Parent->Lock, &oldlvl);
  InsertTailList(&Parent->head, &Header->Entry);
  KeReleaseSpinLock(&Parent->Lock, oldlvl);
}

VOID
ObpRemoveEntryDirectory(POBJECT_HEADER Header)
/*
 * FUNCTION: Remove an entry from a namespace directory
 * ARGUMENTS:
 *         Header = Header of the object to remove
 */
{
  KIRQL oldlvl;

  DPRINT("ObpRemoveEntryDirectory(Header %x)\n",Header);

  KeAcquireSpinLock(&(Header->Parent->Lock),&oldlvl);
  RemoveEntryList(&(Header->Entry));
  KeReleaseSpinLock(&(Header->Parent->Lock),oldlvl);
}

PVOID
ObpFindEntryDirectory(PDIRECTORY_OBJECT DirectoryObject,
		      PWSTR Name,
		      ULONG Attributes)
{
   PLIST_ENTRY current = DirectoryObject->head.Flink;
   POBJECT_HEADER current_obj;

   DPRINT("ObFindEntryDirectory(dir %x, name %S)\n",DirectoryObject, Name);
   
   if (Name[0]==0)
     {
	return(DirectoryObject);
     }
   if (Name[0]=='.' && Name[1]==0)
     {
	return(DirectoryObject);
     }
   if (Name[0]=='.' && Name[1]=='.' && Name[2]==0)
     {
	return(BODY_TO_HEADER(DirectoryObject)->Parent);
     }
   while (current!=(&(DirectoryObject->head)))
     {
	current_obj = CONTAINING_RECORD(current,OBJECT_HEADER,Entry);
	DPRINT("  Scanning: %S for: %S\n",current_obj->Name.Buffer, Name);
	if (Attributes & OBJ_CASE_INSENSITIVE)
	  {
	     if (_wcsicmp(current_obj->Name.Buffer, Name)==0)
	       {
		  DPRINT("Found it %x\n",HEADER_TO_BODY(current_obj));
		  return(HEADER_TO_BODY(current_obj));
	       }
	  }
	else
	  {
	     if ( wcscmp(current_obj->Name.Buffer, Name)==0)
	       {
		  DPRINT("Found it %x\n",HEADER_TO_BODY(current_obj));
		  return(HEADER_TO_BODY(current_obj));
	       }
	  }
	current = current->Flink;
     }
   DPRINT("    Not Found: %s() = NULL\n",__FUNCTION__);
   return(NULL);
}

NTSTATUS STDCALL
ObpParseDirectory(PVOID Object,
		  PVOID * NextObject,
		  PUNICODE_STRING FullPath,
		  PWSTR * Path,
		  ULONG Attributes)
{
   PWSTR end;
   PVOID FoundObject;
   
   DPRINT("ObpParseDirectory(Object %x, Path %x, *Path %S)\n",
	  Object,Path,*Path);
   
   *NextObject = NULL;
   
   if ((*Path) == NULL)
     {
	return STATUS_UNSUCCESSFUL;
     }
   
   end = wcschr((*Path)+1, '\\');
   if (end != NULL)
     {
	*end = 0;
     }
   
   FoundObject = ObpFindEntryDirectory(Object, (*Path)+1, Attributes);
   
   if (FoundObject == NULL)
     {
	if (end != NULL)
	  {
	     *end = '\\';
	  }
	return STATUS_UNSUCCESSFUL;
     }
   
   ObReferenceObjectByPointer(FoundObject,
			      STANDARD_RIGHTS_REQUIRED,
			      NULL,
			      UserMode);
   
   if (end != NULL)
     {
	*end = '\\';
	*Path = end;
     }
   else
     {
	*Path = NULL;
     }
   
   *NextObject = FoundObject;
   
   return STATUS_SUCCESS;
}

NTSTATUS STDCALL
ObpCreateDirectory(PVOID ObjectBody,
		   PVOID Parent,
		   PWSTR RemainingPath,
		   POBJECT_ATTRIBUTES ObjectAttributes)
{
  PDIRECTORY_OBJECT DirectoryObject = (PDIRECTORY_OBJECT)ObjectBody;

  DPRINT("ObpCreateDirectory(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	 ObjectBody, Parent, RemainingPath);

  if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  InitializeListHead(&DirectoryObject->head);
  KeInitializeSpinLock(&DirectoryObject->Lock);

  return(STATUS_SUCCESS);
}


VOID
ObInit(VOID)
/*
 * FUNCTION: Initialize the object manager namespace
 */
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING Name;

  /* create 'directory' object type */
  ObDirectoryType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
  
  ObDirectoryType->Tag = TAG('D', 'I', 'R', 'T');
  ObDirectoryType->TotalObjects = 0;
  ObDirectoryType->TotalHandles = 0;
  ObDirectoryType->MaxObjects = ULONG_MAX;
  ObDirectoryType->MaxHandles = ULONG_MAX;
  ObDirectoryType->PagedPoolCharge = 0;
  ObDirectoryType->NonpagedPoolCharge = sizeof(DIRECTORY_OBJECT);
  ObDirectoryType->Mapping = &ObpDirectoryMapping;
  ObDirectoryType->Dump = NULL;
  ObDirectoryType->Open = NULL;
  ObDirectoryType->Close = NULL;
  ObDirectoryType->Delete = NULL;
  ObDirectoryType->Parse = ObpParseDirectory;
  ObDirectoryType->Security = NULL;
  ObDirectoryType->QueryName = NULL;
  ObDirectoryType->OkayToClose = NULL;
  ObDirectoryType->Create = ObpCreateDirectory;
  ObDirectoryType->DuplicationNotify = NULL;
  
  RtlInitUnicodeStringFromLiteral(&ObDirectoryType->TypeName,
		       L"Directory");

  /* create 'type' object type*/
  ObTypeObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
  
  ObTypeObjectType->Tag = TAG('T', 'y', 'p', 'T');
  ObTypeObjectType->TotalObjects = 0;
  ObTypeObjectType->TotalHandles = 0;
  ObTypeObjectType->MaxObjects = ULONG_MAX;
  ObTypeObjectType->MaxHandles = ULONG_MAX;
  ObTypeObjectType->PagedPoolCharge = 0;
  ObTypeObjectType->NonpagedPoolCharge = sizeof(TYPE_OBJECT);
  ObTypeObjectType->Mapping = &ObpTypeMapping;
  ObTypeObjectType->Dump = NULL;
  ObTypeObjectType->Open = NULL;
  ObTypeObjectType->Close = NULL;
  ObTypeObjectType->Delete = NULL;
  ObTypeObjectType->Parse = NULL;
  ObTypeObjectType->Security = NULL;
  ObTypeObjectType->QueryName = NULL;
  ObTypeObjectType->OkayToClose = NULL;
  ObTypeObjectType->Create = NULL;
  ObTypeObjectType->DuplicationNotify = NULL;
  
  RtlInitUnicodeStringFromLiteral(&ObTypeObjectType->TypeName,
		       L"ObjectType");

  /* create root directory */
  ObCreateObject(NULL,
		 STANDARD_RIGHTS_REQUIRED,
		 NULL,
		 ObDirectoryType,
		 (PVOID*)&NameSpaceRoot);

  /* create '\ObjectTypes' directory */
  RtlInitUnicodeStringFromLiteral(&Name,
		       L"\\ObjectTypes");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_PERMANENT,
			     NULL,
			     NULL);
  ObCreateObject(NULL,
		 STANDARD_RIGHTS_REQUIRED,
		 &ObjectAttributes,
		 ObDirectoryType,
		 NULL);

  ObpCreateTypeObject(ObDirectoryType);
  ObpCreateTypeObject(ObTypeObjectType);
}


NTSTATUS
ObpCreateTypeObject(POBJECT_TYPE ObjectType)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR NameString[120];
  PTYPE_OBJECT TypeObject = NULL;
  UNICODE_STRING Name;
  NTSTATUS Status;

  DPRINT("ObpCreateTypeObject(ObjectType: %wZ)\n", &ObjectType->TypeName);
  wcscpy(NameString, L"\\ObjectTypes\\");
  wcscat(NameString, ObjectType->TypeName.Buffer);
  RtlInitUnicodeString(&Name,
		       NameString);

  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_PERMANENT,
			     NULL,
			     NULL);
  Status = ObCreateObject(NULL,
			  STANDARD_RIGHTS_REQUIRED,
			  &ObjectAttributes,
			  ObTypeObjectType,
			  (PVOID*)&TypeObject);
  if (NT_SUCCESS(Status))
    {
      TypeObject->ObjectType = ObjectType;
    }

  return(STATUS_SUCCESS);
}


/* EOF */
