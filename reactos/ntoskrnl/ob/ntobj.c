/* $Id: ntobj.c,v 1.15 2003/10/04 17:11:58 ekohl Exp $
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
			IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
			IN PVOID ObjectInformation,
			IN ULONG Length)
{
#if 0
  POBJECT_HEADER ObjectHeader;
  PVOID Object;
  NTSTATUS Status;

  if (ObjectInformationClass != ObjectHandleInformation)
    return STATUS_INVALID_INFO_CLASS;

  if (Length != sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION))
    return STATUS_INFO_LENGTH_MISMATCH;

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

  /* FIXME: Change handle attributes here... */

  ObDereferenceObject(Object);

  return Status;
#endif

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
	       IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
	       OUT PVOID ObjectInformation,
	       IN ULONG Length,
	       OUT PULONG ReturnLength OPTIONAL)
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
      case ObjectBasicInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

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

      case ObjectAllTypesInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ObjectHandleInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      default:
	Status = STATUS_INVALID_INFO_CLASS;
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
				     KeGetPreviousMode(),
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
