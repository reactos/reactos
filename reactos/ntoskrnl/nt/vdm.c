/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/vdm.c
 * PURPOSE:         Virtual DOS machine support
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* GLOBALS *******************************************************************/

/* static */ UCHAR OrigIVT[1024];
/* static UCHAR OrigBDA[]; */
/* static UCHAR OrigEBDA[]; */

/* FUNCTIONS *****************************************************************/

VOID
NtEarlyInitVdm(VOID)
{
  /*
   * Save various BIOS data tables. At this point the lower 4MB memory
   * map is still active so we can just copy the data from low memory.
   */
  memcpy(OrigIVT, (PVOID)0x0, 1024);
}

NTSTATUS STDCALL NtVdmControl(ULONG ControlCode,
			      PVOID ControlData)
{
  memcpy(ControlData, OrigIVT, 1024);
  return(STATUS_SUCCESS);
}

/* EOF */
