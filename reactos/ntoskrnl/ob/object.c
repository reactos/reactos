/* $Id: object.c,v 1.60 2003/06/02 10:03:52 ekohl Exp $
 * 
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/ob/object.c
 * PURPOSE:       Implements generic object managment functions
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *               10/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <internal/id.h>
#include <internal/ke.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS ************************************************************/

PVOID HEADER_TO_BODY(POBJECT_HEADER obj)
{
   return(((void *)obj)+sizeof(OBJECT_HEADER)-sizeof(COMMON_BODY_HEADER));
}


POBJECT_HEADER BODY_TO_HEADER(PVOID body)
{
   PCOMMON_BODY_HEADER chdr = (PCOMMON_BODY_HEADER)body;
   return(CONTAINING_RECORD((&(chdr->Type)),OBJECT_HEADER,Type));
}


/**********************************************************************
 * NAME							PRIVATE
 * 	ObInitializeObject
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
VOID ObInitializeObject(POBJECT_HEADER ObjectHeader,
			PHANDLE Handle,
			ACCESS_MASK DesiredAccess,
			POBJECT_TYPE Type,
			POBJECT_ATTRIBUTES ObjectAttributes)
{
   ObjectHeader->HandleCount = 0;
   ObjectHeader->RefCount = 1;
   ObjectHeader->ObjectType = Type;
   if (ObjectAttributes != NULL &&
       ObjectAttributes->Attributes & OBJ_PERMANENT)
     {
	ObjectHeader->Permanent = TRUE;
     }
   else
     {
	ObjectHeader->Permanent = FALSE;
     }
   RtlInitUnicodeString(&(ObjectHeader->Name),NULL);
   if (Handle != NULL)
     {
	ObCreateHandle(PsGetCurrentProcess(),
		       HEADER_TO_BODY(ObjectHeader),
		       DesiredAccess,
		       ObjectAttributes && (ObjectAttributes->Attributes & OBJ_INHERIT) ? TRUE : FALSE,
		       Handle);
     }
}


/**********************************************************************
 * NAME							PRIVATE
 * 	ObFindObject@16
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	ObjectAttributes
 *
 *	ReturnedObject
 *
 *	RemainigPath
 *		Pointer to a unicode string that will contain the
 *		remaining path if the function returns successfully.
 *		The caller must free the buffer after use by calling
 *		RtlFreeUnicodeString ().
 *
 *	ObjectType
 *		Optional pointer to an object type. This is used to
 *		descide if a symbolic link object will be parsed or not.
 *
 * RETURN VALUE
 */
NTSTATUS ObFindObject(POBJECT_ATTRIBUTES ObjectAttributes,
		      PVOID* ReturnedObject,
		      PUNICODE_STRING RemainingPath,
		      POBJECT_TYPE ObjectType)
{
   PVOID NextObject;
   PVOID CurrentObject;
   PVOID RootObject;
   POBJECT_HEADER CurrentHeader;
   NTSTATUS Status;
   PWSTR Path;
   PWSTR current;
   UNICODE_STRING PathString;
   ULONG Attributes;
   
   DPRINT("ObFindObject(ObjectAttributes %x, ReturnedObject %x, "
	  "RemainingPath %x)\n",ObjectAttributes,ReturnedObject,RemainingPath);
   DPRINT("ObjectAttributes->ObjectName %wZ\n",
	  ObjectAttributes->ObjectName);

   RtlInitUnicodeString (RemainingPath, NULL);

   if (ObjectAttributes->RootDirectory == NULL)
     {
	ObReferenceObjectByPointer(NameSpaceRoot,
				   DIRECTORY_TRAVERSE,
				   NULL,
				   UserMode);
	CurrentObject = NameSpaceRoot;
     }
   else
     {
	Status = ObReferenceObjectByHandle(ObjectAttributes->RootDirectory,
					   DIRECTORY_TRAVERSE,
					   NULL,
					   UserMode,
					   &CurrentObject,
					   NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
     }
   
   Path = ObjectAttributes->ObjectName->Buffer;
   
   if (Path[0] == 0)
     {
	*ReturnedObject = CurrentObject;
	return(STATUS_SUCCESS);
     }
   
   if ((ObjectAttributes->RootDirectory == NULL) && (Path[0] != '\\'))
     {
	ObDereferenceObject(CurrentObject);
	return(STATUS_UNSUCCESSFUL);
     }
   
   if (Path)
     {
	RtlCreateUnicodeString (&PathString, Path);
	current = PathString.Buffer;
     }
   else
     {
	RtlInitUnicodeString (&PathString, NULL);
	current = NULL;
     }

   RootObject = CurrentObject;
   Attributes = ObjectAttributes->Attributes;
   if (ObjectType == ObSymbolicLinkType)
     Attributes |= OBJ_OPENLINK;

   while (TRUE)
     {
	DPRINT("current %S\n",current);
	CurrentHeader = BODY_TO_HEADER(CurrentObject);

	DPRINT("Current ObjectType %wZ\n",
	       &CurrentHeader->ObjectType->TypeName);

	if (CurrentHeader->ObjectType->Parse == NULL)
	  {
	     DPRINT("Current object can't parse\n");
	     break;
	  }
	Status = CurrentHeader->ObjectType->Parse(CurrentObject,
						  &NextObject,
						  &PathString,
						  &current,
						  Attributes);
	if (Status == STATUS_REPARSE)
	  {
	     /* reparse the object path */
	     NextObject = RootObject;
	     current = PathString.Buffer;
	     
	     ObReferenceObjectByPointer(NextObject,
					DIRECTORY_TRAVERSE,
					NULL,
					UserMode);
	  }

	if (NextObject == NULL)
	  {
	     break;
	  }
	ObDereferenceObject(CurrentObject);
	CurrentObject = NextObject;
     }
   
   if (current)
      RtlCreateUnicodeString (RemainingPath, current);
   RtlFreeUnicodeString (&PathString);
   *ReturnedObject = CurrentObject;
   
   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	ObQueryNameString@16
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
NTSTATUS STDCALL
ObQueryNameString (IN PVOID Object,
		   OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
		   IN ULONG Length,
		   OUT PULONG ReturnLength)
{
  POBJECT_NAME_INFORMATION LocalInfo;
  POBJECT_HEADER ObjectHeader;
  ULONG LocalReturnLength;
  NTSTATUS Status;

  *ReturnLength = 0;

  if (Length < sizeof(OBJECT_NAME_INFORMATION) + sizeof(WCHAR))
    return STATUS_INVALID_BUFFER_SIZE;

  ObjectNameInfo->Name.MaximumLength = Length - sizeof(OBJECT_NAME_INFORMATION);
  ObjectNameInfo->Name.Length = 0;
  ObjectNameInfo->Name.Buffer =
    (PWCHAR)((ULONG_PTR)ObjectNameInfo + sizeof(OBJECT_NAME_INFORMATION));
  ObjectNameInfo->Name.Buffer[0] = 0;

  ObjectHeader = BODY_TO_HEADER(Object);

  if (ObjectHeader->ObjectType != NULL &&
      ObjectHeader->ObjectType->QueryName != NULL)
    {
      DPRINT ("Calling %x\n", ObjectHeader->ObjectType->QueryName);
      Status = ObjectHeader->ObjectType->QueryName (Object,
						    ObjectNameInfo,
						    Length,
						    ReturnLength);
      if (!NT_SUCCESS (Status))
	return Status;

      Status = RtlAppendUnicodeStringToString (&ObjectNameInfo->Name,
					       &ObjectHeader->Name);
    }
  else if (ObjectHeader->Name.Length > 0 && ObjectHeader->Name.Buffer != NULL)
    {
      DPRINT ("Object does not have a 'QueryName' function\n");

      if (ObjectHeader->Parent == NameSpaceRoot)
	{
	  DPRINT ("Reached the root directory\n");
	  ObjectNameInfo->Name.Buffer[0] = 0;
	  Status = STATUS_SUCCESS;
	}
      else if (ObjectHeader->Parent != NULL)
	{
	  LocalInfo = ExAllocatePool (NonPagedPool,
				      sizeof(OBJECT_NAME_INFORMATION) +
				      MAX_PATH * sizeof(WCHAR));
	  if (LocalInfo == NULL)
	    return STATUS_INSUFFICIENT_RESOURCES;

	  Status = ObQueryNameString (ObjectHeader->Parent,
				      LocalInfo,
				      MAX_PATH * sizeof(WCHAR),
				      &LocalReturnLength);
	  if (!NT_SUCCESS (Status))
	    {
	      ExFreePool (LocalInfo);
	      return Status;
	    }

	  Status = RtlAppendUnicodeStringToString (&ObjectNameInfo->Name,
						   &LocalInfo->Name);

	  ExFreePool (LocalInfo);

	  if (!NT_SUCCESS (Status))
	    return Status;
	}

      DPRINT ("Object path %wZ\n", &ObjectHeader->Name);
      Status = RtlAppendUnicodeToString (&ObjectNameInfo->Name,
					 L"\\");
      if (!NT_SUCCESS (Status))
	return Status;

      Status = RtlAppendUnicodeStringToString (&ObjectNameInfo->Name,
					       &ObjectHeader->Name);
    }
  else
    {
      DPRINT1 ("Object is unnamed\n");

      /* FIXME */

      Status = STATUS_UNSUCCESSFUL;
    }

  if (NT_SUCCESS (Status))
    {
      ObjectNameInfo->Name.MaximumLength =
	ObjectNameInfo->Name.Length + sizeof(WCHAR);
      *ReturnLength =
	sizeof(OBJECT_NAME_INFORMATION) + ObjectNameInfo->Name.MaximumLength;
      DPRINT ("Returned object path: %wZ\n", &ObjectNameInfo->Name);
    }

  return Status;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	ObCreateObject@36
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
NTSTATUS STDCALL
ObCreateObject (OUT PHANDLE Handle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN POBJECT_TYPE Type,
		OUT PVOID *Object)
{
  PVOID Parent = NULL;
  UNICODE_STRING RemainingPath;
  POBJECT_HEADER Header;
  POBJECT_HEADER ParentHeader = NULL;
  NTSTATUS Status;
  BOOLEAN ObjectAttached = FALSE;
  PWCHAR NamePtr;

  assert_irql(APC_LEVEL);

  DPRINT("ObCreateObject(Handle %x, ObjectAttributes %x, Type %x)\n",
	 Handle, ObjectAttributes, Type);

  if (ObjectAttributes != NULL &&
      ObjectAttributes->ObjectName != NULL &&
      ObjectAttributes->ObjectName->Buffer != NULL)
    {
      Status = ObFindObject(ObjectAttributes,
			    &Parent,
			    &RemainingPath,
			    NULL);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("ObFindObject() failed! (Status 0x%x)\n", Status);
	  return(Status);
	}
    }
  else
    {
      RtlInitUnicodeString(&RemainingPath, NULL);
    }

  RtlMapGenericMask(&DesiredAccess,
		    Type->Mapping);

  Header = (POBJECT_HEADER)ExAllocatePoolWithTag(NonPagedPool,
						 OBJECT_ALLOC_SIZE(Type),
						 Type->Tag);
  ObInitializeObject(Header,
		     NULL,
		     DesiredAccess,
		     Type,
		     ObjectAttributes);

  if (Parent != NULL)
    {
      ParentHeader = BODY_TO_HEADER(Parent);
    }

  if (ParentHeader != NULL &&
      ParentHeader->ObjectType == ObDirectoryType &&
      RemainingPath.Buffer != NULL)
    {
      NamePtr = RemainingPath.Buffer;
      if (*NamePtr == L'\\')
	NamePtr++;

      ObpAddEntryDirectory(Parent,
			   Header,
			   NamePtr);

      ObjectAttached = TRUE;
    }

  if ((Header->ObjectType != NULL) &&
      (Header->ObjectType->Create != NULL))
    {
      DPRINT("Calling %x\n", Header->ObjectType->Create);
      Status = Header->ObjectType->Create(HEADER_TO_BODY(Header),
					  Parent,
					  RemainingPath.Buffer,
					  ObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  if (ObjectAttached == TRUE)
	    {
	      ObpRemoveEntryDirectory(Header);
	    }
	  if (Parent)
	    {
	      ObDereferenceObject(Parent);
	    }
	  RtlFreeUnicodeString(&Header->Name);
	  RtlFreeUnicodeString(&RemainingPath);
	  ExFreePool(Header);
	  return(Status);
	}
    }
  RtlFreeUnicodeString( &RemainingPath );

  if (Object != NULL)
    {
      *Object = HEADER_TO_BODY(Header);
    }

  if (Handle != NULL)
  {
     ObCreateHandle(PsGetCurrentProcess(),
		    *Object,
		    DesiredAccess,
		    ObjectAttributes && (ObjectAttributes->Attributes & OBJ_INHERIT) ? TRUE : FALSE,
		    Handle);
  }

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
ObReferenceObjectByPointer(IN PVOID Object,
			   IN ACCESS_MASK DesiredAccess,
			   IN POBJECT_TYPE ObjectType,
			   IN KPROCESSOR_MODE AccessMode)
/*
 * FUNCTION: Increments the pointer reference count for a given object
 * ARGUMENTS:
 *         ObjectBody = Object's body
 *         DesiredAccess = Desired access to the object
 *         ObjectType = Points to the object type structure
 *         AccessMode = Type of access check to perform
 * RETURNS: Status
 */
{
   POBJECT_HEADER Header;

//   DPRINT("ObReferenceObjectByPointer(Object %x, ObjectType %x)\n",
//	  Object,ObjectType);
   
   Header = BODY_TO_HEADER(Object);
   
   if (ObjectType != NULL && Header->ObjectType != ObjectType)
     {
	DPRINT("Failed %x (type was %x %S) should be %x %S\n",
		Header,
		Header->ObjectType,
		Header->ObjectType->TypeName.Buffer,
		ObjectType,
    ObjectType->TypeName.Buffer);
	return(STATUS_UNSUCCESSFUL);
     }
   if (Header->ObjectType == PsProcessType)
     {
	DPRINT("Ref p 0x%x refcount %d type %x ",
		Object, Header->RefCount, PsProcessType);
	DPRINT("eip %x\n", ((PULONG)&Object)[-1]);
     }
   if (Header->ObjectType == PsThreadType)
     {
	DPRINT("Deref t 0x%x with refcount %d type %x ",
		Object, Header->RefCount, PsThreadType);
	DPRINT("eip %x\n", ((PULONG)&Object)[-1]);
     }
 
   if (Header->CloseInProcess)
   {
      return(STATUS_UNSUCCESSFUL);
   }

   InterlockedIncrement(&Header->RefCount);
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
ObOpenObjectByPointer(IN POBJECT Object,
		      IN ULONG HandleAttributes,
		      IN PACCESS_STATE PassedAccessState,
		      IN ACCESS_MASK DesiredAccess,
		      IN POBJECT_TYPE ObjectType,
		      IN KPROCESSOR_MODE AccessMode,
		      OUT PHANDLE Handle)
{
   NTSTATUS Status;
   
   DPRINT("ObOpenObjectByPointer()\n");
   
   Status = ObReferenceObjectByPointer(Object,
				       0,
				       ObjectType,
				       AccessMode);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }
   
   Status = ObCreateHandle(PsGetCurrentProcess(),
			   Object,
			   DesiredAccess,
			   HandleAttributes & OBJ_INHERIT,
			   Handle);
   
   ObDereferenceObject(Object);
   
   return STATUS_SUCCESS;
}


static NTSTATUS
ObpPerformRetentionChecks(POBJECT_HEADER Header)
{
//  DPRINT("ObPerformRetentionChecks(Header %x), RefCount %d, HandleCount %d\n",
//	  Header,Header->RefCount,Header->HandleCount);
  
  if (Header->RefCount < 0)
    {
      CPRINT("Object %x/%x has invalid reference count (%d)\n",
	     Header, HEADER_TO_BODY(Header), Header->RefCount);
      KeBugCheck(0);
    }
  if (Header->HandleCount < 0)
    {
      CPRINT("Object %x/%x has invalid handle count (%d)\n",
	     Header, HEADER_TO_BODY(Header), Header->HandleCount);
      KeBugCheck(0);
    }
  
  if (Header->RefCount == 0 &&
      Header->HandleCount == 0 &&
      Header->Permanent == FALSE)
    {
      if (Header->CloseInProcess)
      {
	 KeBugCheck(0);
	 return STATUS_UNSUCCESSFUL;
      }
      Header->CloseInProcess = TRUE;
      if (Header->ObjectType != NULL &&
	  Header->ObjectType->Delete != NULL)
	{
	  Header->ObjectType->Delete(HEADER_TO_BODY(Header));
	}
      if (Header->Name.Buffer != NULL)
	{
	  ObpRemoveEntryDirectory(Header);
	  RtlFreeUnicodeString(&Header->Name);
	}
      DPRINT("ObPerformRetentionChecks() = Freeing object\n");
      ExFreePool(Header);
    }
  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	ObfReferenceObject@4
 *
 * DESCRIPTION
 *	Increments a given object's reference count and performs
 *	retention checks.
 *
 * ARGUMENTS
 *  ObjectBody = Body of the object.
 *
 * RETURN VALUE
 * 	None.
 */
VOID FASTCALL
ObfReferenceObject(IN PVOID Object)
{
  POBJECT_HEADER Header;

  assert(Object);

  Header = BODY_TO_HEADER(Object);

  if (Header->CloseInProcess)
  {
      KeBugCheck(0);
  }
  InterlockedIncrement(&Header->RefCount);

  ObpPerformRetentionChecks(Header);
}


/**********************************************************************
 * NAME							EXPORTED
 *	ObfDereferenceObject@4
 *
 * DESCRIPTION
 *	Decrements a given object's reference count and performs
 *	retention checks.
 *
 * ARGUMENTS
 *	ObjectBody = Body of the object.
 *
 * RETURN VALUE
 * 	None.
 */
VOID FASTCALL
ObfDereferenceObject(IN PVOID Object)
{
  POBJECT_HEADER Header;
  extern POBJECT_TYPE PsProcessType;

  assert(Object);

  Header = BODY_TO_HEADER(Object);

  if (Header->ObjectType == PsProcessType)
    {
      DPRINT("Deref p 0x%x with refcount %d type %x ",
	     Object, Header->RefCount, PsProcessType);
      DPRINT("eip %x\n", ((PULONG)&Object)[-1]);
    }
  if (Header->ObjectType == PsThreadType)
    {
      DPRINT("Deref t 0x%x with refcount %d type %x ",
	     Object, Header->RefCount, PsThreadType);
      DPRINT("eip %x\n", ((PULONG)&Object)[-1]);
    }
  
  InterlockedDecrement(&Header->RefCount);
  
  ObpPerformRetentionChecks(Header);
}


/**********************************************************************
 * NAME							EXPORTED
 *	ObGetObjectPointerCount@4
 *
 * DESCRIPTION
 *	Retrieves the pointer(reference) count of the given object.
 *
 * ARGUMENTS
 *	ObjectBody = Body of the object.
 *
 * RETURN VALUE
 * 	Reference count.
 */
ULONG STDCALL
ObGetObjectPointerCount(PVOID Object)
{
  POBJECT_HEADER Header;

  assert(Object);
  Header = BODY_TO_HEADER(Object);

  return(Header->RefCount);
}


/**********************************************************************
 * NAME							INTERNAL
 *	ObGetObjectHandleCount@4
 *
 * DESCRIPTION
 *	Retrieves the handle count of the given object.
 *
 * ARGUMENTS
 *	ObjectBody = Body of the object.
 *
 * RETURN VALUE
 * 	Reference count.
 */
ULONG
ObGetObjectHandleCount(PVOID Object)
{
  POBJECT_HEADER Header;

  assert(Object);
  Header = BODY_TO_HEADER(Object);

  return(Header->HandleCount);
}

/* EOF */
