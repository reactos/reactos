/* $Id: object.c,v 1.39 2001/06/16 14:10:55 ekohl Exp $
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
#include <internal/config.h>
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
		       FALSE,
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
						  ObjectType,
						  ObjectAttributes->Attributes);
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
 * 	ObCreateObject@36
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
NTSTATUS STDCALL
ObCreateObject(PHANDLE Handle,
	       ACCESS_MASK DesiredAccess,
	       POBJECT_ATTRIBUTES ObjectAttributes,
	       POBJECT_TYPE Type,
	       PVOID *Object)
{
   PVOID Parent = NULL;
   UNICODE_STRING RemainingPath;
   POBJECT_HEADER Header;
   NTSTATUS Status;
   
   assert_irql(APC_LEVEL);
   
   DPRINT("ObCreateObject(Handle %x, ObjectAttributes %x, Type %x)\n");
   if (ObjectAttributes != NULL &&
       ObjectAttributes->ObjectName != NULL)
     {
	DPRINT("ObjectAttributes->ObjectName->Buffer %S\n",
	       ObjectAttributes->ObjectName->Buffer);
     }
   
   if (ObjectAttributes != NULL &&
       ObjectAttributes->ObjectName != NULL)
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
	RtlInitUnicodeString (&RemainingPath, NULL);
     }

   RtlMapGenericMask(&DesiredAccess,
		     Type->Mapping);

   Header = (POBJECT_HEADER)ExAllocatePoolWithTag(NonPagedPool,
						  OBJECT_ALLOC_SIZE(Type),
						  Type->Tag);
   ObInitializeObject(Header,
		      Handle,
		      DesiredAccess,
		      Type,
		      ObjectAttributes);
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
	     if (Parent)
	       {
		  ObDereferenceObject( Parent );
	       }
	    RtlFreeUnicodeString( &Header->Name );
	    RtlFreeUnicodeString( &RemainingPath );
	    ExFreePool( Header );
	    return(Status);
	  }
     }
   RtlFreeUnicodeString( &RemainingPath );
   *Object = HEADER_TO_BODY(Header);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
ObReferenceObjectByPointer(PVOID ObjectBody,
			   ACCESS_MASK DesiredAccess,
			   POBJECT_TYPE ObjectType,
			   KPROCESSOR_MODE AccessMode)
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
   POBJECT_HEADER ObjectHeader;

//   DPRINT("ObReferenceObjectByPointer(ObjectBody %x, ObjectType %x)\n",
//	  ObjectBody,ObjectType);
   
   ObjectHeader = BODY_TO_HEADER(ObjectBody);
   
   if (ObjectType != NULL && ObjectHeader->ObjectType != ObjectType)
     {
	DPRINT("Failed %x (type was %x %S) should be %x %S\n",
		ObjectHeader,
		ObjectHeader->ObjectType,
		ObjectHeader->ObjectType->TypeName.Buffer,
		ObjectType,
    ObjectType->TypeName.Buffer);
	return(STATUS_UNSUCCESSFUL);
     }
   if (ObjectHeader->ObjectType == PsProcessType)
     {
	DPRINT("Ref p 0x%x refcount %d type %x ",
		ObjectBody, ObjectHeader->RefCount, PsProcessType);
	DPRINT("eip %x\n", ((PULONG)&ObjectBody)[-1]);
     }
   if (ObjectHeader->ObjectType == PsThreadType)
     {
	DPRINT("Deref t 0x%x with refcount %d type %x ",
		ObjectBody, ObjectHeader->RefCount, PsThreadType);
	DPRINT("eip %x\n", ((PULONG)&ObjectBody)[-1]);
     }
   
   ObjectHeader->RefCount++;
   
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
			   FALSE,
			   Handle);
   
   ObDereferenceObject(Object);
   
   return STATUS_SUCCESS;
}


NTSTATUS ObPerformRetentionChecks(POBJECT_HEADER Header)
{
//   DPRINT("ObPerformRetentionChecks(Header %x), RefCount %d, HandleCount %d\n",
//	   Header,Header->RefCount,Header->HandleCount);
   
   if (Header->RefCount < 0)
     {
	DbgPrint("Object %x/%x has invalid reference count (%d)\n",
		 Header, HEADER_TO_BODY(Header), Header->RefCount);
	KeBugCheck(0);
     }
   if (Header->HandleCount < 0)
     {
	DbgPrint("Object %x/%x has invalid handle count (%d)\n",
		 Header, HEADER_TO_BODY(Header), Header->HandleCount);
	KeBugCheck(0);
     }
   
   if (Header->RefCount == 0 && Header->HandleCount == 0 &&
       !Header->Permanent)
     {
	if (Header->ObjectType != NULL &&
	    Header->ObjectType->Delete != NULL)
	  {
	     Header->ObjectType->Delete(HEADER_TO_BODY(Header));
	  }
	if (Header->Name.Buffer != NULL)
	  {
	     ObRemoveEntry(Header);
	     RtlFreeUnicodeString( &Header->Name );
	  }
	DPRINT("ObPerformRetentionChecks() = Freeing object\n");
	ExFreePool(Header);
     }
   return(STATUS_SUCCESS);
}

ULONG ObGetReferenceCount(PVOID ObjectBody)
{
   POBJECT_HEADER Header = BODY_TO_HEADER(ObjectBody);
   
   return(Header->RefCount);
}

ULONG ObGetHandleCount(PVOID ObjectBody)
{
   POBJECT_HEADER Header = BODY_TO_HEADER(ObjectBody);
   
   return(Header->HandleCount);
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
VOID FASTCALL ObfReferenceObject(PVOID Object)
{
   POBJECT_HEADER	Header;

   assert (Object);

   Header = BODY_TO_HEADER(Object);

   Header->RefCount++;

   ObPerformRetentionChecks (Header);
}


VOID FASTCALL ObfDereferenceObject (PVOID Object)
/*
 * FUNCTION: Decrements a given object's reference count and performs
 * retention checks
 * ARGUMENTS:
 *        Object = Body of the object
 */
{
   POBJECT_HEADER Header;
   extern POBJECT_TYPE PsProcessType;

   assert (Object);

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
   
   Header->RefCount--;
   
   ObPerformRetentionChecks(Header);
}

/* EOF */
