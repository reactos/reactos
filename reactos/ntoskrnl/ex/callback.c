/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/callback.c
 * PURPOSE:         User callbacks
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID ExCallUserCallBack(PVOID fn)
/*
 * FUNCTION: Transfer control to a user callback
 */
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtCallbackReturn(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtW32Call(VOID)
{
   UNIMPLEMENTED;
}
