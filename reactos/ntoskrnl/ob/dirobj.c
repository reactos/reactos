/* $Id: dirobj.c,v 1.17 2003/06/02 10:03:52 ekohl Exp $
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

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/io.h>

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
NTSTATUS STDCALL
NtOpenDirectoryObject (OUT PHANDLE DirectoryHandle,
		       IN ACCESS_MASK DesiredAccess,
		       IN POBJECT_ATTRIBUTES ObjectAttributes)
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
 * 	Reads information from a directory in the system namespace.
 * 	
 * ARGUMENTS
 * 	DirObjHandle
 * 		Handle, obtained with NtOpenDirectoryObject(), which
 * 		must grant DIRECTORY_QUERY access to the directory
 * 		object.
 * 		
 *	DirObjInformation (OUT)
 *		Buffer to hold the data read.
 *		
 *	BufferLength
 *		Size of the buffer in bytes.
 *		
 *	ReturnSingleEntry
 *		When TRUE, only 1 entry is written in DirObjInformation;
 *		otherwise as many as will fit in the buffer.
 *		
 *	RestartScan
 *		If TRUE start reading at index 0.
 *		If FALSE start reading at the index specified
 *		by object index *ObjectIndex.
 *		
 *	ObjectIndex
 *		Zero based index into the directory, interpretation
 *		depends on RestartScan.
 *		
 *	DataWritten (OUT)
 *		Caller supplied storage for the number of bytes
 *		written (or NULL).
 *
 * RETURN VALUE
 * 	Status.
 *
 * REVISIONS
 * 	2001-05-01 (ea)
 * 		Changed 4th, and 5th parameter names after
 * 		G.Nebbett "WNT/W2k Native API Reference".
 * 		Mostly rewritten.
 */
NTSTATUS STDCALL
NtQueryDirectoryObject (IN HANDLE DirObjHandle,
			OUT POBJDIR_INFORMATION DirObjInformation,
			IN ULONG BufferLength,
			IN BOOLEAN ReturnSingleEntry,
			IN BOOLEAN RestartScan,
			IN OUT PULONG ObjectIndex,
			OUT PULONG DataWritten OPTIONAL)
{
    PDIRECTORY_OBJECT   dir = NULL;
    PLIST_ENTRY         current_entry = NULL;
    POBJECT_HEADER      current = NULL;
    ULONG               i = 0;
    NTSTATUS            Status = STATUS_SUCCESS;
    DWORD		DirectoryCount = 0;
    DWORD		DirectorySize = 0;
    ULONG               SpaceLeft = BufferLength;
    ULONG               SpaceRequired = 0;
    ULONG               NameLength = 0;
    ULONG               TypeNameLength = 0;
    POBJDIR_INFORMATION current_odi = DirObjInformation;
    PBYTE               FirstFree = (PBYTE) DirObjInformation;


    DPRINT("NtQueryDirectoryObject(DirObjHandle %x)\n", DirObjHandle);

    /* FIXME: if previous mode == user, use ProbeForWrite
     * on user params. */

    /* Reference the DIRECTORY_OBJECT */
    Status = ObReferenceObjectByHandle(DirObjHandle,
				      DIRECTORY_QUERY,
				      ObDirectoryType,
				      UserMode,
				      (PVOID*)&dir,
				      NULL);
    if (!NT_SUCCESS(Status))
      {
        return (Status);
      }
    /* Check ObjectIndex is not NULL */
    if (NULL == ObjectIndex)
      {
        return (STATUS_INVALID_PARAMETER);
      }
    /*
     * Compute the number of directory entries
     * and the size of the array (in bytes).
     * One more entry marks the end of the array.
     */
    if (FALSE == ReturnSingleEntry)
    {
        for ( current_entry = dir->head.Flink;
              (current_entry != & dir->head);
              current_entry = current_entry->Flink
              )
        {
          ++ DirectoryCount;
        }
    }
    else
    {
	DirectoryCount = 1;
    }
    // count is DirectoryCount + one null entry
    DirectorySize = (DirectoryCount + 1) * sizeof (OBJDIR_INFORMATION);
    if (DirectorySize > SpaceLeft)
    {
	return (STATUS_BUFFER_TOO_SMALL);
    }
    /*
     * Optionally, skip over some entries at the start of the directory
     * (use *ObjectIndex value)
     */
    current_entry = dir->head.Flink;
    if (FALSE == RestartScan)
      {
	/* RestartScan == FALSE */
        register ULONG EntriesToSkip = *ObjectIndex;

	CHECKPOINT;
	
	for (	;
		((EntriesToSkip --) && (current_entry != & dir->head));
	        current_entry = current_entry->Flink
		);
	if ((EntriesToSkip) && (current_entry == & dir->head))
	  {
            return (STATUS_NO_MORE_ENTRIES);
	  }
      }
    /*
     * Initialize the array of OBJDIR_INFORMATION.
     */
    RtlZeroMemory (FirstFree, DirectorySize);
    /*
     * Move FirstFree to point to the Unicode strings area
     */
    FirstFree += DirectorySize;
    /*
     * Compute how much space is left after allocating the
     * array in the user buffer.
     */
    SpaceLeft -= DirectorySize;
    /* Scan the directory */
    do
    { 
        /*
         * Check if we reached the end of the directory.
         */
        if (current_entry == & dir->head)
          {
      /* Any data? */
	    if (i) break; /* DONE */
	    /* FIXME: better error handling here! */
	    return (STATUS_NO_MORE_ENTRIES);
          }
  /*
	 * Compute the current OBJECT_HEADER memory
	 * object's address.
	 */
   current = CONTAINING_RECORD(current_entry, OBJECT_HEADER, Entry);
  /*
   * Compute the space required in the user buffer to copy
   * the data from the current object:
	 *
	 * Name (WCHAR) 0 TypeName (WCHAR) 0
   */
	NameLength = (wcslen (current->Name.Buffer) * sizeof (WCHAR));
	TypeNameLength = (wcslen (current->ObjectType->TypeName.Buffer) * sizeof (WCHAR));
  SpaceRequired = (NameLength + 1) * sizeof (WCHAR)
    + (TypeNameLength + 1) * sizeof (WCHAR);
	/*
	 * Check for free space in the user buffer.
	 */
	if (SpaceRequired > SpaceLeft)
	{
		return (STATUS_BUFFER_TOO_SMALL);
	}
  /*
   * Copy the current directory entry's data into the buffer
	 * and update the OBJDIR_INFORMATION entry in the array.
   */
	/* --- Object's name --- */
	current_odi->ObjectName.Length        = NameLength;
	current_odi->ObjectName.MaximumLength = (NameLength + sizeof (WCHAR));
	current_odi->ObjectName.Buffer        = (PWCHAR) FirstFree;
	wcscpy ((PWCHAR) FirstFree, current->Name.Buffer);
	FirstFree += (current_odi->ObjectName.MaximumLength);
	/* --- Object type's name --- */
	current_odi->ObjectTypeName.Length        = TypeNameLength;
	current_odi->ObjectTypeName.MaximumLength = (TypeNameLength + sizeof (WCHAR));
	current_odi->ObjectTypeName.Buffer        = (PWCHAR) FirstFree;
	wcscpy ((PWCHAR) FirstFree, current->ObjectType->TypeName.Buffer);
	FirstFree += (current_odi->ObjectTypeName.MaximumLength);
	/* Next entry in the array */
	++ current_odi;
	/* Decrease the space left count */	
	SpaceLeft -= SpaceRequired;
	/* Increase the object index number */
	++ i;
	/* Next object in the directory */
	current_entry = current_entry->Flink;

    } while (FALSE == ReturnSingleEntry);
    /*
     * Store current index in ObjectIndex
     */
    *ObjectIndex += DirectoryCount;
    /*
     * Report to the caller how much bytes
     * we wrote in the user buffer.
     */
    if (NULL != DataWritten) 
      {
        *DataWritten = (BufferLength - SpaceLeft);
      }
    return (STATUS_SUCCESS);
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
NTSTATUS STDCALL
NtCreateDirectoryObject (OUT PHANDLE DirectoryHandle,
			 IN ACCESS_MASK DesiredAccess,
			 IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   PDIRECTORY_OBJECT dir;

   DPRINT("NtCreateDirectoryObject(DirectoryHandle %x, "
	  "DesiredAccess %x, ObjectAttributes %x, "
	  "ObjectAttributes->ObjectName %S)\n",
	  DirectoryHandle, DesiredAccess, ObjectAttributes,
	  ObjectAttributes->ObjectName);
   
   return(ObCreateObject(DirectoryHandle,
			 DesiredAccess,
			 ObjectAttributes,
			 ObDirectoryType,
			 (PVOID*)&dir));
}

/* EOF */
