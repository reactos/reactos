/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/channel.c
 * PURPOSE:         Channels (??)
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/*
 * NOTES:
 * 
 * An article on System Internals (http://www.sysinternals.com) reports
 * that these functions are unimplemented on nt version 3-5.
 * 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtCreateChannel(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtListenChannel(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtOpenChannel(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtReplyWaitSendChannel(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtSendWaitReplyChannel(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtSetContextChannel(VOID)
{
   UNIMPLEMENTED;
}
