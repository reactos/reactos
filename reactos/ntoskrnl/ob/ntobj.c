/* $Id: ntobj.c,v 1.14 2003/07/10 21:34:29 royce Exp $
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
  POBJECT_TYPE_INFORMATION typeinfo;
  POBJECT_HEADER ObjectHeader;
  PVOID Object;
  NTSTATUS Status;

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
	Status = ObQueryNameString (Object,
				    (POBJECT_NAME_INFORMATION)ObjectInformation,
				    Length,
				    ReturnLength);
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
 *
 * @implemented
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
