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

/* FUNCTIONS ****************************************************************/

extern unsigned int _text_start__, _text_end__;

STATIC BOOLEAN 
print_address(PVOID address)
{
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
	     DbgPrint("<%ws: %x>", current->Name, 
		      address - current->Base);
	     return(TRUE);
	  }

	current_entry = current_entry->Flink;
     }
#if 0
   DbgPrint("<%x>", address);
#endif
   return(FALSE);
}

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
   PULONG stack;
   NTSTATUS Status;
   ULONG Esp0;
   static char *TypeStrings[] = 
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
    * Check for stack underflow
    */
   if (PsGetCurrentThread() != NULL &&
       Esp0 < (ULONG)PsGetCurrentThread()->Tcb.StackLimit)
     {
	DbgPrint("Stack underflow (tf->esp %x Limit %x)\n",
		 Esp0, (ULONG)PsGetCurrentThread()->Tcb.StackLimit);
	ExceptionNr = 12;
     }
   
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

#if 0
   if ((Tf->Cs & 0xFFFF) == USER_CS)
     {
       return(KiUserTrapHandler(Tf, ExceptionNr, (PVOID)cr2));
     }
#endif   

   /*
    * Print out the CPU registers
    */
   if (ExceptionNr < 19)
     {
	DbgPrint("%s Exception: %d(%x)\n",TypeStrings[ExceptionNr],
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
   else
     {
	DbgPrint("User ESP %.8x\n", Tf->Esp);
     }
  if ((Tf->Cs & 0xffff) == KERNEL_CS)
    {
       DbgPrint("ESP %x\n", Esp0);
       stack = (PULONG) (Esp0 + 24);
       stack = (PULONG)(((ULONG)stack) & (~0x3));
       
       DbgPrint("stack<%p>: ", stack);
	 
       for (i = 0; i < 18; i = i + 6)
	 {
	    DbgPrint("%.8x %.8x %.8x %.8x\n", 
		     stack[i], stack[i+1], 
		     stack[i+2], stack[i+3], 
		     stack[i+4], stack[i+5]);
	 }
       DbgPrint("Frames:\n");
       for (i = 0; i < 32; i++)
	 {
	    if (stack[i] > ((unsigned int) &_text_start__) &&
	      !(stack[i] >= ((ULONG)&init_stack) &&
		stack[i] <= ((ULONG)&init_stack_top)))
	      {
		 //              DbgPrint("  %.8x", stack[i]);
		 print_address((PVOID)stack[i]);
		 DbgPrint(" ");
	      }
	 }
    }
   else
     {
#if 1
	DbgPrint("SS:ESP %x:%x\n", Tf->Ss, Tf->Esp);
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
     }
   
   DbgPrint("\n");
   if ((Tf->Cs&0xffff) == USER_CS &&
       Tf->Eip < KERNEL_BASE)
     {
	DbgPrint("Killing current task\n");
	KeLowerIrql(PASSIVE_LEVEL);
	if ((Tf->Cs&0xffff) == USER_CS)
	  {
	     ZwTerminateProcess(NtCurrentProcess(),
				STATUS_NONCONTINUABLE_EXCEPTION);
	  }
     }   
   for(;;);
}

VOID KeDumpStackFrames(PVOID _Stack, ULONG NrFrames)
{
   PULONG Stack = (PULONG)_Stack;
   ULONG i;
   
   Stack = (PVOID)(((ULONG)Stack) & (~0x3));
   DbgPrint("Stack: %x\n", Stack);
   if (PsGetCurrentThread() != NULL)
     {
	DbgPrint("kernel stack base %x\n",
		 PsGetCurrentThread()->Tcb.StackLimit);
     }
   
   DbgPrint("Frames:\n");
   for (i=0; i<NrFrames; i++)
     {
//	if (Stack[i] > KERNEL_BASE && Stack[i] < ((ULONG)&etext))
	if (Stack[i] > KERNEL_BASE)
	  {
//	     DbgPrint("%.8x  ",Stack[i]);
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

void KeInitExceptions(void)
/*
 * FUNCTION: Initalize CPU exception handling
 */
{
   int i;
   
   DPRINT("KeInitExceptions()\n",0);
   
   set_interrupt_gate(0, (ULONG)KiTrap0);
   set_interrupt_gate(1, (ULONG)KiTrap1);
   set_interrupt_gate(2, (ULONG)KiTrap2);
   set_interrupt_gate(3, (ULONG)KiTrap3);
   set_interrupt_gate(4, (ULONG)KiTrap4);
   set_interrupt_gate(5, (ULONG)KiTrap5);
   set_interrupt_gate(6, (ULONG)KiTrap6);
   set_interrupt_gate(7, (ULONG)KiTrap7);
   set_interrupt_gate(8, (ULONG)KiTrap8);
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
