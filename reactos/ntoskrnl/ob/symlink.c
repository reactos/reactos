/* $Id: symlink.c,v 1.1 2003/02/25 16:49:08 ekohl Exp $
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

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>

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
  PSYMLNK_OBJECT SymlinkObject = (PSYMLNK_OBJECT)ObjectBody;

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
  PSYMLNK_OBJECT SymlinkObject = (PSYMLNK_OBJECT) Object;
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
VOID ObInitSymbolicLinkImplementation (VOID)
{
  ObSymbolicLinkType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));

  ObSymbolicLinkType->Tag = TAG('S', 'Y', 'M', 'T');
  ObSymbolicLinkType->TotalObjects = 0;
  ObSymbolicLinkType->TotalHandles = 0;
  ObSymbolicLinkType->MaxObjects = ULONG_MAX;
  ObSymbolicLinkType->MaxHandles = ULONG_MAX;
  ObSymbolicLinkType->PagedPoolCharge = 0;
  ObSymbolicLinkType->NonpagedPoolCharge = sizeof(SYMLNK_OBJECT);
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

  RtlInitUnicodeStringFromLiteral(&ObSymbolicLinkType->TypeName,
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
NtCreateSymbolicLinkObject(OUT PHANDLE SymbolicLinkHandle,
			   IN ACCESS_MASK DesiredAccess,
			   IN POBJECT_ATTRIBUTES ObjectAttributes,
			   IN PUNICODE_STRING DeviceName)
{
  PSYMLNK_OBJECT SymbolicLink;
  NTSTATUS Status;

  assert_irql(PASSIVE_LEVEL);

  DPRINT("NtCreateSymbolicLinkObject(SymbolicLinkHandle %p, DesiredAccess %ul, ObjectAttributes %p, DeviceName %wZ)\n",
	 SymbolicLinkHandle,
	 DesiredAccess,
	 ObjectAttributes,
	 DeviceName);

  Status = ObCreateObject(SymbolicLinkHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  ObSymbolicLinkType,
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
			    KeGetPreviousMode(),
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
			  IN OUT PUNICODE_STRING LinkTarget,
			  OUT PULONG ReturnedLength OPTIONAL)
{
  PSYMLNK_OBJECT SymlinkObject;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle(LinkHandle,
				     SYMBOLIC_LINK_QUERY,
				     ObSymbolicLinkType,
				     KeGetPreviousMode(),
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

/* EOF */
