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

OBJECT_TYPE SymlinkObjectType = {{NULL,0,0},
                                0,
                                0,
                                ULONG_MAX,
                                ULONG_MAX,
                                sizeof(SYMLNK_OBJECT),
                                0,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               };                           

/* FUNCTIONS *****************************************************************/

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
   *LinkHandle = ObAddHandle(Object);
   return(STATUS_SUCCESS);
}

NTSTATUS ZwQuerySymbolicLinkObject(IN HANDLE LinkHandle,
				   IN OUT PUNICODE_STRING LinkTarget,
				   OUT PULONG ReturnedLength OPTIONAL)
{
   COMMON_BODY_HEADER* hdr = ObGetObjectByHandle(LinkHandle);
   PSYMLNK_OBJECT SymlinkObject = (PSYMLNK_OBJECT)hdr;

   if (hdr==NULL)
     {
	return(STATUS_INVALID_HANDLE);
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

VOID IoInitSymbolicLinkImplementation(VOID)
{
   ANSI_STRING astring;
   
   RtlInitAnsiString(&astring,"Symbolic Link");
   RtlAnsiStringToUnicodeString(&SymlinkObjectType.TypeName,&astring,TRUE);
   ObRegisterType(OBJTYP_SYMLNK,&SymlinkObjectType);   
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
   SymbolicLink = ObGenericCreateObject(&SymbolicLinkHandle,0,
					&ObjectAttributes,OBJTYP_SYMLNK);
   if (SymbolicLink == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   SymbolicLink->TargetName.Buffer = ExAllocatePool(NonPagedPool,
					 ((wstrlen(DeviceName->Buffer)+1)*2));
   SymbolicLink->TargetName.MaximumLength = wstrlen(DeviceName->Buffer);
   SymbolicLink->TargetName.Length = 0;
   RtlCopyUnicodeString(&(SymbolicLink->TargetName),DeviceName);
   DPRINT("DeviceName %w\n",SymbolicLink->TargetName.Buffer);
   InitializeObjectAttributes(&(SymbolicLink->Target),
			      &(SymbolicLink->TargetName),0,NULL,NULL);
   DPRINT("%s() = STATUS_SUCCESS\n",__FUNCTION__);
   return(STATUS_SUCCESS);
}

NTSTATUS IoDeleteSymbolicLink(PUNICODE_STRING DeviceName)
{
   UNIMPLEMENTED;
}
