/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/ob/namespc.c
 * PURPOSE:        Manages the system namespace
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 22/05/98: Created
 */

/* INCLUDES ***************************************************************/

#include <windows.h>
#include <wstring.h>
#include <ddk/ntddk.h>
#include <internal/objmgr.h>
#include <internal/string.h>
#include <internal/kernel.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ****************************************************************/

OBJECT_TYPE DirectoryObjectType = {{0,0,NULL},
                                   0,
                                   0,
                                   ULONG_MAX,
                                   ULONG_MAX,
                                   sizeof(DEVICE_OBJECT),
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   };

static struct
{
   OBJECT_HEADER hdr;
//   DIRECTORY_OBJECT directory;
   LIST_ENTRY head;
} namespc_root;

/* FUNCTIONS **************************************************************/

NTSTATUS ZwOpenDirectoryObject(PHANDLE DirectoryHandle,
			       ACCESS_MASK DesiredAccess,
			       POBJECT_ATTRIBUTES ObjectAttributes)
{
   PVOID Object;
   NTSTATUS Status;
   
   Status = ObOpenObjectByName(ObjectAttributes,&Object);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
       
   if (BODY_TO_HEADER(Object)->Type!=OBJTYP_DIRECTORY)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   *DirectoryHandle = ObAddHandle(Object);
   return(STATUS_SUCCESS);
}

NTSTATUS ZwQueryDirectoryObject(IN HANDLE DirObjHandle,
				OUT POBJDIR_INFORMATION DirObjInformation, 
				IN ULONG                BufferLength, 
				IN BOOLEAN              GetNextIndex, 
				IN BOOLEAN              IgnoreInputIndex, 
				IN OUT PULONG           ObjectIndex,
				OUT PULONG              DataWritten OPTIONAL)
{
   POBJECT_HEADER hdr = ObGetObjectByHandle(DirObjHandle);
   PDIRECTORY_OBJECT dir = (PDIRECTORY_OBJECT)(HEADER_TO_BODY(hdr));
   PLIST_ENTRY current_entry;
   POBJECT_HEADER current;
   PWSTR outbuffer = (PWSTR)(ObjectIndex);
   
   current_entry = dir->head.Flink;
   while (current_entry!=NULL)
     {
	current = CONTAINING_RECORD(current_entry,OBJECT_HEADER,entry);
	if (BufferLength < wstrlen(current->name.Buffer))
	     {
		return(STATUS_SUCCESS);
	     }
	BufferLength = BufferLength - wstrlen(current->name.Buffer);
//	wcscpy(outbuffer,current->name.Buffer);
	outbuffer = outbuffer + wstrlen(current->name.Buffer);
	current_entry = current_entry->Flink;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS ObOpenObjectByName(POBJECT_ATTRIBUTES ObjectAttributes,
			    PVOID* Object)
{
   
   DPRINT("ObOpenObjectByName(ObjectAttributes %x, Object %x)\n",
	  ObjectAttributes,Object);
   DPRINT("ObjectAttributes = {ObjectName %x ObjectName->Buffer %w}\n",
	  ObjectAttributes->ObjectName,ObjectAttributes->ObjectName->Buffer);
   
   *Object = ObLookupObject(ObjectAttributes->RootDirectory, 
                              ObjectAttributes->ObjectName->Buffer);
   DPRINT("*Object %x\n",*Object);
   if ((*Object)==NULL)
     {
	return(STATUS_NO_SUCH_FILE);
     }
   return(STATUS_SUCCESS);
}

void ObjNamespcInit(void)
/*
 * FUNCTION: Initialize the object manager namespace
 */
{
   ANSI_STRING ansi_str;
   
   ObInitializeObjectHeader(OBJTYP_DIRECTORY,NULL,&namespc_root.hdr);
   InitializeListHead(&namespc_root.head);
   
   RtlInitAnsiString(&ansi_str,"Directory");
   RtlAnsiStringToUnicodeString(&DirectoryObjectType.TypeName,&ansi_str,
				TRUE);
   ObRegisterType(OBJTYP_DIRECTORY,&DirectoryObjectType);
}

NTSTATUS ZwCreateDirectoryObject(PHANDLE DirectoryHandle,
				 ACCESS_MASK DesiredAccess,
				 POBJECT_ATTRIBUTES ObjectAttributes)
/*
 * FUNCTION: Creates or opens a directory object (a container for other
 * objects)
 * ARGUMENTS:
 *        DirectoryHandle (OUT) = Caller supplied storage for the handle
 *                                of the directory
 *        DesiredAccess = Access desired to the directory
 *        ObjectAttributes = Object attributes initialized with
 *                           InitializeObjectAttributes
 * RETURNS: Status
 */
{
   PDIRECTORY_OBJECT dir;
   
   dir = ObGenericCreateObject(DirectoryHandle,DesiredAccess,ObjectAttributes,
			       OBJTYP_DIRECTORY);
   
   /*
    * Initialize the object body
    */
   InitializeListHead(&dir->head);

   return(STATUS_SUCCESS);
}

VOID InitializeObjectAttributes(POBJECT_ATTRIBUTES InitializedAttributes,
				PUNICODE_STRING ObjectName,
				ULONG Attributes,
				HANDLE RootDirectory,
				PSECURITY_DESCRIPTOR SecurityDescriptor)
/*
 * FUNCTION: Sets up a parameter of type OBJECT_ATTRIBUTES for a 
 * subsequent call to ZwCreateXXX or ZwOpenXXX
 * ARGUMENTS:
 *        InitializedAttributes (OUT) = Caller supplied storage for the
 *                                      object attributes
 *        ObjectName = Full path name for object
 *        Attributes = Attributes for the object
 *        RootDirectory = Where the object should be placed or NULL
 *        SecurityDescriptor = Ignored
 * 
 * NOTE:
 *     Either ObjectName is a fully qualified pathname or a path relative
 *     to RootDirectory
 */
{
   DPRINT("InitializeObjectAttributes(InitializedAttributes %x "
	  "ObjectName %x Attributes %x RootDirectory %x)\n",
	  InitializedAttributes,ObjectName,Attributes,RootDirectory);
   InitializedAttributes->Length=sizeof(OBJECT_ATTRIBUTES);
   InitializedAttributes->RootDirectory=RootDirectory;
   InitializedAttributes->ObjectName=ObjectName;
   InitializedAttributes->Attributes=Attributes;
   InitializedAttributes->SecurityDescriptor=SecurityDescriptor;
   InitializedAttributes->SecurityQualityOfService=NULL;
}

static PVOID ObDirLookup(PDIRECTORY_OBJECT dir, PWSTR name)
/*
 * FUNCTION: Looks up an entry within a namespace directory
 * ARGUMENTS:
 *        dir = Directory to lookup in
 *        name = Entry name to find
 * RETURNS: A pointer to the object body if found
 *          NULL otherwise
 */
{
   LIST_ENTRY* current = ((PDIRECTORY_OBJECT)dir)->head.Flink;
   POBJECT_HEADER current_obj;
   DPRINT("ObDirLookup(dir %x, name %w)\n",dir,name);
   if (name[0]==0)
     {
	return(BODY_TO_HEADER(dir));
     }
   if (name[0]=='.'&&name[1]==0)
     {
	return(BODY_TO_HEADER(dir));
     }
   if (name[0]=='.'&&name[1]=='.'&&name[2]==0)
     {
	UNIMPLEMENTED;
	return(NULL);
     }
   while (current!=NULL)
     {
	current_obj = CONTAINING_RECORD(current,OBJECT_HEADER,entry);
	if ( wcscmp(current_obj->name.Buffer, name)==0)
	  {
	     return(current_obj);
	  }
	current = current->Flink;
     }
   return(NULL);
}


VOID ObCreateEntry(PDIRECTORY_OBJECT parent,POBJECT_HEADER Object)
/*
 * FUNCTION: Add an entry to a namespace directory
 * ARGUMENTS:
 *         parent = directory to add in
 *         name = Name to give the entry
 *         Object = Header of the object to add the entry for
 */
{
   DPRINT("ObjCreateEntry(%x,%x,%x,%w)\n",parent,Object,Object->name.Buffer,
	  Object->name.Buffer);
   DPRINT("root type %d\n",namespc_root.hdr.Type);
   DPRINT("%x\n",&(namespc_root.hdr.Type));
   DPRINT("type %x\n",&(parent->Type));
   DPRINT("type %x\n",&(BODY_TO_HEADER(parent)->Type));
   DPRINT("type %d\n",parent->Type);
   assert(parent->Type == OBJTYP_DIRECTORY);
   
   /*
    * Insert ourselves in our parents list
    */
   InsertTailList(&parent->head,&Object->entry);
}

PVOID ObLookupObject(HANDLE rooth, PWSTR string)
/*
 * FUNCTION: Lookup an object within the system namespc
 * ARGUMENTS:
 *         root = Directory to start lookup from
 *         _string = Pathname to lookup
 * RETURNS: On success a pointer to the object body
 *          On failure NULL
 */
{
   PWSTR current;
   PWSTR next;
   PDIRECTORY_OBJECT current_dir = NULL;
   POBJECT_HEADER current_hdr;
   
   DPRINT("root %x string %w\n",rooth,string);
   
   if (rooth==NULL)
     {
	current_dir = HEADER_TO_BODY(&(namespc_root.hdr));
     }
   else
     {
	ObReferenceObjectByHandle(rooth,DIRECTORY_TRAVERSE,NULL,
				  UserMode,(PVOID*)&current_dir,NULL);
     }
  
   /*
    * Bit of a hack this
    */
   if (string[0]==0)
   {
      DPRINT("current_dir %x\n",current_dir);
      DPRINT("type %d\n",current_dir->Type);
      return(current_dir);
   }

   DPRINT("string = %w\n",string);
   
   if (string[0]!='\\')
     {
        printk("(%s:%d) Non absolute pathname passed to %s\n",__FILE__,
               __LINE__,__FUNCTION__);
	return(NULL);
     }
      
   current = string+1;
   DPRINT("current %w\n",current);
   next = wcschr(string+1,'\\');
   if (next!=NULL)
     {
	*next=0;
     }
   DPRINT("next %x\n",next);
   
   while (next!=NULL)
     {
        DPRINT("Scanning %w next %w current %x\n",current,next+1,
	       current_dir);
	
	/*
	 * Check the current object is a directory
	 */
	if (current_dir->Type!=OBJTYP_DIRECTORY)
	  {
             printk("(%s:%d) Bad path component\n",__FILE__,
                    __LINE__);
	     ExFreePool(string);
	     return(NULL);
	  }
	
	/*
	 * Lookup the next component of the path in the directory
	 */
	current_hdr=(PDIRECTORY_OBJECT)ObDirLookup(current_dir,current);
	if (current_hdr==NULL)
	  {
             printk("(%s:%d) Path component not found\n",__FILE__,
                    __LINE__);
	     ExFreePool(string);
	     return(NULL);
	  }
	current_dir = HEADER_TO_BODY(current_hdr);
	  
	current = next+1;
	next = wcschr(next+1,'\\');
	if (next!=NULL)
	  {
	     *next=0;
	  }
     }
   
   DPRINT("current_dir %x current %x\n",current_dir,current);
   DPRINT("current %w\n",current);
   current_hdr = ObDirLookup(current_dir,current);
   if (current_hdr==NULL)
   {
        return(NULL);
   }
   DPRINT("Returning %x %x\n",current_hdr,HEADER_TO_BODY(current_hdr));
   return(HEADER_TO_BODY(current_hdr));
}
 
