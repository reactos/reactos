/*
 * COPYRIGHT:  See COPYING in the top directory
 * PROJECT:    ReactOS kernel v0.0.2
 * FILE:       kernel/error.cc
 * PURPOSE:    Error reason setting/getting
 * PROGRAMMER: David Welch
 * UPDATE HISTORY:
 *             16/4/98: Created
 */

/* INCLUDE *****************************************************************/

#include <ddk/ntddk.h>

/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL NtRaiseHardError(VOID)
{
}

NTSTATUS 
STDCALL 
NtSetDefaultHardErrorPort(
	IN HANDLE PortHandle
	)
{
}
