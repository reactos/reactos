/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: process.c,v 1.15 2003/07/10 17:44:06 royce Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/process.c
 * PURPOSE:         Microkernel process management
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * PORTABILITY:     No.
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/mm.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL
KeAttachProcess (PEPROCESS Process)
{
   KIRQL oldlvl;
   PETHREAD CurrentThread;
   PULONG AttachedProcessPageDir;
   ULONG PageDir;
   
   DPRINT("KeAttachProcess(Process %x)\n",Process);
   
   CurrentThread = PsGetCurrentThread();

   if (CurrentThread->OldProcess != NULL)
     {
	DbgPrint("Invalid attach (thread is already attached)\n");
	KeBugCheck(0);
     }
   
   KeRaiseIrql(DISPATCH_LEVEL, &oldlvl);

   /* The stack of the current process may be located in a page which is
      not present in the page directory of the process we're attaching to.
      That would lead to a page fault when this function returns. However,
      since the processor can't call the page fault handler 'cause it can't
      push EIP on the stack, this will show up as a stack fault which will
      crash the entire system.
      To prevent this, make sure the page directory of the process we're
      attaching to is up-to-date. */

   AttachedProcessPageDir = ExAllocatePageWithPhysPage(Process->Pcb.DirectoryTableBase);
   MmUpdateStackPageDir(AttachedProcessPageDir, &CurrentThread->Tcb);
   ExUnmapPage(AttachedProcessPageDir);

   CurrentThread->OldProcess = PsGetCurrentProcess();
   CurrentThread->ThreadsProcess = Process;
   PageDir = Process->Pcb.DirectoryTableBase.u.LowPart;
   DPRINT("Switching process context to %x\n",PageDir)
   __asm__("movl %0,%%cr3\n\t"
	   : /* no outputs */
	   : "r" (PageDir));
   
   
   KeLowerIrql(oldlvl);
}

/*
 * @implemented
 */
VOID STDCALL
KeDetachProcess (VOID)
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
   PageDir = CurrentThread->ThreadsProcess->Pcb.DirectoryTableBase.u.LowPart;
   __asm__("movl %0,%%cr3\n\t"
	   : /* no inputs */
	   : "r" (PageDir));
   
   KeLowerIrql(oldlvl);
}

/* EOF */
