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
 * FILE:                 ntoskrnl/ke/i386/exp.c
 * PURPOSE:              Handling exceptions
 * PROGRAMMER:           David Welch (welch@cwcom.net)
 * REVISION HISTORY:
 *              ??/??/??: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/config.h>
#include <internal/ntoskrnl.h>
#include <internal/ke.h>
#include <internal/i386/segment.h>
#include <internal/i386/mm.h>
#include <internal/module.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/trap.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#define _STR(x) #x
#define STR(x) _STR(x)

extern void interrupt_handler2e(void);
extern void interrupt_handler2d(void);

extern VOID KiTrap0(VOID);
extern VOID KiTrap1(VOID);
extern VOID KiTrap2(VOID);
extern VOID KiTrap3(VOID);
extern VOID KiTrap4(VOID);
extern VOID KiTrap5(VOID);
extern VOID KiTrap6(VOID);
extern VOID KiTrap7(VOID);
extern VOID KiTrap8(VOID);
extern VOID KiTrap9(VOID);
extern VOID KiTrap10(VOID);
extern VOID KiTrap11(VOID);
extern VOID KiTrap12(VOID);
extern VOID KiTrap13(VOID);
extern VOID KiTrap14(VOID);
extern VOID KiTrap15(VOID);
extern VOID KiTrap16(VOID);
extern VOID KiTrapUnknown(VOID);

extern ULONG init_stack;
extern ULONG init_stack_top;

static char KiNullLdt[8] = {0,};

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

extern unsigned int _text_start__, _text_end__;

STATIC BOOLEAN 
print_address(PVOID address)
{
#ifdef KDBG
   ULONG Offset;
   PSYMBOL Symbol, NextSymbol;
   BOOLEAN Printed = FALSE;
   ULONG NextAddress;
#endif /* KDBG */
   PLIST_ENTRY current_entry;
   MODULE_TEXT_SECTION* current;
   extern LIST_ENTRY ModuleTextListHead;
   
   current_entry = ModuleTextListHead.Flink;
   
   while (current_entry != &ModuleTextListHead &&
	  current_entry != NULL)
     {
	current = 
	  CONTAINING_RECORD(current_entry, MODULE_TEXT_SECTION, ListEntry);
	
	if (address >= (PVOID)current->Base &&
	    address < (PVOID)(current->Base + current->Length))
	  {

#ifdef KDBG

      Offset = (ULONG)((ULONG)address - current->Base);
      Symbol = current->Symbols.Symbols;
      while (Symbol != NULL)
        {
          NextSymbol = Symbol->Next;
          if (NextSymbol != NULL)
            NextAddress = NextSymbol->RelativeAddress;
          else
            NextAddress = current->Length;

          if ((Offset >= Symbol->RelativeAddress) &&
              (Offset < NextAddress))
            {
              DbgPrint("<%ws: %x (%wZ)>", current->Name, Offset, &Symbol->Name);
              Printed = TRUE;
              break;
            }
          Symbol = NextSymbol;
        }
      if (!Printed)
        DbgPrint("<%ws: %x>", current->Name, Offset);

#else /* KDBG */

	     DbgPrint("<%ws: %x>", current->Name, 
		      address - current->Base);

#endif /* KDBG */

	     return(TRUE);
	  }

	current_entry = current_entry->Flink;
     }
   return(FALSE);
}

#if 0
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
#endif

ULONG
KiUserTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr, PVOID Cr2)
{
  PULONG Frame;
  ULONG cr3;
  ULONG i;

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
  print_address((PVOID)Tf->Eip);
  DbgPrint("\n");
  __asm__("movl %%cr3,%0\n\t" : "=d" (cr3));
  DbgPrint("CR2 %x CR3 %x ", Cr2, cr3);
  DbgPrint("Proc: %x ",PsGetCurrentProcess());
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

#if 0
  stack=(PULONG)(Tf->Esp);
  
  DbgPrint("Stack:\n");
  for (i=0; i<64; i++)
    {
      if (MmIsPagePresent(NULL,&stack[i]))
	{
	  DbgPrint("%.8x ",stack[i]);
	  if (((i+1)%8) == 0)
	    {
	      DbgPrint("\n");
	    }
	}
    }
#endif
	
#if 0
  if (MmIsPagePresent(NULL, (PVOID)Tf->Eip))
    {
      unsigned char instrs[512];
      
      memcpy(instrs, (PVOID)Tf->Eip, 512);
	   
      DbgPrint("Instrs: ");
      
      for (i=0; i<10; i++)
	{
	  DbgPrint("%x ", instrs[i]);
	}
    }
#endif

  /*
   * Dump the stack frames
   */
  DbgPrint("Frames:   ");
  i = 1;
  Frame = (PULONG)Tf->Ebp;
  while (Frame != NULL)
    {
      DbgPrint("%.8x  ", Frame[1]);
      Frame = (PULONG)Frame[0];
      i++;
    }
  if ((i % 8) != 0)
    {
      DbgPrint("\n");
    }

  /*
   * Kill the faulting task
   */
  __asm__("sti\n\t");
  ZwTerminateProcess(NtCurrentProcess(),
		     STATUS_NONCONTINUABLE_EXCEPTION);

  /*
   * If terminating the process fails then bugcheck
   */
  KeBugCheck(0);
  return(0);
}

ULONG
KiDoubleFaultHandler(VOID)
{
  unsigned int cr2;
  unsigned int i;
  PULONG stack;
  ULONG StackLimit;
  ULONG Esp0;
  ULONG ExceptionNr = 8;
  KTSS* OldTss;
  
  /* Use the address of the trap frame as approximation to the ring0 esp */
  OldTss = KeGetCurrentKPCR()->TSS;
  Esp0 = OldTss->Esp0;
  
  /* Get CR2 */
  __asm__("movl %%cr2,%0\n\t" : "=d" (cr2));
   
   /*
    * Check for stack underflow
    */
   if (PsGetCurrentThread() != NULL &&
       Esp0 < (ULONG)PsGetCurrentThread()->Tcb.StackLimit)
     {
	DbgPrint("Stack underflow (tf->esp %x Limit %x)\n",
		 Esp0, (ULONG)PsGetCurrentThread()->Tcb.StackLimit);
	ExceptionNr = 12;
     }
   
   /*
    * Print out the CPU registers
    */
   if (ExceptionNr < 19)
     {
       DbgPrint("%s Exception: %d(%x)\n", ExceptionTypeStrings[ExceptionNr],
		ExceptionNr, 0);
     }
   else
     {
       DbgPrint("Exception: %d(%x)\n", ExceptionNr, 0);
     }
   DbgPrint("CS:EIP %x:%x ", OldTss->Cs, OldTss->Eip);
   print_address((PVOID)OldTss->Eip);
   DbgPrint("\n");
   DbgPrint("cr2 %x cr3 %x ", cr2, OldTss->Cr3);
   DbgPrint("Proc: %x ",PsGetCurrentProcess());
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
   DbgPrint("DS %x ES %x FS %x GS %x\n", OldTss->Ds, OldTss->Es,
	    OldTss->Fs, OldTss->Gs);
   DbgPrint("EAX: %.8x   EBX: %.8x   ECX: %.8x\n", OldTss->Eax, OldTss->Ebx, 
	    OldTss->Ecx);
   DbgPrint("EDX: %.8x   EBP: %.8x   ESI: %.8x\n", OldTss->Edx, OldTss->Ebp, 
	    OldTss->Esi);
   DbgPrint("EDI: %.8x   EFLAGS: %.8x ", OldTss->Edi, OldTss->Eflags);
   if (OldTss->Cs == KERNEL_CS)
     {
	DbgPrint("kESP %.8x ", Esp0);
	if (PsGetCurrentThread() != NULL)
	  {
	     DbgPrint("kernel stack base %x\n",
		      PsGetCurrentThread()->Tcb.StackLimit);
		      	     
	  }
     }
   else
     {
	DbgPrint("User ESP %.8x\n", OldTss->Esp);
     }
  if ((OldTss->Cs & 0xffff) == KERNEL_CS)
    {
       DbgPrint("ESP %x\n", Esp0);
       stack = (PULONG) (Esp0 + 24);
       stack = (PULONG)(((ULONG)stack) & (~0x3));
       if (PsGetCurrentThread() != NULL)
	 {
	   StackLimit = (ULONG)PsGetCurrentThread()->Tcb.StackBase;
	 }
       else
	 {
	   StackLimit = (ULONG)&init_stack_top;
	 }
       
       DbgPrint("stack<%p>: ", stack);
	 
       for (i = 0; i < 18 && (((ULONG)&stack[i+5]) < StackLimit); i = i + 6)
	 {
	    DbgPrint("%.8x %.8x %.8x %.8x\n", 
		     stack[i], stack[i+1], 
		     stack[i+2], stack[i+3], 
		     stack[i+4], stack[i+5]);
	 }
       DbgPrint("Frames:\n");
       for (i = 0; i < 32 && (((ULONG)&stack[i]) < StackLimit); i++)
	 {
	    if (stack[i] > ((unsigned int) &_text_start__) &&
	      !(stack[i] >= ((ULONG)&init_stack) &&
		stack[i] <= ((ULONG)&init_stack_top)))
	      {
		 print_address((PVOID)stack[i]);
		 DbgPrint(" ");
	      }
	 }
    }
   
   DbgPrint("\n");
   for(;;);
}

ULONG
KiTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr)
/*
 * FUNCTION: Called by the lowlevel execption handlers to print an amusing 
 * message and halt the computer
 * ARGUMENTS:
 *        Complete CPU context
 */
{
   unsigned int cr2, cr3;
   unsigned int i;
//   unsigned int j, sym;
   NTSTATUS Status;
   ULONG Esp0;
   ULONG StackLimit;
   PULONG Frame;

   /* Use the address of the trap frame as approximation to the ring0 esp */
   Esp0 = (ULONG)&Tf->Eip;
   
   /* Get CR2 */
   __asm__("movl %%cr2,%0\n\t" : "=d" (cr2));
   
   /*
    * If this was a V86 mode exception then handle it specially
    */
   if (Tf->Eflags & (1 << 17))
     {
       return(KeV86Exception(ExceptionNr, Tf, cr2));
     }

   /*
    * Check for stack underflow, this may be obsolete
    */
   if (PsGetCurrentThread() != NULL &&
       Esp0 < (ULONG)PsGetCurrentThread()->Tcb.StackLimit)
     {
	DbgPrint("Stack underflow (tf->esp %x Limit %x)\n",
		 Esp0, (ULONG)PsGetCurrentThread()->Tcb.StackLimit);
	ExceptionNr = 12;
     }

   /*
    * Maybe handle the page fault and return
    */
   if (ExceptionNr == 14)
     {
	__asm__("sti\n\t");
	Status = MmPageFault(Tf->Cs&0xffff,
			     &Tf->Eip,
			     &Tf->Eax,
			     cr2,
			     Tf->ErrorCode);
	if (NT_SUCCESS(Status))
	  {
	     return(0);
	  }
     }

   /*
    * Handle user exceptions differently
    */
   if ((Tf->Cs & 0xFFFF) == USER_CS)
     {
       return(KiUserTrapHandler(Tf, ExceptionNr, (PVOID)cr2));
     }

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
   DbgPrint("Processor: %d CS:EIP %x:%x ", KeGetCurrentProcessorNumber(),
	    Tf->Cs&0xffff, Tf->Eip);
   print_address((PVOID)Tf->Eip);
   DbgPrint("\n");
   __asm__("movl %%cr3,%0\n\t" : "=d" (cr3));
   DbgPrint("cr2 %x cr3 %x ", cr2, cr3);
   DbgPrint("Proc: %x ",PsGetCurrentProcess());
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
   if ((Tf->Cs&0xffff) == KERNEL_CS)
     {
	DbgPrint("kESP %.8x ", Esp0);
	if (PsGetCurrentThread() != NULL)
	  {
	     DbgPrint("kernel stack base %x\n",
		      PsGetCurrentThread()->Tcb.StackLimit);
		      	     
	  }
     }

   DbgPrint("ESP %x\n", Esp0);

   if (PsGetCurrentThread() != NULL)
     {
       StackLimit = (ULONG)PsGetCurrentThread()->Tcb.StackBase;
     }
   else
     {
       StackLimit = (ULONG)&init_stack_top;
     }
   
   /*
    * Dump the stack frames
    */
   DbgPrint("Frames: ");
   i = 1;
   Frame = (PULONG)Tf->Ebp;
   while (Frame != NULL)
     {
       print_address((PVOID)Frame[1]);
       Frame = (PULONG)Frame[0];
       i++;
     }
   if ((i % 8) != 0)
     {
       DbgPrint("\n");
     }

   for(;;);
}

VOID 
KeDumpStackFrames(PVOID _Stack, ULONG NrFrames)
{
   PULONG Stack = (PULONG)_Stack;
   ULONG i;
   ULONG StackLimit;
   
   Stack = (PVOID)(((ULONG)Stack) & (~0x3));
   DbgPrint("Stack: %x\n", Stack);
   if (PsGetCurrentThread() != NULL)
     {
	DbgPrint("kernel stack base %x\n",
		 PsGetCurrentThread()->Tcb.StackLimit);
     }

   if (PsGetCurrentThread() != NULL)
     {
       StackLimit = (ULONG)PsGetCurrentThread()->Tcb.StackBase;
     }
   else
     {
       StackLimit = (ULONG)&init_stack_top;
     }
   
   DbgPrint("Frames:\n");
   for (i=0; i<NrFrames && ((ULONG)&Stack[i] < StackLimit); i++)
     {
	if (Stack[i] > KERNEL_BASE)
	  {
	     print_address((PVOID)Stack[i]);
	     DbgPrint(" ");
	  }
	if (Stack[i] == 0xceafbeef)
	  {
	     DbgPrint("IRQ ");
	  }
     }
   DbgPrint("\n");
}

static void set_system_call_gate(unsigned int sel, unsigned int func)
{
   DPRINT("sel %x %d\n",sel,sel);
   KiIdt[sel].a = (((int)func)&0xffff) +
     (KERNEL_CS << 16);
   KiIdt[sel].b = 0xef00 + (((int)func)&0xffff0000);
   DPRINT("idt[sel].b %x\n",KiIdt[sel].b);
}

static void set_interrupt_gate(unsigned int sel, unsigned int func)
{
   DPRINT("set_interrupt_gate(sel %d, func %x)\n",sel,func);
   KiIdt[sel].a = (((int)func)&0xffff) +
     (KERNEL_CS << 16);
   KiIdt[sel].b = 0x8f00 + (((int)func)&0xffff0000);         
}

static void
set_task_gate(unsigned int sel, unsigned task_sel)
{
  KiIdt[sel].a = task_sel << 16;
  KiIdt[sel].b = 0x8500;
}

void KeInitExceptions(void)
/*
 * FUNCTION: Initalize CPU exception handling
 */
{
   int i;
   ULONG base, length;
   extern USHORT KiBootGdt[];

   DPRINT("KeInitExceptions()\n",0);

   /*
    * Set up an a descriptor for the LDT
    */
   memset(KiNullLdt, 0, sizeof(KiNullLdt));
   base = (unsigned int)&KiNullLdt;
   length = sizeof(KiNullLdt) - 1;
   
   KiBootGdt[(LDT_SELECTOR / 2) + 0] = (length & 0xFFFF);
   KiBootGdt[(LDT_SELECTOR / 2) + 1] = (base & 0xFFFF);
   KiBootGdt[(LDT_SELECTOR / 2) + 2] = ((base & 0xFF0000) >> 16) | 0x8200;
   KiBootGdt[(LDT_SELECTOR / 2) + 3] = ((length & 0xF0000) >> 16) |
     ((base & 0xFF000000) >> 16);

   /*
    * Set up the other gates
    */
   set_interrupt_gate(0, (ULONG)KiTrap0);
   set_interrupt_gate(1, (ULONG)KiTrap1);
   set_interrupt_gate(2, (ULONG)KiTrap2);
   set_interrupt_gate(3, (ULONG)KiTrap3);
   set_interrupt_gate(4, (ULONG)KiTrap4);
   set_interrupt_gate(5, (ULONG)KiTrap5);
   set_interrupt_gate(6, (ULONG)KiTrap6);
   set_interrupt_gate(7, (ULONG)KiTrap7);
   set_task_gate(8, TRAP_TSS_SELECTOR);
   set_interrupt_gate(9, (ULONG)KiTrap9);
   set_interrupt_gate(10, (ULONG)KiTrap10);
   set_interrupt_gate(11, (ULONG)KiTrap11);
   set_interrupt_gate(12, (ULONG)KiTrap12);
   set_interrupt_gate(13, (ULONG)KiTrap13);
   set_interrupt_gate(14, (ULONG)KiTrap14);
   set_interrupt_gate(15, (ULONG)KiTrap15);
   set_interrupt_gate(16, (ULONG)KiTrap16);
   
   for (i=17;i<256;i++)
        {
	   set_interrupt_gate(i,(int)KiTrapUnknown);
        }
   
   set_system_call_gate(0x2d,(int)interrupt_handler2d);
   set_system_call_gate(0x2e,(int)interrupt_handler2e);
}
