/*
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel
 * FILE:        ntoskrnl/mm/virtual.c
 * PURPOSE:     implementing the Virtualxxx section of the win32 api
 * PROGRAMMER:  David Welch
 * UPDATE HISTORY:
 *              09/4/98: Created
 *              10/6/98: Corrections from Fatahi (i_fatahi@hotmail.com)
 *              30/9/98: Implemented ZwxxxVirtualMemory functions
 */
 
/* INCLUDE *****************************************************************/

#include <internal/i386/segment.h>
#include <internal/mm.h>
#include <internal/mmhal.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

extern unsigned int etext;
extern unsigned int end;

static MEMORY_AREA* kernel_text_desc = NULL;
static MEMORY_AREA* kernel_data_desc = NULL;
static MEMORY_AREA* kernel_param_desc = NULL;
static MEMORY_AREA* kernel_pool_desc = NULL;

/* FUNCTIONS ****************************************************************/

void VirtualInit(boot_param* bp)
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
   
   DPRINT("VirtualInit() %x\n",bp);
   
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
   
   Length = PAGE_ROUND_UP(((ULONG)&end)) - PAGE_ROUND_UP(((ULONG)&etext));
   ParamLength = ParamLength - Length;
   DPRINT("Length %x\n",Length);
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&etext));
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
   CHECKPOINT;
   
   MmInitSectionImplementation();
}

ULONG MmCommitedSectionHandleFault(MEMORY_AREA* MemoryArea, PVOID Address)
{
   MmSetPage(PsGetCurrentProcess(),
	     Address,
	     MemoryArea->Attributes,
	     get_free_page());
   return(TRUE);
}

NTSTATUS MmSectionHandleFault(MEMORY_AREA* MemoryArea, PVOID Address)
{
   LARGE_INTEGER Offset;
   IO_STATUS_BLOCK IoStatus;
   
   DPRINT("MmSectionHandleFault(MemoryArea %x, Address %x)\n",
	    MemoryArea,Address);
   
   MmSetPage(NULL,
	     Address,
	     MemoryArea->Attributes,
	     get_free_page());
   
   LARGE_INTEGER_QUAD_PART(Offset) = (Address - MemoryArea->BaseAddress) + 
     MemoryArea->Data.SectionData.ViewOffset;
   
   DPRINT("MemoryArea->Data.SectionData.Section->FileObject %x\n",
	    MemoryArea->Data.SectionData.Section->FileObject);
   
   if (MemoryArea->Data.SectionData.Section->FileObject == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   IoPageRead(MemoryArea->Data.SectionData.Section->FileObject,
	      (PVOID)Address,
	      &Offset,
	      &IoStatus);
   
   DPRINT("Returning from MmSectionHandleFault()\n");
   
   return(STATUS_SUCCESS);
}

asmlinkage int page_fault_handler(unsigned int cs,
                                  unsigned int eip)
/*
 * FUNCTION: Handle a page fault
 */
{
   KPROCESSOR_MODE FaultMode;
   MEMORY_AREA* MemoryArea;
   KIRQL oldlvl;
   ULONG stat;
   
   /*
    * Get the address for the page fault
    */
   unsigned int cr2;
   __asm__("movl %%cr2,%0\n\t" : "=d" (cr2));                
   DPRINT("Page fault at address %x with eip %x\n",cr2,eip);

   cr2 = PAGE_ROUND_DOWN(cr2);
   
   if (KeGetCurrentIrql()!=PASSIVE_LEVEL)
     {
	DbgPrint("Page fault at high IRQL\n");
	return(0);
//	KeBugCheck(0);
     }
   
   KeRaiseIrql(DISPATCH_LEVEL,&oldlvl);
   
   /*
    * Find the memory area for the faulting address
    */
   if (cr2>=KERNEL_BASE)
     {
	/*
	 * Check permissions
	 */
	if (cs!=KERNEL_CS)
	  {
	     printk("%s:%d\n",__FILE__,__LINE__);
	     return(0);
	  }
	FaultMode = UserMode;
     }
   else
     {
	FaultMode = KernelMode;
     }
   
   MemoryArea = MmOpenMemoryAreaByAddress(PsGetCurrentProcess(),(PVOID)cr2);
   if (MemoryArea==NULL)
     {
	printk("%s:%d\n",__FILE__,__LINE__);
	return(0);
     }
   
   switch (MemoryArea->Type)
     {
      case MEMORY_AREA_SYSTEM:
	stat = 0;
	break;
	
      case MEMORY_AREA_SECTION_VIEW_COMMIT:
        if (MmSectionHandleFault(MemoryArea, (PVOID)cr2)==STATUS_SUCCESS)
	  {
	     stat=1;
	  }
	else
	  {
	     stat = 0;
	  }
	break;
	
      case MEMORY_AREA_COMMIT:
	stat = MmCommitedSectionHandleFault(MemoryArea,(PVOID)cr2);
	break;
	
      default:
	stat = 0;
	break;
     }
   if (stat)
     {
	KeLowerIrql(oldlvl);
     }
   return(stat);
}

BOOLEAN MmIsNonPagedSystemAddressValid(PVOID VirtualAddress)
{
   UNIMPLEMENTED;
}

BOOLEAN MmIsAddressValid(PVOID VirtualAddress)
/*
 * FUNCTION: Checks whether the given address is valid for a read or write
 * ARGUMENTS:
 *          VirtualAddress = address to check
 * RETURNS: True if the access would be valid
 *          False if the access would cause a page fault
 * NOTES: This function checks whether a byte access to the page would
 *        succeed. Is this realistic for RISC processors which don't
 *        allow byte granular access?
 */
{
   MEMORY_AREA* MemoryArea;
   
   MemoryArea = MmOpenMemoryAreaByAddress(PsGetCurrentProcess(),
					  VirtualAddress);

   if (MemoryArea == NULL)
     {
	return(FALSE);
     }
   return(TRUE);
}

NTSTATUS
STDCALL
NtAllocateVirtualMemory( 
	IN HANDLE ProcessHandle,
	IN OUT PVOID *BaseAddress,
	IN ULONG  ZeroBits,
	IN OUT PULONG  RegionSize,
	IN ULONG  AllocationType, 
	IN ULONG  Protect
	)
{
   return(ZwAllocateVirtualMemory(ProcessHandle,
				  BaseAddress,
				  ZeroBits,
				  RegionSize,
				  AllocationType,
				  Protect));
}

NTSTATUS
STDCALL
ZwAllocateVirtualMemory( 
	IN HANDLE ProcessHandle,
	IN OUT PVOID *BaseAddress,
	IN ULONG  ZeroBits,
	IN OUT PULONG  RegionSize,
	IN ULONG  AllocationType, 
	IN ULONG  Protect
	)
/*
 * FUNCTION: Allocates a block of virtual memory in the process address space
 * ARGUMENTS:
 *      ProcessHandle = The handle of the process which owns the virtual memory
 *      BaseAddress   = A pointer to the virtual memory allocated. If you 
 *                      supply a non zero value the system will try to 
 *                      allocate the memory at the address supplied. It round 
 *                      it down to a multiple  of the page size.
 *      ZeroBits  = (OPTIONAL) You can specify the number of high order bits 
 *                      that must be zero, ensuring that the memory will be 
 *                      allocated at a address below a certain value.
 *      RegionSize = The number of bytes to allocate
 *      AllocationType = Indicates the type of virtual memory you like to 
 *                       allocated, can be one of the values : MEM_COMMIT, 
 *                       MEM_RESERVE, MEM_RESET, MEM_TOP_DOWN
 *      Protect = Indicates the protection type of the pages allocated, can be
 *                a combination of PAGE_READONLY, PAGE_READWRITE, 
 *                PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE, PAGE_GUARD, 
 *                PAGE_NOACCESS
 * REMARKS:
 *       This function maps to the win32 VirtualAllocEx. Virtual memory is 
 *       process based so the  protocol starts with a ProcessHandle. I 
 *       splitted the functionality of obtaining the actual address and 
 *       specifying the start address in two parameters ( BaseAddress and 
 *       StartAddress ) The NumberOfBytesAllocated specify the range and the 
 *       AllocationType and ProctectionType map to the other two parameters.
 * RETURNS: Status
 */
{
   PEPROCESS Process;
   MEMORY_AREA* MemoryArea;
   ULONG Type;
   NTSTATUS Status;
   
   DPRINT("ZwAllocateVirtualMemory(ProcessHandle %x, *BaseAddress %x, "
	    "ZeroBits %d, *RegionSize %x, AllocationType %x, Protect %x)\n",
	    ProcessHandle,*BaseAddress,ZeroBits,*RegionSize,AllocationType,
	    Protect);
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      NULL,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	DPRINT("ZwAllocateVirtualMemory() = %x\n",Status);
	return(Status);
     }
   
   if (AllocationType & MEM_RESERVE)
     {
	Type = MEMORY_AREA_RESERVE;
     }
   else
     {
	Type = MEMORY_AREA_COMMIT;
     }
   
   if ((*BaseAddress) != 0)
     {
	MemoryArea = MmOpenMemoryAreaByAddress(Process, *BaseAddress);
	
	if (MemoryArea != NULL)
	  {
	     if (MemoryArea->BaseAddress == (*BaseAddress) &&
		 MemoryArea->Length == *RegionSize)
	       {
		  MemoryArea->Type = Type;
		  MemoryArea->Attributes =Protect;
		  DPRINT("*BaseAddress %x\n",*BaseAddress);
		  return(STATUS_SUCCESS);
	       }
	     
	     MemoryArea = MmSplitMemoryArea(Process,
					    MemoryArea,
					    *BaseAddress,
					    *RegionSize,
					    Type,
					    Protect);
	     DPRINT("*BaseAddress %x\n",*BaseAddress);
	     return(STATUS_SUCCESS);
	  }
     }
   
 //FIXME RegionSize should be passed as pointer


   Status = MmCreateMemoryArea(UserMode,
			       Process,
			       Type,
			       BaseAddress,
			       *RegionSize,
			       Protect,
			       &MemoryArea);
   
   if (Status != STATUS_SUCCESS)
     {
	DPRINT("ZwAllocateVirtualMemory() = %x\n",Status);
	return(Status);
     }
   
   DPRINT("*BaseAddress %x\n",*BaseAddress);
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtFlushVirtualMemory(IN HANDLE ProcessHandle,
				      IN PVOID BaseAddress,
				      IN ULONG NumberOfBytesToFlush,
				      OUT PULONG NumberOfBytesFlushed OPTIONAL)
{
   return(ZwFlushVirtualMemory(ProcessHandle,
			       BaseAddress,
			       NumberOfBytesToFlush,
			       NumberOfBytesFlushed));
}

NTSTATUS STDCALL ZwFlushVirtualMemory(IN HANDLE ProcessHandle,
				      IN PVOID BaseAddress,
				      IN ULONG NumberOfBytesToFlush,
				      OUT PULONG NumberOfBytesFlushed OPTIONAL)

/*
 * FUNCTION: Flushes virtual memory to file
 * ARGUMENTS:
 *        ProcessHandle = Points to the process that allocated the virtual 
 *                        memory
 *        BaseAddress = Points to the memory address
 *        NumberOfBytesToFlush = Limits the range to flush,
 *        NumberOfBytesFlushed = Actual number of bytes flushed
 * RETURNS: Status 
 */
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtFreeVirtualMemory(IN HANDLE ProcessHandle,
				     IN PVOID  *BaseAddress,	
				     IN PULONG  RegionSize,	
				     IN ULONG  FreeType)
{
   return(ZwFreeVirtualMemory(ProcessHandle,
			      BaseAddress,
			      RegionSize,
			      FreeType));
}

NTSTATUS STDCALL ZwFreeVirtualMemory(IN HANDLE ProcessHandle,
				     IN PVOID  *BaseAddress,	
 				     IN PULONG  RegionSize,	
				     IN ULONG  FreeType)

/*
 * FUNCTION: Frees a range of virtual memory
 * ARGUMENTS:
 *        ProcessHandle = Points to the process that allocated the virtual 
 *                        memory
 *        BaseAddress = Points to the memory address, rounded down to a 
 *                      multiple of the pagesize
 *        RegionSize = Limits the range to free, rounded up to a multiple of 
 *                     the paging size
 *        FreeType = Can be one of the values:  MEM_DECOMMIT, or MEM_RELEASE
 * RETURNS: Status 
 */
{
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   PEPROCESS Process;
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }

   MemoryArea = MmOpenMemoryAreaByAddress(Process,*BaseAddress);
   if (MemoryArea == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   switch (FreeType)
     {
      case MEM_RELEASE:
	if (MemoryArea->BaseAddress != (*BaseAddress))
	  {
	     return(STATUS_UNSUCCESSFUL);
	  }
	MmFreeMemoryArea(PsGetCurrentProcess(),
			 BaseAddress,
			 0,
			 TRUE);
	return(STATUS_SUCCESS);
	
      case MEM_DECOMMIT:	
	MmSplitMemoryArea(PsGetCurrentProcess(),
			  MemoryArea,
			  *BaseAddress,
			  *RegionSize,
			  MEMORY_AREA_RESERVE,
			  MemoryArea->Attributes);
	return(STATUS_SUCCESS);
     }
   
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS STDCALL NtLockVirtualMemory(HANDLE ProcessHandle,
				     PVOID BaseAddress,
				     ULONG NumberOfBytesToLock,
				     PULONG NumberOfBytesLocked)
{
   return(ZwLockVirtualMemory(ProcessHandle,
			      BaseAddress,
			      NumberOfBytesToLock,
			      NumberOfBytesLocked));
}

NTSTATUS STDCALL ZwLockVirtualMemory(HANDLE ProcessHandle,
				     PVOID BaseAddress,
				     ULONG NumberOfBytesToLock,
				     PULONG NumberOfBytesLocked)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtProtectVirtualMemory(IN HANDLE ProcessHandle,
					IN PVOID BaseAddress,
					IN ULONG NumberOfBytesToProtect,
					IN ULONG NewAccessProtection,
					OUT PULONG OldAccessProtection)
{
   return(ZwProtectVirtualMemory(ProcessHandle,
				 BaseAddress,
				 NumberOfBytesToProtect,
				 NewAccessProtection,
				 OldAccessProtection));
}

VOID MmChangeAreaProtection(PEPROCESS Process, 
			    PVOID BaseAddress,
			    ULONG Length, 
			    ULONG Protect)
{
   ULONG i;
   
   for (i=0; i<(Length/PAGESIZE); i++)
     {
	if (MmIsPagePresent(Process, BaseAddress + (i*PAGESIZE)))
	  {
	     MmSetPageProtect(Process, BaseAddress + (i*PAGESIZE), Protect);
	  }
     }
}

NTSTATUS STDCALL ZwProtectVirtualMemory(IN HANDLE ProcessHandle,
					IN PVOID BaseAddress,
					IN ULONG NumberOfBytesToProtect,
					IN ULONG NewAccessProtection,
					OUT PULONG OldAccessProtection)
{
   PMEMORY_AREA MemoryArea;
   PEPROCESS Process;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	DbgPrint("ZwProtectVirtualMemory() = %x\n",Status);
	return(Status);
     }

   MemoryArea = MmOpenMemoryAreaByAddress(Process,BaseAddress);
   if (MemoryArea == NULL)
     {
	DbgPrint("ZwProtectVirtualMemory() = %x\n",STATUS_UNSUCCESSFUL);
	return(STATUS_UNSUCCESSFUL);
     }

   *OldAccessProtection = MemoryArea->Attributes;

   if (MemoryArea->BaseAddress == BaseAddress &&
       MemoryArea->Length == NumberOfBytesToProtect)
     {
	MemoryArea->Attributes = NewAccessProtection;	
     }
   else
     {
	MemoryArea = MmSplitMemoryArea(Process,
				       MemoryArea,
				       BaseAddress,
				       NumberOfBytesToProtect,
				       MemoryArea->Type,
				       NewAccessProtection);
     }
   MmChangeAreaProtection(Process,BaseAddress,NumberOfBytesToProtect,
			  NewAccessProtection);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtQueryVirtualMemory(IN HANDLE ProcessHandle,
				      IN PVOID Address,
				      IN IN CINT VirtualMemoryInformationClass,
				      OUT PVOID VirtualMemoryInformation,
				      IN ULONG Length,
				      OUT PULONG ResultLength)
{
   return(ZwQueryVirtualMemory(ProcessHandle,
			       Address,
			       VirtualMemoryInformationClass,
			       VirtualMemoryInformation,
			       Length,
			       ResultLength));
}

NTSTATUS STDCALL ZwQueryVirtualMemory(IN HANDLE ProcessHandle,
				      IN PVOID Address,
				      IN CINT VirtualMemoryInformationClass,
				      OUT PVOID VirtualMemoryInformation,
				      IN ULONG Length,
				      OUT PULONG ResultLength)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtReadVirtualMemory(IN HANDLE ProcessHandle,
				     IN PVOID BaseAddress,
				     OUT PVOID Buffer,
				     IN ULONG  NumberOfBytesToRead,
				     OUT PULONG NumberOfBytesRead)
{
   return(ZwReadVirtualMemory(ProcessHandle,
			      BaseAddress,
			      Buffer,
			      NumberOfBytesToRead,
			      NumberOfBytesRead));
}

NTSTATUS STDCALL ZwReadVirtualMemory(IN HANDLE ProcessHandle,
				     IN PVOID BaseAddress,
				     OUT PVOID Buffer,
				     IN ULONG  NumberOfBytesToRead,
				     OUT PULONG NumberOfBytesRead)
{
   PEPROCESS Process;
   MEMORY_AREA* MemoryArea;
   ULONG i;
   NTSTATUS Status;
   PULONG CurrentEntry;
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_READ,
				      NULL,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }

   MemoryArea = MmOpenMemoryAreaByAddress(Process,BaseAddress);
   
   if (MemoryArea == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (MemoryArea->Length > NumberOfBytesToRead)
     {
	NumberOfBytesToRead = MemoryArea->Length;
     }
   
   *NumberOfBytesRead = NumberOfBytesToRead;
   
   for (i=0; i<(NumberOfBytesToRead/PAGESIZE); i++)
     {
	CurrentEntry = MmGetPageEntry(Process, 
				   (PVOID)((DWORD)BaseAddress + (i*PAGESIZE)));
	RtlCopyMemory(Buffer + (i*PAGESIZE),
		      (PVOID)physical_to_linear(PAGE_MASK(*CurrentEntry)),
		      PAGESIZE);
	
     }
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtUnlockVirtualMemory(HANDLE ProcessHandle,
				       PVOID BaseAddress,
				       ULONG  NumberOfBytesToUnlock,
				       PULONG NumberOfBytesUnlocked OPTIONAL)
{
   return(ZwUnlockVirtualMemory(ProcessHandle,
				BaseAddress,
				NumberOfBytesToUnlock,
				NumberOfBytesUnlocked));
}

NTSTATUS STDCALL ZwUnlockVirtualMemory(HANDLE ProcessHandle,
				       PVOID BaseAddress,
				       ULONG  NumberOfBytesToUnlock,
				       PULONG NumberOfBytesUnlocked OPTIONAL)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtWriteVirtualMemory(IN HANDLE ProcessHandle,
				      IN PVOID  BaseAddress,
				      IN PVOID Buffer,
				      IN ULONG NumberOfBytesToWrite,
				      OUT PULONG NumberOfBytesWritten)
{
   return(ZwWriteVirtualMemory(ProcessHandle,
			       BaseAddress,
			       Buffer,
			       NumberOfBytesToWrite,
			       NumberOfBytesWritten));
}

NTSTATUS STDCALL ZwWriteVirtualMemory(IN HANDLE ProcessHandle,
				      IN PVOID BaseAddress,
				      IN PVOID Buffer,
				      IN ULONG NumberOfBytesToWrite,
				      OUT PULONG NumberOfBytesWritten)
{
   PEPROCESS Process;
   PMEMORY_AREA OutMemoryArea;
   ULONG i;
   NTSTATUS Status;
   PULONG CurrentEntry;
   
   DPRINT("ZwWriteVirtualMemory(ProcessHandle %x, BaseAddress %x, "
	    "Buffer %x, NumberOfBytesToWrite %d)\n",ProcessHandle,BaseAddress,
	    Buffer,NumberOfBytesToWrite);
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_WRITE,
				      NULL,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }

   OutMemoryArea = MmOpenMemoryAreaByAddress(Process,BaseAddress);   
   if (OutMemoryArea == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
  
   *NumberOfBytesWritten = NumberOfBytesToWrite;
   
   DPRINT("*Buffer %x\n",((PULONG)Buffer)[0]);
   
   for (i=0; i<(PAGE_ROUND_DOWN(NumberOfBytesToWrite)/PAGESIZE); i++)     
     {
	if (!MmIsPagePresent(Process, BaseAddress + (i*PAGESIZE)))
	  {
	     DPRINT("OutMemoryArea->Attributes %x\n",
		      OutMemoryArea->Attributes);
	     MmSetPage(Process,
		       BaseAddress + (i*PAGESIZE),
		       OutMemoryArea->Attributes,
		       get_free_page());
	  }
	CurrentEntry = MmGetPageEntry(Process, 
				      (PVOID)((DWORD)BaseAddress + 
				      (i*PAGESIZE)));
	RtlCopyMemory((PVOID)physical_to_linear(PAGE_MASK(*CurrentEntry)) +
		      (((DWORD)BaseAddress)%PAGESIZE),
		      Buffer + (i*PAGESIZE),
		      PAGESIZE);
     }
   if ((NumberOfBytesToWrite % PAGESIZE) != 0)
     {
	if (!MmIsPagePresent(Process, BaseAddress + (i*PAGESIZE)))
	  {
	     MmSetPage(Process,
		       BaseAddress + (i*PAGESIZE),
		       OutMemoryArea->Attributes,
		       get_free_page());
	  }
	CurrentEntry = MmGetPageEntry(Process, 
				      BaseAddress + (i*PAGESIZE));
	DPRINT("addr %x\n",
		 physical_to_linear(PAGE_MASK(*CurrentEntry)) +
		 (((DWORD)BaseAddress)%PAGESIZE));
	RtlCopyMemory((PVOID)physical_to_linear(PAGE_MASK(*CurrentEntry)) +
		      (((DWORD)BaseAddress)%PAGESIZE),
		      Buffer + (i*PAGESIZE),
		      NumberOfBytesToWrite % PAGESIZE);
     }
   return(STATUS_SUCCESS);
}


