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

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID IoAssignArcName(PUNICODE_STRING ArcName,
		     PUNICODE_STRING DeviceName)
{
   IoCreateSymbolicLink(ArcName,DeviceName);
}

VOID IoDeassignArcName(PUNICODE_STRING ArcName)
{
   IoDeleteSymbolicLink(ArcName);
}
