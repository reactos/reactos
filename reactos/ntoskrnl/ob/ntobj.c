/* $Id: ntobj.c,v 1.12 2003/06/01 15:09:34 ekohl Exp $
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

/**********************************************************************
 * NAME							EXPORTED
 *	NtSetInformationObject
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL
NtSetInformationObject (IN HANDLE ObjectHandle,
			IN CINT ObjectInformationClass,
			IN PVOID ObjectInformation,
			IN ULONG Length)
{
  UNIMPLEMENTED;
}


NTSTATUS
internalNameBuilder(POBJECT_HEADER ObjectHeader,
		    PUNICODE_STRING String)
/* So, what's the purpose of this function?
   It will take any OBJECT_HEADER and traverse the Parent structure up to the root
   and form the name, i.e. this will only work on objects where the Parent/Name fields
   have any meaning (not files) */
{
  NTSTATUS Status;

  if (ObjectHeader->Parent)
    {
      Status = internalNameBuilder(BODY_TO_HEADER(ObjectHeader->Parent),
				   String);
      if (Status != STATUS_SUCCESS)
	{
	  return Status;
	}
    }

  if (ObjectHeader->Name.Buffer)
    {
      Status = RtlAppendUnicodeToString(String,
					L"\\");
      if (Status != STATUS_SUCCESS)
	return Status;

      return RtlAppendUnicodeStringToString(String,
					    &ObjectHeader->Name);
    }

  return STATUS_SUCCESS;
}


/*Very, very, very new implementation. Test it!

  Probably we should add meaning to QueryName in POBJECT_TYPE, no matter if we know
  the correct parameters or not. For FILE_OBJECTs, it would probably look like the code
  for ObjectNameInformation below. You give it a POBJECT and a PUNICODE_STRING, and it
  returns the full path in the PUNICODE_STRING

  If we don't do it this way, we should anyway add switches and separate functions to handle
  the different object types*/

/**********************************************************************
 * NAME							EXPORTED
 *	NtQueryObject
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL
NtQueryObject (IN HANDLE ObjectHandle,
	       IN CINT ObjectInformationClass,
	       OUT PVOID ObjectInformation,
	       IN ULONG Length,
	       OUT PULONG ReturnLength)
{
  POBJECT_NAME_INFORMATION NameInfo;
  POBJECT_TYPE_INFORMATION typeinfo;
  PFILE_NAME_INFORMATION filenameinfo;
  PVOID Object;
  NTSTATUS Status;
  POBJECT_HEADER ObjectHeader;
  PFILE_OBJECT fileob;

  Status = ObReferenceObjectByHandle (ObjectHandle,
				      0,
				      NULL,
				      KeGetPreviousMode(),
				      &Object,
				      NULL);
  if (!NT_SUCCESS (Status))
    {
      return Status;
    }

  ObjectHeader = BODY_TO_HEADER(Object);

  switch (ObjectInformationClass)
    {
      case ObjectNameInformation:
#if 0
	Status = ObQueryNameString (Object,
				    (POBJECT_NAME_INFORMATION)ObjectInformation,
				    Length,
				    ReturnLength);
	break;
#endif

	if (Length < sizeof(OBJECT_NAME_INFORMATION) + sizeof(WCHAR))
	  return STATUS_INVALID_BUFFER_SIZE;

	NameInfo = (POBJECT_NAME_INFORMATION)ObjectInformation;
	*ReturnLength = 0;

	NameInfo->Name.MaximumLength = sizeof(WCHAR);
	NameInfo->Name.Length = 0;
	NameInfo->Name.Buffer = (PWCHAR)((ULONG_PTR)NameInfo + sizeof(OBJECT_NAME_INFORMATION));
	NameInfo->Name.Buffer[0] = 0;

	// FIXME: Temporary QueryName implementation, or at least separate functions
	if (ObjectHeader->Type==InternalFileType)
	  {
	    NameInfo->Name.MaximumLength = Length - sizeof(OBJECT_NAME_INFORMATION);
	    fileob = (PFILE_OBJECT) Object;
	    Status = internalNameBuilder(BODY_TO_HEADER(fileob->DeviceObject->Vpb->RealDevice),
					 &NameInfo->Name);
	    if (Status != STATUS_SUCCESS)
	      {
		NameInfo->Name.MaximumLength = 0;
		break;
	      }

	    filenameinfo = ExAllocatePool (NonPagedPool,
					   MAX_PATH * sizeof(WCHAR) + sizeof(ULONG));
	    if (filenameinfo == NULL)
	      {
		NameInfo->Name.MaximumLength = 0;
		Status = STATUS_INSUFFICIENT_RESOURCES;
		break;
	      }

	    Status = IoQueryFileInformation (fileob,
					     FileNameInformation,
					     MAX_PATH * sizeof(WCHAR) + sizeof(ULONG),
					     filenameinfo,
					     NULL);
	    if (Status != STATUS_SUCCESS)
	      {
		NameInfo->Name.MaximumLength = 0;
		ExFreePool (filenameinfo);
		break;
	      }

	    Status = RtlAppendUnicodeToString (&(NameInfo->Name),
					       filenameinfo->FileName);

	    ExFreePool (filenameinfo);

	    if (NT_SUCCESS(Status))
	      {
	        NameInfo->Name.MaximumLength = NameInfo->Name.Length + sizeof(WCHAR);
	        *ReturnLength = sizeof(OBJECT_NAME_INFORMATION) + NameInfo->Name.MaximumLength;
	      }
	  }
	else if (ObjectHeader->Name.Buffer)
	  {
	    // If it's got a name there, we can probably just make the full path through Name and Parent
	    NameInfo->Name.MaximumLength = Length - sizeof(OBJECT_NAME_INFORMATION);

	    Status = internalNameBuilder (ObjectHeader,
					  &NameInfo->Name);
	    if (NT_SUCCESS(Status))
	      {
	        NameInfo->Name.MaximumLength = NameInfo->Name.Length + sizeof(WCHAR);
	        *ReturnLength = sizeof(OBJECT_NAME_INFORMATION) + NameInfo->Name.MaximumLength;
	      }
	  }
	else
	  {
	    Status = STATUS_NOT_IMPLEMENTED;
	  }
	break;

      case ObjectTypeInformation:
	typeinfo = (POBJECT_TYPE_INFORMATION)ObjectInformation;
	if (Length!=sizeof(OBJECT_TYPE_INFORMATION))
	  return STATUS_INVALID_BUFFER_SIZE;

	// FIXME: Is this supposed to only be the header's Name field?
	// Can somebody check/verify this?
	RtlCopyUnicodeString(&typeinfo->Name,&ObjectHeader->Name);

	if (Status != STATUS_SUCCESS)
	  {
	    break;
	  }

	RtlCopyUnicodeString(&typeinfo->Type,&ObjectHeader->ObjectType->TypeName);
	//This should be info from the object header, not the object type, right?
	typeinfo->TotalHandles = ObjectHeader-> HandleCount;
	typeinfo->ReferenceCount = ObjectHeader -> RefCount;
	break;

      default:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
    }

  ObDereferenceObject(Object);

  return Status;
}


/**********************************************************************
 * NAME							EXPORTED
 *	ObMakeTemporaryObject
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
VOID STDCALL
ObMakeTemporaryObject (IN PVOID ObjectBody)
{
  POBJECT_HEADER ObjectHeader;

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
NtMakeTemporaryObject (IN HANDLE Handle)
{
  POBJECT_HEADER ObjectHeader;
  PVOID Object;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle(Handle,
				     0,
				     NULL,
				     KernelMode,
				     & Object,
				     NULL);
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
