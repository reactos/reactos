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

static UCHAR OrigIVT[1024];
static UCHAR OrigBDA[256];
/* static UCHAR OrigEBDA[]; */

/* FUNCTIONS *****************************************************************/

VOID
NtEarlyInitVdm(VOID)
{
  PVOID start = (PVOID)0x0;
  
  /*
   * Save various BIOS data tables. At this point the lower 4MB memory
   * map is still active so we can just copy the data from low memory.
   */
  memcpy(OrigIVT, start, 1024);
  memcpy(OrigBDA, (PVOID)0x400, 256);
}

/*
 * @implemented
 */
NTSTATUS STDCALL NtVdmControl(ULONG ControlCode,
			      PVOID ControlData)
{
  switch (ControlCode)
    {
    case 0:
      memcpy(ControlData, OrigIVT, 1024);
      break;

    case 1:
      memcpy(ControlData, OrigBDA, 256);
      break;
    }
  return(STATUS_SUCCESS);
}

/* EOF */
