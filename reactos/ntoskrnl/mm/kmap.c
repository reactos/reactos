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
   PVOID TempAddress;

   TempAddress = MiMapPagesToZeroInHyperSpace(Page);
   if (TempAddress == NULL)
   {
      return(STATUS_NO_MEMORY);
   }
   memset(TempAddress, 0, PAGE_SIZE);
   MiUnmapPagesInZeroSpace(TempAddress);
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

