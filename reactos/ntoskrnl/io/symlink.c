/*
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

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

typedef struct
{
   CSHORT Type;
   CSHORT Size;
   UNICODE_STRING TargetName;
   OBJECT_ATTRIBUTES Target;
} SYMLNK_OBJECT, *PSYMLNK_OBJECT;

POBJECT_TYPE IoSymbolicLinkType = NULL;

/* FUNCTIONS *****************************************************************/

VOID IoInitSymbolicLinkImplementation(VOID)
{
   ANSI_STRING AnsiString;
   
   IoSymbolicLinkType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   IoSymbolicLinkType->TotalObjects = 0;
   IoSymbolicLinkType->TotalHandles = 0;
   IoSymbolicLinkType->MaxObjects = ULONG_MAX;
   IoSymbolicLinkType->MaxHandles = ULONG_MAX;
   IoSymbolicLinkType->PagedPoolCharge = 0;
   IoSymbolicLinkType->NonpagedPoolCharge = sizeof(SYMLNK_OBJECT);
   IoSymbolicLinkType->Dump = NULL;
   IoSymbolicLinkType->Open = NULL;
   IoSymbolicLinkType->Close = NULL;
   IoSymbolicLinkType->Delete = NULL;
   IoSymbolicLinkType->Parse = NULL;
   IoSymbolicLinkType->Security = NULL;
   IoSymbolicLinkType->QueryName = NULL;
   IoSymbolicLinkType->OkayToClose = NULL;
   
   RtlInitAnsiString(&AnsiString,"Symbolic Link");
   RtlAnsiStringToUnicodeString(&IoSymbolicLinkType->TypeName,
				&AnsiString,TRUE);
}


NTSTATUS NtOpenSymbolicLinkObject(OUT PHANDLE LinkHandle,
				  IN ACCESS_MASK DesiredAccess,
				  IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwOpenSymbolicLinkObject(LinkHandle,
				   DesiredAccess,
				   ObjectAttributes));
}

NTSTATUS ZwOpenSymbolicLinkObject(OUT PHANDLE LinkHandle,
				  IN ACCESS_MASK DesiredAccess,
				  IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   PVOID Object;
   PWSTR Ignored;

   Status =  ObOpenObjectByName(ObjectAttributes,&Object,&Ignored);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   *LinkHandle = ObInsertHandle(KeGetCurrentProcess(),Object,
				DesiredAccess,FALSE);
   return(STATUS_SUCCESS);
}

NTSTATUS NtQuerySymbolicLinkObject(IN HANDLE LinkHandle,
				   IN OUT PUNICODE_STRING LinkTarget,
				   OUT PULONG ReturnedLength OPTIONAL)
{
   return(ZwQuerySymbolicLinkObject(LinkHandle,LinkTarget,ReturnedLength));
}

NTSTATUS ZwQuerySymbolicLinkObject(IN HANDLE LinkHandle,
				   IN OUT PUNICODE_STRING LinkTarget,
				   OUT PULONG ReturnedLength OPTIONAL)
{
   PSYMLNK_OBJECT SymlinkObject;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(LinkHandle,
				      SYMBOLIC_LINK_QUERY,
				      IoSymbolicLinkType,
				      UserMode,
				      (PVOID*)&SymlinkObject,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   RtlCopyUnicodeString(LinkTarget,SymlinkObject->Target.ObjectName);
   if (ReturnedLength!=NULL)
     {
	*ReturnedLength=SymlinkObject->Target.Length;
     }
   return(STATUS_SUCCESS);
}


POBJECT IoOpenSymlink(POBJECT _Symlink)
{
   PVOID Result;
   PSYMLNK_OBJECT Symlink = (PSYMLNK_OBJECT)_Symlink;
   PWSTR Ignored;
   
   DPRINT("IoOpenSymlink(_Symlink %x)\n",Symlink);
   
   DPRINT("Target %w\n",Symlink->Target.ObjectName->Buffer);
   
   ObOpenObjectByName(&(Symlink->Target),&Result,&Ignored);
   return(Result);
}

NTSTATUS IoCreateUnprotectedSymbolicLink(PUNICODE_STRING SymbolicLinkName,
					 PUNICODE_STRING DeviceName)
{
   return(IoCreateSymbolicLink(SymbolicLinkName,DeviceName));
}

NTSTATUS IoCreateSymbolicLink(PUNICODE_STRING SymbolicLinkName,
			      PUNICODE_STRING DeviceName)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE SymbolicLinkHandle;
   PSYMLNK_OBJECT SymbolicLink;
   PUNICODE_STRING TargetName;
   
   DPRINT("IoCreateSymbolicLink(SymbolicLinkName %w, DeviceName %w)\n",
	  SymbolicLinkName->Buffer,DeviceName->Buffer);
   
   InitializeObjectAttributes(&ObjectAttributes,SymbolicLinkName,0,NULL,NULL);
   SymbolicLink = ObGenericCreateObject(&SymbolicLinkHandle,
					SYMBOLIC_LINK_ALL_ACCESS,
					&ObjectAttributes,
					IoSymbolicLinkType);
   if (SymbolicLink == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   SymbolicLink->TargetName.Length = 0;
   SymbolicLink->TargetName.MaximumLength = 
     ((wstrlen(DeviceName->Buffer) + 1) * sizeof(WCHAR));
   SymbolicLink->TargetName.Buffer = ExAllocatePool(NonPagedPool,
                                                    SymbolicLink->TargetName.MaximumLength);
   RtlCopyUnicodeString(&(SymbolicLink->TargetName), DeviceName);
   DPRINT("DeviceName %w\n", SymbolicLink->TargetName.Buffer);
   InitializeObjectAttributes(&(SymbolicLink->Target),
			      &(SymbolicLink->TargetName),0,NULL,NULL);
   DPRINT("%s() = STATUS_SUCCESS\n",__FUNCTION__);
   return(STATUS_SUCCESS);
}

NTSTATUS IoDeleteSymbolicLink(PUNICODE_STRING DeviceName)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtCreateSymbolicLinkObject(
	                            OUT PHANDLE SymbolicLinkHandle,
	                            IN ACCESS_MASK DesiredAccess,
	                            IN POBJECT_ATTRIBUTES ObjectAttributes,
	                            IN PUNICODE_STRING Name)
{
   return(NtCreateSymbolicLinkObject(SymbolicLinkHandle,
				     DesiredAccess,
				     ObjectAttributes,
				     Name));
}

NTSTATUS STDCALL ZwCreateSymbolicLinkObject(
				    OUT PHANDLE SymbolicLinkHandle,
				    IN ACCESS_MASK DesiredAccess,
				    IN POBJECT_ATTRIBUTES ObjectAttributes,
				    IN PUNICODE_STRING Name)
{
   UNIMPLEMENTED;
}

