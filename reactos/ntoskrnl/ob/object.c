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
#include <internal/objmgr.h>
#include <internal/kernel.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ****************************************************************/

/*
 * List of pointers to object types
 */
static POBJECT_TYPE ObjectTypes[OBJTYP_MAX]={NULL,};

/* FUNCTIONS ************************************************************/

NTSTATUS ZwMakeTemporaryObject(HANDLE Handle)
{
   UNIMPLEMENTED;
}

PVOID ObGenericCreateObject(PHANDLE Handle,
			    ACCESS_MASK DesiredAccess,
			    POBJECT_ATTRIBUTES ObjectAttributes,
			    CSHORT Type)
{
   POBJECT_HEADER hdr = NULL;
   
   DPRINT("ObGenericCreateObject(Handle %x ObjectAttributes %x Type %x)\n",
	  Handle,ObjectAttributes,Type);
   
   /*
    * Allocate the object body and header
    */
   hdr=(POBJECT_HEADER)ExAllocatePool(NonPagedPool,OBJECT_ALLOC_SIZE(Type));
   if (hdr==NULL)
     {
	return(NULL);
     }
   
   /*
    * Initialize the object header
    */
   if (ObjectAttributes!=NULL)
     {
	ObInitializeObjectHeader(Type,&ObjectAttributes->name,hdr);
     }
   else
     {	
	ObInitializeObjectHeader(Type,NULL,hdr);
     }
   
//   DPRINT("ObjectAttributes->parent->Type %d\n",
//	  ObjectAttributes->parent->Type);
   if (ObjectAttributes!=NULL)
     {
	/*
	 * Add the object to its parent directory
	 */
	DPRINT("hdr->name.Buffer %x\n",hdr->name.Buffer);
        ObCreateEntry(ObjectAttributes->parent,hdr);
     }
   
   DPRINT("Handle %x\n",Handle);
   *Handle = ObAddHandle(HEADER_TO_BODY(hdr));
   
   return(HEADER_TO_BODY(hdr));
}

ULONG ObSizeOf(CSHORT Type)
{
   return(ObjectTypes[Type]->PagedPoolCharge);
}

VOID ObRegisterType(CSHORT id, POBJECT_TYPE type)
/*
 * FUNCTION: Registers a new type of object
 * ARGUMENTS:
 *         typ = Pointer to the type definition to register
 */
{
   DPRINT("ObRegisterType(id %d, type %x)\n",id,type);
   ObjectTypes[id]=type;
}

VOID ObInitializeObjectHeader(CSHORT id, PUNICODE_STRING name, 
			      POBJECT_HEADER obj)
/*
 * FUNCTION: Creates a new object
 * ARGUMENT:
 *        id = Identifier for the type of object
 *        obj = Pointer to the header of the object
 */
{
   if (name!=NULL)
     {
	DPRINT("ObInitializeObjectHeader(id %d name %w obj %x)\n",id,
	       name->Buffer,obj);
     }
   else
     {
   	DPRINT("ObInitializeObjectHeader(id %d name %x obj %x)\n",id,
	       name,obj);
     }
   
   obj->HandleCount = 0;
   obj->RefCount = 0;
   obj->Type = id;
   if (name==NULL)
     {
	obj->name.Length=0;
	obj->name.Buffer=NULL;
     }
   else
     {
	obj->name.Length = 0;
	obj->name.MaximumLength = name->Length;
	obj->name.Buffer = ExAllocatePool(NonPagedPool,
					  (name->Length+1)*sizeof(WCHAR));
	DPRINT("obj->name.Buffer %x\n",obj->name.Buffer);
	RtlCopyUnicodeString(&obj->name,name);
	DPRINT("obj->name.Buffer %x\n",obj->name.Buffer);
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
   POBJECT_HEADER Object = BODY_TO_HEADER(ObjectBody);

   DPRINT("ObReferenceObjectByPointer(%x %x)\n",ObjectBody,Object);

   Object->RefCount++;
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
   POBJECT_HEADER Object = BODY_TO_HEADER(ObjectBody);
   Object->RefCount--;
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
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   
   ObjectBody = ObGetObjectByHandle(Handle);   
   if (ObjectBody == NULL)
     {
	return(STATUS_INVALID_HANDLE);
     }
   ObDereferenceObject(ObjectBody);
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
   PVOID ObjectBody;
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   assert(HandleInformation==NULL);
   assert(Object!=NULL);
   assert(Handle!=NULL);

   ObjectBody = ObGetObjectByHandle(Handle);   
   if (ObjectBody == NULL)
     {
	return(STATUS_INVALID_HANDLE);
     }   
   return(ObReferenceObjectByPointer(ObjectBody,DesiredAccess,
				     ObjectType,AccessMode));
}
