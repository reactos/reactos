/* $Id: symlink.c,v 1.23 2001/12/05 01:40:24 dwelch Exp $
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

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

typedef struct
{
	CSHORT			Type;
	CSHORT			Size;
	UNICODE_STRING		TargetName;
	OBJECT_ATTRIBUTES	Target;
} SYMLNK_OBJECT, *PSYMLNK_OBJECT;

POBJECT_TYPE IoSymbolicLinkType = NULL;

static GENERIC_MAPPING IopSymbolicLinkMapping = {
	STANDARD_RIGHTS_READ|SYMBOLIC_LINK_QUERY,
	STANDARD_RIGHTS_WRITE,
	STANDARD_RIGHTS_EXECUTE|SYMBOLIC_LINK_QUERY,
	SYMBOLIC_LINK_ALL_ACCESS};

#define TAG_SYMLINK_TTARGET     TAG('S', 'Y', 'T', 'T')
#define TAG_SYMLINK_TARGET      TAG('S', 'Y', 'M', 'T')

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
NTSTATUS STDCALL
IopCreateSymbolicLink(PVOID Object,
		      PVOID Parent,
		      PWSTR RemainingPath,
		      POBJECT_ATTRIBUTES ObjectAttributes)
{
   if ((Parent != NULL)
       && (RemainingPath != NULL))
     {
	ObAddEntryDirectory(Parent,
			    Object,
			    RemainingPath + 1);
     }
   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							INTERNAL
 *	IopParseSymbolicLink
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
IopParseSymbolicLink(PVOID Object,
		     PVOID * NextObject,
		     PUNICODE_STRING FullPath,
		     PWSTR * RemainingPath,
		     POBJECT_TYPE ObjectType,
		     ULONG Attributes)
{
   NTSTATUS Status;
   PSYMLNK_OBJECT SymlinkObject = (PSYMLNK_OBJECT) Object;
   PVOID ReturnedObject;
   UNICODE_STRING TargetPath;

   DPRINT("IopParseSymbolicLink (RemainingPath %S)\n", *RemainingPath);
   /*
    * Stop parsing if the entire path has been parsed and
    * the desired object is a symbolic link object.
    */
   if (((*RemainingPath == NULL) || (**RemainingPath == 0)) &&
       (ObjectType == IoSymbolicLinkType))
     {
	DPRINT("Parsing stopped!\n");
	*NextObject = NULL;
	return STATUS_SUCCESS;
     }

   Status = ObReferenceObjectByName(SymlinkObject->Target.ObjectName,
				    0,
				    NULL,
				    STANDARD_RIGHTS_REQUIRED,
				    NULL,
				    UserMode,
				    NULL,
				    &ReturnedObject);
   if (NT_SUCCESS(Status))
     {
	*NextObject = ReturnedObject;
	return STATUS_SUCCESS;
     }

   /* build the expanded path */
   TargetPath.MaximumLength = SymlinkObject->TargetName.Length + sizeof(WCHAR);
   if (RemainingPath && *RemainingPath)
     {
	TargetPath.MaximumLength += (wcslen(*RemainingPath) * sizeof(WCHAR));
     }
   TargetPath.Length = TargetPath.MaximumLength - sizeof(WCHAR);
   TargetPath.Buffer = ExAllocatePoolWithTag(NonPagedPool,
					     TargetPath.MaximumLength,
					     TAG_SYMLINK_TTARGET);
   wcscpy(TargetPath.Buffer, SymlinkObject->TargetName.Buffer);
   if (RemainingPath && *RemainingPath)
     {
	wcscat(TargetPath.Buffer, *RemainingPath);
     }

   /* transfer target path buffer into FullPath */
   RtlFreeUnicodeString(FullPath);
   FullPath->Length = TargetPath.Length;
   FullPath->MaximumLength = TargetPath.MaximumLength;
   FullPath->Buffer = TargetPath.Buffer;

   /* reinitialize RemainingPath for reparsing */
   *RemainingPath = FullPath->Buffer;

   *NextObject = NULL;
   return STATUS_REPARSE;
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
VOID IoInitSymbolicLinkImplementation (VOID)
{
   IoSymbolicLinkType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
   
   IoSymbolicLinkType->Tag = TAG('S', 'Y', 'M', 'T');
   IoSymbolicLinkType->TotalObjects = 0;
   IoSymbolicLinkType->TotalHandles = 0;
   IoSymbolicLinkType->MaxObjects = ULONG_MAX;
   IoSymbolicLinkType->MaxHandles = ULONG_MAX;
   IoSymbolicLinkType->PagedPoolCharge = 0;
   IoSymbolicLinkType->NonpagedPoolCharge = sizeof (SYMLNK_OBJECT);
   IoSymbolicLinkType->Mapping = &IopSymbolicLinkMapping;
   IoSymbolicLinkType->Dump = NULL;
   IoSymbolicLinkType->Open = NULL;
   IoSymbolicLinkType->Close = NULL;
   IoSymbolicLinkType->Delete = NULL;
   IoSymbolicLinkType->Parse = IopParseSymbolicLink;
   IoSymbolicLinkType->Security = NULL;
   IoSymbolicLinkType->QueryName = NULL;
   IoSymbolicLinkType->OkayToClose = NULL;
   IoSymbolicLinkType->Create = IopCreateSymbolicLink;
   IoSymbolicLinkType->DuplicationNotify = NULL;
   
   RtlInitUnicodeString(&IoSymbolicLinkType->TypeName,
			L"SymbolicLink");
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
NTSTATUS STDCALL
NtOpenSymbolicLinkObject(OUT PHANDLE LinkHandle,
			 IN ACCESS_MASK DesiredAccess,
			 IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	NTSTATUS	Status;
	PVOID		Object;

	DPRINT("NtOpenSymbolicLinkObject (Name %wZ)\n",
	       ObjectAttributes->ObjectName);

	Status = ObReferenceObjectByName(
			ObjectAttributes->ObjectName,
			ObjectAttributes->Attributes,
			NULL,
			DesiredAccess,
			IoSymbolicLinkType,
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
NTSTATUS STDCALL
NtQuerySymbolicLinkObject(IN HANDLE LinkHandle,
			  IN OUT PUNICODE_STRING LinkTarget,
			  OUT PULONG ReturnedLength OPTIONAL)
{
   PSYMLNK_OBJECT SymlinkObject;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(LinkHandle,
				      SYMBOLIC_LINK_QUERY,
				      IoSymbolicLinkType,
				      UserMode,
				      (PVOID *)&SymlinkObject,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   RtlCopyUnicodeString(LinkTarget,
			SymlinkObject->Target.ObjectName);
   if (ReturnedLength != NULL)
     {
	*ReturnedLength = SymlinkObject->Target.Length;
     }
   ObDereferenceObject(SymlinkObject);
   
   return(STATUS_SUCCESS);
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
NTSTATUS STDCALL
IoCreateUnprotectedSymbolicLink(PUNICODE_STRING SymbolicLinkName,
				PUNICODE_STRING DeviceName)
{
   return IoCreateSymbolicLink(SymbolicLinkName,
			       DeviceName);
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
NTSTATUS STDCALL
IoCreateSymbolicLink(PUNICODE_STRING SymbolicLinkName,
		     PUNICODE_STRING DeviceName)
{
	OBJECT_ATTRIBUTES	ObjectAttributes;
	PSYMLNK_OBJECT		SymbolicLink;
	NTSTATUS		Status;

	assert_irql(PASSIVE_LEVEL);

	DPRINT(
		"IoCreateSymbolicLink(SymbolicLinkName %S, DeviceName %S)\n",
		SymbolicLinkName->Buffer,
		DeviceName->Buffer
		);

	InitializeObjectAttributes(
		& ObjectAttributes,
		SymbolicLinkName,
		OBJ_PERMANENT,
		NULL,
		NULL
		);
	Status = ObCreateObject(
			NULL,
			SYMBOLIC_LINK_ALL_ACCESS,
			& ObjectAttributes,
			IoSymbolicLinkType,
			(PVOID*)&SymbolicLink
			);
	if (!NT_SUCCESS(Status))
	{
		return(Status);
	}
	SymbolicLink->TargetName.Length = 0;
	SymbolicLink->TargetName.MaximumLength = 
		((wcslen(DeviceName->Buffer) + 1) * sizeof(WCHAR));
	SymbolicLink->TargetName.Buffer =
		ExAllocatePoolWithTag(NonPagedPool,
			       SymbolicLink->TargetName.MaximumLength,
			       TAG_SYMLINK_TARGET);
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
	ObDereferenceObject( SymbolicLink );
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
NTSTATUS STDCALL
IoDeleteSymbolicLink(PUNICODE_STRING SymbolicLinkName)
{
	OBJECT_ATTRIBUTES	ObjectAttributes;
	HANDLE			Handle;
	NTSTATUS		Status;

	assert_irql(PASSIVE_LEVEL);

	DPRINT("IoDeleteSymbolicLink (SymbolicLinkName %S)\n",
	       SymbolicLinkName->Buffer);

	InitializeObjectAttributes (&ObjectAttributes,
	                            SymbolicLinkName,
	                            0,
	                            NULL,
	                            NULL);

	Status = NtOpenSymbolicLinkObject (&Handle,
	                                   SYMBOLIC_LINK_ALL_ACCESS,
	                                   &ObjectAttributes);
	if (!NT_SUCCESS(Status))
		return Status;

	Status = NtMakeTemporaryObject (Handle);
	NtClose (Handle);

	return Status;
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
NTSTATUS STDCALL
NtCreateSymbolicLinkObject(OUT PHANDLE SymbolicLinkHandle,
			   IN ACCESS_MASK DesiredAccess,
			   IN POBJECT_ATTRIBUTES ObjectAttributes,
			   IN PUNICODE_STRING DeviceName)
{
   PSYMLNK_OBJECT SymbolicLink;
   NTSTATUS Status;
   
   assert_irql(PASSIVE_LEVEL);
   
   DPRINT("NtCreateSymbolicLinkObject(SymbolicLinkHandle %p, DesiredAccess %ul, ObjectAttributes %p, DeviceName %S)\n",
	  SymbolicLinkHandle,
	  DesiredAccess,
	  ObjectAttributes,
	  DeviceName->Buffer);

   Status = ObCreateObject(SymbolicLinkHandle,
			   DesiredAccess,
			   ObjectAttributes,
			   IoSymbolicLinkType,
			   (PVOID*)&SymbolicLink);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   SymbolicLink->TargetName.Length = 0;
   SymbolicLink->TargetName.MaximumLength = 
     ((wcslen(DeviceName->Buffer) + 1) * sizeof(WCHAR));
   SymbolicLink->TargetName.Buffer = 
     ExAllocatePoolWithTag(NonPagedPool,
			   SymbolicLink->TargetName.MaximumLength,
			   TAG_SYMLINK_TARGET);
   RtlCopyUnicodeString(&SymbolicLink->TargetName,
			DeviceName);
   
   DPRINT("DeviceName %S\n", SymbolicLink->TargetName.Buffer);
   
   InitializeObjectAttributes(&SymbolicLink->Target,
			      &SymbolicLink->TargetName,
			      0,
			      NULL,
			      NULL);
   
   DPRINT("%s() = STATUS_SUCCESS\n",__FUNCTION__);
   ObDereferenceObject(SymbolicLink);
   return(STATUS_SUCCESS);
}

/* EOF */
