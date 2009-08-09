/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/wset.c
 * PURPOSE:         Manages working sets
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
MmTrimUserMemory(ULONG Target, ULONG Priority, PULONG NrFreedPages)
{
   PFN_TYPE CurrentPage;
   PFN_TYPE NextPage;
   NTSTATUS Status;

   (*NrFreedPages) = 0;

   CurrentPage = MmGetLRUFirstUserPage();
   while (CurrentPage != 0 && Target > 0)
   {
      NextPage = MmGetLRUNextUserPage(CurrentPage);

      Status = MmPageOutPhysicalAddress(CurrentPage);
      if (NT_SUCCESS(Status))
      {
         DPRINT("Succeeded\n");
         Target--;
         (*NrFreedPages)++;
      }
      else if (Status == STATUS_PAGEFILE_QUOTA)
      {
         MmRemoveLRUUserPage(CurrentPage);
         MmInsertLRULastUserPage(CurrentPage);
      }

      CurrentPage = NextPage;
   }
   return(STATUS_SUCCESS);
}

/*
 * @unimplemented
 */
ULONG
NTAPI
MmTrimAllSystemPagableMemory (
	IN ULONG PurgeTransitionList
	)
{
	UNIMPLEMENTED;
	return 0;
}
