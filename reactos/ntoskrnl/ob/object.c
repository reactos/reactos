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
#include <wstring.h>

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
   UNICODE_STRING ObjectName;
   PWSTR path;
   PWSTR name;
   PDIRECTORY_OBJECT parent;
   
   DPRINT("ObGenericCreateObject(Handle %x, DesiredAccess %x,"
	  "ObjectAttributes %x, Type %x)\n",Handle,DesiredAccess,ObjectAttributes,
	  Type);
   
   /*
    * Allocate the object body and header
    */
   hdr=(POBJECT_HEADER)ExAllocatePool(NonPagedPool,OBJECT_ALLOC_SIZE(Type));
   if (hdr==NULL)
     {
	return(NULL);
     }

   /*
    * If unnamed then initalize
    */
   if (ObjectAttributes==NULL)
     {
	ObInitializeObjectHeader(Type,NULL,hdr);
	*Handle = ObAddHandle(HEADER_TO_BODY(hdr));
	return(HEADER_TO_BODY(hdr));
     }
   
   /*
    * Copy the object name into a buffer
    */
   DPRINT("ObjectAttributes->ObjectName %x\n",ObjectAttributes->ObjectName);
   DPRINT("ObjectAttributes->ObjectName->Length %d\n",
	  ObjectAttributes->ObjectName->Length);
   ObjectName.MaximumLength = ObjectAttributes->ObjectName->Length;
   ObjectName.Buffer = ExAllocatePool(NonPagedPool,
			       ((ObjectAttributes->ObjectName->Length+1)*2));
   if (ObjectName.Buffer==NULL)
     {
	return(NULL);	
     }
   RtlCopyUnicodeString(&ObjectName,ObjectAttributes->ObjectName);
   
   /*
    * Seperate the name into a path and name 
    */
   name = wcsrchr(ObjectName.Buffer,'\\');
   if (name==NULL)
     {
	name=ObjectName.Buffer;
	path=NULL;
     }
   else
     {
	path=ObjectName.Buffer;
	*name=0;
	name=name+1;
     }
   
   hdr->Parent = ObLookupObject(ObjectAttributes->RootDirectory,path);
   
   /*
    * Initialize the object header
    */
   ObInitializeObjectHeader(Type,name,hdr);
   ObCreateEntry(hdr->Parent,hdr);
      
   DPRINT("Handle %x\n",Handle);
   *Handle = ObAddHandle(HEADER_TO_BODY(hdr));
   
   return(HEADER_TO_BODY(hdr));
}

ULONG ObSizeOf(CSHORT Type)
{
   DPRINT("ObSizeOf(Type %d)\n",Type);
   DPRINT("ObSizeOf() Returning %d\n",ObjectTypes[Type]->PagedPoolCharge);
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

VOID ObInitializeObjectHeader(CSHORT id, PWSTR name,
			      POBJECT_HEADER obj)
/*
 * FUNCTION: Creates a new object
 * ARGUMENT:
 *        id = Identifier for the type of object
 *        obj = Pointer to the header of the object
 */
{
   PWSTR temp_name;
   
   if (name!=NULL)
     {
	DPRINT("ObInitializeObjectHeader(id %d name %w obj %x)\n",id,
	       name,obj);
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
	DPRINT("name %w\n",name);
	obj->name.MaximumLength = wstrlen(name);
	obj->name.Buffer = ExAllocatePool(NonPagedPool,
					  (obj->name.MaximumLength+1)*2);
	DPRINT("name %w\n",name);
	RtlInitUnicodeString(&obj->name,name);
	DPRINT("name %w\n",obj->name.Buffer);
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
   
   assert_irql(PASSIVE_LEVEL);
   
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
   assert(HandleInformationPtr==NULL);
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
