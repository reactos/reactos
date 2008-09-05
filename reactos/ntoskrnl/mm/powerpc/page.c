/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/i386/page.c
 * PURPOSE:         Low level memory managment manipulation
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 * Revised for PowerPC by arty
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#include <ppcmmu/mmu.h>
//#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitGlobalKernelPageDirectory)
#pragma alloc_text(INIT, MiInitPageDirectoryMap)
#endif

/* GLOBALS *****************************************************************/

#define HYPERSPACE_PAGEDIR_PTR  ((PVOID)0xc0000000)

#define PA_PRESENT (1ll<<63)
#define PA_USER (1ll<<62)
#define PA_ACCESSED 0x200
#define PA_DIRTY 0x100
#define PA_WT 0x20
#define PA_CD 0x10
#define PA_READWRITE 3

#define HYPERSPACE		(0xc0400000)
#define IS_HYPERSPACE(v)	(((ULONG)(v) >= HYPERSPACE && (ULONG)(v) < HYPERSPACE + 0x400000))

#define PTE_TO_PFN(X)  ((X) >> PAGE_SHIFT)
#define PFN_TO_PTE(X)  ((X) << PAGE_SHIFT)

#if defined(__GNUC__)
#define PTE_TO_PAGE(X) ((LARGE_INTEGER)(LONGLONG)(PAGE_MASK(X)))
#else
__inline LARGE_INTEGER PTE_TO_PAGE(ULONG npage)
{
   LARGE_INTEGER dummy;
   dummy.QuadPart = (LONGLONG)(PAGE_MASK(npage));
   return dummy;
}
#endif

/* FUNCTIONS ***************************************************************/

VOID
STDCALL
MiFlushTlbIpiRoutine(PVOID Address)
{
   if (Address == (PVOID)0xffffffff)
   {
      __asm__("tlbsync");
   }
   else if (Address == (PVOID)0xfffffffe)
   {
      __asm__("tlbsync");
   }
   else
   {
      __asm__("tlbi %0" : "=r" (Address));
   }
}

VOID
MiFlushTlb(PULONG Pt, PVOID Address)
{
    __asm__("tlbi %0" : "=r" (Address));
}



PULONG
MmGetPageDirectory(VOID)
{
   unsigned int page_dir=0;
   return((PULONG)page_dir);
}

static ULONG
ProtectToFlags(ULONG flProtect)
{
    return MMU_ALL_RW; // XXX hack
}

NTSTATUS
STDCALL
MmCopyMmInfo(PEPROCESS Src,
             PEPROCESS Dest,
             PPHYSICAL_ADDRESS DirectoryTableBase)
{
    DPRINT("MmCopyMmInfo(Src %x, Dest %x)\n", Src, Dest);

    ASSERT(FALSE);

    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmInitializeHandBuiltProcess(IN PEPROCESS Process,
                             IN PLARGE_INTEGER DirectoryTableBase)
{
    /* Share the directory base with the idle process */
    *DirectoryTableBase = PsGetCurrentProcess()->Pcb.DirectoryTableBase;

    /* Initialize the Addresss Space */
    MmInitializeAddressSpace(Process, (PMADDRESS_SPACE)&Process->VadRoot);

    /* The process now has an address space */
    Process->HasAddressSpace = TRUE;
    return STATUS_SUCCESS;
}

BOOLEAN
STDCALL
MmCreateProcessAddressSpace(IN ULONG MinWs,
                            IN PEPROCESS Process,
                            IN PLARGE_INTEGER DirectoryTableBase)
{
    ASSERT(FALSE);
    return TRUE;
}

VOID
NTAPI
MmDeletePageTable(PEPROCESS Process, PVOID Address)
{
    PEPROCESS CurrentProcess = PsGetCurrentProcess();

    DPRINT1("DeletePageTable: Process: %x CurrentProcess %x\n", 
            Process, CurrentProcess);

    if (Process != NULL && Process != CurrentProcess)
    {
        KeAttachProcess(&Process->Pcb);
    }
    
    if (Process)
    {
        DPRINT1("Revoking VSID %d\n", (paddr_t)Process->UniqueProcessId);
        MmuRevokeVsid((paddr_t)Process->UniqueProcessId, -1);
    }
    else
    {
        DPRINT1("No vsid to revoke\n");
    }
    
    if (Process != NULL && Process != CurrentProcess)
    {
        KeDetachProcess();
    }    
}

VOID
NTAPI
MmFreePageTable(PEPROCESS Process, PVOID Address)
{
    MmDeletePageTable(Process, Address);
}

PVOID
NTAPI
MmGetPhysicalAddressProcess(PEPROCESS Process, PVOID Addr)
{
    ppc_map_info_t info = { 0 };
    info.proc = Process ? (int)Process->UniqueProcessId : 0;
    info.addr = (vaddr_t)Addr;
    MmuInqPage(&info, 1);
    return (PVOID)info.phys;
}

/*
 * @implemented
 */
PHYSICAL_ADDRESS STDCALL
MmGetPhysicalAddress(PVOID vaddr)
/*
 * FUNCTION: Returns the physical address corresponding to a virtual address
 */
{
    PHYSICAL_ADDRESS Addr;
    Addr.QuadPart = (ULONG)MmGetPhysicalAddressProcess(PsGetCurrentProcess()->UniqueProcessId, vaddr);
    return Addr;
}

PFN_TYPE
NTAPI
MmGetPfnForProcess(PEPROCESS Process,
                   PVOID Address)
{
    return((PFN_TYPE)MmGetPhysicalAddressProcess(Process, Address) >> PAGE_SHIFT);
}

VOID
NTAPI
MmDisableVirtualMapping(PEPROCESS Process, PVOID Address, BOOLEAN* WasDirty, PPFN_TYPE Page)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
    ppc_map_info_t info = { 0 };
    info.proc = Process ? (int)Process->UniqueProcessId : 0;
    info.addr = (vaddr_t)Address;
    MmuUnmapPage(&info, 1);
}

VOID
NTAPI
MmRawDeleteVirtualMapping(PVOID Address)
{
}

VOID
NTAPI
MmDeleteVirtualMapping(PEPROCESS Process, PVOID Address, BOOLEAN FreePage,
                       BOOLEAN* WasDirty, PPFN_TYPE Page)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
    ppc_map_info_t info = { 0 };

    DPRINT("MmDeleteVirtualMapping(%x, %x, %d, %x, %x)\n",
	   Process, Address, FreePage, WasDirty, Page);

    info.proc = Process ? (int)Process->UniqueProcessId : 0;
    info.addr = (vaddr_t)Address;
    MmuInqPage(&info, 1);

    if (FreePage && info.phys)
    {
	MmReleasePageMemoryConsumer(MC_NPPOOL, info.phys >> PAGE_SHIFT);
    }

    /*
     * Return some information to the caller
     */
    if (WasDirty != NULL)
    {
	*WasDirty = !!(info.flags & MMU_PAGE_DIRTY);
    }
    if (Page != NULL)
    {
	*Page = info.phys >> PAGE_SHIFT;
    }
}

VOID
NTAPI
MmDeletePageFileMapping(PEPROCESS Process, PVOID Address,
                        SWAPENTRY* SwapEntry)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
    ppc_map_info_t info = { 0 };
    /*
     * Decrement the reference count for this page table.
     */
    if (Process != NULL &&
	((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable != NULL &&
	Address < MmSystemRangeStart)
    {
	PUSHORT Ptrc;

	Ptrc = ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable;
	MmFreePageTable(Process, Address);
    }

    /*
     * Return some information to the caller
     */
    MmuInqPage(&info, 1);
    *SwapEntry = info.phys;
}

BOOLEAN
NTAPI
MmIsDirtyPage(PEPROCESS Process, PVOID Address)
{
    ppc_map_info_t info = { 0 };
    info.proc = Process ? (int)Process->UniqueProcessId : 0;
    info.addr = (vaddr_t)Address;
    MmuInqPage(&info, 1);
    return !!(info.flags & MMU_PAGE_DIRTY);
}

BOOLEAN
NTAPI
MmIsAccessedAndResetAccessPage(PEPROCESS Process, PVOID Address)
{
    ppc_map_info_t info = { 0 };

    if (Address < MmSystemRangeStart && Process == NULL)
    {
	DPRINT1("MmIsAccessedAndResetAccessPage is called for user space without a process.\n");
	ASSERT(FALSE);
    }

    info.proc = Process ? (int)Process->UniqueProcessId : 0;
    info.addr = (vaddr_t)Address;
    MmuInqPage(&info, 1);
    return !!(info.flags /*& MMU_PAGE_ACCESS*/);
}

VOID
NTAPI
MmSetCleanPage(PEPROCESS Process, PVOID Address)
{
}

VOID
NTAPI
MmSetDirtyPage(PEPROCESS Process, PVOID Address)
{
}

VOID
NTAPI
MmEnableVirtualMapping(PEPROCESS Process, PVOID Address)
{
}

BOOLEAN
NTAPI
MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
    ppc_map_info_t info = { 0 };
    info.proc = Process ? (int)Process->UniqueProcessId : 0;
    info.addr = (vaddr_t)Address;
    MmuInqPage(&info, 1);
    return !!info.phys;
}

ULONGLONG MmGetPageEntryForProcess(PEPROCESS Process, PVOID Address)
{
    return 0; // XXX arty
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(PEPROCESS Process, PVOID Address)
{
    ULONG Entry;
    Entry = MmGetPageEntryForProcess(Process, Address);
    return !(Entry & PA_PRESENT) && Entry != 0 ? TRUE : FALSE;
}

NTSTATUS
NTAPI
MmCreateVirtualMappingForKernel(PVOID Address,
                                ULONG flProtect,
                                PPFN_TYPE Pages,
				ULONG PageCount)
{
    ULONG i;
    PVOID Addr;

    DPRINT("MmCreateVirtualMappingForKernel(%x, %x, %x, %d)\n",
           Address, flProtect, Pages, PageCount);

    if (Address < MmSystemRangeStart)
    {
	DPRINT1("MmCreateVirtualMappingForKernel is called for user space\n");
	ASSERT(FALSE);
    }

    Addr = Address;

    for (i = 0; i < PageCount; i++, Addr = (PVOID)((ULONG_PTR)Addr + PAGE_SIZE))
    {
#if 0
	if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
	{
            DPRINT1("Setting physical address but not allowing access at address "
                    "0x%.8X with attributes %x/%x.\n",
                    Addr, Attributes, flProtect);
            ASSERT(FALSE);
	}
	(void)InterlockedExchangeUL(Pt, PFN_TO_PTE(Pages[i]) | Attributes);
#endif
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmCreatePageFileMapping(PEPROCESS Process,
                        PVOID Address,
                        SWAPENTRY SwapEntry)
{
    if (Process == NULL && Address < MmSystemRangeStart)
    {
	DPRINT1("No process\n");
	ASSERT(FALSE);
    }
    if (Process != NULL && Address >= MmSystemRangeStart)
    {
	DPRINT1("Setting kernel address with process context\n");
	ASSERT(FALSE);
    }
    if (SwapEntry & (1 << 31))
    {
	ASSERT(FALSE);
    }

    // XXX arty

    return(STATUS_SUCCESS);
}


NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(PEPROCESS Process,
                             PVOID Address,
                             ULONG flProtect,
                             PPFN_TYPE Pages,
                             ULONG PageCount)
{
    ULONG Attributes;
    PVOID Addr;
    ULONG i;
    ppc_map_info_t info = { 0 };

    DPRINT("MmCreateVirtualMappingUnsafe(%x, %x, %x, %x (%x), %d)\n",
	   Process, Address, flProtect, Pages, *Pages, PageCount);

    if (Process == NULL)
    {
	if (Address < MmSystemRangeStart)
	{
	    DPRINT1("No process\n");
	    ASSERT(FALSE);
	}
	if (PageCount > 0x10000 ||
	    (ULONG_PTR) Address / PAGE_SIZE + PageCount > 0x100000)
	{
	    DPRINT1("Page count to large\n");
	    ASSERT(FALSE);
	}
    }
    else
    {
	if (Address >= MmSystemRangeStart)
	{
	    DPRINT1("Setting kernel address with process context\n");
	    ASSERT(FALSE);
	}
	if (PageCount > (ULONG_PTR)MmSystemRangeStart / PAGE_SIZE ||
	    (ULONG_PTR) Address / PAGE_SIZE + PageCount >
	    (ULONG_PTR)MmSystemRangeStart / PAGE_SIZE)
	{
	    DPRINT1("Page Count to large\n");
	    ASSERT(FALSE);
	}
    }

    Attributes = ProtectToFlags(flProtect);
    Addr = Address;

    for (i = 0; i < PageCount; i++, Addr = (PVOID)((ULONG_PTR)Addr + PAGE_SIZE))
    {
	Process = PsGetCurrentProcess();
	info.proc = ((Addr < MmSystemRangeStart) && Process) ? 
            (int)Process->UniqueProcessId : 0;
	info.addr = (vaddr_t)Addr;
	info.flags = Attributes;
	MmuMapPage(&info, 1);
	//(void)InterlockedExchangeUL(Pt, PFN_TO_PTE(Pages[i]) | Attributes);
	if (Address < MmSystemRangeStart &&
	    ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable != NULL &&
	    Attributes & PA_PRESENT)
	{
#if 0
            PUSHORT Ptrc;

            Ptrc = ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable;

            Ptrc[ADDR_TO_PAGE_TABLE(Addr)]++;
#endif
	}
    }
    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmCreateVirtualMapping(PEPROCESS Process,
                       PVOID Address,
                       ULONG flProtect,
                       PPFN_TYPE Pages,
                       ULONG PageCount)
{
   ULONG i;

   for (i = 0; i < PageCount; i++)
   {
      if (!MmIsUsablePage(Pages[i]))
      {
         DPRINT1("Page at address %x not usable\n", PFN_TO_PTE(Pages[i]));
         ASSERT(FALSE);
      }
   }

   return(MmCreateVirtualMappingUnsafe(Process,
                                       Address,
                                       flProtect,
                                       Pages,
                                       PageCount));
}

ULONG
NTAPI
MmGetPageProtect(PEPROCESS Process, PVOID Address)
{
    ULONG Protect = 0;
    ppc_map_info_t info = { 0 };

    info.proc = Process ? (int)Process->UniqueProcessId : 0;
    info.addr = (vaddr_t)Address;
    MmuInqPage(&info, 1);

    if (!info.phys) { return PAGE_NOACCESS; }
    if (!(info.flags & MMU_KMASK))
    {
	Protect |= PAGE_SYSTEM;
	if ((info.flags & MMU_KR) && (info.flags & MMU_KW))
	    Protect = PAGE_READWRITE;
	else if (info.flags & MMU_KR)
	    Protect = PAGE_EXECUTE_READ;
    }
    else
    {
	if ((info.flags & MMU_UR) && (info.flags & MMU_UW))
	    Protect = PAGE_READWRITE;
	else
	    Protect = PAGE_EXECUTE_READ;
    }
    return(Protect);
}

VOID
NTAPI
MmSetPageProtect(PEPROCESS Process, PVOID Address, ULONG flProtect)
{
   //ULONG Attributes = 0;

   DPRINT("MmSetPageProtect(Process %x  Address %x  flProtect %x)\n",
          Process, Address, flProtect);

#if 0
   Attributes = ProtectToPTE(flProtect);

   Pt = MmGetPageTableForProcess(Process, Address, FALSE);
   if (Pt == NULL)
   {
       ASSERT(FALSE);
   }
   InterlockedExchange((PLONG)Pt, PAGE_MASK(*Pt) | Attributes | (*Pt & (PA_ACCESSED|PA_DIRTY)));
   MiFlushTlb(Pt, Address);
#endif
}

PVOID
NTAPI
MmCreateHyperspaceMapping(PFN_TYPE Page)
{
    PVOID Address;
    ppc_map_info_t info = { 0 };

    Address = (PVOID)((ULONG_PTR)HYPERSPACE * PAGE_SIZE);
    info.proc = 0;
    info.addr = (vaddr_t)Address;
    info.flags = MMU_KRW;
    MmuMapPage(&info, 1);

    return Address;
}

PFN_TYPE
NTAPI
MmChangeHyperspaceMapping(PVOID Address, PFN_TYPE NewPage)
{
    PFN_TYPE OldPage;
    ppc_map_info_t info = { 0 };

    info.proc = 0;
    info.addr = (vaddr_t)Address;
    MmuUnmapPage(&info, 1);
    OldPage = info.phys;
    info.phys = (paddr_t)NewPage;
    MmuMapPage(&info, 1);

    return NewPage;
}

PFN_TYPE
NTAPI
MmDeleteHyperspaceMapping(PVOID Address)
{
    ppc_map_info_t info = { 0 };
    ASSERT (IS_HYPERSPACE(Address));

    info.proc = 0;
    info.addr = (vaddr_t)Address;

    MmuUnmapPage(&info, 1);

    return (PFN_TYPE)info.phys;
}

VOID
INIT_FUNCTION
NTAPI
MmInitGlobalKernelPageDirectory(VOID)
{
}

VOID
INIT_FUNCTION
NTAPI
MiInitPageDirectoryMap(VOID)
{
}

ULONG
NTAPI
MiGetUserPageDirectoryCount(VOID)
{
    return 0;
}

VOID
NTAPI
MmUpdatePageDir(PEPROCESS Process, PVOID Address, ULONG Size)
{
}

/* Create a simple, primitive mapping at the specified address on a new page */
NTSTATUS MmPPCCreatePrimitiveMapping(ULONG_PTR PageAddr)
{
    NTSTATUS result;
    ppc_map_info_t info = { 0 };
    info.flags = MMU_KRW;
    info.addr = (vaddr_t)PageAddr;
    result = MmuMapPage(&info, 1) ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    return result;
}

/* Use our primitive allocator */
PFN_TYPE MmPPCPrimitiveAllocPage()
{
    paddr_t Result = MmuGetPage();
    DbgPrint("Got Page %x\n", Result);
    return Result / PAGE_SIZE;
}

/* EOF */
