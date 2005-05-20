/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ob/symlink.c
 * PURPOSE:         Implements symbolic links
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
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

/* FUNCTIONS ****************************************************************/

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

  ExFreePool(SymlinkObject->TargetName.Buffer);
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
   ExFreePool(FullPath->Buffer);
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
VOID 
INIT_FUNCTION
ObInitSymbolicLinkImplementation (VOID)
{
  UNICODE_STRING Name;
  OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    
  DPRINT1("Creating SymLink Object Type\n");
  
  /*  Initialize the Directory type  */
  RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
  RtlInitUnicodeString(&Name, L"SymbolicLink");
  ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
  ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(SYMLINK_OBJECT);
  ObjectTypeInitializer.GenericMapping = ObpSymbolicLinkMapping;
  ObjectTypeInitializer.PoolType = NonPagedPool;
  ObjectTypeInitializer.ValidAccessMask = SYMBOLIC_LINK_ALL_ACCESS;
  ObjectTypeInitializer.UseDefaultObject = TRUE;
  ObjectTypeInitializer.ParseProcedure = ObpParseSymbolicLink;
  ObjectTypeInitializer.DeleteProcedure = ObpDeleteSymbolicLink;
  ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ObSymbolicLinkType);
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
  HANDLE hLink;
  PSYMLINK_OBJECT SymbolicLink;
  UNICODE_STRING CapturedLinkTarget;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;

  PAGED_CODE();

  PreviousMode = ExGetPreviousMode();

  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeForWrite(LinkHandle,
                    sizeof(HANDLE),
                    sizeof(ULONG));
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }

  Status = RtlCaptureUnicodeString(&CapturedLinkTarget,
                                   PreviousMode,
                                   PagedPool,
                                   FALSE,
                                   LinkTarget);
  if(!NT_SUCCESS(Status))
  {
    DPRINT1("NtCreateSymbolicLinkObject: Capturing the target link failed!\n");
    return Status;
  }

  DPRINT("NtCreateSymbolicLinkObject(LinkHandle %p, DesiredAccess %ul, ObjectAttributes %p, LinkTarget %wZ)\n",
	 LinkHandle,
	 DesiredAccess,
	 ObjectAttributes,
	 &CapturedLinkTarget);

  Status = ObCreateObject(ExGetPreviousMode(),
			  ObSymbolicLinkType,
			  ObjectAttributes,
			  PreviousMode,
			  NULL,
			  sizeof(SYMLINK_OBJECT),
			  0,
			  0,
			  (PVOID*)&SymbolicLink);
  if (NT_SUCCESS(Status))
  {
    SymbolicLink->TargetName.Length = 0;
    SymbolicLink->TargetName.MaximumLength =
      ((wcslen(LinkTarget->Buffer) + 1) * sizeof(WCHAR));
    SymbolicLink->TargetName.Buffer =
      ExAllocatePoolWithTag(NonPagedPool,
			    SymbolicLink->TargetName.MaximumLength,
			    TAG_SYMLINK_TARGET);
    RtlCopyUnicodeString(&SymbolicLink->TargetName,
		         &CapturedLinkTarget);

    DPRINT("DeviceName %S\n", SymbolicLink->TargetName.Buffer);

    ZwQuerySystemTime (&SymbolicLink->CreateTime);

    Status = ObInsertObject ((PVOID)SymbolicLink,
			     NULL,
			     DesiredAccess,
			     0,
			     NULL,
			     &hLink);
    if (NT_SUCCESS(Status))
    {
      _SEH_TRY
      {
        *LinkHandle = hLink;
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;
    }
    ObDereferenceObject(SymbolicLink);
  }

  RtlReleaseCapturedUnicodeString(&CapturedLinkTarget,
                                  PreviousMode,
                                  FALSE);

  return Status;
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
  HANDLE hLink;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;

  PAGED_CODE();

  PreviousMode = ExGetPreviousMode();

  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeForWrite(LinkHandle,
                    sizeof(HANDLE),
                    sizeof(ULONG));
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }

  DPRINT("NtOpenSymbolicLinkObject (Name %wZ)\n",
	 ObjectAttributes->ObjectName);

  Status = ObOpenObjectByName(ObjectAttributes,
			      ObSymbolicLinkType,
			      NULL,
			      PreviousMode,
			      DesiredAccess,
			      NULL,
			      &hLink);
  if(NT_SUCCESS(Status))
  {
    _SEH_TRY
    {
      *LinkHandle = hLink;
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
  }

  return Status;
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
  UNICODE_STRING SafeLinkTarget;
  PSYMLINK_OBJECT SymlinkObject;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;

  PAGED_CODE();

  PreviousMode = ExGetPreviousMode();

  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      /* probe the unicode string and buffers supplied */
      ProbeForWrite(LinkTarget,
                    sizeof(UNICODE_STRING),
                    sizeof(ULONG));
      SafeLinkTarget = *LinkTarget;
      ProbeForWrite(SafeLinkTarget.Buffer,
                    SafeLinkTarget.MaximumLength,
                    sizeof(WCHAR));

      if(ResultLength != NULL)
      {
        ProbeForWrite(ResultLength,
                      sizeof(ULONG),
                      sizeof(ULONG));
      }
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }
  else
  {
    SafeLinkTarget = *LinkTarget;
  }

  Status = ObReferenceObjectByHandle(LinkHandle,
				     SYMBOLIC_LINK_QUERY,
				     ObSymbolicLinkType,
				     PreviousMode,
				     (PVOID *)&SymlinkObject,
				     NULL);
  if (NT_SUCCESS(Status))
  {
    ULONG LengthRequired = SymlinkObject->TargetName.Length + sizeof(WCHAR);

    _SEH_TRY
    {
      if(SafeLinkTarget.MaximumLength >= LengthRequired)
      {
        /* don't pass TargetLink to RtlCopyUnicodeString here because the caller
           might have modified the structure which could lead to a copy into
           kernel memory! */
        RtlCopyUnicodeString(&SafeLinkTarget,
                             &SymlinkObject->TargetName);
        SafeLinkTarget.Buffer[SafeLinkTarget.Length / sizeof(WCHAR)] = L'\0';
        /* copy back the new UNICODE_STRING structure */
        *LinkTarget = SafeLinkTarget;
      }
      else
      {
        Status = STATUS_BUFFER_TOO_SMALL;
      }

      if(ResultLength != NULL)
      {
        *ResultLength = LengthRequired;
      }
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    ObDereferenceObject(SymlinkObject);
  }

  return Status;
}

/* EOF */
