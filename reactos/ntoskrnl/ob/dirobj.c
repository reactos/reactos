/* $Id: dirobj.c,v 1.8 2000/03/26 22:00:09 dwelch Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/ob/dirobj.c
 * PURPOSE:        Interface functions to directory object
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 22/05/98: Created
 */

/* INCLUDES ***************************************************************/

#include <wchar.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS **************************************************************/


/**********************************************************************
 * NAME							EXPORTED
 * 	NtOpenDirectoryObject
 *
 * DESCRIPTION
 * 	Opens a namespace directory object.
 * 	
 * ARGUMENTS
 *	DirectoryHandle (OUT)
 *		Variable which receives the directory handle.
 *		
 *	DesiredAccess
 *		Desired access to the directory.
 *		
 *	ObjectAttributes
 *		Structure describing the directory.
 *		
 * RETURN VALUE
 * 	Status.
 * 	
 * NOTES
 * 	Undocumented.
 */
NTSTATUS STDCALL NtOpenDirectoryObject(PHANDLE DirectoryHandle,
				       ACCESS_MASK DesiredAccess,
				       POBJECT_ATTRIBUTES ObjectAttributes)
{
   PVOID Object;
   NTSTATUS Status;

   *DirectoryHandle = 0;
   
   Status = ObReferenceObjectByName(ObjectAttributes->ObjectName,
				    ObjectAttributes->Attributes,
				    NULL,
				    DesiredAccess,
				    ObDirectoryType,
				    UserMode,
				    NULL,
				    &Object);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }
   
   Status = ObCreateHandle(PsGetCurrentProcess(),
			   Object,
			   DesiredAccess,
			   FALSE,
			   DirectoryHandle);
   return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtQueryDirectoryObject
 * 
 * DESCRIPTION
 * 	Reads information from a namespace directory.
 * 	
 * ARGUMENTS
 *	DirObjInformation (OUT)
 *		Buffer to hold the data read.
 *		
 *	BufferLength
 *		Size of the buffer in bytes.
 *		
 *	GetNextIndex
 *		If TRUE then set ObjectIndex to the index of the
 *		next object.
 *		If FALSE then set ObjectIndex to the number of
 *		objects in the directory.
 *		
 *	IgnoreInputIndex
 *		If TRUE start reading at index 0.
 *		If FALSE start reading at the index specified
 *		by object index.
 *		
 *	ObjectIndex
 *		Zero based index into the directory, interpretation
 *		depends on IgnoreInputIndex and GetNextIndex.
 *		
 *	DataWritten (OUT)
 *		Caller supplied storage for the number of bytes
 *		written (or NULL).
 *
 * RETURN VALUE
 * 	Status.
 */
NTSTATUS STDCALL NtQueryDirectoryObject (IN HANDLE DirObjHandle,
					 OUT POBJDIR_INFORMATION 
					            DirObjInformation, 
					 IN ULONG BufferLength, 
					 IN BOOLEAN GetNextIndex, 
					 IN BOOLEAN IgnoreInputIndex, 
					 IN OUT PULONG ObjectIndex,
					 OUT PULONG DataWritten OPTIONAL)
{
   PDIRECTORY_OBJECT dir = NULL;
   PLIST_ENTRY current_entry;
   POBJECT_HEADER current;
   ULONG i = 0;
   ULONG EntriesToSkip;
   NTSTATUS Status;
   ULONG SpaceRequired;
   ULONG FirstFree;
   
   DPRINT("NtQueryDirectoryObject(DirObjHandle %x)\n", DirObjHandle);

   Status = ObReferenceObjectByHandle(DirObjHandle,
				      DIRECTORY_QUERY,
				      ObDirectoryType,
				      UserMode,
				      (PVOID*)&dir,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   /*
    * Optionally, skip over some entries at the start of the directory
    */
   if (!IgnoreInputIndex)
     {
	CHECKPOINT;
	
	EntriesToSkip = *ObjectIndex;
	i = 0;
	current_entry = dir->head.Flink;
	
	while ((i < EntriesToSkip) && (current_entry != &dir->head))
	  {
	     current_entry = current_entry->Flink;
	     i++;
	  }
     }
   else
     {
	current_entry = dir->head.Flink;
	i = 0;
     }
   
   /*
    * Check if we have reached the end of the directory
    */
   if (current_entry != &dir->head)
     {
	*DataWritten = 0;
	return(STATUS_NO_MORE_ENTRIES);
     }
   
   /*
    * Read the current entry into the buffer
    */
   FirstFree = sizeof(OBJDIR_INFORMATION);
   
   current = CONTAINING_RECORD(current_entry, OBJECT_HEADER, Entry);
   
   SpaceRequired = (wcslen(current->Name.Buffer) + 1) * 2;
   SpaceRequired = SpaceRequired + 
     ((wcslen(current->ObjectType->TypeName.Buffer) + 1) * 2);
   SpaceRequired = SpaceRequired + sizeof(OBJDIR_INFORMATION);
   
   if (SpaceRequired <= BufferLength)
     {
	
	DirObjInformation->ObjectName.Length = 
	  current->Name.Length;
	DirObjInformation->ObjectName.MaximumLength = 
	  current->Name.Length;
	DirObjInformation->ObjectName.Buffer = 
	  (((PVOID)DirObjInformation) + FirstFree);
	FirstFree = FirstFree + (wcslen(current->Name.Buffer + 1) * 2);
	wcscpy(DirObjInformation->ObjectName.Buffer,
	       current->Name.Buffer);
		
	DirObjInformation->ObjectTypeName.Length = 
	  current->ObjectType->TypeName.Length;
	DirObjInformation->ObjectTypeName.MaximumLength = 
	  current->ObjectType->TypeName.Length;
	DirObjInformation->ObjectName.Buffer = 
	  (((PVOID)DirObjInformation) + FirstFree);
	FirstFree = FirstFree + 
	  (wcslen(current->ObjectType->TypeName.Buffer + 1) * 2);
	wcscpy(DirObjInformation->ObjectTypeName.Buffer,
	       current->ObjectType->TypeName.Buffer);
	
	*DataWritten = SpaceRequired;
	Status = STATUS_SUCCESS;
     }
   else
     {
	Status = STATUS_BUFFER_TOO_SMALL;
     }
   
   /*
    * Store into ObjectIndex
    */
   if (GetNextIndex)
     {
	*ObjectIndex = i + 1;
     }
   else
     {
	i = 0;
	current_entry = dir->head.Flink;
	while (current_entry != (&dir->head))
	  {
	     current_entry = current_entry->Flink;
	     i++;
	  }
	*ObjectIndex = i;
     }
   
   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME						(EXPORTED as Zw)
 * 	NtCreateDirectoryObject
 * 	
 * DESCRIPTION
 * 	Creates or opens a directory object (a container for other
 *	objects).
 *	
 * ARGUMENTS
 *	DirectoryHandle (OUT)
 *		Caller supplied storage for the handle of the 
 *		directory.
 *		
 *	DesiredAccess
 *		Access desired to the directory.
 *		
 *	ObjectAttributes
 *		Object attributes initialized with
 *		InitializeObjectAttributes.
 *		
 * RETURN VALUE
 * 	Status.
 */
NTSTATUS STDCALL NtCreateDirectoryObject (PHANDLE DirectoryHandle,
					  ACCESS_MASK DesiredAccess,
					  POBJECT_ATTRIBUTES ObjectAttributes)
{
   PDIRECTORY_OBJECT dir;

   DPRINT("NtCreateDirectoryObject(DirectoryHandle %x, "
	  "DesiredAccess %x, ObjectAttributes %x, "
	  "ObjectAttributes->ObjectName %S)\n",
	  DirectoryHandle, DesiredAccess, ObjectAttributes,
	  ObjectAttributes->ObjectName);
   
   dir = ObCreateObject(DirectoryHandle,
			DesiredAccess,
			ObjectAttributes,
			ObDirectoryType);
   return(STATUS_SUCCESS);
}

/* EOF */
