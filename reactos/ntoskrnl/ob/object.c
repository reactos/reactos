/*
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/ob/object.c
 * PURPOSE:       Implements generic object managment functions
 * PROGRAMMER:    David Welch (welch@mcmail.com)
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

NTSTATUS STDCALL NtSetInformationObject(IN HANDLE ObjectHandle,
					IN CINT ObjectInformationClass,
					IN PVOID ObjectInformation,
					IN ULONG Length)
{
   return(ZwSetInformationObject(ObjectHandle,
				 ObjectInformationClass,
				 ObjectInformation,
				 Length));
}

NTSTATUS STDCALL ZwSetInformationObject(IN HANDLE ObjectHandle,
					IN CINT ObjectInformationClass,
					IN PVOID ObjectInformation,
					IN ULONG Length)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQueryObject(IN HANDLE ObjectHandle,
			       IN CINT ObjectInformationClass,
			       OUT PVOID ObjectInformation,
			       IN ULONG Length,
			       OUT PULONG ResultLength)
{
   return(ZwQueryObject(ObjectHandle,
			ObjectInformationClass,
			ObjectInformation,
			Length,
			ResultLength));
}

NTSTATUS STDCALL ZwQueryObject(IN HANDLE ObjectHandle,
			       IN CINT ObjectInformationClass,
			       OUT PVOID ObjectInformation,
			       IN ULONG Length,
			       OUT PULONG ResultLength)
{
   UNIMPLEMENTED
}

NTSTATUS NtMakeTemporaryObject(HANDLE Handle)
{
   return(ZwMakeTemporaryObject(Handle));
}

NTSTATUS ZwMakeTemporaryObject(HANDLE Handle)
{
   PVOID Object;
   NTSTATUS Status;  
   POBJECT_HEADER ObjectHeader;
   
   Status = ObReferenceObjectByHandle(Handle,
				      0,
				      NULL,
				      KernelMode,
				      &Object,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }

   ObjectHeader = BODY_TO_HEADER(Object);
   ObjectHeader->Permanent = FALSE;
   
   ObDereferenceObject(Object);
   
   return(STATUS_SUCCESS);
}

PVOID ObGenericCreateObject(PHANDLE Handle,
			    ACCESS_MASK DesiredAccess,
			    POBJECT_ATTRIBUTES ObjectAttributes,
			    POBJECT_TYPE Type)
{
   POBJECT_HEADER hdr = NULL;
   PWSTR path;
   PWSTR name;
   PWSTR Ignored;
   PULONG addr;
   PWSTR Buffer;
   
   DPRINT("ObGenericCreateObject(Handle %x, DesiredAccess %x,"
	  "ObjectAttributes %x, Type %x)\n",Handle,DesiredAccess,
	  ObjectAttributes,Type);
   
   /*
    * Allocate the object body and header
    */
   hdr=(POBJECT_HEADER)ExAllocatePool(NonPagedPool,OBJECT_ALLOC_SIZE(Type));
   DPRINT("OBJECT_ALLOC_SIZE(Type) %d\n",OBJECT_ALLOC_SIZE(Type));
   if (hdr==NULL)
     {
	return(NULL);
     }
   DPRINT("hdr %x\n",hdr);


   /*
    * If unnamed then initalize
    */
   if (ObjectAttributes==NULL || ObjectAttributes->ObjectName==NULL)
     {
	ObInitializeObjectHeader(Type,NULL,hdr);
	if (Handle != NULL)
	  {
	     *Handle = ObInsertHandle(KeGetCurrentProcess(),
				      HEADER_TO_BODY(hdr),
				      DesiredAccess,
				      FALSE);
	  }
	return(HEADER_TO_BODY(hdr));
     }

   
   /*
    * Copy the object name into a buffer
    */
//   DbgPrint("ObjectAttributes->ObjectName %x\n",ObjectAttributes->ObjectName);
//   DbgPrint("ObjectAttributes->ObjectName->Length %d\n",
//	    ObjectAttributes->ObjectName->Length);
//   DbgPrint("ObjectAttributes->ObjectName->MaximumLength %d\n",
//	    ObjectAttributes->ObjectName->MaximumLength);
   Buffer = ExAllocatePool(NonPagedPool,
			   ((ObjectAttributes->ObjectName->Length+1)*2));   
   if (Buffer==NULL)
     {
	return(NULL);	
     }
   memcpy(Buffer, ObjectAttributes->ObjectName->Buffer,
	  (ObjectAttributes->ObjectName->Length+1)*2);
   
   /*
    * Seperate the name into a path and name 
    */
   name = wcsrchr(Buffer,'\\');
   if (name==NULL)
     {
	name=Buffer;
	path=NULL;
     }
   else
     {
	path=Buffer;
	*name=0;
	name=name+1;
     }
   DPRINT("name %w path %w\n",name,path);
   
   ObLookupObject(ObjectAttributes->RootDirectory,
		  path,
		  &hdr->Parent,
		  &Ignored, 
		  0L);

   /*
    * Initialize the object header
    */
   ObInitializeObjectHeader(Type,name,hdr);


   ObCreateEntry(hdr->Parent,hdr);

   DPRINT("Handle %x\n",Handle);
   if (Handle != NULL)
     {
	*Handle = ObInsertHandle(KeGetCurrentProcess(),
				 HEADER_TO_BODY(hdr),
				 DesiredAccess,
				 FALSE);
     }
   
   return(HEADER_TO_BODY(hdr));
}

VOID ObInitializeObjectHeader(POBJECT_TYPE Type, PWSTR name,
			      POBJECT_HEADER ObjectHeader)
/*
 * FUNCTION: Creates a new object
 * ARGUMENT:
 *        id = Identifier for the type of object
 *        obj = Pointer to the header of the object
 */
{
   PWSTR temp_name;
   extern unsigned long long ticks;
   
   DPRINT("ObInitializeObjectHeader(id %x name %w obj %x)\n",Type,
	  name,ObjectHeader);

   ObjectHeader->HandleCount = 0;
   ObjectHeader->RefCount = 0;
   ObjectHeader->ObjectType = Type;
   ObjectHeader->Permanent = FALSE;
   if (name==NULL)
     {
	ObjectHeader->Name.Length=0;
	ObjectHeader->Name.Buffer=NULL;
     }
   else
     {
        RtlInitUnicodeString(&(ObjectHeader->Name),name);
     }
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
   POBJECT_HEADER Object;

   DPRINT("ObReferenceObjectByPointer(%x)\n",ObjectBody);
   
   Object = BODY_TO_HEADER(ObjectBody);
   Object->RefCount++;
   return(STATUS_SUCCESS);
}

NTSTATUS ObPerformRetentionChecks(POBJECT_HEADER Header)
{
   if (Header->RefCount == 0 && Header->HandleCount == 0 &&
       !Header->Permanent)
     {
	ObRemoveEntry(Header);
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
   Header->RefCount--;
   ObPerformRetentionChecks(Header);
}


NTSTATUS NtClose(HANDLE Handle)
{
   return(ZwClose(Handle));
}

NTSTATUS ZwClose(HANDLE Handle)
/*
 * FUNCTION: Closes a handle reference to an object
 * ARGUMENTS:
 *         Handle = handle to close
 * RETURNS: Status
 */
{
   PVOID ObjectBody;
   POBJECT_HEADER Header;
   PHANDLE_REP HandleRep;
   
   assert_irql(PASSIVE_LEVEL);
   
   HandleRep = ObTranslateHandle(KeGetCurrentProcess(),Handle);
   if (HandleRep == NULL)
     {
	return(STATUS_INVALID_HANDLE);
     }   
   ObjectBody = HandleRep->ObjectBody;
   
   HandleRep->ObjectBody = NULL;
   
   Header = BODY_TO_HEADER(ObjectBody);
   
   Header->HandleCount--;
   ObPerformRetentionChecks(Header);
   
   return(STATUS_SUCCESS);
}

NTSTATUS ObReferenceObjectByHandle(HANDLE Handle,
				   ACCESS_MASK DesiredAccess,
				   POBJECT_TYPE ObjectType,
				   KPROCESSOR_MODE AccessMode,
				   PVOID* Object,
				   POBJECT_HANDLE_INFORMATION 
				           HandleInformationPtr
				   )
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
