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

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

static exception_hook* exception_hooks[256]={NULL,};

#define _STR(x) #x
#define STR(x) _STR(x)

extern void interrupt_handler2e(void);
extern void interrupt_handler2d(void);

extern ULONG init_stack;
extern ULONG init_stack_top;

/* FUNCTIONS ****************************************************************/

#define EXCEPTION_HANDLER_WITH_ERROR(x,y)  \
      void exception_handler##y (void);   \
      void tmp_exception_handler##y (void) { \
       __asm__("\n\t_exception_handler"##x":\n\t" \
                "pushl %gs\n\t" \
                "pushl %fs\n\t" \
                "pushl %es\n\t" \
                "pushl %ds\n\t"    \
                "pushl $"##x"\n\t"                        \
                "pusha\n\t"                          \
                "movw $"STR(KERNEL_DS)",%ax\n\t"        \
                "movw %ax,%ds\n\t"      \
                "movw %ax,%es\n\t"      \
                "movw %ax,%fs\n\t"      \
                "movw %ax,%gs\n\t"      \
                "call _exception_handler\n\t"        \
                "popa\n\t" \
                "addl $4,%esp\n\t"                   \
                "popl %ds\n\t"      \
                "popl %es\n\t"      \
                "popl %fs\n\t"      \
                "popl %gs\n\t"      \
                "addl $4,%esp\n\t" \
                "iret\n\t"); }

#define EXCEPTION_HANDLER_WITHOUT_ERROR(x,y)           \
        asmlinkage void exception_handler##y (void);        \
        void tmp_exception_handler##y (void) { \
        __asm__("\n\t_exception_handler"##x":\n\t"           \
                "pushl $0\n\t"                        \
                "pushl %gs\n\t" \
                "pushl %fs\n\t" \
                "pushl %es\n\t" \
                "pushl %ds\n\t"   \
                "pushl $"##x"\n\t"                       \
                "pusha\n\t"                          \
                "movw $"STR(KERNEL_DS)",%ax\n\t"        \
                "movw %ax,%ds\n\t"      \
                "movw %ax,%es\n\t"      \
                "movw %ax,%fs\n\t"      \
                "movw %ax,%gs\n\t"      \
                "call _exception_handler\n\t"        \
                "popa\n\t"                           \
                "addl $4,%esp\n\t"                 \
                "popl %ds\n\t"  \
                "popl %es\n\t"  \
                "popl %fs\n\t"  \
                "popl %gs\n\t"  \
                "addl $4,%esp\n\t" \
                "iret\n\t"); }

asmlinkage void exception_handler_unknown(void);
asmlinkage void tmp_exception_handler_unknown(void)
{
        __asm__("\n\t_exception_handler_unknown:\n\t"           
                "pushl $0\n\t"
                "pushl %gs\n\t" 
                "pushl %fs\n\t" 
                "pushl %es\n\t" 
                "pushl %ds\n\t"   
                "pushl %ds\n\t"
                "pushl $0xff\n\t"                       
                "pusha\n\t"                          
                "movw $"STR(KERNEL_DS)",%ax\n\t"        
                "movw %ax,%ds\n\t"      
                "movw %ax,%es\n\t"      
                "movw %ax,%fs\n\t"      
                "movw %ax,%gs\n\t"      
                "call _exception_handler\n\t"        
                "popa\n\t"                           
                "addl $8,%esp\n\t"                 
                "iret\n\t");
}

EXCEPTION_HANDLER_WITHOUT_ERROR("0",0);
EXCEPTION_HANDLER_WITHOUT_ERROR("1",1);
EXCEPTION_HANDLER_WITHOUT_ERROR("2",2);
EXCEPTION_HANDLER_WITHOUT_ERROR("3",3);
EXCEPTION_HANDLER_WITHOUT_ERROR("4",4);
EXCEPTION_HANDLER_WITHOUT_ERROR("5",5);
EXCEPTION_HANDLER_WITHOUT_ERROR("6",6);
EXCEPTION_HANDLER_WITHOUT_ERROR("7",7);
EXCEPTION_HANDLER_WITH_ERROR("8",8);
EXCEPTION_HANDLER_WITHOUT_ERROR("9",9);
EXCEPTION_HANDLER_WITH_ERROR("10",10);
EXCEPTION_HANDLER_WITH_ERROR("11",11);
EXCEPTION_HANDLER_WITH_ERROR("12",12);
EXCEPTION_HANDLER_WITH_ERROR("13",13);
EXCEPTION_HANDLER_WITH_ERROR("14",14);
EXCEPTION_HANDLER_WITH_ERROR("15",15);
EXCEPTION_HANDLER_WITHOUT_ERROR("16",16);

extern unsigned int stext, etext;

static void print_address(PVOID address)
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
	     DbgPrint("<%S: %x>", current->Name, 
		      address - current->Base);
	     return;
	  }

	current_entry = current_entry->Flink;
     }
   DbgPrint("<%x>", address);
}

asmlinkage void exception_handler(unsigned int edi,
                                  unsigned int esi, unsigned int ebp,
                                  unsigned int esp, unsigned int ebx,
                                  unsigned int edx, unsigned int ecx,
                                  unsigned int eax,
                                  unsigned int type,
				  unsigned int ds,
				  unsigned int es,
                                  unsigned int fs,
				  unsigned int gs,
                                  unsigned int error_code,
                                  unsigned int eip,
                                  unsigned int cs, unsigned int eflags,
                                  unsigned int esp0, unsigned int ss0)
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
   
   __asm__("cli\n\t");
   
   if (type==14)
     {
	if (MmPageFault(cs&0xffff, eip, error_code))
	  {
	     return;
	  }
     }
   if (type==1)
     {
	DbgPrint("Trap at CS:EIP %x:%x\n",cs&0xffff,eip);
	return;
     }
   
   /*
    * Activate any hook for the exception
    */
   if (exception_hooks[type]!=NULL)
     {
	exception_hooks[type](NULL,type);
     }
   
   /*
    * Print out the CPU registers
    */
   if (type < 19)
     {
	DbgPrint("%s Exception: %d(%x)\n",TypeStrings[type],type,
		 error_code&0xffff);
     }
   else
     {
	DbgPrint("Exception: %d(%x)\n",type,error_code&0xffff);
     }
   DbgPrint("CS:EIP %x:%x\n",cs&0xffff,eip);
   DbgPrint("CS:EIP %x:", cs&0xffff);
   print_address((PVOID)eip);
   DbgPrint("\n");
   __asm__("movl %%cr2,%0\n\t"
	   : "=d" (cr2));
   __asm__("movl %%cr3,%0\n\t"
	   : "=d" (cr3));
   DbgPrint("cr2 %x cr3 %x\n",cr2,cr3);
//   for(;;);
   DbgPrint("Process: %x\n",PsGetCurrentProcess());
   if (PsGetCurrentProcess() != NULL)
     {
	DbgPrint("Process id: %x\n", PsGetCurrentProcess()->UniqueProcessId);
     }
   if (PsGetCurrentThread() != NULL)
     {
	DbgPrint("Thread: %x Thread id: %x\n",
		 PsGetCurrentThread(),
		 PsGetCurrentThread()->Cid.UniqueThread);
     }
   DbgPrint("DS %x ES %x FS %x GS %x\n",ds&0xffff,es&0xffff,fs&0xffff,
	    gs&0xfff);
   DbgPrint("EAX: %.8x   EBX: %.8x   ECX: %.8x\n",eax,ebx,ecx);
   DbgPrint("EDX: %.8x   EBP: %.8x   ESI: %.8x\n",edx,ebp,esi);
   DbgPrint("EDI: %.8x   EFLAGS: %.8x ",edi,eflags);
   if ((cs&0xffff) == KERNEL_CS)
     {
	DbgPrint("kESP %.8x\n",esp);
	if (PsGetCurrentThread() != NULL)
	  {
	     DbgPrint("kernel stack base %x\n",
		      PsGetCurrentThread()->Tcb.Context.KernelStackBase);
		      	     
	  }
     }
   else
     {
	DbgPrint("kernel ESP %.8x\n",esp);
     }
  if ((cs & 0xffff) == KERNEL_CS)
    {
      DbgPrint("ESP %x\n",esp);
       stack = (PULONG) (esp + 24);
       stack = (PULONG)(((ULONG)stack) & (~0x3));
       
      DbgPrint("Stack:\n");
      for (i = 0; i < 16; i = i + 4)
        {
          DbgPrint("%.8x %.8x %.8x %.8x\n", 
                   stack[i], 
                   stack[i+1], 
                   stack[i+2], 
                   stack[i+3]);
        }
      DbgPrint("Frames:\n");
      for (i = 0; i < 32; i++)
        {
          if (stack[i] > ((unsigned int) &stext) &&
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
#if 0
	DbgPrint("SS:ESP %x:%x\n",ss0,esp0);
        stack=(PULONG)(esp0);
       
        DbgPrint("Stack:\n");
        for (i=0; i<16; i++)
        {
	   if (MmIsPagePresent(NULL,&stack[i]))
	     {
		DbgPrint("%.8x ",stack[i]);
		if (((i+1)%4) == 0)
		  {
		     DbgPrint("\n");
		  }
	     }
        }
#endif
     }
   
   DbgPrint("\n");
   if ((cs&0xffff) == USER_CS &&
       eip < KERNEL_BASE)
     {
	DbgPrint("Killing current task\n");
	//   for(;;);
	KeLowerIrql(PASSIVE_LEVEL);
	if ((cs&0xffff) == USER_CS)
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
		 PsGetCurrentThread()->Tcb.Context.KernelStackBase);
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

asmlinkage unsigned int ExHookException(exception_hook fn, unsigned int exp)
/*
 * FUNCTION: Hook an exception
 */
{
        if (exp>=256)
        {
                return(1);
        }
        exception_hooks[exp]=fn;
        return(0);
}

asmlinkage void KeInitExceptions(void)
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
