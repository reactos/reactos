/* $Id: ntobj.c,v 1.5 1999/08/29 06:59:11 ea Exp $
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

#include <wchar.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <string.h>

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
STDCALL
NtQueryObject (
	IN	HANDLE	ObjectHandle,
	IN	CINT	ObjectInformationClass,
	OUT	PVOID	ObjectInformation,
	IN	ULONG	Length,
	OUT	PULONG	ResultLength
	)
{
	UNIMPLEMENTED
}


VOID
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
