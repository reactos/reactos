/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/base/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID IoUpdateShareAccess(PFILE_OBJECT FileObject, PSHARE_ACCESS ShareAccess)
{
   UNIMPLEMENTED;
}

NTSTATUS IoCheckShareAccess(ACCESS_MASK DesiredAccess,
			    ULONG DesiredShareAccess,
			    PFILE_OBJECT FileObject,
			    PSHARE_ACCESS ShareAccess,
			    BOOLEAN Update)
{
   UNIMPLEMENTED;
}

VOID IoRemoveShareAccess(PFILE_OBJECT FileObject,
			 PSHARE_ACCESS ShareAccess)
{
   UNIMPLEMENTED;
}

VOID IoSetShareAccess(ACCESS_MASK DesiredAccess,
		      ULONG DesiredShareAccess,
		      PFILE_OBJECT FileObject,
		      PSHARE_ACCESS ShareAccess)
{
   UNIMPLEMENTED;
}
