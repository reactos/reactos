/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/hal/x86/exp.c
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
#include <internal/mmhal.h>
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

extern void exception_handler0(void);
extern void exception_handler1(void);
extern void exception_handler2(void);
extern void exception_handler3(void);
extern void exception_handler4(void);
extern void exception_handler5(void);
extern void exception_handler6(void);
extern void exception_handler7(void);
extern void exception_handler8(void);
extern void exception_handler9(void);
extern void exception_handler10(void);
extern void exception_handler11(void);
extern void exception_handler12(void);
extern void exception_handler13(void);
extern void exception_handler14(void);
extern void exception_handler15(void);
extern void exception_handler16(void);
extern void exception_handler_unknown(void);

extern ULONG init_stack;
extern ULONG init_stack_top;

/* FUNCTIONS ****************************************************************/

extern unsigned int _text_start__, _text_end__;

STATIC BOOLEAN 
print_address(PVOID address)
{
   PLIST_ENTRY current_entry;
   PMODULE_OBJECT current;
   extern LIST_ENTRY ModuleListHead;
   
   current_entry = ModuleListHead.Flink;
   
   while (current_entry != &ModuleListHead &&
	  current_entry != NULL)
     {
	current = CONTAINING_RECORD(current_entry, MODULE_OBJECT, ListEntry);
	
	if (address >= current->Base &&
	    address < (current->Base + current->Length))
	  {
	     DbgPrint("<%wZ: %x>", &current->FullName, 
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
exception_handler(struct trap_frame* tf)
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
   
   __asm__("movl %%cr2,%0\n\t"
	   : "=d" (cr2));
   
   if (tf->eflags & (1 << 17))
     {
       return(KeV86Exception(tf, cr2));
     }

   if (PsGetCurrentThread() != NULL &&
       tf->esp < (ULONG)PsGetCurrentThread()->Tcb.StackLimit)
     {
	DbgPrint("Stack underflow (tf->esp %x Limit %x)\n",
		 tf->esp, (ULONG)PsGetCurrentThread()->Tcb.StackLimit);
	tf->type = 12;
     }
   
   if (tf->type == 14)
     {
	__asm__("sti\n\t");
	Status = MmPageFault(tf->cs&0xffff,
			     &tf->eip,
			     &tf->eax,
			     cr2,
			     tf->error_code);
	if (NT_SUCCESS(Status))
	  {
	     return(0);
	  }
     }
   
   /*
    * FIXME: Something better
    */
   if (tf->type==1)
     {
	DbgPrint("Trap at CS:EIP %x:%x\n",tf->cs&0xffff,tf->eip);
	return(0);
     }
   
   /*
    * Print out the CPU registers
    */
   if (tf->type < 19)
     {
	DbgPrint("%s Exception: %d(%x)\n",TypeStrings[tf->type],tf->type,
		 tf->error_code&0xffff);
     }
   else
     {
	DbgPrint("Exception: %d(%x)\n",tf->type,tf->error_code&0xffff);
     }
   DbgPrint("CS:EIP %x:%x ",tf->cs&0xffff,tf->eip);
   print_address((PVOID)tf->eip);
   DbgPrint("\n");
   __asm__("movl %%cr3,%0\n\t"
	   : "=d" (cr3));
   DbgPrint("cr2 %x cr3 %x ",cr2,cr3);
//   for(;;);
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
   DbgPrint("DS %x ES %x FS %x GS %x\n", tf->ds&0xffff, tf->es&0xffff,
	    tf->fs&0xffff, tf->gs&0xfff);
   DbgPrint("EAX: %.8x   EBX: %.8x   ECX: %.8x\n", tf->eax, tf->ebx, tf->ecx);
   DbgPrint("EDX: %.8x   EBP: %.8x   ESI: %.8x\n", tf->edx, tf->ebp, tf->esi);
   DbgPrint("EDI: %.8x   EFLAGS: %.8x ", tf->edi, tf->eflags);
   if ((tf->cs&0xffff) == KERNEL_CS)
     {
	DbgPrint("kESP %.8x ", tf->esp);
	if (PsGetCurrentThread() != NULL)
	  {
	     DbgPrint("kernel stack base %x\n",
		      PsGetCurrentThread()->Tcb.StackLimit);
		      	     
	  }
     }
   else
     {
	DbgPrint("kernel ESP %.8x\n", tf->esp);
     }
  if ((tf->cs & 0xffff) == KERNEL_CS)
    {
       DbgPrint("ESP %x\n", tf->esp);
       stack = (PULONG) (tf->esp + 24);
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
	DbgPrint("SS:ESP %x:%x\n", tf->ss0, tf->esp0);
        stack=(PULONG)(tf->esp0);
       
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
	
	if (MmIsPagePresent(NULL, (PVOID)tf->eip))
	  {
	     unsigned char instrs[512];
	     
	     memcpy(instrs, (PVOID)tf->eip, 512);
	     
	     DbgPrint("Instrs: ");
	     
	     for (i=0; i<10; i++)
	       {
		  DbgPrint("%x ", instrs[i]);
	       }
	  }
#endif
     }
   
   DbgPrint("\n");
   if ((tf->cs&0xffff) == USER_CS &&
       tf->eip < KERNEL_BASE)
     {
	DbgPrint("Killing current task\n");
	//   for(;;);
	KeLowerIrql(PASSIVE_LEVEL);
	if ((tf->cs&0xffff) == USER_CS)
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
   
   set_interrupt_gate(0,(int)exception_handler0);
   set_interrupt_gate(1,(int)exception_handler1);
   set_interrupt_gate(2,(int)exception_handler2);
   set_interrupt_gate(3,(int)exception_handler3);
   set_interrupt_gate(4,(int)exception_handler4);
   set_interrupt_gate(5,(int)exception_handler5);
   set_interrupt_gate(6,(int)exception_handler6);
   set_interrupt_gate(7,(int)exception_handler7);
   set_interrupt_gate(8,(int)exception_handler8);
   set_interrupt_gate(9,(int)exception_handler9);
   set_interrupt_gate(10,(int)exception_handler10);
   set_interrupt_gate(11,(int)exception_handler11);
   set_interrupt_gate(12,(int)exception_handler12);
   set_interrupt_gate(13,(int)exception_handler13);
   set_interrupt_gate(14,(int)exception_handler14);
   set_interrupt_gate(15,(int)exception_handler15);
   set_interrupt_gate(16,(int)exception_handler16);
   
   for (i=17;i<256;i++)
        {
	   set_interrupt_gate(i,(int)exception_handler_unknown);
        }
   
   set_system_call_gate(0x2d,(int)interrupt_handler2d);
   set_system_call_gate(0x2e,(int)interrupt_handler2e);
}
