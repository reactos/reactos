/* $Id$
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

#include <ntoskrnl.h>
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

static NTSTATUS
CopyDirectoryString(PUNICODE_STRING UnsafeTarget, PUNICODE_STRING Source, PUCHAR *Buffer)
{
    UNICODE_STRING Target;
    NTSTATUS Status;
    WCHAR NullWchar;

    Target.Length        = Source->Length;
    Target.MaximumLength = (Source->Length + sizeof (WCHAR));
    Target.Buffer        = (PWCHAR) *Buffer;
    Status = MmCopyToCaller(UnsafeTarget, &Target, sizeof(UNICODE_STRING));
    if (! NT_SUCCESS(Status))
      {
	return Status;
      }
    Status = MmCopyToCaller(*Buffer, Source->Buffer, Source->Length);
    if (! NT_SUCCESS(Status))
      {
	return Status;
      }
    *Buffer += Source->Length;
    NullWchar = L'\0';
    Status = MmCopyToCaller(*Buffer, &NullWchar, sizeof(WCHAR));
    if (! NT_SUCCESS(Status))
      {
	return Status;
      }
    *Buffer += sizeof(WCHAR);

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
 * 	DirectoryHandle
 * 		Handle, obtained with NtOpenDirectoryObject(), which
 * 		must grant DIRECTORY_QUERY access to the directory
 * 		object.
 * 		
 *	Buffer (OUT)
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
 *	Context
 *		Zero based index into the directory, interpretation
 *		depends on RestartScan.
 *		
 *	ReturnLength (OUT)
 *		Caller supplied storage for the number of bytes
 *		written (or NULL).
 *
 * RETURN VALUE
 * 	STATUS_SUCCESS - At least one (possibly more, depending on
 *                       parameters and buffer size) dir entry is
 *                       returned.
 *      STATUS_NO_MORE_ENTRIES - Directory is exhausted
 *      STATUS_BUFFER_TOO_SMALL - There isn't enough room in the
 *                                buffer to return even 1 entry.
 *                                ReturnLength will hold the required
 *                                buffer size to return all remaining
 *                                dir entries
 *      Other - Status code
 *
 *
 * NOTES
 *      Although you can iterate over the directory by calling this
 *      function multiple times, the directory is unlocked between
 *      calls. This means that another thread can change the directory
 *      and so iterating doesn't guarantee a consistent picture of the
 *      directory. Best thing is to retrieve all directory entries in
 *      one call.
 */
NTSTATUS STDCALL
NtQueryDirectoryObject (IN HANDLE DirectoryHandle,
			OUT PVOID Buffer,
			IN ULONG BufferLength,
			IN BOOLEAN ReturnSingleEntry,
			IN BOOLEAN RestartScan,
			IN OUT PULONG UnsafeContext,
			OUT PULONG UnsafeReturnLength OPTIONAL)
{
    PDIRECTORY_OBJECT   dir = NULL;
    PLIST_ENTRY         current_entry = NULL;
    PLIST_ENTRY         start_entry;
    POBJECT_HEADER      current = NULL;
    NTSTATUS            Status = STATUS_SUCCESS;
    ULONG               DirectoryCount = 0;
    ULONG               DirectoryIndex = 0;
    POBJECT_DIRECTORY_INFORMATION current_odi = (POBJECT_DIRECTORY_INFORMATION) Buffer;
    OBJECT_DIRECTORY_INFORMATION ZeroOdi;
    PUCHAR              FirstFree = (PUCHAR) Buffer;
    ULONG               Context;
    ULONG               RequiredSize;
    ULONG               NewValue;
    KIRQL               OldLevel;

    DPRINT("NtQueryDirectoryObject(DirectoryHandle %x)\n", DirectoryHandle);

    /* Check Context is not NULL */
    if (NULL == UnsafeContext)
      {
        return STATUS_INVALID_PARAMETER;
      }

    /* Reference the DIRECTORY_OBJECT */
    Status = ObReferenceObjectByHandle(DirectoryHandle,
				      DIRECTORY_QUERY,
				      ObDirectoryType,
				      UserMode,
				      (PVOID*)&dir,
				      NULL);
    if (!NT_SUCCESS(Status))
      {
        return Status;
      }

    KeAcquireSpinLock(&dir->Lock, &OldLevel);

    /*
     * Optionally, skip over some entries at the start of the directory
     * (use *ObjectIndex value)
     */
    start_entry = dir->head.Flink;
    if (! RestartScan)
      {
        register ULONG EntriesToSkip;

	Status = MmCopyFromCaller(&Context, UnsafeContext, sizeof(ULONG));
	if (! NT_SUCCESS(Status))
	  {
	    KeReleaseSpinLock(&dir->Lock, OldLevel);
            ObDereferenceObject(dir);
	    return Status;
	  }
	EntriesToSkip = Context;

	CHECKPOINT;
	
	for (; 0 != EntriesToSkip-- && start_entry != &dir->head;
	     start_entry = start_entry->Flink)
	  {
	    ;
	  }
	if ((0 != EntriesToSkip) && (start_entry == &dir->head))
	  {
	    KeReleaseSpinLock(&dir->Lock, OldLevel);
            ObDereferenceObject(dir);
            return STATUS_NO_MORE_ENTRIES;
	  }
      }

    /*
     * Compute number of entries that we will copy into the buffer and
     * the total size of all entries (even if larger than the buffer size)
     */
    DirectoryCount = 0;
    /* For the end sentenil */
    RequiredSize = sizeof(OBJECT_DIRECTORY_INFORMATION);
    for (current_entry = start_entry;
         current_entry != &dir->head;
         current_entry = current_entry->Flink)
      {
	current = CONTAINING_RECORD(current_entry, OBJECT_HEADER, Entry);

	RequiredSize += sizeof(OBJECT_DIRECTORY_INFORMATION) +
	                current->Name.Length + sizeof(WCHAR) +
	                current->ObjectType->TypeName.Length + sizeof(WCHAR);
	if (RequiredSize <= BufferLength &&
	    (! ReturnSingleEntry || DirectoryCount < 1))
	  {
	    DirectoryCount++;
	  }
      }

    /*
     * If there's no room to even copy a single entry then return error
     * status.
     */
    if (0 == DirectoryCount && 
        !(IsListEmpty(&dir->head) && BufferLength >= RequiredSize))
      {
	KeReleaseSpinLock(&dir->Lock, OldLevel);
	ObDereferenceObject(dir);
	if (NULL != UnsafeReturnLength)
	  {
	    Status = MmCopyToCaller(UnsafeReturnLength, &RequiredSize, sizeof(ULONG));
	  }
	return NT_SUCCESS(Status) ? STATUS_BUFFER_TOO_SMALL : Status;
      }

    /*
     * Move FirstFree to point to the Unicode strings area
     */
    FirstFree += (DirectoryCount + 1) * sizeof(OBJECT_DIRECTORY_INFORMATION);

    /* Scan the directory */
    current_entry = start_entry;
    for (DirectoryIndex = 0; DirectoryIndex < DirectoryCount; DirectoryIndex++) 
      {
	current = CONTAINING_RECORD(current_entry, OBJECT_HEADER, Entry);

	/*
	 * Copy the current directory entry's data into the buffer
	 * and update the OBJDIR_INFORMATION entry in the array.
	 */
	/* --- Object's name --- */
	Status = CopyDirectoryString(&current_odi->ObjectName, &current->Name, &FirstFree);
	if (! NT_SUCCESS(Status))
	  {
	    KeReleaseSpinLock(&dir->Lock, OldLevel);
	    ObDereferenceObject(dir);
	    return Status;
	  }
	/* --- Object type's name --- */
	Status = CopyDirectoryString(&current_odi->ObjectTypeName, &current->ObjectType->TypeName, &FirstFree);
	if (! NT_SUCCESS(Status))
	  {
	    KeReleaseSpinLock(&dir->Lock, OldLevel);
	    ObDereferenceObject(dir);
	    return Status;
	  }

	/* Next entry in the array */
	current_odi++;
	/* Next object in the directory */
	current_entry = current_entry->Flink;
    }

    /*
     * Don't need dir object anymore
     */
    KeReleaseSpinLock(&dir->Lock, OldLevel);
    ObDereferenceObject(dir);

    /* Terminate with all zero entry */
    memset(&ZeroOdi, '\0', sizeof(OBJECT_DIRECTORY_INFORMATION));
    Status = MmCopyToCaller(current_odi, &ZeroOdi, sizeof(OBJECT_DIRECTORY_INFORMATION));
    if (! NT_SUCCESS(Status))
      {
        return Status;
      }

    /*
     * Store current index in Context
     */
    if (RestartScan)
      {
	Context = DirectoryCount;
      }
    else
      {
	Context += DirectoryCount;
      }
    Status = MmCopyToCaller(UnsafeContext, &Context, sizeof(ULONG));
    if (! NT_SUCCESS(Status))
      {
        return Status;
      }

    /*
     * Report to the caller how much bytes
     * we wrote in the user buffer.
     */
    if (NULL != UnsafeReturnLength)
      {
	NewValue = FirstFree - (PUCHAR) Buffer;
	Status = MmCopyToCaller(UnsafeReturnLength, &NewValue, sizeof(ULONG));
	if (! NT_SUCCESS(Status))
	  {
	    return Status;
	  }
      }

    return Status;
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
  PDIRECTORY_OBJECT DirectoryObject;
  NTSTATUS Status;

  DPRINT("NtCreateDirectoryObject(DirectoryHandle %x, "
	 "DesiredAccess %x, ObjectAttributes %x, "
	 "ObjectAttributes->ObjectName %wZ)\n",
	 DirectoryHandle, DesiredAccess, ObjectAttributes,
	 ObjectAttributes->ObjectName);

  Status = NtOpenDirectoryObject (DirectoryHandle,
		                  DesiredAccess,
		                  ObjectAttributes);

  if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
  {
     Status = ObCreateObject (ExGetPreviousMode(),
			      ObDirectoryType,
			      ObjectAttributes,
			      ExGetPreviousMode(),
			      NULL,
			      sizeof(DIRECTORY_OBJECT),
			      0,
			      0,
			      (PVOID*)&DirectoryObject);
     if (!NT_SUCCESS(Status))
     {
        return Status;
     }

     Status = ObInsertObject ((PVOID)DirectoryObject,
			      NULL,
			      DesiredAccess,
			      0,
			      NULL,
			      DirectoryHandle);

     ObDereferenceObject(DirectoryObject);
  }

  return Status;
}

/* EOF */
