/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/main.c
 * PURPOSE:         Initalizes the kernel
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/* INCLUDES *****************************************************************/

#include <windows.h>

#include <internal/ntoskrnl.h>
#include <internal/version.h>
#include <internal/mm.h>
#include <internal/string.h>
#include <internal/symbol.h>
#include <internal/module.h>
#include <internal/ldr.h>

#include <internal/mmhal.h>
#include <internal/i386/segment.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/
                                            
void set_breakpoint(unsigned int i, unsigned int addr, unsigned int type,
		    unsigned int len)
/*
 * FUNCTION: Sets a hardware breakpoint
 * ARGUMENTS:
 *          i = breakpoint to set (0 to 3)
 *          addr = linear address to break on
 *          type = Type of access to break on
 *          len = length of the variable to watch
 * NOTES:
 *       The variable to watch must be aligned to its length (i.e. a dword
 *	 breakpoint must be aligned to a dword boundary)
 * 
 *       A fatal exception will be generated on the access to the variable.
 *       It is (at the moment) only really useful for catching undefined
 *       pointers if you know the variable effected but not the buggy
 *       routine. 
 * 
 * FIXME: Extend to call out to kernel debugger on breakpoint
 *        Add support for I/O breakpoints
 * REFERENCES: See the i386 programmer manual for more details
 */ 
{
   unsigned int mask;
   
   if (i>3)
     {
	printk("Invalid breakpoint index at %s:%d\n",__FILE__,__LINE__);
	return;
     }
   
   /*
    * Load the linear address
    */
   switch (i)
     {
      case 0:
	__asm("movl %0,%%db0\n\t"
	      : /* no outputs */
	      : "d" (addr));
	break;
	
      case 1:
	__asm__("movl %0,%%db1\n\t"
		: /* no outputs */
		: "d" (addr));
	break;                        
	
      case 2:
	__asm__("movl %0,%%db2\n\t"
		: /* no outputs */
		: "d" (addr));
	break;                        
	
      case 3:
	__asm__("movl %0,%%db3\n\t"
		: /* no outputs */
		: "d" (addr));
	break;                        
     }
   
   /*
    * Setup mask for dr7
    */
   mask = (len<<(16 + 2 + i*4)) + (type<<(16 + i*4)) + (1<<(i*2));
   __asm__("movl %%db7,%%eax\n\t"
           "orl %0,%%eax\n\t"
	   "movl %%eax,%%db7\n\t"
	   : /* no outputs */
	   : "d" (mask)
	   : "ax");
}

extern int edata;
extern int end;

asmlinkage void _main(boot_param* _bp)
/*
 * FUNCTION: Called by the boot loader to start the kernel
 * ARGUMENTS:
 *          _bp = Pointer to boot parameters initialized by the boot loader
 * NOTE: The boot parameters are stored in low memory which will become
 * invalid after the memory managment is initialized so we make a local copy.
 */
{
   unsigned int i;
   unsigned int start;
   unsigned int start1;
   boot_param bp;
   
   memset((void *)&edata,0,((int)&end)-((int)&edata));
   
   /*
    * Copy the parameters to a local buffer because lowmem will go away
    */
   memcpy(&bp,_bp,sizeof(boot_param));
      
   /*
    * Initalize the console (before printing anything)
    */
   HalInitConsole(&bp);
   
   DbgPrint("Starting ReactOS "KERNEL_VERSION"\n");

   start = KERNEL_BASE + PAGE_ROUND_UP(bp.module_length[0]);
   if (start < ((int)&end))
     {
        DbgPrint("start %x end %x\n",start,(int)&end);
	DbgPrint("Kernel booted incorrectly, aborting\n");
	DbgPrint("Reduce the amount of uninitialized data\n");
	for(;;);
     }   
   start1 = start+PAGE_ROUND_UP(bp.module_length[1]);

   
   /*
    * Initalize various critical subsystems
    */
   HalInit(&bp);
   MmInitialize(&bp);
   KeInit();
   ObInit();
   PsInit();
   IoInit();
   LdrInitModuleManagement();
      
   /*
    * Initalize services loaded at boot time
    */
   DPRINT("%d files loaded\n",bp.nr_files);
    
   start = KERNEL_BASE + PAGE_ROUND_UP(bp.module_length[0]);
   start1 = start+PAGE_ROUND_UP(bp.module_length[1]);
   for (i=1;i<bp.nr_files;i++)
     {
        DPRINT("process module at %08lx\n", start);
      	LdrProcessDriver(start);
        start=start+PAGE_ROUND_UP(bp.module_length[i]);
     }
   
   /*
    * Load Auto configured drivers
    */
   LdrLoadAutoConfigDrivers();
   
#ifdef KRNL_TEST
   /*
    * Test various features of the kernel
    */
   TstBegin();
#endif

  /*
   *  Launch initial thread
   */
  LdrLoadInitialProcess();

   /*
    * Enter idle loop
    */
   printk("Finished main()\n");
   PsTerminateSystemThread(STATUS_SUCCESS);
}

