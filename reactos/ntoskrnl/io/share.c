/* $Id: share.c,v 1.2 2000/03/26 19:38:26 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/share.c
 * PURPOSE:         
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
IoUpdateShareAccess(PFILE_OBJECT FileObject, PSHARE_ACCESS ShareAccess)
{
	UNIMPLEMENTED;
}


VOID
STDCALL
IoCheckDesiredAccess (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
IoCheckEaBufferValidity (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


NTSTATUS
STDCALL
IoCheckFunctionAccess (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


NTSTATUS
STDCALL
IoCheckShareAccess(ACCESS_MASK DesiredAccess,
			    ULONG DesiredShareAccess,
			    PFILE_OBJECT FileObject,
			    PSHARE_ACCESS ShareAccess,
			    BOOLEAN Update)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


VOID
STDCALL
IoRemoveShareAccess(PFILE_OBJECT FileObject,
			 PSHARE_ACCESS ShareAccess)
{
   UNIMPLEMENTED;
}


VOID
STDCALL
IoSetShareAccess(ACCESS_MASK DesiredAccess,
		      ULONG DesiredShareAccess,
		      PFILE_OBJECT FileObject,
		      PSHARE_ACCESS ShareAccess)
{
   UNIMPLEMENTED;
}


NTSTATUS
STDCALL
IoSetInformation (
	IN	PFILE_OBJECT		FileObject,
	IN	FILE_INFORMATION_CLASS	FileInformationClass,
	IN	ULONG			Length,
	OUT	PVOID			FileInformation
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


VOID
STDCALL
IoFastQueryNetworkAttributes (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	UNIMPLEMENTED;
}


/* EOF */
