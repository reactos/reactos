/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/sysdll.c
 * PURPOSE:         Loaders for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 * UPDATE HISTORY:
 *   DW   26/01/00  Created
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS LdrpMapImage(HANDLE ProcessHandle,
		      HANDLE SectionHandle,
		      PVOID* ReturnedImageBase)
/*
 * FUNCTION: LdrpMapImage maps a user-mode image into an address space
 * PARAMETERS:
 *   ProcessHandle
 *              Points to the process to map the image into
 * 
 *   SectionHandle
 *              Points to the section to map
 * 
 * RETURNS: Status
 */
{   
  ULONG ViewSize;
  PVOID ImageBase;
  NTSTATUS Status;

  ViewSize = 0;
  ImageBase = 0;
  
  Status = ZwMapViewOfSection(SectionHandle,
			      ProcessHandle,
			      (PVOID*)&ImageBase,
			      0,
			      ViewSize,
			      NULL,
			      &ViewSize,
			      0,
			      MEM_COMMIT,
			      PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("Image map view of section failed (Status %x)", Status);
      return(Status);
    }
  
   *ReturnedImageBase = ImageBase;
   
   return(STATUS_SUCCESS);
}
