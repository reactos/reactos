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
/*
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/i386/usertrap.c
 * PURPOSE:              Handling usermode exceptions.
 * PROGRAMMER:           David Welch (welch@cwcom.net)
 * REVISION HISTORY:
 *              18/11/01: Split from ntoskrnl/ke/i386/exp.c
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
//#include <ntdll/ldr.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *****************************************************************/

static char *ExceptionTypeStrings[] = 
  {
    "Divide Error",
    "Debug Trap",
    "NMI",
    "Breakpoint",
    "Overflow",
    "BOUND range exceeded",
    "Invalid Opcode",
    "No Math Coprocessor",
    "Double Fault",
    "Unknown(9)",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection",
    "Page Fault",
    "Math Fault",
    "Alignment Check",
    "Machine Check"
  };

/* FUNCTIONS ****************************************************************/

STATIC BOOLEAN 
print_user_address(PVOID address)
{
   PLIST_ENTRY current_entry;
   PLDR_MODULE current;
   PEPROCESS CurrentProcess;
   PPEB Peb = NULL;
   ULONG_PTR RelativeAddress;
   PPEB_LDR_DATA Ldr;
   NTSTATUS Status;

   CurrentProcess = PsGetCurrentProcess();
   if (NULL != CurrentProcess)
     {
       Peb = CurrentProcess->Peb;
     }
   
   if (NULL == Peb)
     {
       DbgPrint("<%x>", address);
       return(TRUE);
     }

   Status = MmSafeCopyFromUser(&Ldr, &Peb->Ldr, sizeof(PPEB_LDR_DATA));
   if (!NT_SUCCESS(Status))
     {
       DbgPrint("<%x>", address);
       return(TRUE);
     }
   current_entry = Ldr->InLoadOrderModuleList.Flink;
   
   while (current_entry != &Ldr->InLoadOrderModuleList &&
	  current_entry != NULL)
     {
	current = 
	  CONTAINING_RECORD(current_entry, LDR_MODULE, InLoadOrderModuleList);
	
	if (address >= (PVOID)current->BaseAddress &&
	    address < (PVOID)(current->BaseAddress + current->SizeOfImage))
	  {
            RelativeAddress = 
	      (ULONG_PTR) address - (ULONG_PTR)current->BaseAddress;
	    DbgPrint("<%wZ: %x>", &current->BaseDllName, RelativeAddress);
	    return(TRUE);
	  }

	current_entry = current_entry->Flink;
     }
   return(FALSE);
}

#if 0
/*
 * Disabled until SEH support is implemented.
 */
ULONG
KiUserTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr, PVOID Cr2)
{
  EXCEPTION_RECORD Er;

  if (ExceptionNr == 0)
    {
      Er.ExceptionCode = STATUS_INTEGER_DIVIDE_BY_ZERO;
    }
  else if (ExceptionNr == 1)
    {
      Er.ExceptionCode = STATUS_SINGLE_STEP;
    }
  else if (ExceptionNr == 3)
    {
      Er.ExceptionCode = STATUS_BREAKPOINT;
    }
  else if (ExceptionNr == 4)
    {
      Er.ExceptionCode = STATUS_INTEGER_OVERFLOW;
    }
  else if (ExceptionNr == 5)
    {
      Er.ExceptionCode = STATUS_ARRAY_BOUNDS_EXCEEDED;
    }
  else if (ExceptionNr == 6)
    {
      Er.ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
    }
  else
    {
      Er.ExceptionCode = STATUS_ACCESS_VIOLATION;
    }
  Er.ExceptionFlags = 0;
  Er.ExceptionRecord = NULL;
  Er.ExceptionAddress = (PVOID)Tf->Eip;
  if (ExceptionNr == 14)
    {
      Er.NumberParameters = 2;
      Er.ExceptionInformation[0] = Tf->ErrorCode & 0x1;
      Er.ExceptionInformation[1] = (ULONG)Cr2;
    }
  else
    {
      Er.NumberParameters = 0;
    }
  

  KiDispatchException(&Er, 0, Tf, UserMode, TRUE);
  return(0);
}
#else
ULONG
KiUserTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr, PVOID Cr2)
{
  PULONG Frame;
  ULONG cr3;
  ULONG i;
  ULONG ReturnAddress;
  ULONG NextFrame;
  NTSTATUS Status;

  /*
   * Get the PDBR
   */
  __asm__("movl %%cr3,%0\n\t" : "=d" (cr3));

   /*
    * Print out the CPU registers
    */
  if (ExceptionNr < 19)
    {
      DbgPrint("%s Exception: %d(%x)\n", ExceptionTypeStrings[ExceptionNr],
	       ExceptionNr, Tf->ErrorCode&0xffff);
    }
  else
    {
      DbgPrint("Exception: %d(%x)\n", ExceptionNr, Tf->ErrorCode&0xffff);
    }
  DbgPrint("CS:EIP %x:%x ", Tf->Cs&0xffff, Tf->Eip);
  print_user_address((PVOID)Tf->Eip);
  DbgPrint("\n");
  __asm__("movl %%cr3,%0\n\t" : "=d" (cr3));
  DbgPrint("CR2 %x CR3 %x ", Cr2, cr3);
  DbgPrint("Process: %x ",PsGetCurrentProcess());
  if (PsGetCurrentProcess() != NULL)
    {
      DbgPrint("Pid: %x <", PsGetCurrentProcess()->UniqueProcessId);
	DbgPrint("%.8s> ", PsGetCurrentProcess()->ImageFileName);
    }
  if (PsGetCurrentThread() != NULL)
    {
      DbgPrint("Thrd: %x Tid: %x",
	       PsGetCurrentThread(),
	       PsGetCurrentThread()->Cid.UniqueThread);
    }
  DbgPrint("\n");
  DbgPrint("DS %x ES %x FS %x GS %x\n", Tf->Ds&0xffff, Tf->Es&0xffff,
	   Tf->Fs&0xffff, Tf->Gs&0xfff);
  DbgPrint("EAX: %.8x   EBX: %.8x   ECX: %.8x\n", Tf->Eax, Tf->Ebx, Tf->Ecx);
  DbgPrint("EDX: %.8x   EBP: %.8x   ESI: %.8x\n", Tf->Edx, Tf->Ebp, Tf->Esi);
  DbgPrint("EDI: %.8x   EFLAGS: %.8x ", Tf->Edi, Tf->Eflags);
  DbgPrint("SS:ESP %x:%x\n", Tf->Ss, Tf->Esp);

  /*
   * Dump the stack frames
   */
  DbgPrint("Frames:   ");
  i = 1;
  Frame = (PULONG)Tf->Ebp;
  while (Frame != NULL && i < 50)
    {
      Status = MmSafeCopyFromUser(&ReturnAddress, &Frame[1], sizeof(ULONG));
      if (!NT_SUCCESS(Status))
	{
	  DbgPrint("????????\n");
	  break;
	}
      print_user_address((PVOID)ReturnAddress);
      Status = MmSafeCopyFromUser(&NextFrame, &Frame[0], sizeof(ULONG));
      if (!NT_SUCCESS(Status))
	{
	  DbgPrint("Frame is inaccessible.\n");
	  break;
	}
      if ((NextFrame + sizeof(ULONG)) >= KERNEL_BASE)
	{
	  DbgPrint("Next frame is in kernel space!\n");
	  break;
	}
      if (NextFrame != 0 && NextFrame <= (ULONG)Frame)
	{
	  DbgPrint("Next frame is not above current frame!\n");
	  break;
	}
      Frame = (PULONG)NextFrame;
      i++;
    }

  /*
   * Kill the faulting process
   */
  __asm__("sti\n\t");
  ZwTerminateProcess(NtCurrentProcess(), STATUS_NONCONTINUABLE_EXCEPTION);

  /*
   * If terminating the process fails then bugcheck
   */
  KeBugCheck(0);
  return(0);
}
#endif
