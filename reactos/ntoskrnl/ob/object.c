/*
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
#include <internal/ob.h>
#include <wstring.h>
#include <string.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ************************************************************/

VOID ObInitializeObject(POBJECT_HEADER ObjectHeader,
			PHANDLE Handle,
			ACCESS_MASK DesiredAccess,
			POBJECT_TYPE Type,
			POBJECT_ATTRIBUTES ObjectAttributes)
{
   ObjectHeader->HandleCount = 1;
   ObjectHeader->RefCount = 1;
   ObjectHeader->ObjectType = Type;
   ObjectHeader->Permanent = FALSE;
   RtlInitUnicodeString(&(ObjectHeader->Name),NULL);
   if (Handle != NULL)
     {
	*Handle = ObInsertHandle(KeGetCurrentProcess(),
				 HEADER_TO_BODY(ObjectHeader),
				 DesiredAccess,
				 FALSE);
     }
}

NTSTATUS ObFindObject(POBJECT_ATTRIBUTES ObjectAttributes,
		      PVOID* ReturnedObject,
		      PWSTR* RemainingPath)
{
   PVOID NextObject;
   PVOID CurrentObject;
   POBJECT_HEADER CurrentHeader;
   NTSTATUS Status;
   PWSTR Path;
   PWSTR current;
   
   DPRINT("ObFindObject(ObjectAttributes %x, ReturnedObject %x, "
	  "RemainingPath %x)\n",ObjectAttributes,ReturnedObject,RemainingPath);
   DPRINT("ObjectAttributes->ObjectName->Buffer %x\n",
	  ObjectAttributes->ObjectName->Buffer);
   
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
   
   if (Path[0] != '\\')
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   current = Path;

   while (TRUE)
     {
	DPRINT("current %w\n",current);
	CurrentHeader = BODY_TO_HEADER(CurrentObject);
	if (CurrentHeader->ObjectType->Parse == NULL)
	  {
	     break;
	  }
	NextObject = CurrentHeader->ObjectType->Parse(CurrentObject,
						      &current);
	if (NextObject == NULL)
	  {
	     break;
	  }
	ObDereferenceObject(CurrentObject);
	CurrentObject = NextObject;
     }
   
   *RemainingPath = current;
   *ReturnedObject = CurrentObject;
   
   return(STATUS_SUCCESS);
}

PVOID ObCreateObject(PHANDLE Handle,
		     ACCESS_MASK DesiredAccess,
		     POBJECT_ATTRIBUTES ObjectAttributes,
		     POBJECT_TYPE Type)
{
   PVOID Parent = NULL;
   PWSTR RemainingPath = NULL;
   POBJECT_HEADER Header;
   NTSTATUS Status;
   
   DPRINT("ObCreateObject(Handle %x, ObjectAttributes %x, Type %x)\n");
   if (ObjectAttributes != NULL &&
       ObjectAttributes->ObjectName != NULL)
     {
	DPRINT("ObjectAttributes->ObjectName->Buffer %w\n",
	       ObjectAttributes->ObjectName->Buffer);
     }
   
   if (ObjectAttributes != NULL &&
       ObjectAttributes->ObjectName != NULL)
     {
	ObFindObject(ObjectAttributes,
		     &Parent,
		     &RemainingPath);
     }
   
   Header = (POBJECT_HEADER)ExAllocatePool(NonPagedPool,
					   OBJECT_ALLOC_SIZE(Type));
   ObInitializeObject(Header, 
		      Handle, 
		      DesiredAccess, 
		      Type, 
		      ObjectAttributes);
   if (Header->ObjectType != NULL &&
       Header->ObjectType->Create != NULL)
     {
	Status = Header->ObjectType->Create(HEADER_TO_BODY(Header),
					    Parent,
					    RemainingPath,
					    ObjectAttributes);
	if (!NT_SUCCESS(Status))
	  {
	     return(NULL);
	  }
     }
   return(HEADER_TO_BODY(Header));
}

NTSTATUS ObReferenceObjectByPointer(PVOID ObjectBody,
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

   DPRINT("ObReferenceObjectByPointer(ObjectBody %x, ObjectType %x)\n",
	  ObjectBody,ObjectType);
   
   ObjectHeader = BODY_TO_HEADER(ObjectBody);
   
   if (ObjectType != NULL && ObjectHeader->ObjectType != ObjectType)
     {
	DPRINT("Failed (type was %x %w)\n",ObjectHeader->ObjectType,
	       ObjectHeader->ObjectType->TypeName.Buffer);
	return(STATUS_UNSUCCESSFUL);
     }
   
   ObjectHeader->RefCount++;
   return(STATUS_SUCCESS);
}

NTSTATUS ObPerformRetentionChecks(POBJECT_HEADER Header)
{
   DPRINT("ObPerformRetentionChecks(Header %x), RefCount %d, HandleCount %d\n",
	   Header,Header->RefCount,Header->HandleCount);
   
   if (Header->RefCount <  0 || Header->HandleCount < 0)
     {
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
	  }
	DPRINT("ObPerformRetentionChecks() = Freeing object\n");
	ExFreePool(Header);
     }
   return(STATUS_SUCCESS);
}

VOID ObDereferenceObject(PVOID ObjectBody)
/*
 * FUNCTION: Decrements a given object's reference count and performs
 * retention checks
 * ARGUMENTS:
 *        ObjectBody = Body of the object
 */
{
   POBJECT_HEADER Header = BODY_TO_HEADER(ObjectBody);
   
   DPRINT("ObDeferenceObject(ObjectBody %x) RefCount %d\n",ObjectBody,
	  Header->RefCount);
   
   Header->RefCount--;
   ObPerformRetentionChecks(Header);
}

NTSTATUS ObReferenceObjectByHandle(HANDLE Handle,
				   ACCESS_MASK DesiredAccess,
				   POBJECT_TYPE ObjectType,
				   KPROCESSOR_MODE AccessMode,
				   PVOID* Object,
				   POBJECT_HANDLE_INFORMATION 
				           HandleInformationPtr)
/*
 * FUNCTION: Increments the reference count for an object and returns a 
 * pointer to its body
 * ARGUMENTS:
 *         Handle = Handle for the object
 *         DesiredAccess = Desired access to the object
 *         ObjectType
 *         AccessMode 
 *         Object (OUT) = Points to the object body on return
 *         HandleInformation (OUT) = Contains information about the handle 
 *                                   on return
 * RETURNS: Status
 */
{
   PHANDLE_REP HandleRep;
   POBJECT_HEADER ObjectHeader;
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   
   DPRINT("ObReferenceObjectByHandle(Handle %x, DesiredAccess %x, "
	  "ObjectType %x, AccessMode %d, Object %x)\n",Handle,DesiredAccess,
	  ObjectType,AccessMode,Object);
   
   if (Handle == NtCurrentProcess())
     {
	*Object = PsGetCurrentProcess();
	return(STATUS_SUCCESS);
     }
   if (Handle == NtCurrentThread())
     {
	*Object = PsGetCurrentThread();
	return(STATUS_SUCCESS);
     }
   
   HandleRep = ObTranslateHandle(KeGetCurrentProcess(),Handle);
   if (HandleRep == NULL || HandleRep->ObjectBody == NULL)
     {
	return(STATUS_INVALID_HANDLE);
     }
   
   ObjectHeader = BODY_TO_HEADER(HandleRep->ObjectBody);
   
   if (ObjectType != NULL && ObjectType != ObjectHeader->ObjectType)
     {
	return(STATUS_UNSUCCESSFUL);
     }   
   
   if (!(HandleRep->GrantedAccess & DesiredAccess))
     {
	return(STATUS_ACCESS_DENIED);
     }
   
   ObjectHeader->RefCount++;
   
   *Object = HandleRep->ObjectBody;
   
   return(STATUS_SUCCESS);
}
