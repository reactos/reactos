/* $Id: dirobj.c,v 1.6 1999/11/27 03:31:08 ekohl Exp $
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
NTSTATUS
STDCALL
NtOpenDirectoryObject (
	PHANDLE			DirectoryHandle,
	ACCESS_MASK		DesiredAccess,
	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
	PVOID		Object;
	NTSTATUS	Status;

	*DirectoryHandle = 0;

	Status = ObReferenceObjectByName(
			ObjectAttributes->ObjectName,
			ObjectAttributes->Attributes,
			NULL,
			DesiredAccess,
			ObDirectoryType,
			UserMode,
			NULL,
			& Object
			);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	Status = ObCreateHandle(
			PsGetCurrentProcess(),
			Object,
			DesiredAccess,
			FALSE,
			DirectoryHandle
			);
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
NTSTATUS
STDCALL
NtQueryDirectoryObject (
	IN	HANDLE			DirObjHandle,
	OUT	POBJDIR_INFORMATION	DirObjInformation, 
	IN	ULONG			BufferLength, 
	IN	BOOLEAN			GetNextIndex, 
	IN	BOOLEAN			IgnoreInputIndex, 
	IN OUT	PULONG			ObjectIndex,
	OUT	PULONG			DataWritten	OPTIONAL
	)
{
	PDIRECTORY_OBJECT	dir = NULL;
	ULONG			EntriesToRead;
	PLIST_ENTRY		current_entry;
	POBJECT_HEADER		current;
	ULONG			i=0;
	ULONG			EntriesToSkip;
	NTSTATUS		Status;


	DPRINT(
		"NtQueryDirectoryObject(DirObjHandle %x)\n",
		DirObjHandle
		);
	DPRINT(
		"dir %x namespc_root %x\n",
		dir,
		HEADER_TO_BODY(&(namespc_root.hdr))
		);

//	assert_irql(PASSIVE_LEVEL);

	Status = ObReferenceObjectByHandle(
			DirObjHandle,
			DIRECTORY_QUERY,
			ObDirectoryType,
			UserMode,
			(PVOID *) & dir,
			NULL
			);
	if (Status != STATUS_SUCCESS)
	{
		return Status;
	}

	EntriesToRead = BufferLength / sizeof (OBJDIR_INFORMATION);
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
		while ( (i < EntriesToSkip) && (current_entry != NULL))
		{
			current_entry = current_entry->Flink;
		}
	}

	DPRINT("DirObjInformation %x\n",DirObjInformation);

	/*
	 * Read the maximum entries possible into the buffer
	 */
	while ( (i < EntriesToRead) && (current_entry != (&(dir->head))))
	{
		current = CONTAINING_RECORD(
				current_entry,
				OBJECT_HEADER,
				Entry
				);
		DPRINT(
			"Scanning %w\n",
			current->Name.Buffer
			);
		
		DirObjInformation[i].ObjectName.Buffer = 
			ExAllocatePool(
				NonPagedPool,
				(current->Name.Length + 1) * 2
				);
		DirObjInformation[i].ObjectName.Length =
			current->Name.Length;
		DirObjInformation[i].ObjectName.MaximumLength =
			current->Name.Length;
		
		DPRINT(
			"DirObjInformation[i].ObjectName.Buffer %x\n",
			DirObjInformation[i].ObjectName.Buffer
			);
		
		RtlCopyUnicodeString(
			& DirObjInformation[i].ObjectName,
			& (current->Name)
			);
		i++;
		current_entry = current_entry->Flink;
		(*DataWritten) = (*DataWritten) + sizeof (OBJDIR_INFORMATION);

		CHECKPOINT;
	}
	CHECKPOINT;

	/*
	 * Optionally, count the number of entries in the directory
	 */
	if (GetNextIndex)
	{
		*ObjectIndex = i;
	}
	else
	{
		while ( current_entry != (&(dir->head)) )
		{
			current_entry = current_entry->Flink;
			i++;
		}
		*ObjectIndex = i;
	}
	return STATUS_SUCCESS;
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
NTSTATUS
STDCALL
NtCreateDirectoryObject (
	PHANDLE			DirectoryHandle,
	ACCESS_MASK		DesiredAccess,
	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
	PDIRECTORY_OBJECT dir;

	dir = ObCreateObject(
		DirectoryHandle,
		DesiredAccess,
		ObjectAttributes,
		ObDirectoryType
		);
	return STATUS_SUCCESS;
}

/* EOF */
