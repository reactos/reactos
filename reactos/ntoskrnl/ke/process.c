/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/process.c
 * PURPOSE:         Microkernel process management
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/mm.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID KeAttachProcess(PEPROCESS Process)
{
   KIRQL oldlvl;
   PETHREAD CurrentThread;
   ULONG PageDir;
   
   DPRINT("KeAttachProcess(Process %x)\n",Process);
   
   CurrentThread = PsGetCurrentThread();

   if (CurrentThread->OldProcess != NULL)
     {
	DbgPrint("Invalid attach (thread is already attached)\n");
	KeBugCheck(0);
     }
   
   KeRaiseIrql(DISPATCH_LEVEL, &oldlvl);
   
   CurrentThread->OldProcess = PsGetCurrentProcess();
   CurrentThread->ThreadsProcess = Process;
   PageDir = (ULONG)CurrentThread->ThreadsProcess->Pcb.PageTableDirectory;
   CurrentThread->Tcb.Context.cr3 = PageDir;
   __asm__("movl %0,%%cr3\n\t"
	   : /* no inputs */
	   : "r" (PageDir));
   
   
   KeLowerIrql(oldlvl);
}

VOID KeDetachProcess(VOID)
{
   KIRQL oldlvl;
   PETHREAD CurrentThread;
   ULONG PageDir;
   
   DPRINT("KeDetachProcess()\n");
   
   CurrentThread = PsGetCurrentThread();

   if (CurrentThread->OldProcess == NULL)
     {
	DbgPrint("Invalid detach (thread was not attached)\n");
	KeBugCheck(0);
     }
   
   KeRaiseIrql(DISPATCH_LEVEL, &oldlvl);
   
   CurrentThread->ThreadsProcess = CurrentThread->OldProcess;
   CurrentThread->OldProcess = NULL;
   PageDir = (ULONG)CurrentThread->ThreadsProcess->Pcb.PageTableDirectory;
   CurrentThread->Tcb.Context.cr3 = PageDir;
   __asm__("movl %0,%%cr3\n\t"
	   : /* no inputs */
	   : "r" (PageDir));
   
   KeLowerIrql(oldlvl);
}
