/* $Id: symlink.c,v 1.10 2000/01/12 19:02:40 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/symlink.c
 * PURPOSE:         Implements symbolic links
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <wchar.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

typedef
struct
{
	CSHORT			Type;
	CSHORT			Size;
	UNICODE_STRING		TargetName;
	OBJECT_ATTRIBUTES	Target;
	
} SYMLNK_OBJECT, *PSYMLNK_OBJECT;


POBJECT_TYPE
IoSymbolicLinkType = NULL;

/* FUNCTIONS *****************************************************************/


/**********************************************************************
 * NAME							INTERNAL
 *	IopCreateSymbolicLink
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURNN VALUE
 *	Status.
 *
 * REVISIONS
 */
NTSTATUS
IopCreateSymbolicLink (
	PVOID			Object,
	PVOID			Parent,
	PWSTR			RemainingPath,
	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
	if (	(Parent != NULL)
		&& (RemainingPath != NULL)
		)
	{
		ObAddEntryDirectory(
			Parent,
			Object,
			RemainingPath + 1
			);
	}
	return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							INTERNAL
 *	IopParseSymbolicLink
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURNN VALUE
 *
 * REVISIONS
 */
PVOID
IopParseSymbolicLink (
	PVOID	Object,
	PWSTR	* RemainingPath
	)
{
	NTSTATUS	Status;
	PSYMLNK_OBJECT	SymlinkObject = (PSYMLNK_OBJECT) Object;
	PVOID		ReturnedObject;
   
	Status = ObReferenceObjectByName(
			SymlinkObject->Target.ObjectName,
			0,
			NULL,
			STANDARD_RIGHTS_REQUIRED,
			NULL,
			UserMode,
			NULL,
			& ReturnedObject
			);
	if (NT_SUCCESS(Status))
	{
		return ReturnedObject;
	}
	return NULL;
}


/**********************************************************************
 * NAME							INTERNAL
 *	IoInitSymbolicLinkImplementation
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * 	None.
 *
 * RETURNN VALUE
 * 	None.
 *
 * REVISIONS
 */
VOID
IoInitSymbolicLinkImplementation (VOID)
{
	ANSI_STRING AnsiString;
   
	IoSymbolicLinkType = ExAllocatePool(
				NonPagedPool,
				sizeof (OBJECT_TYPE)
				);
   
	IoSymbolicLinkType->TotalObjects = 0;
	IoSymbolicLinkType->TotalHandles = 0;
	IoSymbolicLinkType->MaxObjects = ULONG_MAX;
	IoSymbolicLinkType->MaxHandles = ULONG_MAX;
	IoSymbolicLinkType->PagedPoolCharge = 0;
	IoSymbolicLinkType->NonpagedPoolCharge = sizeof (SYMLNK_OBJECT);
	IoSymbolicLinkType->Dump = NULL;
	IoSymbolicLinkType->Open = NULL;
	IoSymbolicLinkType->Close = NULL;
	IoSymbolicLinkType->Delete = NULL;
	IoSymbolicLinkType->Parse = IopParseSymbolicLink;
	IoSymbolicLinkType->Security = NULL;
	IoSymbolicLinkType->QueryName = NULL;
	IoSymbolicLinkType->OkayToClose = NULL;
	IoSymbolicLinkType->Create = IopCreateSymbolicLink;
   
	RtlInitAnsiString(
		& AnsiString,
		"Symbolic Link"
		);
	RtlAnsiStringToUnicodeString(
		& IoSymbolicLinkType->TypeName,
		& AnsiString,
		TRUE
		);
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtOpenSymbolicLinkObject
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
NtOpenSymbolicLinkObject (
	OUT	PHANDLE			LinkHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
	NTSTATUS	Status;
	PVOID		Object;

	Status = ObReferenceObjectByName(
			ObjectAttributes->ObjectName,
			ObjectAttributes->Attributes,
			NULL,
			DesiredAccess,
			NULL,
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
			LinkHandle
			);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
   
	return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtQuerySymbolicLinkObject
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
NtQuerySymbolicLinkObject (
	IN	HANDLE		LinkHandle,
	IN OUT	PUNICODE_STRING	LinkTarget,
	OUT	PULONG		ReturnedLength	OPTIONAL
	)
{
	PSYMLNK_OBJECT	SymlinkObject;
	NTSTATUS	Status;
   
	Status = ObReferenceObjectByHandle(
			LinkHandle,
			SYMBOLIC_LINK_QUERY,
			IoSymbolicLinkType,
			UserMode,
			(PVOID *) & SymlinkObject,
			NULL
			);
	if (Status != STATUS_SUCCESS)
	{
		return Status;
	}
   
	RtlCopyUnicodeString(
		LinkTarget,
		SymlinkObject->Target.ObjectName
		);
	if (ReturnedLength != NULL)
	{
		*ReturnedLength = SymlinkObject->Target.Length;
	}
	ObDereferenceObject(SymlinkObject);
	
	return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							EXPORTED
 *	IoCreateUnprotectedSymbolicLink
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
IoCreateUnprotectedSymbolicLink (
	PUNICODE_STRING	SymbolicLinkName,
	PUNICODE_STRING	DeviceName
	)
{
	return IoCreateSymbolicLink(
			SymbolicLinkName,
			DeviceName
			);
}


/**********************************************************************
 * NAME							EXPORTED
 *	IoCreateSymbolicLink
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
IoCreateSymbolicLink (
	PUNICODE_STRING	SymbolicLinkName,
	PUNICODE_STRING	DeviceName
	)
{
	OBJECT_ATTRIBUTES	ObjectAttributes;
	HANDLE			SymbolicLinkHandle;
	PSYMLNK_OBJECT		SymbolicLink;
   
	assert_irql(PASSIVE_LEVEL);
   
	DPRINT(
		"IoCreateSymbolicLink(SymbolicLinkName %S, DeviceName %S)\n",
		SymbolicLinkName->Buffer,
		DeviceName->Buffer
		);
   
	InitializeObjectAttributes(
		& ObjectAttributes,
		SymbolicLinkName,
		0,
		NULL,
		NULL
		);
	SymbolicLink = ObCreateObject(
			& SymbolicLinkHandle,
			SYMBOLIC_LINK_ALL_ACCESS,
			& ObjectAttributes,
			IoSymbolicLinkType
			);
	if (SymbolicLink == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
   
	ZwClose(SymbolicLinkHandle);
	
	SymbolicLink->TargetName.Length = 0;
	SymbolicLink->TargetName.MaximumLength = 
		((wcslen(DeviceName->Buffer) + 1) * sizeof(WCHAR));
	SymbolicLink->TargetName.Buffer =
		ExAllocatePool(
			NonPagedPool,
                        SymbolicLink->TargetName.MaximumLength
			);
	RtlCopyUnicodeString(
		& (SymbolicLink->TargetName),
		DeviceName
		);
	
	DPRINT("DeviceName %S\n", SymbolicLink->TargetName.Buffer);
	
	InitializeObjectAttributes(
		& (SymbolicLink->Target),
		& (SymbolicLink->TargetName),
		0,
		NULL,
		NULL
		);
	
	DPRINT("%s() = STATUS_SUCCESS\n",__FUNCTION__);

	return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							EXPORTED
 *	IoDeleteSymbolicLink
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
IoDeleteSymbolicLink (
	PUNICODE_STRING	DeviceName
	)
{
	UNIMPLEMENTED;
}


/**********************************************************************
 * NAME						(EXPORTED as Zw)
 *	NtCreateSymbolicLinkObject
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS
STDCALL
NtCreateSymbolicLinkObject (
	OUT	PHANDLE			SymbolicLinkHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	PUNICODE_STRING		Name
	)
{
	UNIMPLEMENTED;
}


/* EOF */
