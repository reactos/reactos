/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ob/symlink.c
 * PURPOSE:         Implements symbolic links
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


/* GLOBALS ******************************************************************/

POBJECT_TYPE ObSymbolicLinkType = NULL;

static GENERIC_MAPPING ObpSymbolicLinkMapping = {
	STANDARD_RIGHTS_READ|SYMBOLIC_LINK_QUERY,
	STANDARD_RIGHTS_WRITE,
	STANDARD_RIGHTS_EXECUTE|SYMBOLIC_LINK_QUERY,
	SYMBOLIC_LINK_ALL_ACCESS};

#define TAG_SYMLINK_TTARGET     TAG('S', 'Y', 'T', 'T')
#define TAG_SYMLINK_TARGET      TAG('S', 'Y', 'M', 'T')


/* FUNCTIONS ****************************************************************/

/**********************************************************************
 * NAME							INTERNAL
 *	ObpCreateSymbolicLink
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
ObpCreateSymbolicLink(PVOID Object,
		      PVOID Parent,
		      PWSTR RemainingPath,
		      POBJECT_ATTRIBUTES ObjectAttributes)
{
  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							INTERNAL
 *	ObpDeleteSymbolicLink
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
VOID STDCALL
ObpDeleteSymbolicLink(PVOID ObjectBody)
{
  PSYMLINK_OBJECT SymlinkObject = (PSYMLINK_OBJECT)ObjectBody;

  RtlFreeUnicodeString(&SymlinkObject->TargetName);
}


/**********************************************************************
 * NAME							INTERNAL
 *	ObpParseSymbolicLink
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
ObpParseSymbolicLink(PVOID Object,
		     PVOID * NextObject,
		     PUNICODE_STRING FullPath,
		     PWSTR * RemainingPath,
		     ULONG Attributes)
{
  PSYMLINK_OBJECT SymlinkObject = (PSYMLINK_OBJECT) Object;
  UNICODE_STRING TargetPath;

  DPRINT("ObpParseSymbolicLink (RemainingPath %S)\n", *RemainingPath);

  /*
   * Stop parsing if the entire path has been parsed and
   * the desired object is a symbolic link object.
   */
  if (((*RemainingPath == NULL) || (**RemainingPath == 0)) &&
      (Attributes & OBJ_OPENLINK))
    {
      DPRINT("Parsing stopped!\n");
      *NextObject = NULL;
      return(STATUS_SUCCESS);
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
 *	ObInitSymbolicLinkImplementation
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
VOID INIT_FUNCTION
ObInitSymbolicLinkImplementation (VOID)
{
  ObSymbolicLinkType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));

  ObSymbolicLinkType->Tag = TAG('S', 'Y', 'M', 'T');
  ObSymbolicLinkType->TotalObjects = 0;
  ObSymbolicLinkType->TotalHandles = 0;
  ObSymbolicLinkType->PeakObjects = 0;
  ObSymbolicLinkType->PeakHandles = 0;
  ObSymbolicLinkType->PagedPoolCharge = 0;
  ObSymbolicLinkType->NonpagedPoolCharge = sizeof(SYMLINK_OBJECT);
  ObSymbolicLinkType->Mapping = &ObpSymbolicLinkMapping;
  ObSymbolicLinkType->Dump = NULL;
  ObSymbolicLinkType->Open = NULL;
  ObSymbolicLinkType->Close = NULL;
  ObSymbolicLinkType->Delete = ObpDeleteSymbolicLink;
  ObSymbolicLinkType->Parse = ObpParseSymbolicLink;
  ObSymbolicLinkType->Security = NULL;
  ObSymbolicLinkType->QueryName = NULL;
  ObSymbolicLinkType->OkayToClose = NULL;
  ObSymbolicLinkType->Create = ObpCreateSymbolicLink;
  ObSymbolicLinkType->DuplicationNotify = NULL;

  RtlRosInitUnicodeStringFromLiteral(&ObSymbolicLinkType->TypeName,
				  L"SymbolicLink");

  ObpCreateTypeObject(ObSymbolicLinkType);
}


/**********************************************************************
 * NAME						EXPORTED
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
NtCreateSymbolicLinkObject(OUT PHANDLE LinkHandle,
			   IN ACCESS_MASK DesiredAccess,
			   IN POBJECT_ATTRIBUTES ObjectAttributes,
			   IN PUNICODE_STRING LinkTarget)
{
  PSYMLINK_OBJECT SymbolicLink;
  NTSTATUS Status;

  ASSERT_IRQL(PASSIVE_LEVEL);

  DPRINT("NtCreateSymbolicLinkObject(LinkHandle %p, DesiredAccess %ul, ObjectAttributes %p, LinkTarget %wZ)\n",
	 LinkHandle,
	 DesiredAccess,
	 ObjectAttributes,
	 LinkTarget);

  Status = ObCreateObject(ExGetPreviousMode(),
			  ObSymbolicLinkType,
			  ObjectAttributes,
			  ExGetPreviousMode(),
			  NULL,
			  sizeof(SYMLINK_OBJECT),
			  0,
			  0,
			  (PVOID*)&SymbolicLink);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Status = ObInsertObject ((PVOID)SymbolicLink,
			   NULL,
			   DesiredAccess,
			   0,
			   NULL,
			   LinkHandle);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (SymbolicLink);
      return Status;
    }

  SymbolicLink->TargetName.Length = 0;
  SymbolicLink->TargetName.MaximumLength = 
    ((wcslen(LinkTarget->Buffer) + 1) * sizeof(WCHAR));
  SymbolicLink->TargetName.Buffer = 
    ExAllocatePoolWithTag(NonPagedPool,
			  SymbolicLink->TargetName.MaximumLength,
			  TAG_SYMLINK_TARGET);
  RtlCopyUnicodeString(&SymbolicLink->TargetName,
		       LinkTarget);

  DPRINT("DeviceName %S\n", SymbolicLink->TargetName.Buffer);

  NtQuerySystemTime (&SymbolicLink->CreateTime);

  DPRINT("%s() = STATUS_SUCCESS\n",__FUNCTION__);
  ObDereferenceObject(SymbolicLink);

  return(STATUS_SUCCESS);
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
  DPRINT("NtOpenSymbolicLinkObject (Name %wZ)\n",
	 ObjectAttributes->ObjectName);

  return(ObOpenObjectByName(ObjectAttributes,
			    ObSymbolicLinkType,
			    NULL,
			    (KPROCESSOR_MODE)KeGetPreviousMode(),
			    DesiredAccess,
			    NULL,
			    LinkHandle));
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
			  OUT PUNICODE_STRING LinkTarget,
			  OUT PULONG ResultLength  OPTIONAL)
{
  PSYMLINK_OBJECT SymlinkObject;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle(LinkHandle,
				     SYMBOLIC_LINK_QUERY,
				     ObSymbolicLinkType,
				     (KPROCESSOR_MODE)KeGetPreviousMode(),
				     (PVOID *)&SymlinkObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  if (ResultLength != NULL)
    {
      *ResultLength = (ULONG)SymlinkObject->TargetName.Length + sizeof(WCHAR);
    }

  if (LinkTarget->MaximumLength >= SymlinkObject->TargetName.Length + sizeof(WCHAR))
    {
      RtlCopyUnicodeString(LinkTarget,
			   &SymlinkObject->TargetName);
      Status = STATUS_SUCCESS;
    }
  else
    {
      Status = STATUS_BUFFER_TOO_SMALL;
    }

  ObDereferenceObject(SymlinkObject);

  return Status;
}

/* EOF */
