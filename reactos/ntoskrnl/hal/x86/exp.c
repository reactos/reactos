/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/hal/x86/exp.c
 * PURPOSE:              Handling exceptions
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *              ??/??/??: Created
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <internal/ntoskrnl.h>
#include <internal/ke.h>
#include <internal/hal/segment.h>
#include <internal/hal/page.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

typedef unsigned int (exception_hook)(CONTEXT* c, unsigned int exp);
asmlinkage unsigned int ExHookException(exception_hook fn, UINT exp);

extern descriptor idt[256];
static exception_hook* exception_hooks[256]={NULL,};

#define _STR(x) #x
#define STR(x) _STR(x)

/* FUNCTIONS ****************************************************************/

#define EXCEPTION_HANDLER_WITH_ERROR(x,y)  \
      void exception_handler##y (void);   \
       __asm__("\n\t_exception_handler"##x":\n\t" \
                "pushl %ds\n\t"    \
                "pushl $"##x"\n\t"                        \
                "pusha\n\t"                          \
                "movw $"STR(KERNEL_DS)",%ax\n\t"        \
                "movw %ax,%ds\n\t"      \
                "call _exception_handler\n\t"        \
                "popa\n\t" \
                "addl $8,%esp\n\t"                   \
                "iret\n\t")

#define EXCEPTION_HANDLER_WITHOUT_ERROR(x,y)           \
        asmlinkage void exception_handler##y (void);        \
        __asm__("\n\t_exception_handler"##x":\n\t"           \
                "pushl $0\n\t"                        \
                "pushl %ds\n\t"   \
                "pushl $"##x"\n\t"                       \
                "pusha\n\t"                          \
                "movw $"STR(KERNEL_DS)",%ax\n\t"        \
                "movw %ax,%ds\n\t"      \
                "call _exception_handler\n\t"        \
                "popa\n\t"                           \
                "addl $8,%esp\n\t"                 \
                "iret\n\t")

asmlinkage void exception_handler_unknown(void);        
        __asm__("\n\t_exception_handler_unknown:\n\t"           
                "pushl $0\n\t"
                "pushl %ds\n\t"
                "pushl $0xff\n\t"                       
                "pusha\n\t"                          
                "movw $"STR(KERNEL_DS)",%ax\n\t"        
                "movw %ax,%ds\n\t"      
                "call _exception_handler\n\t"        
                "popa\n\t"                           
                "addl $8,%esp\n\t"                 
                "iret\n\t");


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

/*
 * The page fault handler is defined by the memory managment because it is
 * special
 */
//EXCEPTION_HANDLER_WITH_ERROR("14",14);
asmlinkage void exception_handler14(void);

EXCEPTION_HANDLER_WITH_ERROR("15",15);
EXCEPTION_HANDLER_WITHOUT_ERROR("16",16);

extern unsigned int etext;

asmlinkage void exception_handler(unsigned int edi,
                                  unsigned int esi, unsigned int ebp,
                                  unsigned int esp, unsigned int ebx,
                                  unsigned int edx, unsigned int ecx,
                                  unsigned int eax, 
                                  unsigned int type,
                                  unsigned int ds,
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
   unsigned int cr2;
   unsigned int i;
   unsigned int* stack;
   
   __asm__("cli\n\t");
   
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
   printk("Exception: %d(%x)\n",type,error_code&0xffff);
   printk("CS:EIP %x:%x\n",cs&0xffff,eip);
//   for(;;);
   printk("EAX: %.8x   EBX: %.8x   ECX: %.8x\n",eax,ebx,ecx);
   printk("EDX: %.8x   EBP: %.8x   ESI: %.8x\n",edx,ebp,esi);
   printk("EDI: %.8x   EFLAGS: %.8x ",edi,eflags);
   if ((cs&0xffff)==KERNEL_CS)
     {
	printk("ESP %.8x\n",esp);
     }
   
   __asm__("movl %%cr2,%0\n\t"
	   : "=d" (cr2));
   printk("cr2 %x\n",cr2);
   
   if ((cs&0xffff)==KERNEL_CS)
     {
	printk("ESP %x\n",esp);
        stack=(unsigned int *)(esp+24);
	
//        #if 0
        printk("Stack:\n");
        for (i=0;i<16;i=i+4)
        {
                printk("%.8x %.8x %.8x %.8x\n",stack[i],stack[i+1],stack[i+2],
                       stack[i+3]);
        }
        printk("Frames:\n");
        for (i=0;i<32;i++)
        {
                if (stack[i] > KERNEL_BASE &&
                    stack[i] < ((unsigned int)&etext) )
                    {
                        printk("%.8x  ",stack[i]);
                    }
        }
//        #endif
     }
   else
     {
	printk("SS:ESP %x:%x\n",ss0,esp0);
     }
   
   for(;;);
}

static void set_interrupt_gate(unsigned int sel, unsigned int func)
{
        idt[sel].a = (((int)func)&0xffff) +
                           (KERNEL_CS << 16);
        idt[sel].b = 0x8f00 + (((int)func)&0xffff0000);         
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
}
