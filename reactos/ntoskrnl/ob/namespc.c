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
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ****************************************************************/

POBJECT_TYPE ObDirectoryType = NULL;

static struct
{
   OBJECT_HEADER hdr;
//   DIRECTORY_OBJECT directory;
   LIST_ENTRY head;
   KSPIN_LOCK Lock;
} namespc_root = {{0,},};

/* FUNCTIONS **************************************************************/

NTSTATUS NtOpenDirectoryObject(PHANDLE DirectoryHandle,
			       ACCESS_MASK DesiredAccess,
			       POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwOpenDirectoryObject(DirectoryHandle,
				DesiredAccess,
				ObjectAttributes));
}

NTSTATUS ZwOpenDirectoryObject(PHANDLE DirectoryHandle,
			       ACCESS_MASK DesiredAccess,
			       POBJECT_ATTRIBUTES ObjectAttributes)
/*
 * FUNCTION: Opens a namespace directory object
 * ARGUMENTS:
 *       DirectoryHandle (OUT) = Variable which receives the directory handle
 *       DesiredAccess = Desired access to the directory
 *       ObjectAttributes = Structure describing the directory
 * RETURNS: Status
 * NOTES: Undocumented
 */
{
   PVOID Object;
   NTSTATUS Status;
   PWSTR Ignored;
   
   *DirectoryHandle = 0;
   
   Status = ObOpenObjectByName(ObjectAttributes,&Object,&Ignored);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
       
   if (BODY_TO_HEADER(Object)->Type!=OBJTYP_DIRECTORY)
     {	
	return(STATUS_UNSUCCESSFUL);
     }
   
   *DirectoryHandle = ObInsertHandle(KeGetCurrentProcess(),Object,
				     DesiredAccess,FALSE);
   CHECKPOINT;
   return(STATUS_SUCCESS);
}

NTSTATUS NtQueryDirectoryObject(IN HANDLE DirObjHandle,
				OUT POBJDIR_INFORMATION DirObjInformation, 
				IN ULONG                BufferLength, 
				IN BOOLEAN              GetNextIndex, 
				IN BOOLEAN              IgnoreInputIndex, 
				IN OUT PULONG           ObjectIndex,
				OUT PULONG              DataWritten OPTIONAL)
{
   return(ZwQueryDirectoryObject(DirObjHandle,
				 DirObjInformation,
				 BufferLength,
				 GetNextIndex,
				 IgnoreInputIndex,
				 ObjectIndex,
				 DataWritten));
}

NTSTATUS ZwQueryDirectoryObject(IN HANDLE DirObjHandle,
				OUT POBJDIR_INFORMATION DirObjInformation, 
				IN ULONG                BufferLength, 
				IN BOOLEAN              GetNextIndex, 
				IN BOOLEAN              IgnoreInputIndex, 
				IN OUT PULONG           ObjectIndex,
				OUT PULONG              DataWritten OPTIONAL)
/*
 * FUNCTION: Reads information from a namespace directory
 * ARGUMENTS:
 *        DirObjInformation (OUT) = Buffer to hold the data read
 *        BufferLength = Size of the buffer in bytes
 *        GetNextIndex = If TRUE then set ObjectIndex to the index of the
 *                       next object
 *                       If FALSE then set ObjectIndex to the number of
 *                       objects in the directory
 *        IgnoreInputIndex = If TRUE start reading at index 0
 *                           If FALSE start reading at the index specified
 *                           by object index
 *        ObjectIndex = Zero based index into the directory, interpretation
 *                      depends on IgnoreInputIndex and GetNextIndex
 *        DataWritten (OUT) = Caller supplied storage for the number of bytes
 *                            written (or NULL)
 * RETURNS: Status
 */
{
   PDIRECTORY_OBJECT dir = NULL;
   ULONG EntriesToRead;
   PLIST_ENTRY current_entry;
   POBJECT_HEADER current;
   ULONG i=0;
   ULONG EntriesToSkip;
   NTSTATUS Status;
   
   DPRINT("ZwQueryDirectoryObject(DirObjHandle %x)\n",DirObjHandle);
   DPRINT("dir %x namespc_root %x\n",dir,HEADER_TO_BODY(&(namespc_root.hdr)));
   
//   assert_irql(PASSIVE_LEVEL);
   
   Status = ObReferenceObjectByHandle(DirObjHandle,
				      DIRECTORY_QUERY,
				      ObDirectoryType,
				      UserMode,
				      (PVOID*)&dir,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   EntriesToRead = BufferLength / sizeof(OBJDIR_INFORMATION);
   *DataWritten = 0;
   
   DPRINT("EntriesToRead %d\n",EntriesToRead);
   
   current_entry = dir->head.Flink;
   
   /*
    * Optionally, skip over some entries at the start of the directory
    */
   if (!IgnoreInputIndex)
     {
	CHECKPOINT;
	
	EntriesToSkip = *ObjectIndex;
	while ( i<EntriesToSkip && current_entry!=NULL)
	  {
	     current_entry = current_entry->Flink;
	  }
     }
   
   DPRINT("DirObjInformation %x\n",DirObjInformation);
   
   /*
    * Read the maximum entries possible into the buffer
    */
   while ( i<EntriesToRead && current_entry!=(&(dir->head)))
     {
	current = CONTAINING_RECORD(current_entry,OBJECT_HEADER,Entry);
	DPRINT("Scanning %w\n",current->Name.Buffer);
	DirObjInformation[i].ObjectName.Buffer = 
	               ExAllocatePool(NonPagedPool, current->Name.Length + sizeof(WCHAR));
	DirObjInformation[i].ObjectName.Length = current->Name.Length;
	DirObjInformation[i].ObjectName.MaximumLength = current->Name.Length + sizeof(WCHAR);
	DPRINT("DirObjInformation[i].ObjectName.Buffer %x\n",
	       DirObjInformation[i].ObjectName.Buffer);
	RtlCopyUnicodeString(&DirObjInformation[i].ObjectName,
			     &(current->Name));
	i++;
	current_entry = current_entry->Flink;
	(*DataWritten) = (*DataWritten) + sizeof(OBJDIR_INFORMATION);
	CHECKPOINT;
     }
   CHECKPOINT;
   
   /*
    * Optionally, count the number of entries in the directory
    */
   if (GetNextIndex)
     {
	*ObjectIndex=i;
     }
   else
     {
	while ( current_entry!=(&(dir->head)) )
	  {
	     current_entry=current_entry->Flink;
	     i++;
	  }
	*ObjectIndex=i;
     }
   return(STATUS_SUCCESS);
}


NTSTATUS ObReferenceObjectByName(PUNICODE_STRING ObjectPath,
				 ULONG Attributes,
				 PACCESS_STATE PassedAccessState,
				 ACCESS_MASK DesiredAccess,
				 POBJECT_TYPE ObjectType,
				 KPROCESSOR_MODE AccessMode,
				 PVOID ParseContext,
				 PVOID* ObjectPtr)
{
   UNIMPLEMENTED;
}

NTSTATUS ObOpenObjectByName(POBJECT_ATTRIBUTES ObjectAttributes,
			    PVOID* Object, PWSTR* UnparsedSection)
{
   NTSTATUS Status;
   
   DPRINT("ObOpenObjectByName(ObjectAttributes %x, Object %x)\n",
	  ObjectAttributes,Object);
   DPRINT("ObjectAttributes = {ObjectName %x ObjectName->Buffer %w}\n",
	  ObjectAttributes->ObjectName,ObjectAttributes->ObjectName->Buffer);
   DPRINT("ObjectAttributes->ObjectName->Length %d\n",
	  ObjectAttributes->ObjectName->Length);
   
   *Object = NULL;
   Status = ObLookupObject(ObjectAttributes->RootDirectory, 
			   ObjectAttributes->ObjectName->Buffer, 
			   Object,
			   UnparsedSection,
			   ObjectAttributes->Attributes);
   DPRINT("*Object %x\n",*Object);
   DPRINT("ObjectAttributes->ObjectName->Length %d\n",
	  ObjectAttributes->ObjectName->Length);
   return(Status);
}

void ObInit(void)
/*
 * FUNCTION: Initialize the object manager namespace
 */
{
   ANSI_STRING AnsiString;
   
   ObDirectoryType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   ObDirectoryType->TotalObjects = 0;
   ObDirectoryType->TotalHandles = 0;
   ObDirectoryType->MaxObjects = ULONG_MAX;
   ObDirectoryType->MaxHandles = ULONG_MAX;
   ObDirectoryType->PagedPoolCharge = 0;
   ObDirectoryType->NonpagedPoolCharge = sizeof(DIRECTORY_OBJECT);
   ObDirectoryType->Dump = NULL;
   ObDirectoryType->Open = NULL;
   ObDirectoryType->Close = NULL;
   ObDirectoryType->Delete = NULL;
   ObDirectoryType->Parse = NULL;
   ObDirectoryType->Security = NULL;
   ObDirectoryType->QueryName = NULL;
   ObDirectoryType->OkayToClose = NULL;
   
   RtlInitAnsiString(&AnsiString,"Directory");
   RtlAnsiStringToUnicodeString(&ObDirectoryType->TypeName,
				&AnsiString,TRUE);
   
   ObInitializeObjectHeader(ObDirectoryType,NULL,&namespc_root.hdr);
   InitializeListHead(&namespc_root.head);
}

NTSTATUS NtCreateDirectoryObject(PHANDLE DirectoryHandle,
				 ACCESS_MASK DesiredAccess,
				 POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwCreateDirectoryObject(DirectoryHandle,
				  DesiredAccess,
				  ObjectAttributes));
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
			       ObDirectoryType);
   
   /*
    * Initialize the object body
    */
   InitializeListHead(&dir->head);
   KeInitializeSpinLock(&(dir->Lock));
   
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

static PVOID ObDirLookup(PDIRECTORY_OBJECT dir, PWSTR name,
			 ULONG Attributes)
/*
 * FUNCTION: Looks up an entry within a namespace directory
 * ARGUMENTS:
 *        dir = Directory to lookup in
 *        name = Entry name to find
 * RETURNS: A pointer to the object body if found
 *          NULL otherwise
 */
{
   LIST_ENTRY* current = dir->head.Flink;
   POBJECT_HEADER current_obj;
   
   DPRINT("ObDirLookup(dir %x, name %w)\n",dir,name);
   
   if (name[0]==0)
     {
	return(dir);
     }
   if (name[0]=='.'&&name[1]==0)
     {
	return(dir);
     }
   if (name[0]=='.'&&name[1]=='.'&&name[2]==0)
     {
	return(BODY_TO_HEADER(dir)->Parent);
     }
   while (current!=(&(dir->head)))
     {
	current_obj = CONTAINING_RECORD(current,OBJECT_HEADER,Entry);
	DPRINT("Scanning %w\n",current_obj->Name.Buffer);
	if (Attributes & OBJ_CASE_INSENSITIVE)
	  {
	     if (wcsicmp(current_obj->Name.Buffer, name)==0)
	       {
		  DPRINT("Found it %x\n",HEADER_TO_BODY(current_obj));
		  return(HEADER_TO_BODY(current_obj));
	       }
	  }
	else
	  {
	     if ( wcscmp(current_obj->Name.Buffer, name)==0)
	       {
		  DPRINT("Found it %x\n",HEADER_TO_BODY(current_obj));
		  return(HEADER_TO_BODY(current_obj));
	       }
	  }
	current = current->Flink;
     }
   DPRINT("%s() = NULL\n",__FUNCTION__);
   return(NULL);
}

VOID ObRemoveEntry(POBJECT_HEADER Header)
{
   KIRQL oldlvl;
   
   DPRINT("ObRemoveEntry(Header %x)\n",Header);
   
   KeAcquireSpinLock(&(Header->Parent->Lock),&oldlvl);
   RemoveEntryList(&(Header->Entry));
   KeReleaseSpinLock(&(Header->Parent->Lock),oldlvl);
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
   DPRINT("ObjCreateEntry(%x,%x,%x,%w)\n",parent,Object,Object->Name.Buffer,
	  Object->Name.Buffer);
   
   /*
    * Insert ourselves in our parents list
    */
   InsertTailList(&parent->head,&Object->Entry);
}

NTSTATUS ObLookupObject(HANDLE rootdir, PWSTR string, PVOID* Object,
			PWSTR* UnparsedSection, ULONG Attributes)
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
   NTSTATUS Status;
   
   DPRINT("ObLookupObject(rootdir %x, string %x, string %w, Object %x, "
	  "UnparsedSection %x)\n",rootdir,string,string,Object,
	  UnparsedSection);
			  
   
   *UnparsedSection = NULL;
   *Object = NULL;
   
   if (rootdir==NULL)
     {
	current_dir = HEADER_TO_BODY(&(namespc_root.hdr));
     }
   else
     {
	ObReferenceObjectByHandle(rootdir,DIRECTORY_TRAVERSE,NULL,
				  UserMode,(PVOID*)&current_dir,NULL);
     }
  
   /*
    * Bit of a hack this
    */
   if (string[0]==0)
   {
      *Object=current_dir;
      return(STATUS_SUCCESS);
   }

   if (string[0]!='\\')
     {
        DbgPrint("(%s:%d) Non absolute pathname passed to %s\n",__FILE__,
               __LINE__,__FUNCTION__);
	return(STATUS_UNSUCCESSFUL);
     }
      
   next = &string[0];
   current = next+1;
   
   while (next!=NULL && 
	  BODY_TO_HEADER(current_dir)->ObjectType==ObDirectoryType)
     {		
	*next = '\\';
	current = next+1;
	next = wcschr(next+1,'\\');
	if (next!=NULL)
	  {
	     *next=0;
	  }

	DPRINT("current %w current[5] %x next %x ",current,current[5],next);
	if (next!=NULL)
	  {
	     DPRINT("(next+1) %w",next+1);
	  }
	DPRINT("\n",0);
	
	current_dir=(PDIRECTORY_OBJECT)ObDirLookup(current_dir,current,
						   Attributes);
	if (current_dir==NULL)
	  {
             DbgPrint("(%s:%d) Path component %w not found\n",__FILE__,
                    __LINE__,current);
	     return(STATUS_UNSUCCESSFUL);	   	     
	  }
	
	if (BODY_TO_HEADER(current_dir)->ObjectType==IoSymbolicLinkType)
	  {
	     current_dir = IoOpenSymlink(current_dir);	   
	  }
	
     }
   DPRINT("next %x\n",next);
   DPRINT("current %x current %w\n",current,current);
   if (next==NULL)
     {
	if (current_dir==NULL)
	  {
	     Status = STATUS_UNSUCCESSFUL;
	  }
	else
	  {
	     Status = STATUS_SUCCESS;
	  }
     }
   else
     {
	CHECKPOINT;
	*next = '\\';
	*UnparsedSection = next;
	if (BODY_TO_HEADER(current_dir)->ObjectType == IoDeviceType)
	  {
	     Status = STATUS_FS_QUERY_REQUIRED;
	  }
	else
	  {
	     Status = STATUS_UNSUCCESSFUL;
	  }     
     }
   CHECKPOINT;
   *Object = current_dir;
   DPRINT("(%s:%d) current_dir %x\n",__FILE__,__LINE__,current_dir);

   return(Status);
}
 

