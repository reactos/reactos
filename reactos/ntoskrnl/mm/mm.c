/* $Id: mm.c,v 1.25 2000/03/26 19:38:30 ea Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/mm.c
 * PURPOSE:     kernel memory managment functions
 * PROGRAMMER:  David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              Created 9/4/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/hal/io.h>
#include <internal/i386/segment.h>
#include <internal/stddef.h>
#include <internal/mm.h>
#include <string.h>
#include <internal/string.h>
#include <internal/ntoskrnl.h>
#include <internal/bitops.h>
#include <internal/string.h>
#include <internal/io.h>

#include <internal/mmhal.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

/*
 * Size of extended memory (kb) (fixed for now)
 */
#define EXTENDED_MEMORY_SIZE  (3*1024*1024)

/*
 * Compiler defined symbol 
 */
extern unsigned int stext;
extern unsigned int etext;
extern unsigned int end;

static BOOLEAN IsThisAnNtAsSystem = FALSE;
static MM_SYSTEM_SIZE MmSystemSize = MmSmallSystem;

extern unsigned int etext;
extern unsigned int _bss_end__;

static MEMORY_AREA* kernel_text_desc = NULL;
static MEMORY_AREA* kernel_data_desc = NULL;
static MEMORY_AREA* kernel_param_desc = NULL;
static MEMORY_AREA* kernel_pool_desc = NULL;

/*
 * All pagefaults are synchronized on this 
 */
static KSPIN_LOCK MiPageFaultLock;

/* FUNCTIONS ****************************************************************/

VOID MiShutdownMemoryManager(VOID)
{
}

VOID MmInitVirtualMemory(boot_param* bp)
/*
 * FUNCTION: Intialize the memory areas list
 * ARGUMENTS:
 *           bp = Pointer to the boot parameters
 *           kernel_len = Length of the kernel
 */
{
   unsigned int kernel_len = bp->end_mem - bp->start_mem;
   PVOID BaseAddress;
   ULONG Length;
   ULONG ParamLength = kernel_len;
   
   DPRINT("MmInitVirtualMemory(%x)\n",bp);
   
   MmInitMemoryAreas();
   ExInitNonPagedPool(KERNEL_BASE+ PAGE_ROUND_UP(kernel_len) + PAGESIZE);
   
   
   /*
    * Setup the system area descriptor list
    */
   BaseAddress = (PVOID)KERNEL_BASE;
   Length = PAGE_ROUND_UP(((ULONG)&etext)) - KERNEL_BASE;
   ParamLength = ParamLength - Length;
   MmCreateMemoryArea(KernelMode,NULL,MEMORY_AREA_SYSTEM,&BaseAddress,
		      Length,0,&kernel_text_desc);
   
   Length = PAGE_ROUND_UP(((ULONG)&_bss_end__)) - 
            PAGE_ROUND_UP(((ULONG)&etext));
   ParamLength = ParamLength - Length;
   DPRINT("Length %x\n",Length);
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&etext));
   DPRINT("BaseAddress %x\n",BaseAddress);
   MmCreateMemoryArea(KernelMode,
		      NULL,
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_data_desc);
   
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&end));
   Length = ParamLength;
   MmCreateMemoryArea(KernelMode,NULL,MEMORY_AREA_SYSTEM,&BaseAddress,
		      Length,0,&kernel_param_desc);

   BaseAddress = (PVOID)(KERNEL_BASE + PAGE_ROUND_UP(kernel_len) + PAGESIZE);
   Length = NONPAGED_POOL_SIZE;
   MmCreateMemoryArea(KernelMode,NULL,MEMORY_AREA_SYSTEM,&BaseAddress,
		      Length,0,&kernel_pool_desc);

//   MmDumpMemoryAreas();
   DPRINT("MmInitVirtualMemory() done\n");
}

NTSTATUS MmCommitedSectionHandleFault(MEMORY_AREA* MemoryArea, PVOID Address)
{
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&MiPageFaultLock, &oldIrql);
   
   if (MmIsPagePresent(NULL, Address))
     {
	KeReleaseSpinLock(&MiPageFaultLock, oldIrql);
	return(STATUS_SUCCESS);
     }
   
   MmSetPage(PsGetCurrentProcess(),
	     Address,
	     MemoryArea->Attributes,
	     (ULONG)MmAllocPage());
   
   KeReleaseSpinLock(&MiPageFaultLock, oldIrql);
   
   return(STATUS_SUCCESS);
}

NTSTATUS MmSectionHandleFault(MEMORY_AREA* MemoryArea, 
			      PVOID Address)
{
   LARGE_INTEGER Offset;
   IO_STATUS_BLOCK IoStatus;
   PMDL Mdl;
   PVOID Page;
   NTSTATUS Status;
   KIRQL oldIrql;
   
   DPRINT("MmSectionHandleFault(MemoryArea %x, Address %x)\n",
	  MemoryArea,Address);
   
   KeAcquireSpinLock(&MiPageFaultLock, &oldIrql);
   
   if (MmIsPagePresent(NULL, Address))
     {
	KeReleaseSpinLock(&MiPageFaultLock, oldIrql);
	return(STATUS_SUCCESS);
     }
   
   Offset.QuadPart = (Address - MemoryArea->BaseAddress) + 
     MemoryArea->Data.SectionData.ViewOffset;
   
   DPRINT("MemoryArea->Data.SectionData.Section->FileObject %x\n",
	    MemoryArea->Data.SectionData.Section->FileObject);
   DPRINT("MemoryArea->Data.SectionData.ViewOffset %x\n",
	  MemoryArea->Data.SectionData.ViewOffset);
   DPRINT("Offset.QuadPart %x\n", (ULONG)Offset.QuadPart);
   DPRINT("MemoryArea->BaseAddress %x\n", MemoryArea->BaseAddress);
   
   if (MemoryArea->Data.SectionData.Section->FileObject == NULL)
     {
	ULONG Page;
	
	Page = (ULONG)MiTryToSharePageInSection(
			       MemoryArea->Data.SectionData.Section,
						(ULONG)Offset.QuadPart);
	
	if (Page == 0)
	  {
	     Page = (ULONG)MmAllocPage();
	  }
	
	MmSetPage(PsGetCurrentProcess(),
		  Address,
		  MemoryArea->Attributes,
		  Page);
	return(STATUS_SUCCESS);
     }
   
   Mdl = MmCreateMdl(NULL, NULL, PAGESIZE);
   MmBuildMdlFromPages(Mdl);
     
   Page = MmGetMdlPageAddress(Mdl, 0);
   
   KeReleaseSpinLock(&MiPageFaultLock, oldIrql);
   
   Status = IoPageRead(MemoryArea->Data.SectionData.Section->FileObject,
		       Mdl,
		       &Offset,
		       &IoStatus,
		       0 /* FIXME: UNKNOWN ARG */
		       );
   
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
      
   KeAcquireSpinLock(&MiPageFaultLock, &oldIrql);
   
   if (MmIsPagePresent(NULL, Address))
     {
	KeReleaseSpinLock(&MiPageFaultLock, oldIrql);
	return(STATUS_SUCCESS);
     }
   
   MmSetPage(NULL,
	     Address,
	     MemoryArea->Attributes,
	     (ULONG)Page);
   
   KeReleaseSpinLock(&MiPageFaultLock, oldIrql);
   
   DPRINT("Returning from MmSectionHandleFault()\n");
   
   return(STATUS_SUCCESS);
}

ULONG MmPageFault(ULONG cs, ULONG eip, ULONG error_code)
/*
 * FUNCTION: Handle a page fault
 */
{
   KPROCESSOR_MODE FaultMode;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   unsigned int cr2;
//   unsigned int cr3;
   
   /*
    * Get the address for the page fault
    */
   __asm__("movl %%cr2,%0\n\t" : "=d" (cr2));
//   __asm__("movl %%cr3,%0\n\t" : "=d" (cr3));
//   DPRINT1("Page fault address %x eip %x process %x code %x cr3 %x\n",cr2,eip,
//	  PsGetCurrentProcess(), error_code, cr3);

   MmSetPageProtect(PsGetCurrentProcess(),
		    (PVOID)PAGE_ROUND_DOWN(PsGetCurrentProcess()),
		    0x7);
   
   cr2 = PAGE_ROUND_DOWN(cr2);
   
   if (error_code & 0x1)
     {
	DbgPrint("Page protection fault at %x with eip %x\n", cr2, eip);
	return(0);
     }
   
//   DbgPrint("(%%");
   
   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
     {
	DbgPrint("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
	return(0);
//	KeBugCheck(0);
     }
   if (PsGetCurrentProcess() == NULL)
     {
	DbgPrint("No current process\n");
	return(0);
     }
   
   /*
    * Find the memory area for the faulting address
    */
   if (cr2 >= KERNEL_BASE)
     {
	/*
	 * Check permissions
	 */
	if (cs != KERNEL_CS)
	  {
	     DbgPrint("%s:%d\n",__FILE__,__LINE__);
	     return(0);
	  }
	FaultMode = UserMode;
     }
   else
     {
	FaultMode = KernelMode;
     }
   
   MemoryArea = MmOpenMemoryAreaByAddress(PsGetCurrentProcess(),(PVOID)cr2);
   if (MemoryArea == NULL)
     {
	DbgPrint("%s:%d\n",__FILE__,__LINE__);
	return(0);
     }
   
   switch (MemoryArea->Type)
     {
      case MEMORY_AREA_SYSTEM:
	Status = STATUS_UNSUCCESSFUL;
	break;
	
      case MEMORY_AREA_SECTION_VIEW_COMMIT:
	Status = MmSectionHandleFault(MemoryArea, (PVOID)cr2);
	break;
	
      case MEMORY_AREA_COMMIT:
	Status = MmCommitedSectionHandleFault(MemoryArea,(PVOID)cr2);
	break;
	
      default:
	Status = STATUS_UNSUCCESSFUL;
	break;
     }
   DPRINT("Completed page fault handling\n");
   return(NT_SUCCESS(Status));
}

BOOLEAN MmIsThisAnNtAsSystem(VOID)
{
   return(IsThisAnNtAsSystem);
}

MM_SYSTEM_SIZE MmQuerySystemSize(VOID)
{
   return(MmSystemSize);
}

void MmInitialize(boot_param* bp, ULONG LastKernelAddress)
/*
 * FUNCTION: Initalize memory managment
 */
{
   unsigned int first_krnl_phys_addr;
   unsigned int last_krnl_phys_addr;
   int i;
   unsigned int kernel_len;
   
   DPRINT("MmInitialize(bp %x, LastKernelAddress %x)\n", bp, 
	  LastKernelAddress);

   /*
    * Unmap low memory
    */
   MmDeletePageTable(NULL, 0);
   
   /*
    * Free all pages not used for kernel memory
    * (we assume the kernel occupies a continuous range of physical
    * memory)
    */
   first_krnl_phys_addr = bp->start_mem;
   last_krnl_phys_addr = bp->end_mem;
   DPRINT("first krnl %x\nlast krnl %x\n",first_krnl_phys_addr,
	  last_krnl_phys_addr);
   
   /*
    * Free physical memory not used by the kernel
    */
   LastKernelAddress = (ULONG)MmInitializePageList(
						   (PVOID)first_krnl_phys_addr,
						   (PVOID)last_krnl_phys_addr,
						   1024,
					      PAGE_ROUND_UP(LastKernelAddress));
   kernel_len = last_krnl_phys_addr - first_krnl_phys_addr;
   
   /*
    * Create a trap for null pointer references and protect text
    * segment
    */
   CHECKPOINT;
   DPRINT("stext %x etext %x\n",(int)&stext,(int)&etext);
   for (i=PAGE_ROUND_UP(((int)&stext));
	i<PAGE_ROUND_DOWN(((int)&etext));i=i+PAGESIZE)
     {
	MmSetPageProtect(NULL,
			 (PVOID)i,
			 PAGE_EXECUTE_READ);
     }
   
   DPRINT("Invalidating between %x and %x\n",
	  LastKernelAddress,
	  KERNEL_BASE + PAGE_TABLE_SIZE);
   for (i=(LastKernelAddress); 
	i<(KERNEL_BASE + PAGE_TABLE_SIZE); 
	i=i+PAGESIZE)
     {
	MmSetPage(NULL, (PVOID)(i), PAGE_NOACCESS, 0);
     }
   DPRINT("Almost done MmInit()\n");
   
   /*
    * Intialize memory areas
    */
   MmInitVirtualMemory(bp);
}

VOID
MmInitSystem (ULONG Phase, boot_param* bp, ULONG LastKernelAddress)
{
	if (Phase == 0)
	{
		/* Phase 0 Initialization */
		MmInitialize (bp, LastKernelAddress);
	}
	else
	{
		/* Phase 1 Initialization */
		MmInitSectionImplementation();
		MmInitPagingFile();
	}
}

/* EOF */
