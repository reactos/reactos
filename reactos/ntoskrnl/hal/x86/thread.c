/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/hal/x86/thread.c
 * PURPOSE:              HAL multitasking functions
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *             27/06/98: Created
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <internal/ps.h>
#include <internal/string.h>
#include <internal/hal.h>
#include <internal/hal/segment.h>
#include <internal/hal/page.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ***************************************************************/

#define FIRST_TSS_SELECTOR (KERNEL_DS + 0x8)
#define FIRST_TSS_OFFSET (FIRST_TSS_SELECTOR / 8)

static char null_ldt[8]={0,};
static unsigned int null_ldt_sel=0;
static PKTHREAD FirstThread=NULL;

/* FUNCTIONS **************************************************************/

void HalTaskSwitch(PKTHREAD thread)
/*
 * FUNCTION: Switch tasks
 * ARGUMENTS:
 *         thread = Thread to switch to
 * NOTE: This function will not return until the current thread is scheduled
 * again
 */
{
   DPRINT("Scheduling thread %x\n",thread->Context.nr);
   DPRINT("previous task %x reserved1 %x esp0 %x ss0 %x\n",
          thread->Context.previous_task,thread->Context.reserved1,
          thread->Context.esp0,thread->Context.ss0);
   DPRINT("reserved2 %x esp1 %x ss1 %x reserved3 %x esp2 %x ss2 %x\n",
          thread->Context.reserved2,thread->Context.esp1,thread->Context.ss1,
          thread->Context.reserved3,thread->Context.esp2,thread->Context.ss2);
   DPRINT("reserved4 %x cr3 %x eip %x eflags %x eax %x\n",
          thread->Context.reserved4,thread->Context.cr3,thread->Context.eip,
          thread->Context.eflags,thread->Context.eax);
   DPRINT("ecx %x edx %x ebx %x esp %x ebp %x esi %x\n",
          thread->Context.ecx,thread->Context.edx,thread->Context.ebx,
          thread->Context.esp,thread->Context.ebp,thread->Context.esi);
   DPRINT("edi %x es %x reserved5 %x cs %x reserved6 %x\n",
          thread->Context.edi,thread->Context.es,thread->Context.reserved5,
          thread->Context.cs,thread->Context.reserved6);
   DPRINT("ss %x reserved7 %x ds %x reserved8 %x fs %x\n",
          thread->Context.ss,thread->Context.reserved7,thread->Context.ds,
          thread->Context.reserved8,thread->Context.fs);
   DPRINT("reserved9 %x gs %x reserved10 %x ldt %x reserved11 %x\n",
          thread->Context.reserved9,thread->Context.gs,
          thread->Context.reserved10,thread->Context.ldt,
          thread->Context.reserved11);
   DPRINT("trap %x iomap_base %x nr %x io_bitmap[0] %x\n",
          thread->Context.trap,thread->Context.iomap_base,
          thread->Context.nr,thread->Context.io_bitmap[0]);
   __asm__("pushfl\n\t"
	   "cli\n\t"
	   "ljmp %0\n\t"
	   "popfl\n\t"
	   : /* No outputs */
	   : "m" (*(((unsigned char *)(&(thread->Context.nr)))-4) )
	   : "ax","dx");
//   set_breakpoint(0,&(FirstThread->Context.gs),HBP_READWRITE,HBP_DWORD);
}

static unsigned int allocate_tss_descriptor(void)
/*
 * FUNCTION: Allocates a slot within the GDT to describe a TSS
 * RETURNS: The offset within the GDT of the slot allocated on succcess
 *          Zero on failure
 */
{
   unsigned int i;
   for (i=0;i<16;i++)
     {
	if (gdt[FIRST_TSS_OFFSET + i].a==0 &&
	    gdt[FIRST_TSS_OFFSET + i].b==0)
	  {
	     return(FIRST_TSS_OFFSET + i);
	  }
     }
   return(0);
}

static void begin_thread(PKSTART_ROUTINE fn, PVOID start_context)
/*
 * FUNCTION: This function is the start point for all new threads
 * ARGUMENTS:
 *          fn = Actual start point of the thread
 *          start_context = Parameter to pass to the start routine
 * RETURNS: Can't 
 */
{
   NTSTATUS ret;
//   DPRINT("begin_thread %x %x\n",fn,start_context);
   KeLowerIrql(PASSIVE_LEVEL);
   ret = fn(start_context);
   PsTerminateSystemThread(ret);
   for(;;);
}

BOOLEAN HalInitTask(PKTHREAD thread, PKSTART_ROUTINE fn, 
		    PVOID StartContext)
/*
 * FUNCTION: Initializes the HAL portion of a thread object
 * ARGUMENTS:
 *       thread = Object describes the thread
 *       fn = Entrypoint for the thread
 *       StartContext = parameter to pass to the thread entrypoint
 * RETURNS: True if the function succeeded
 */
{
   unsigned int desc = allocate_tss_descriptor();
   unsigned int length = sizeof(hal_thread_state) - 1;
   unsigned int base = (unsigned int)(&(thread->Context));
   unsigned int* kernel_stack = ExAllocatePool(NonPagedPool,4096);
   
   DPRINT("HalInitTask(Thread %x, fn %x, StartContext %x)\n",
          thread,fn,StartContext);

   /*
    * Make sure
    */
   assert(sizeof(hal_thread_state)>=0x68);
   
   /*
    * Setup a TSS descriptor
    */
   gdt[desc].a = (length & 0xffff) | ((base & 0xffff) << 16);
   gdt[desc].b = ((base & 0xff0000)>>16) | 0x8900 | (length & 0xf0000)
                 | (base & 0xff000000);
   
   /*
    * Initialize the stack for the thread (including the two arguments to 
    * the general start routine).					   
    */
   kernel_stack[1023] = (unsigned int)StartContext;
   kernel_stack[1022] = (unsigned int)fn;
   kernel_stack[1021] = NULL;     
   
   /*
    * Initialize the thread context
    */
   memset(&thread->Context,0,sizeof(hal_thread_state));
   thread->Context.ldt = null_ldt_sel;
   thread->Context.eflags = (1<<1)+(1<<9);
   thread->Context.iomap_base = FIELD_OFFSET(hal_thread_state,io_bitmap);
   thread->Context.esp0 = &kernel_stack[1021];
   thread->Context.ss0 = KERNEL_DS;
   thread->Context.esp = &kernel_stack[1021];
   thread->Context.ss = KERNEL_DS;
   thread->Context.cs = KERNEL_CS;
   thread->Context.eip = (unsigned long)begin_thread;
   thread->Context.io_bitmap[0] = 0xff;
   thread->Context.cr3 = ((unsigned int)get_page_directory()) - IDMAP_BASE;
   thread->Context.ds = KERNEL_DS;
   thread->Context.es = KERNEL_DS;
   thread->Context.fs = KERNEL_DS;
   thread->Context.gs = KERNEL_DS;
   thread->Context.nr = desc * 8;
   DPRINT("Allocated %x\n",desc*8);
   

   return(TRUE);
}

void HalInitFirstTask(PKTHREAD thread)
/*
 * FUNCTION: Called to setup the HAL portion of a thread object for the 
 * initial thread
 */
{
   unsigned int base;
   unsigned int length;
   unsigned int desc;
   
   memset(null_ldt,0,sizeof(null_ldt));
   desc = allocate_tss_descriptor();
   base = (unsigned int)&null_ldt;
   length = sizeof(null_ldt) - 1;
   gdt[desc].a = (length & 0xffff) | ((base & 0xffff) << 16);
   gdt[desc].b = ((base & 0xff0000)>>16) | 0x8200 | (length & 0xf0000)
                 | (base & 0xff000000);
   null_ldt_sel = desc*8;
   
   /*
    * Initialize the thread context
    */
   HalInitTask(thread,NULL,NULL);

   /*
    * Load the task register
    */
   __asm__("ltr %%ax" 
	   : /* no output */
           : "a" (thread->Context.nr));
   FirstThread = thread;
}
