/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/port.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtAcceptConnectPort(VOID)
{
}

NTSTATUS STDCALL NtCompleteConnectPort(VOID)
{
}

NTSTATUS STDCALL NtConnectPort(VOID)
{
}

NTSTATUS STDCALL NtCreatePort(VOID)
{
}

NTSTATUS STDCALL NtListenPort(VOID)
{
}

NTSTATUS STDCALL NtReplyPort(VOID)
{
}

NTSTATUS STDCALL NtReplyWaitReceivePort(VOID)
{
}

NTSTATUS STDCALL NtReplyWaitReplyPort(VOID)
{
}

NTSTATUS STDCALL NtRequestPort(VOID)
{
}

NTSTATUS STDCALL NtRequestWaitReplyPort(VOID)
{
}

NTSTATUS STDCALL NtQueryInformationPort(VOID)
{
}
