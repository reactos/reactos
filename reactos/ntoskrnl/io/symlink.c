/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/iomgr/symlink.c
 * PURPOSE:         Implements symbolic links
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS IoCreateUnprotectedSymbolicLink(PUNICODE_STRING SymbolicLinkName,
					 PUNICODE_STRING DeviceName)
{
   UNIMPLEMENTED;
}

NTSTATUS IoCreateSymbolicLink(PUNICODE_STRING SymbolicLinkName,
			      PUNICODE_STRING DeviceName)
{
   UNIMPLEMENTED;
}

NTSTATUS IoDeleteSymbolicLink(PUNICODE_STRING DeviceName)
{
   UNIMPLEMENTED;
}
