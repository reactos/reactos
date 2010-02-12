/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/mpw.c
 * PURPOSE:         Writes data that has been modified in memory but not on
 *                  the disk
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

HANDLE MpwThreadHandle;
static CLIENT_ID MpwThreadId;
KEVENT MpwThreadEvent;
BOOLEAN MpwThreadShouldTerminate;

/* FUNCTIONS *****************************************************************/

NTSTATUS NTAPI
MmWriteDirtyPages(ULONG Target, PULONG Actual)
{
   PFN_TYPE Page;
   PFN_TYPE NextPage;
   NTSTATUS Status;

   Page = MmGetLRUFirstUserPage();
   while (Page != 0 && Target > 0)
   {
      /*
       * FIXME: While the current page is write back it is possible
       *        that the next page is freed and not longer a user page.
       */
      NextPage = MmGetLRUNextUserPage(Page);
      if (MmIsDirtyPageRmap(Page))
      {
         Status = MmWritePagePhysicalAddress(Page);
         if (NT_SUCCESS(Status))
         {
            Target--;
         }
      }
      Page = NextPage;
   }
   *Actual = Target;
   return(STATUS_SUCCESS);
}

NTSTATUS NTAPI
MmMpwThreadMain(PVOID Ignored)
{
   NTSTATUS Status;
   ULONG PagesWritten;
   LARGE_INTEGER Timeout;

   Timeout.QuadPart = -50000000;

   for(;;)
   {
      Status = KeWaitForSingleObject(&MpwThreadEvent,
                                     0,
                                     KernelMode,
                                     FALSE,
                                     &Timeout);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("MpwThread: Wait failed\n");
         KeBugCheck(MEMORY_MANAGEMENT);
         return(STATUS_UNSUCCESSFUL);
      }
      if (MpwThreadShouldTerminate)
      {
         DbgPrint("MpwThread: Terminating\n");
         return(STATUS_SUCCESS);
      }

      PagesWritten = 0;
#if 0
      /*
       *  FIXME: MmWriteDirtyPages doesn't work correctly.
       */
      MmWriteDirtyPages(128, &PagesWritten);
#endif

      CcRosFlushDirtyPages(128, &PagesWritten);
   }
}

NTSTATUS
NTAPI
MmInitMpwThread(VOID)
{
   KPRIORITY Priority;
   NTSTATUS Status;

   MpwThreadShouldTerminate = FALSE;
   KeInitializeEvent(&MpwThreadEvent, SynchronizationEvent, FALSE);

   Status = PsCreateSystemThread(&MpwThreadHandle,
                                 THREAD_ALL_ACCESS,
                                 NULL,
                                 NULL,
                                 &MpwThreadId,
                                 (PKSTART_ROUTINE) MmMpwThreadMain,
                                 NULL);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   Priority = 1;
   NtSetInformationThread(MpwThreadHandle,
                          ThreadPriority,
                          &Priority,
                          sizeof(Priority));

   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmInitBsmThread(VOID)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ThreadHandle;

    /* Create the thread */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  NULL,
                                  NULL,
                                  KeBalanceSetManager,
                                  NULL);

    /* Close the handle and return status */
    ZwClose(ThreadHandle);
    return Status;
}
