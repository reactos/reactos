/* $Id: main.c,v 1.43 2000/04/08 19:10:21 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/main.c
 * PURPOSE:         Initalizes the kernel
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <reactos/buildno.h>
#include <internal/mm.h>
#include <string.h>
#include <internal/string.h>
#include <internal/module.h>
#include <internal/ldr.h>
#include <internal/ex.h>
#include <internal/ps.h>

#include <internal/mmhal.h>
#include <internal/i386/segment.h>

#define NDEBUG
#include <internal/debug.h>

/* DATA *********************************************************************/

USHORT EXPORTED NtBuildNumber = KERNEL_VERSION_BUILD;
ULONG EXPORTED NtGlobalFlag = 0;

/* FUNCTIONS ****************************************************************/

static void
PrintString (char* fmt,...)
{
	char buffer[512];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);

	HalDisplayString (buffer);
}


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
	DbgPrint("Invalid breakpoint index at %s:%d\n",__FILE__,__LINE__);
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

#if 0
static char * INIData =
  "[HKEY_LOCAL_MACHINE\\HARDWARE]\r\n"
  "\r\n"
  "[HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP]\r\n"
  "\r\n"
  "[HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\AtDisk]\r\n"
  "\r\n"
  "[HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\AtDisk\\Controller 0]\r\n"
  "Controller Address=dword:000001f0\r\n"
  "Controller Interrupt=dword:0000000e\r\n"
  "\r\n"
  "\r\n"
  "\r\n"
  "";
#endif

unsigned int old_idt[256][2];
//extern unsigned int idt[];
unsigned int old_idt_valid = 1;

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
   unsigned int last_kernel_address;
   
//   memset((void *)&edata,0,((int)&end)-((int)&edata));
   
   /*
    * Copy the parameters to a local buffer because lowmem will go away
    */
   memcpy(&bp,_bp,sizeof(boot_param));

   /*
    * FIXME: Preliminary hack!!!!
    * Initializes the kernel parameter line.
    * This should be done by the boot loader.
    */
   strcpy (bp.kernel_parameters, "/DEBUGPORT=SCREEN");

   /*
    * Initialization phase 0
    */
   HalInitSystem (0, &bp);

   HalDisplayString("Starting ReactOS "KERNEL_VERSION_STR" (Build "KERNEL_VERSION_BUILD_STR")\n");

   start = KERNEL_BASE + PAGE_ROUND_UP(bp.module_length[0]);
   if (start < ((int)&end))
     {
	PrintString("start %x end %x\n",start,(int)&end);
	PrintString("Kernel booted incorrectly, aborting\n");
	PrintString("Reduce the amount of uninitialized data\n");
	PrintString("\n\n*** The system has halted ***\n");
	for(;;)
	     __asm__("hlt\n\t");
     }
   start1 = start+PAGE_ROUND_UP(bp.module_length[1]);

   last_kernel_address = KERNEL_BASE;
   for (i=0; i<=bp.nr_files; i++)
     {
	last_kernel_address = last_kernel_address +
	  PAGE_ROUND_UP(bp.module_length[i]);
     }

   DPRINT("MmInitSystem()\n");
   MmInitSystem(0, &bp, last_kernel_address);

   /*
    * Initialize the kernel debugger
    */
   KdInitSystem (0, &bp);
   if (KdPollBreakIn ())
     {
	DbgBreakPointWithStatus (DBG_STATUS_CONTROL_C);
     }

   /*
    * Initialization phase 1
    * Initalize various critical subsystems
    */
   DPRINT("Kernel Initialization Phase 1\n");

   DPRINT("HalInitSystem()\n");
   HalInitSystem (1, &bp);
   DPRINT("MmInitSystem()\n");
   MmInitSystem(1, &bp, 0);

   DPRINT("KeInit()\n");
   KeInit();
   DPRINT("ExInit()\n");
   ExInit();
   DPRINT("ObInit()\n");
   ObInit();
   DPRINT("PsInit()\n");
   PiInitProcessManager();
   DPRINT("IoInit()\n");
   IoInit();
   DPRINT("LdrInitModuleManagement()\n");
   LdrInitModuleManagement();
   CmInitializeRegistry();
   NtInit();
   
   /* Report all resources used by hal */
   HalReportResourceUsage ();
   
   memcpy(old_idt, KiIdt, sizeof(old_idt));
   old_idt_valid = 0;
   
   /*
    * Initalize services loaded at boot time
    */
   DPRINT1("%d files loaded\n",bp.nr_files);

  /*  Pass 1: load registry chunks passed in  */
  start = KERNEL_BASE + PAGE_ROUND_UP(bp.module_length[0]);
  for (i = 1; i < bp.nr_files; i++)
    {
      if (!strcmp ((PCHAR) start, "REGEDIT4"))
        {
          DPRINT1("process registry chunk at %08lx\n", start);
          CmImportHive((PCHAR) start);
        }
      start = start + bp.module_length[i];
    }

  /*  Pass 2: process boot loaded drivers  */
  start = KERNEL_BASE + PAGE_ROUND_UP(bp.module_length[0]);
  start1 = start + bp.module_length[1];
  for (i=1;i<bp.nr_files;i++)
    {
      if (strcmp ((PCHAR) start, "REGEDIT4"))
        {
          DPRINT1("process module at %08lx\n", start);
          LdrProcessDriver((PVOID)start);
        }
      start = start + bp.module_length[i];
    }
   
   /*
    * Load Auto configured drivers
    */
   CHECKPOINT;
   LdrLoadAutoConfigDrivers();
   
  /*
   *  Launch initial process
   */
   CHECKPOINT;
   LdrLoadInitialProcess();
   
   /*
    * Enter idle loop
    */
   DbgPrint("Finished main()\n");
   PsTerminateSystemThread(STATUS_SUCCESS);
}


/* EOF */
