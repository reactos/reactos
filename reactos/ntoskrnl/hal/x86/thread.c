/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 kernel/hal/x86/thread.c
 * PURPOSE:              HAL multitasking functions
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *             27/06/98: Created
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/kernel.h>
#include <internal/psmgr.h>
#include <internal/string.h>
#include <internal/hal/hal.h>
#include <internal/hal/segment.h>
#include <internal/hal/page.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ***************************************************************/

#define FIRST_TSS_SELECTOR (KERNEL_DS + 0x8)
#define FIRST_TSS_OFFSET (FIRST_TSS_SELECTOR / 8)

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
   __asm__("pushfl\n\t"
	   "cli\n\t"
	   "ljmp %0\n\t"
	   "popfl\n\t"
	   : /* No outputs */
	   : "m" (*(((unsigned char *)(&(thread->context.nr)))-4) )
	   : "ax","dx");
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
   DPRINT("begin_thread %x %x\n",fn,start_context);
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
   unsigned int base = (unsigned int)(&(thread->context));
   unsigned int* kernel_stack = ExAllocatePool(NonPagedPool,4096);
   
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
   memset(&thread->context,0,sizeof(hal_thread_state));
   thread->context.ldt = 0;
   thread->context.eflags = (1<<1)+(1<<9);
   thread->context.iomap_base = FIELD_OFFSET(hal_thread_state,io_bitmap);
   thread->context.esp0 = &kernel_stack[1021];
   thread->context.ss0 = KERNEL_DS;
   thread->context.esp = &kernel_stack[1021];
   thread->context.ss = KERNEL_DS;
   thread->context.cs = KERNEL_CS;
   thread->context.eip = (unsigned long)begin_thread;
   thread->context.io_bitmap[0] = 0xff;
   thread->context.cr3 = ((unsigned int)get_page_directory()) - IDMAP_BASE;
   thread->context.ds = KERNEL_DS;
   thread->context.es = KERNEL_DS;
   thread->context.fs = KERNEL_DS;
   thread->context.gs = KERNEL_DS;
   thread->context.nr = desc * 8;
   
   return(TRUE);
}

void HalInitFirstTask(PKTHREAD thread)
/*
 * FUNCTION: Called to setup the HAL portion of a thread object for the 
 * initial thread
 */
{
   /*
    * Initialize the thread context
    */
   HalInitTask(thread,NULL,NULL);
   
   /*
    * Load the task register
    */
   __asm__("ltr %%ax" 
	   : /* no output */
           : "a" (FIRST_TSS_OFFSET*8));
}
