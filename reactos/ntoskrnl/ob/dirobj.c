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
   HANDLE hDirectory;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PreviousMode = ExGetPreviousMode();
   
   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(DirectoryHandle,
                     sizeof(HANDLE),
                     sizeof(ULONG));
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
     
     if(!NT_SUCCESS(Status))
     {
       DPRINT1("NtOpenDirectoryObject failed, Status: 0x%x\n", Status);
       return Status;
     }
   }
   
   Status = ObOpenObjectByName(ObjectAttributes,
                               ObDirectoryType,
                               NULL,
                               PreviousMode,
                               DesiredAccess,
                               NULL,
                               &hDirectory);
   if(NT_SUCCESS(Status))
   {
     _SEH_TRY
     {
       *DirectoryHandle = hDirectory;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }
   
   return Status;
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
			IN OUT PULONG Context,
			OUT PULONG ReturnLength OPTIONAL)
{
  PDIRECTORY_OBJECT Directory;
  KPROCESSOR_MODE PreviousMode;
  ULONG SkipEntries = 0;
  ULONG NextEntry = 0;
  NTSTATUS Status = STATUS_SUCCESS;
  
  PreviousMode = ExGetPreviousMode();

  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      /* a test showed that the Buffer pointer just has to be 16 bit aligned,
         propably due to the fact that most information that needs to be copied
         is unicode strings */
      ProbeForWrite(Buffer,
                    BufferLength,
                    sizeof(WCHAR));
      ProbeForWrite(Context,
                    sizeof(ULONG),
                    sizeof(ULONG));
      if(!RestartScan)
      {
        SkipEntries = *Context;
      }
      if(ReturnLength != NULL)
      {
        ProbeForWrite(ReturnLength,
                      sizeof(ULONG),
                      sizeof(ULONG));
      }
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(!NT_SUCCESS(Status))
    {
      DPRINT1("NtQueryDirectoryObject failed, Status: 0x%x\n", Status);
      return Status;
    }
  }
  else if(!RestartScan)
  {
    SkipEntries = *Context;
  }
  
  Status = ObReferenceObjectByHandle(DirectoryHandle,
                                     DIRECTORY_QUERY,
                                     ObDirectoryType,
                                     PreviousMode,
                                     (PVOID*)&Directory,
                                     NULL);
  if(NT_SUCCESS(Status))
  {
    PVOID TemporaryBuffer = ExAllocatePool(PagedPool,
                                           BufferLength);
    if(TemporaryBuffer != NULL)
    {
      POBJECT_HEADER EntryHeader;
      PLIST_ENTRY ListEntry;
      KIRQL OldLevel;
      ULONG RequiredSize = 0;
      ULONG nDirectories = 0;
      POBJECT_DIRECTORY_INFORMATION DirInfo = (POBJECT_DIRECTORY_INFORMATION)TemporaryBuffer;

      KeAcquireSpinLock(&Directory->Lock, &OldLevel);

      for(ListEntry = Directory->head.Flink;
          ListEntry != &Directory->head;
          ListEntry = ListEntry->Flink)
      {
        NextEntry++;
        if(SkipEntries == 0)
        {
          PUNICODE_STRING Name, Type;
          ULONG EntrySize;

          EntryHeader = CONTAINING_RECORD(ListEntry, OBJECT_HEADER, Entry);

          /* calculate the size of the required buffer space for this entry */
          Name = (EntryHeader->Name.Length != 0 ? &EntryHeader->Name : NULL);
          Type = &EntryHeader->ObjectType->TypeName;
          EntrySize = sizeof(OBJECT_DIRECTORY_INFORMATION) +
                      ((Name != NULL) ? ((ULONG)Name->Length + sizeof(WCHAR)) : 0) +
                      (ULONG)EntryHeader->ObjectType->TypeName.Length + sizeof(WCHAR);

          if(RequiredSize + EntrySize <= BufferLength)
          {
            /* the buffer is large enough to receive this entry. It would've
               been much easier if the strings were directly appended to the
               OBJECT_DIRECTORY_INFORMATION structured written into the buffer */
            if(Name != NULL)
              DirInfo->ObjectName = *Name;
            else
            {
              DirInfo->ObjectName.Length = DirInfo->ObjectName.MaximumLength = 0;
              DirInfo->ObjectName.Buffer = NULL;
            }
            DirInfo->ObjectTypeName = *Type;

            nDirectories++;
            RequiredSize += EntrySize;

            if(ReturnSingleEntry)
            {
              /* we're only supposed to query one entry, so bail and copy the
                 strings to the buffer */
              break;
            }
            DirInfo++;
          }
          else
          {
            if(ReturnSingleEntry)
            {
              /* the buffer is too small, so return the number of bytes that
                 would've been required for this query */
              RequiredSize += EntrySize;
              Status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
              /* just copy the entries that fit into the buffer */
              Status = STATUS_NO_MORE_ENTRIES;
            }
            break;
          }
        }
        else
        {
          /* skip the entry */
          SkipEntries--;
        }
      }

      if(NT_SUCCESS(Status))
      {
        if(SkipEntries > 0 || nDirectories == 0)
        {
          /* we skipped more entries than the directory contains, nothing more to do */
          Status = STATUS_NO_MORE_ENTRIES;
        }
        else
        {
          _SEH_TRY
          {
            POBJECT_DIRECTORY_INFORMATION DestDirInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;
            PWSTR strbuf = (PWSTR)((POBJECT_DIRECTORY_INFORMATION)Buffer + nDirectories);

            /* copy all OBJECT_DIRECTORY_INFORMATION structures to the buffer and
               just append all strings (whose pointers are stored in the buffer!)
               and replace the pointers */
            for(DirInfo = (POBJECT_DIRECTORY_INFORMATION)TemporaryBuffer;
                nDirectories > 0;
                nDirectories--, DirInfo++, DestDirInfo++)
            {
              if(DirInfo->ObjectName.Length > 0)
              {
                DestDirInfo->ObjectName.Length = DirInfo->ObjectName.Length;
                DestDirInfo->ObjectName.MaximumLength = DirInfo->ObjectName.MaximumLength;
                DestDirInfo->ObjectName.Buffer = strbuf;
                RtlCopyMemory(strbuf,
                              DirInfo->ObjectName.Buffer,
                              DirInfo->ObjectName.Length);
                /* NULL-terminate the string */
                strbuf[DirInfo->ObjectName.Length / sizeof(WCHAR)] = L'\0';
                strbuf += (DirInfo->ObjectName.Length / sizeof(WCHAR)) + 1;
              }
              
              DestDirInfo->ObjectTypeName.Length = DirInfo->ObjectTypeName.Length;
              DestDirInfo->ObjectTypeName.MaximumLength = DirInfo->ObjectTypeName.MaximumLength;
              DestDirInfo->ObjectTypeName.Buffer = strbuf;
              RtlCopyMemory(strbuf,
                            DirInfo->ObjectTypeName.Buffer,
                            DirInfo->ObjectTypeName.Length);
              /* NULL-terminate the string */
              strbuf[DirInfo->ObjectTypeName.Length / sizeof(WCHAR)] = L'\0';
              strbuf += (DirInfo->ObjectTypeName.Length / sizeof(WCHAR)) + 1;
            }
          }
          _SEH_HANDLE
          {
            Status = _SEH_GetExceptionCode();
          }
          _SEH_END;
        }
      }

      KeReleaseSpinLock(&Directory->Lock, OldLevel);
      ObDereferenceObject(Directory);
      
      ExFreePool(TemporaryBuffer);

      if(NT_SUCCESS(Status) || ReturnSingleEntry)
      {
        _SEH_TRY
        {
          *Context = NextEntry;
          if(ReturnLength != NULL)
          {
            *ReturnLength = RequiredSize;
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
      }
    }
    else
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
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
  PDIRECTORY_OBJECT Directory;
  HANDLE hDirectory;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;
  
  DPRINT("NtCreateDirectoryObject(DirectoryHandle %x, "
	 "DesiredAccess %x, ObjectAttributes %x\n",
	 DirectoryHandle, DesiredAccess, ObjectAttributes);

  PreviousMode = ExGetPreviousMode();

  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeForWrite(DirectoryHandle,
                    sizeof(HANDLE),
                    sizeof(ULONG));
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateDirectoryObject failed, Status: 0x%x\n", Status);
      return Status;
    }
  }

  Status = ObCreateObject(PreviousMode,
                          ObDirectoryType,
                          ObjectAttributes,
                          PreviousMode,
                          NULL,
                          sizeof(DIRECTORY_OBJECT),
                          0,
                          0,
                          (PVOID*)&Directory);
  if(NT_SUCCESS(Status))
  {
    Status = ObInsertObject((PVOID)Directory,
                            NULL,
                            DesiredAccess,
                            0,
                            NULL,
                            &hDirectory);
    ObDereferenceObject(Directory);

    if(NT_SUCCESS(Status))
    {
      _SEH_TRY
      {
        *DirectoryHandle = hDirectory;
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;
    }
  }

  return Status;
}

/* EOF */
