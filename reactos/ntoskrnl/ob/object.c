/* $Id: object.c,v 1.74 2004/01/31 16:52:47 ekohl Exp $
 * 
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/ob/object.c
 * PURPOSE:       Implements generic object managment functions
 * PROGRAMMERS    David Welch (welch@cwcom.net), Skywing (skywing@valhallalegends.com)
 * UPDATE HISTORY:
 *               10/06/98: Created
 *               09/13/03: Fixed various ObXxx routines to not call retention
 *                         checks directly at a raised IRQL.
 */

/* INCLUDES *****************************************************************/

#define NTOS_MODE_KERNEL
#include <ntos.h>
#include <roscfg.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <internal/id.h>
#include <internal/ke.h>

#define NDEBUG
#include <internal/debug.h>


typedef struct _RETENTION_CHECK_PARAMS
{
  WORK_QUEUE_ITEM WorkItem;
  POBJECT_HEADER ObjectHeader;
} RETENTION_CHECK_PARAMS, *PRETENTION_CHECK_PARAMS;


/* FUNCTIONS ************************************************************/

PVOID HEADER_TO_BODY(POBJECT_HEADER obj)
{
   return(((char*)obj)+sizeof(OBJECT_HEADER)-sizeof(COMMON_BODY_HEADER));
}


POBJECT_HEADER BODY_TO_HEADER(PVOID body)
{
   PCOMMON_BODY_HEADER chdr = (PCOMMON_BODY_HEADER)body;
   return(CONTAINING_RECORD((&(chdr->Type)),OBJECT_HEADER,Type));
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
NTSTATUS
ObFindObject(POBJECT_ATTRIBUTES ObjectAttributes,
	     PVOID* ReturnedObject,
	     PUNICODE_STRING RemainingPath,
	     POBJECT_TYPE ObjectType)
{
   PVOID NextObject;
   PVOID CurrentObject;
   PVOID RootObject;
   POBJECT_HEADER CurrentHeader;
   NTSTATUS Status;
   PWSTR current;
   UNICODE_STRING PathString;
   ULONG Attributes;
   PUNICODE_STRING ObjectName;

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

  ObjectName = ObjectAttributes->ObjectName;
  if (ObjectName->Length == 0 ||
      ObjectName->Buffer[0] == UNICODE_NULL)
    {
      *ReturnedObject = CurrentObject;
      return STATUS_SUCCESS;
    }

  if (ObjectAttributes->RootDirectory == NULL &&
      ObjectName->Buffer[0] != L'\\')
    {
      ObDereferenceObject (CurrentObject);
      return STATUS_UNSUCCESSFUL;
    }

  /* Create a zero-terminated copy of the object name */
  PathString.Length = ObjectName->Length;
  PathString.MaximumLength = ObjectName->Length + sizeof(WCHAR);
  PathString.Buffer = ExAllocatePool (NonPagedPool,
				      PathString.MaximumLength);
  if (PathString.Buffer == NULL)
    {
      ObDereferenceObject (CurrentObject);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlCopyMemory (PathString.Buffer,
		 ObjectName->Buffer,
		 ObjectName->Length);
  PathString.Buffer[PathString.Length / sizeof(WCHAR)] = UNICODE_NULL;

  current = PathString.Buffer;

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
	     NextObject = NameSpaceRoot;
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
 *
 * @implemented
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

  ObjectNameInfo->Name.MaximumLength = (USHORT)(Length - sizeof(OBJECT_NAME_INFORMATION));
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
    }
  else if (ObjectHeader->Name.Length > 0 && ObjectHeader->Name.Buffer != NULL)
    {
      DPRINT ("Object does not have a 'QueryName' function\n");

      if (ObjectHeader->Parent == NameSpaceRoot)
	{
	  DPRINT ("Reached the root directory\n");
	  ObjectNameInfo->Name.Length = 0;
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
      DPRINT ("Object is unnamed\n");

      ObjectNameInfo->Name.MaximumLength = 0;
      ObjectNameInfo->Name.Length = 0;
      ObjectNameInfo->Name.Buffer = NULL;

      Status = STATUS_SUCCESS;
    }

  if (NT_SUCCESS (Status))
    {
      ObjectNameInfo->Name.MaximumLength =
	(ObjectNameInfo->Name.Length) ? ObjectNameInfo->Name.Length + sizeof(WCHAR) : 0;
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
 *	Status
 *
 * @implemented
 */
NTSTATUS STDCALL
ObCreateObject (IN KPROCESSOR_MODE ObjectAttributesAccessMode OPTIONAL,
		IN POBJECT_TYPE Type,
		IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
		IN KPROCESSOR_MODE AccessMode,
		IN OUT PVOID ParseContext OPTIONAL,
		IN ULONG ObjectSize,
		IN ULONG PagedPoolCharge OPTIONAL,
		IN ULONG NonPagedPoolCharge OPTIONAL,
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

  DPRINT("ObCreateObject(Type %p ObjectAttributes %p, Object %p)\n",
	 Type, ObjectAttributes, Object);

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

  Header = (POBJECT_HEADER)ExAllocatePoolWithTag(NonPagedPool,
						 OBJECT_ALLOC_SIZE(ObjectSize),
						 Type->Tag);
  if (Header == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  /* Initialize the object header */
  Header->HandleCount = 0;
  Header->RefCount = 1;
  Header->ObjectType = Type;
  if (ObjectAttributes != NULL &&
      ObjectAttributes->Attributes & OBJ_PERMANENT)
    {
      Header->Permanent = TRUE;
    }
  else
    {
      Header->Permanent = FALSE;
    }

  if (ObjectAttributes != NULL &&
      ObjectAttributes->Attributes & OBJ_INHERIT)
    {
      Header->Inherit = TRUE;
    }
  else
    {
      Header->Inherit = FALSE;
    }

  RtlInitUnicodeString(&(Header->Name),NULL);

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

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
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

   DPRINT("ObReferenceObjectByPointer(Object %x, ObjectType %x)\n",
	  Object,ObjectType);
   
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
      if (Header->ObjectType == PsProcessType)
        {
	  return STATUS_PROCESS_IS_TERMINATING;
	}
      if (Header->ObjectType == PsThreadType)
        {
	  return STATUS_THREAD_IS_TERMINATING;
	}
      return(STATUS_UNSUCCESSFUL);
   }

   InterlockedIncrement(&Header->RefCount);
   
   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
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
			   (BOOLEAN)(HandleAttributes & OBJ_INHERIT),
			   Handle);
   
   ObDereferenceObject(Object);
   
   return STATUS_SUCCESS;
}


static NTSTATUS
ObpPerformRetentionChecks(POBJECT_HEADER Header)
{
  DPRINT("ObPerformRetentionChecks(Header %p)\n", Header);
  if (KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
      DPRINT("ObpPerformRetentionChecks called at an unsupported IRQL.  Use ObpPerformRetentionChecksDpcLevel instead.\n");
      KEBUGCHECK(0);
    }

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

  return(STATUS_SUCCESS);
}


VOID STDCALL
ObpPerformRetentionChecksWorkRoutine (IN PVOID Parameter)
{
  PRETENTION_CHECK_PARAMS Params = (PRETENTION_CHECK_PARAMS)Parameter;
  /* ULONG Tag; */ /* See below */

  assert(Params);
  assert(KeGetCurrentIrql() == PASSIVE_LEVEL); /* We need PAGED_CODE somewhere... */

  /* Turn this on when we have ExFreePoolWithTag
  Tag = Params->ObjectHeader->ObjectType->Tag; */
  ObpPerformRetentionChecks(Params->ObjectHeader);
  ExFreePool(Params);
  /* ExFreePoolWithTag(Params, Tag); */
}


static NTSTATUS
ObpPerformRetentionChecksDpcLevel(IN POBJECT_HEADER ObjectHeader)
{
  if (ObjectHeader->RefCount < 0)
    {
      CPRINT("Object %p/%p has invalid reference count (%d)\n",
	     ObjectHeader, HEADER_TO_BODY(ObjectHeader), ObjectHeader->RefCount);
      KEBUGCHECK(0);
    }

  if (ObjectHeader->HandleCount < 0)
    {
      CPRINT("Object %p/%p has invalid handle count (%d)\n",
	     ObjectHeader, HEADER_TO_BODY(ObjectHeader), ObjectHeader->HandleCount);
      KEBUGCHECK(0);
    }

  if (ObjectHeader->RefCount == 0 &&
      ObjectHeader->HandleCount == 0 &&
      ObjectHeader->Permanent == FALSE)
    {
      if (ObjectHeader->CloseInProcess)
	{
	  KEBUGCHECK(0);
	  return STATUS_UNSUCCESSFUL;
	}
      ObjectHeader->CloseInProcess = TRUE;

      switch (KeGetCurrentIrql ())
	{
	  case PASSIVE_LEVEL:
	    return ObpPerformRetentionChecks (ObjectHeader);

	  case APC_LEVEL:
	  case DISPATCH_LEVEL:
	    {
	      PRETENTION_CHECK_PARAMS Params;

	      /*
	       * Can we get rid of this NonPagedPoolMustSucceed call and still be a
	       * 'must succeed' function?  I don't like to bugcheck on no memory!
	       */
	      Params = (PRETENTION_CHECK_PARAMS)ExAllocatePoolWithTag(NonPagedPoolMustSucceed,
								      sizeof(RETENTION_CHECK_PARAMS),
								      ObjectHeader->ObjectType->Tag);
	      Params->ObjectHeader = ObjectHeader;
	      ExInitializeWorkItem(&Params->WorkItem,
				   ObpPerformRetentionChecksWorkRoutine,
				   (PVOID)Params);
	      ExQueueWorkItem(&Params->WorkItem,
			      CriticalWorkQueue);
	    }
	    return STATUS_PENDING;

	  default:
	    DPRINT("ObpPerformRetentionChecksDpcLevel called at unsupported IRQL %u!\n",
		   KeGetCurrentIrql());
	    KEBUGCHECK(0);
	    return STATUS_UNSUCCESSFUL;
	}
    }

  return STATUS_SUCCESS;
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
 *
 * @implemented
 */
VOID FASTCALL
ObfReferenceObject(IN PVOID Object)
{
  POBJECT_HEADER Header;

  assert(Object);

  Header = BODY_TO_HEADER(Object);

  if (Header->CloseInProcess)
    {
      KEBUGCHECK(0);
    }
  InterlockedIncrement(&Header->RefCount);

  ObpPerformRetentionChecksDpcLevel(Header);
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
 *
 * @implemented
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

  ObpPerformRetentionChecksDpcLevel(Header);
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
 *
 * @implemented
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


/**********************************************************************
 * NAME							EXPORTED
 *	ObDereferenceObject@4
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
 *
 * @implemented
 */

#ifdef ObDereferenceObject
#undef ObDereferenceObject
#endif

VOID STDCALL
ObDereferenceObject(IN PVOID Object)
{
  ObfDereferenceObject(Object);
}

/* EOF */
