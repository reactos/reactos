/* $Id: create.c,v 1.4 2001/06/16 14:08:57 ekohl Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/create.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/port.h>
#include <internal/dbg.h>

#define NDEBUG
#include <internal/debug.h>

static
NTSTATUS STDCALL VerifyCreateParameters (
	IN	PHANDLE			PortHandle,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	ULONG			MaxConnectInfoLength,
	IN	ULONG			MaxDataLength,
	IN	ULONG			Reserved
	)
{
	if (NULL == PortHandle)
	{
		return (STATUS_INVALID_PARAMETER_1);
	}
	if (NULL == ObjectAttributes)
	{
		return (STATUS_INVALID_PARAMETER_2);
	}
	if (	(ObjectAttributes->Attributes    & OBJ_OPENLINK)
		|| (ObjectAttributes->Attributes & OBJ_OPENIF)
		|| (ObjectAttributes->Attributes & OBJ_EXCLUSIVE)
		|| (ObjectAttributes->Attributes & OBJ_PERMANENT)
		|| (ObjectAttributes->Attributes & OBJ_INHERIT)
//		|| (ObjectAttributes->Attributes & OBJ_KERNEL_HANDLE)
		)
	{
		return (STATUS_INVALID_PORT_ATTRIBUTES);
	}
	if (MaxConnectInfoLength > 0x104) /* FIXME: use a macro! */
	{
		return (STATUS_INVALID_PARAMETER_3);
	}
	if (MaxDataLength > 0x148) /* FIXME: use a macro! */
	{
		return (STATUS_INVALID_PARAMETER_4);
	}
	/* FIXME: some checking is done also on Reserved */
	return (STATUS_SUCCESS);
}


NTSTATUS
NiCreatePort (
	PVOID			ObjectBody,
	PVOID			Parent,
	PWSTR			RemainingPath,
	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
	NTSTATUS	Status;
   
	if (RemainingPath == NULL)
	{
		return (STATUS_SUCCESS);
	}

	if (wcschr(RemainingPath+1, '\\') != NULL)
	{
		return (STATUS_UNSUCCESSFUL);
	}
   
	Status = ObReferenceObjectByPointer (
			Parent,
			STANDARD_RIGHTS_REQUIRED,
			ObDirectoryType,
			UserMode
			);
	if (!NT_SUCCESS(Status))
	{
		return (Status);
	}
   
	ObAddEntryDirectory (
		Parent,
		ObjectBody,
		(RemainingPath + 1)
		);
	ObDereferenceObject (Parent);
   
	return (STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	NtCreatePort@20
 * 	
 * DESCRIPTION
 *
 * ARGUMENTS
 *	PortHandle,
 *	ObjectAttributes,
 *	MaxConnectInfoLength,
 *	MaxDataLength,
 *	Reserved
 * 
 * RETURN VALUE
 * 
 */
EXPORTED
NTSTATUS
STDCALL
NtCreatePort (
	PHANDLE			PortHandle,
	POBJECT_ATTRIBUTES	ObjectAttributes,
	ULONG			MaxConnectInfoLength,
	ULONG			MaxDataLength,
	ULONG			Reserved
	)
{
	PEPORT		Port;
	NTSTATUS	Status;
   
	DPRINT("NtCreatePort() Name %x\n", ObjectAttributes->ObjectName->Buffer);

	/* Verify parameters */
	Status = VerifyCreateParameters (
			PortHandle,
			ObjectAttributes,
			MaxConnectInfoLength,
			MaxDataLength,
			Reserved
			);
	if (!NT_SUCCESS(Status))
	{
		return (Status);
	}
	/* Ask Ob to create the object */
	Status = ObCreateObject (
			PortHandle,
			PORT_ALL_ACCESS,
			ObjectAttributes,
			ExPortType,
			(PVOID*)&Port
			);
	if (!NT_SUCCESS(Status))
	{
		return (Status);
	}
   
	Status = NiInitializePort (Port);
	Port->MaxConnectInfoLength = 260; /* FIXME: use a macro! */
	Port->MaxDataLength = 328; /* FIXME: use a macro! */
   
	ObDereferenceObject (Port);
   
	return (Status);
}

/**********************************************************************
 * NAME							EXPORTED
 * 	NtCreateWaitablePort@20
 * 	
 * DESCRIPTION
 *	Waitable ports can be connected to with NtSecureConnectPort.
 *	No port interface can be used with waitable ports but
 *	NtReplyWaitReceivePort and NtReplyWaitReceivePortEx.
 * 	Present only in w2k+.
 *
 * ARGUMENTS
 *	PortHandle,
 *	ObjectAttributes,
 *	MaxConnectInfoLength,
 *	MaxDataLength,
 *	Reserved
 * 
 * RETURN VALUE
 * 
 */
EXPORTED
NTSTATUS
STDCALL
NtCreateWaitablePort (
	OUT	PHANDLE			PortHandle,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	ULONG			MaxConnectInfoLength,
	IN	ULONG			MaxDataLength,
	IN	ULONG			Reserved
	)
{
	NTSTATUS Status;

	/* Verify parameters */
	Status = VerifyCreateParameters (
			PortHandle,
			ObjectAttributes,
			MaxConnectInfoLength,
			MaxDataLength,
			Reserved
			);
	if (STATUS_SUCCESS != Status)
	{
		return (Status);
	}
	/* TODO */
	return (STATUS_NOT_IMPLEMENTED);
}

/* EOF */
