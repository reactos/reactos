/* $Id: ntobj.c,v 1.8 2001/03/06 23:34:39 cnettel Exp $
 *
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/ob/ntobj.c
 * PURPOSE:       User mode interface to object manager
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *               10/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/id.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ************************************************************/

NTSTATUS
STDCALL
NtSetInformationObject (
	IN	HANDLE	ObjectHandle,
	IN	CINT	ObjectInformationClass,
	IN	PVOID	ObjectInformation,
	IN	ULONG	Length
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
internalNameBuilder
(
POBJECT_HEADER	ObjectHeader,
PUNICODE_STRING string)
/* So, what's the purpose of this function?
   It will take any OBJECT_HEADER and traverse the Parent structure up to the root
   and form the name, i.e. this will only work on objects where the Parent/Name fields
   have any meaning (not files) */
{
	NTSTATUS status;
	if (ObjectHeader->Parent)
        {
        	status = internalNameBuilder(BODY_TO_HEADER(ObjectHeader->Parent),string);
		if (status != STATUS_SUCCESS)
		{
			return status;
		}
	}
	if (ObjectHeader->Name.Buffer)
	{
		status = RtlAppendUnicodeToString(string, L"\\");
		if (status != STATUS_SUCCESS) return status;
		return RtlAppendUnicodeStringToString(string, &ObjectHeader->Name);
	}
	return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
NtQueryObject (
	IN	HANDLE	ObjectHandle,
	IN	CINT	ObjectInformationClass,
	OUT	PVOID	ObjectInformation,
	IN	ULONG	Length,
	OUT	PULONG	ResultLength
	)
/*Very, very, very new implementation. Test it!

  Probably we should add meaning to QueryName in POBJECT_TYPE, no matter if we know
  the correct parameters or not. For FILE_OBJECTs, it would probably look like the code
  for ObjectNameInformation below. You give it a POBJECT and a PUNICODE_STRING, and it
  returns the full path in the PUNICODE_STRING

  If we don't do it this way, we should anyway add switches and separate functions to handle
  the different object types*/
{
	POBJECT_NAME_INFORMATION nameinfo;
	POBJECT_TYPE_INFORMATION typeinfo;
	PFILE_NAME_INFORMATION filenameinfo;
	PVOID		Object;
	NTSTATUS	Status;  
	POBJECT_HEADER	ObjectHeader;
	PFILE_OBJECT    fileob;
   
	Status = ObReferenceObjectByHandle(
			ObjectHandle,
			0,
			NULL,
			KernelMode,
			& Object,
			NULL
			);
	if (Status != STATUS_SUCCESS)
	{
		return Status;
	}

	ObjectHeader = BODY_TO_HEADER(Object);
   

	switch (ObjectInformationClass)
	{
		case ObjectNameInformation:
			if (Length!=sizeof(OBJECT_NAME_INFORMATION)) return STATUS_INVALID_BUFFER_SIZE;
			nameinfo = (POBJECT_NAME_INFORMATION)ObjectInformation;
			(*ResultLength)=Length;		

			if (ObjectHeader->Type==InternalFileType)  // FIXME: Temporary QueryName implementation, or at least separate functions
			{
			  fileob = (PFILE_OBJECT) Object;
			  Status = internalNameBuilder(BODY_TO_HEADER(fileob->DeviceObject->Vpb->RealDevice), &nameinfo->Name);
			
			  if (Status != STATUS_SUCCESS)
			  {
				  ObDereferenceObject(Object);
				  return Status;
			  }
			  filenameinfo = ExAllocatePool(NonPagedPool,MAX_PATH*sizeof(WCHAR)+sizeof(ULONG));
			  IoQueryFileInformation(fileob,FileNameInformation,MAX_PATH*sizeof(WCHAR)+sizeof(ULONG), filenameinfo,NULL);

			  Status = RtlAppendUnicodeToString(&(nameinfo->Name), filenameinfo->FileName);

			  ExFreePool( filenameinfo);
			  ObDereferenceObject(Object);
			  return Status;
			}
			else
			if (ObjectHeader->Name.Buffer) // If it's got a name there, we can probably just make the full path through Name and Parent
			{
			  Status = internalNameBuilder(ObjectHeader, &nameinfo->Name);
	  		  ObDereferenceObject(Object);
			  return Status;
			}
			ObDereferenceObject(Object);
			return STATUS_NOT_IMPLEMENTED;
		case ObjectTypeInformation:
			typeinfo = (POBJECT_TYPE_INFORMATION)ObjectInformation;
			if (Length!=sizeof(OBJECT_TYPE_INFORMATION)) return STATUS_INVALID_BUFFER_SIZE;

			// FIXME: Is this supposed to only be the header's Name field?
			// Can somebody check/verify this?
			RtlCopyUnicodeString(&typeinfo->Name,&ObjectHeader->Name);

			if (Status != STATUS_SUCCESS)
			  {
				  ObDereferenceObject(Object);
				  return Status;
			  }

			RtlCopyUnicodeString(&typeinfo->Type,&ObjectHeader->ObjectType->TypeName);
//This should be info from the object header, not the object type, right?			
			typeinfo->TotalHandles = ObjectHeader-> HandleCount; 
			typeinfo->ReferenceCount = ObjectHeader -> RefCount;
			
			ObDereferenceObject(Object);
			return Status;
		default:
		ObDereferenceObject(Object);
		return STATUS_NOT_IMPLEMENTED;
	}
}


VOID
STDCALL
ObMakeTemporaryObject (
	PVOID	ObjectBody
	)
{
	POBJECT_HEADER	ObjectHeader;
   
	ObjectHeader = BODY_TO_HEADER(ObjectBody);
	ObjectHeader->Permanent = FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtMakeTemporaryObject
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS
STDCALL
NtMakeTemporaryObject (
	HANDLE	Handle
	)
{
	PVOID		Object;
	NTSTATUS	Status;  
	POBJECT_HEADER	ObjectHeader;
   
	Status = ObReferenceObjectByHandle(
			Handle,
			0,
			NULL,
			KernelMode,
			& Object,
			NULL
			);
	if (Status != STATUS_SUCCESS)
	{
		return Status;
	}

	ObjectHeader = BODY_TO_HEADER(Object);
	ObjectHeader->Permanent = FALSE;
   
	ObDereferenceObject(Object);
   
	return STATUS_SUCCESS;
}


/* EOF */
