/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/x86/thread.c
 * PURPOSE:              HAL multitasking functions
 * PROGRAMMER:           David Welch (welch@cwcom.net)
 * REVISION HISTORY:
 *             27/06/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <internal/ps.h>
#include <string.h>
#include <internal/string.h>
#include <internal/hal.h>
#include <internal/i386/segment.h>
#include <internal/mmhal.h>
#include <internal/ke.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ***************************************************************/

#define NR_TASKS 128

#define FIRST_TSS_SELECTOR (KERNEL_DS + 0x8)
#define FIRST_TSS_OFFSET (FIRST_TSS_SELECTOR / 8)

static char KiNullLdt[8] = {0,};
static unsigned int KiNullLdtSel = 0;
static PETHREAD FirstThread = NULL;

/* FUNCTIONS **************************************************************/

VOID HalTaskSwitch(PKTHREAD thread)
/*
 * FUNCTION: Switch tasks
 * ARGUMENTS:
 *         thread = Thread to switch to
 * NOTE: This function will not return until the current thread is scheduled
 * again (possibly never);
 */
{
   DPRINT("Scheduling thread %x\n",thread);
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
   DPRINT("thread->Context.cr3 %x\n",thread->Context.cr3);
   __asm__("pushfl\n\t"
	   "cli\n\t"
	   "ljmp %0\n\t"
	   "popfl\n\t"
	   : /* No outputs */
	   : "m" (*(((unsigned char *)(&(thread->Context.nr)))-4) )
	   : "ax","dx");
}

#define FLAG_NT (1<<14)
#define FLAG_VM (1<<17)
#define FLAG_IF (1<<9)
#define FLAG_IOPL ((1<<12)+(1<<13))

NTSTATUS KeValidateUserContext(PCONTEXT Context)
/*
 * FUNCTION: Validates a processor context
 * ARGUMENTS:
 *        Context = Context to validate
 * RETURNS: Status
 * NOTE: This only validates the context as not violating system security, it
 * doesn't guararantee the thread won't crash at some point
 * NOTE2: This relies on there only being two selectors which can access 
 * system space
 */
{
   if (Context->Eip >= KERNEL_BASE)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegCs == KERNEL_CS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegDs == KERNEL_DS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegEs == KERNEL_DS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegFs == KERNEL_DS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Context->SegGs == KERNEL_DS)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if ((Context->EFlags & FLAG_IOPL) != 0 ||
       (Context->EFlags & FLAG_NT) ||
       (Context->EFlags & FLAG_VM) ||
       (!(Context->EFlags & FLAG_IF)))
     {
        return(STATUS_UNSUCCESSFUL);
     }
   return(STATUS_SUCCESS);
}

NTSTATUS HalReleaseTask(PETHREAD Thread)
/*
 * FUNCTION: Releases the resource allocated for a thread by
 * HalInitTaskWithContext or HalInitTask
 * NOTE: The thread had better not be running when this is called
 */
{
   KeFreeGdtSelector(Thread->Tcb.Context.nr);
   ExFreePool(Thread->Tcb.Context.KernelStackBase);
   if (Thread->Tcb.Context.SavedKernelStackBase != NULL)
     {
	ExFreePool(Thread->Tcb.Context.SavedKernelStackBase);
     }
   return(STATUS_SUCCESS);
}

NTSTATUS HalInitTaskWithContext(PETHREAD Thread, PCONTEXT Context)
/*
 * FUNCTION: Initialize a task with a user mode context
 * ARGUMENTS:
 *        Thread = Thread to initialize
 *        Context = Processor context to initialize it with
 * RETURNS: Status
 */
{
   unsigned int desc;
   unsigned int length;
   unsigned int base;
   PVOID kernel_stack;
   NTSTATUS Status;
   PVOID stack_start;
   ULONG GdtDesc[2];
   
   DPRINT("HalInitTaskWithContext(Thread %x, Context %x)\n",
          Thread,Context);

   assert(sizeof(hal_thread_state)>=0x68);
   
   if ((Status=KeValidateUserContext(Context))!=STATUS_SUCCESS)
     {
	return(Status);
     }
      
   length = sizeof(hal_thread_state) - 1;
   base = (unsigned int)(&(Thread->Tcb.Context));
//   kernel_stack = ExAllocatePool(NonPagedPool,PAGESIZE);
   kernel_stack = ExAllocatePool(NonPagedPool, 3*PAGESIZE);
   
   /*
    * Setup a TSS descriptor
    */
   GdtDesc[0] = (length & 0xffff) | ((base & 0xffff) << 16);
   GdtDesc[1] = ((base & 0xff0000)>>16) | 0x8900 | (length & 0xf0000)
                 | (base & 0xff000000);
   desc = KeAllocateGdtSelector(GdtDesc);
   if (desc == 0)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   stack_start = kernel_stack + 4096 - sizeof(CONTEXT);
   memcpy(stack_start, Context, sizeof(CONTEXT));
   
   /*
    * Initialize the thread context
    */
   memset(&Thread->Tcb.Context,0,sizeof(hal_thread_state));
   Thread->Tcb.Context.ldt = KiNullLdtSel;
   Thread->Tcb.Context.eflags = (1<<1) + (1<<9);
   Thread->Tcb.Context.iomap_base = FIELD_OFFSET(hal_thread_state,io_bitmap);
   Thread->Tcb.Context.esp0 = (ULONG)stack_start;
   Thread->Tcb.Context.ss0 = KERNEL_DS;
   Thread->Tcb.Context.esp = (ULONG)stack_start;
   Thread->Tcb.Context.ss = KERNEL_DS;
   Thread->Tcb.Context.cs = KERNEL_CS;
   Thread->Tcb.Context.eip = (ULONG)PsBeginThreadWithContextInternal;
   Thread->Tcb.Context.io_bitmap[0] = 0xff;
   Thread->Tcb.Context.cr3 = (ULONG)
     Thread->ThreadsProcess->Pcb.PageTableDirectory;
   Thread->Tcb.Context.ds = KERNEL_DS;
   Thread->Tcb.Context.es = KERNEL_DS;
   Thread->Tcb.Context.fs = KERNEL_DS;
   Thread->Tcb.Context.gs = KERNEL_DS;

   Thread->Tcb.Context.nr = desc * 8;
   Thread->Tcb.Context.KernelStackBase = kernel_stack;
   Thread->Tcb.Context.SavedKernelEsp = 0;
   Thread->Tcb.Context.SavedKernelStackBase = NULL;
   
   return(STATUS_SUCCESS);
}

NTSTATUS HalInitTask(PETHREAD thread, PKSTART_ROUTINE fn, PVOID StartContext)
/*
 * FUNCTION: Initializes the HAL portion of a thread object
 * ARGUMENTS:
 *       thread = Object describes the thread
 *       fn = Entrypoint for the thread
 *       StartContext = parameter to pass to the thread entrypoint
 * RETURNS: True if the function succeeded
 */
{
   unsigned int desc;
   unsigned int length = sizeof(hal_thread_state) - 1;
   unsigned int base = (unsigned int)(&(thread->Tcb.Context));
//   PULONG KernelStack = ExAllocatePool(NonPagedPool,4096);
   PULONG KernelStack;
   ULONG GdtDesc[2];
   
   DPRINT("HalInitTask(Thread %x, fn %x, StartContext %x)\n",
          thread,fn,StartContext);
   DPRINT("thread->ThreadsProcess %x\n",thread->ThreadsProcess);
   
   KernelStack = ExAllocatePool(NonPagedPool, 3*PAGESIZE);
   
   /*
    * Make sure
    */
   assert(sizeof(hal_thread_state)>=0x68);
   
   /*
    * Setup a TSS descriptor
    */
   GdtDesc[0] = (length & 0xffff) | ((base & 0xffff) << 16);
   GdtDesc[1] = ((base & 0xff0000)>>16) | 0x8900 | (length & 0xf0000)
                 | (base & 0xff000000);
   desc = KeAllocateGdtSelector(GdtDesc);
   if (desc == 0)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   
//   DPRINT("sizeof(descriptor) %d\n",sizeof(descriptor));
//   DPRINT("desc %d\n",desc);
   
   /*
    * Initialize the stack for the thread (including the two arguments to 
    * the general start routine).					   
    */
   KernelStack[1023] = (unsigned int)StartContext;
   KernelStack[1022] = (unsigned int)fn;
   KernelStack[1021] = 0;     
   
   /*
    * Initialize the thread context
    */
   memset(&thread->Tcb.Context,0,sizeof(hal_thread_state));
   thread->Tcb.Context.ldt = KiNullLdtSel;
   thread->Tcb.Context.eflags = (1<<1)+(1<<9);
   thread->Tcb.Context.iomap_base = FIELD_OFFSET(hal_thread_state,io_bitmap);
   thread->Tcb.Context.esp0 = (ULONG)&KernelStack[1021];
   thread->Tcb.Context.ss0 = KERNEL_DS;
   thread->Tcb.Context.esp = (ULONG)&KernelStack[1021];
   thread->Tcb.Context.ss = KERNEL_DS;
   thread->Tcb.Context.cs = KERNEL_CS;
   thread->Tcb.Context.eip = (ULONG)PsBeginThread;
   thread->Tcb.Context.io_bitmap[0] = 0xff;
   thread->Tcb.Context.cr3 = (ULONG)
     thread->ThreadsProcess->Pcb.PageTableDirectory;
   thread->Tcb.Context.ds = KERNEL_DS;
   thread->Tcb.Context.es = KERNEL_DS;
   thread->Tcb.Context.fs = KERNEL_DS;
   thread->Tcb.Context.gs = KERNEL_DS;
   thread->Tcb.Context.nr = desc * 8;
   thread->Tcb.Context.KernelStackBase = KernelStack;
   thread->Tcb.Context.SavedKernelEsp = 0;
   thread->Tcb.Context.SavedKernelStackBase = NULL;
   DPRINT("Allocated %x\n",desc*8);
   
   return(STATUS_SUCCESS);
}

void HalInitFirstTask(PETHREAD thread)
/*
 * FUNCTION: Called to setup the HAL portion of a thread object for the 
 * initial thread
 */
{
   ULONG base;
   ULONG length;
   ULONG desc;
   ULONG GdtDesc[2];
   
   memset(KiNullLdt, 0, sizeof(KiNullLdt));
   base = (unsigned int)&KiNullLdt;
   length = sizeof(KiNullLdt) - 1;
   GdtDesc[0] = (length & 0xffff) | ((base & 0xffff) << 16);
   GdtDesc[1] = ((base & 0xff0000)>>16) | 0x8200 | (length & 0xf0000)
                | (base & 0xff000000);
   desc = KeAllocateGdtSelector(GdtDesc);
   KiNullLdtSel = desc*8;
   
   /*
    * Initialize the thread context
    */
   HalInitTask(thread,NULL,NULL);

   /*
    * Load the task register
    */
   __asm__("ltr %%ax" 
	   : /* no output */
           : "a" (thread->Tcb.Context.nr));
   FirstThread = thread;
}
