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
#include <ddk/ntddk.h>
#include <internal/objmgr.h>
#include <internal/string.h>
#include <internal/kernel.h>
#include <wstring.h>

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

PWSTR Rtlstrrchr(PUNICODE_STRING string, WCHAR c)
{
   int i;
   DPRINT("string->Length %d\n",string->Length);
   for (i=(string->Length-1);i>=0;i--)
     {
	if (string->Buffer[i]==c)
	  {
	     return(&string->Buffer[i]);
	  }
     }
   return(NULL);
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
   UNICODE_STRING path;
   PWSTR name = NULL;
   PDIRECTORY_OBJECT parent_dir;
   
   DPRINT("InitalizeObjectAttributes(ObjectName %w)\n",ObjectName->Buffer);
   
   if (RootDirectory!=NULL)
     {
	ObReferenceObjectByHandle(RootDirectory,DIRECTORY_TRAVERSE,NULL,
				  UserMode,(PVOID*)&parent_dir,NULL);
     }
   else
     {
	parent_dir = HEADER_TO_BODY((POBJECT_HEADER)&namespc_root);
     }
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   
   path.Buffer = ExAllocatePool(NonPagedPool,
				ObjectName->Length*sizeof(WCHAR));
   path.MaximumLength = ObjectName->Length;
   RtlCopyUnicodeString(&path,ObjectName);   
 
   /*
    * Seperate the path into the name of the object and the name of its
    * direct parent directory
    */
   name = Rtlstrrchr(&path,'\\');
   *name=0;
   
   /*
    * Find the objects parent directory
    */
   DPRINT("parent_dir %x\n",&(parent_dir->Type));
   parent_dir=(PDIRECTORY_OBJECT)ObLookupObject(parent_dir,&path);
   if (parent_dir==NULL)
     {
	return;
     }

   /*
    * Make sure the parent directory doesn't disappear
    */
   ObReferenceObjectByPointer(parent_dir,DIRECTORY_CREATE_OBJECT,NULL,
			      UserMode);

   InitializedAttributes->Attributes = Attributes;
   InitializedAttributes->parent = parent_dir;
   RtlInitUnicodeString(&InitializedAttributes->name,name+1);
   InitializedAttributes->path = path;
}

int _wcscmp(wchar_t* str1, wchar_t* str2)
{
   while ( (*str1)==(*str2) )
     {
	str1++;
	str2++;
	if ( (*str1)==((wchar_t)0) && (*str1)==((wchar_t)0) )
	  {
	     return(0);
	  }
     }
   return( (*str1) - (*str2) );
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
   DPRINT("ObDirLookup(dir %x, name %w\n",dir,name);
   while (current!=NULL)
     {
	current_obj = CONTAINING_RECORD(current,OBJECT_HEADER,entry);
	DPRINT("current_obj->name %w\n",current_obj->name.Buffer);
	if ( _wcscmp(current_obj->name.Buffer, name)==0)
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

wchar_t* _wcschr(wchar_t* str, wchar_t ch)
{
   while ((*str)!=((wchar_t)0))
     {
	if ((*str)==ch)
	  {
	     return(str);
	  }
	str++;
     }
   return(NULL);
}

PVOID ObLookupObject(PDIRECTORY_OBJECT root, PUNICODE_STRING _string)
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
   PDIRECTORY_OBJECT current_dir = root;
   POBJECT_HEADER current_hdr;
   PWSTR string;
   
   DPRINT("root %x string %w\n",root,_string->Buffer);
   
   if (root==NULL)
     {
	current_dir = HEADER_TO_BODY(&(namespc_root.hdr));
     }
  
   /*
    * Bit of a hack this
    */
   if (_string->Buffer[0]==0)
   {
      DPRINT("current_dir %x\n",current_dir);
      DPRINT("type %d\n",current_dir->Type);
      return(current_dir);
   }

   string=(PWSTR)ExAllocatePool(NonPagedPool,(_string->Length+1)*2);
   wcscpy(string,_string->Buffer);

   DPRINT("string = %w\n",string);
   
   if (string[0]!='\\')
     {
        printk("(%s:%d) Non absolute pathname passed to %s\n",__FILE__,
               __LINE__,__FUNCTION__);
	return(NULL);
     }
      
   current = string+1;
   DPRINT("current %w\n",current);
   next = _wcschr(string+1,'\\');
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
	next = _wcschr(next+1,'\\');
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
        ExFreePool(string);
        return(NULL);
   }
   DPRINT("Returning %x %x\n",current_hdr,HEADER_TO_BODY(current_hdr));
   ExFreePool(string);
   return(HEADER_TO_BODY(current_hdr));
}
 
