/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/ldt.c
 * PURPOSE:         LDT managment
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/



NTSTATUS STDCALL NtSetLdtEntries(PETHREAD Thread,
				 ULONG FirstEntry,
				 PULONG Entries)
{
   UNIMPLEMENTED;
}
