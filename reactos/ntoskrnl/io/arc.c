/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/arc.c
 * PURPOSE:         arc names
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

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
