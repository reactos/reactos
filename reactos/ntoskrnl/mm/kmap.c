/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/kmap.c
 * PURPOSE:         Implements the kernel memory pool
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/

/* FUNCTIONS ***************************************************************/

NTSTATUS
NTAPI
MiZeroPage(PFN_TYPE Page)
{
   PEPROCESS Process;
   KIRQL Irql;
   PVOID TempAddress;

   Process = (PEPROCESS)KeGetCurrentThread()->ApcState.Process;
   TempAddress = MiMapPageInHyperSpace(Process, Page, &Irql);
   if (TempAddress == NULL)
   {
      return(STATUS_NO_MEMORY);
   }
   memset(TempAddress, 0, PAGE_SIZE);
   MiUnmapPageInHyperSpace(Process, TempAddress, Irql);
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MiCopyFromUserPage(PFN_TYPE DestPage, PVOID SourceAddress)
{
   PVOID TempAddress;

   TempAddress = MmCreateHyperspaceMapping(DestPage);
   if (TempAddress == NULL)
   {
      return(STATUS_NO_MEMORY);
   }
   memcpy(TempAddress, SourceAddress, PAGE_SIZE);
   MmDeleteHyperspaceMapping(TempAddress);
   return(STATUS_SUCCESS);
}

