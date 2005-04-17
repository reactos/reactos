/*
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "freeldr.h"
#include "machxen.h"

#include <rosxen.h>
#include <xen.h>
#include <hypervisor.h>

#ifdef CONFIG_XEN_BLKDEV_GRANT
#include <grant_table.h>
#endif /* CONFIG_XEN_BLKDEV_GRANT */

/* Page Directory Entry */
typedef struct _PDE
{
  u32 Pde;
} PDE, *PPDE;

/* Page Table Entry */
typedef struct _PTE
{
  u32 Pte;
} PTE, *PPTE;

#define PGDIR_SHIFT   22
#define PTRS_PER_PD   (PAGE_SIZE / sizeof(PDE))
#define PTRS_PER_PT   (PAGE_SIZE / sizeof(PTE))

/* Page Directory Index of a given virtual address */
#define PD_IDX(Va) ((((ULONG_PTR) Va) >> PGDIR_SHIFT) & (PTRS_PER_PD - 1))
/* Page Table Index of a give virtual address */
#define PT_IDX(Va) ((((ULONG_PTR) Va) >> PAGE_SHIFT) & (PTRS_PER_PT - 1))
/* Convert a Page Directory or Page Table entry to a (machine) address */
#define PAGE_MASK  (~(PAGE_SIZE-1))

/* The PA_* definitions below were copied from ntoskrnl/mm/i386/page.c, maybe
   we need a public header for them?? */
#define PA_BIT_PRESENT   (0)
#define PA_BIT_READWRITE (1)
#define PA_BIT_USER      (2)
#define PA_BIT_WT        (3)
#define PA_BIT_CD        (4)
#define PA_BIT_ACCESSED  (5)
#define PA_BIT_DIRTY     (6)
#define PA_BIT_GLOBAL    (8)

#define PA_PRESENT   (1 << PA_BIT_PRESENT)
#define PA_READWRITE (1 << PA_BIT_READWRITE)
#define PA_USER      (1 << PA_BIT_USER)
#define PA_DIRTY     (1 << PA_BIT_DIRTY)
#define PA_WT        (1 << PA_BIT_WT)
#define PA_CD        (1 << PA_BIT_CD)
#define PA_ACCESSED  (1 << PA_BIT_ACCESSED)
#define PA_GLOBAL    (1 << PA_BIT_GLOBAL)

start_info_t *XenStartInfo;
shared_info_t *XenSharedInfo;

static PPDE XenPageDir;

#if 2 == XEN_VER
#define XEN_MMU_UPDATE(req, count, success, domid) \
  HYPERVISOR_mmu_update((req), (count), (success))
#else /* XEN_VER */
#define XEN_MMU_UPDATE(req, count, success, domid) \
  HYPERVISOR_mmu_update((req), (count), (success), (domid))
#endif /* XEN_VER */

#ifdef CONFIG_XEN_BLKDEV_GRANT
static grant_entry_t *XenMemBlkdevGrantShared;
#endif /* CONFIG_XEN_BLKDEV_GRANT */

ULONG
XenMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize)
{
  ULONG EntryCount;

  /* Don't give the memory manager a hard time, just tell it we have contiguous
   * memory starting at address 0 and handle machine/physical stuff here */
  if (2 <= MaxMemoryMapSize)
    {
      BiosMemoryMap[0].BaseAddress = 0;
      BiosMemoryMap[0].Length = (unsigned long) XenStartInfo;
      BiosMemoryMap[0].Type = MEMTYPE_USABLE;
      BiosMemoryMap[1].BaseAddress = (unsigned long) XenStartInfo;
      BiosMemoryMap[1].Length = XenStartInfo->nr_pages * PAGE_SIZE
                                - (unsigned long) XenStartInfo;
      BiosMemoryMap[1].Type = MEMTYPE_RESERVED;
      EntryCount = 2;
    }
  else
    {
      EntryCount = 0;
    }

  return EntryCount;
}

/*
 * This is an example (virtual) memory layout as presented to us by Xen:
 *
 * +--------------------+ 0xffffffff
 * | Xen private        |
 * +--------------------+ 0xfc000000
 * | Unmapped           |
 * +--------------------+ 0x00400000
 * | Mapped unused      |
 * +--------------------+ 0x0004d000
 * | Stack              |
 * +--------------------+ 0x0004c000
 * | Start info         |
 * +--------------------+ 0x0004b000 StartInfo
 * | Page dir/tables    |
 * +--------------------+ 0x00049000 StartInfo->pt_base
 * | Machine frame list |
 * +--------------------+ 0x00041000 StartInfo->mfn_list
 * | FreeLdr image      |
 * +--------------------+ 0x00008000
 * | Unmapped           |
 * +--------------------+ 0x00000000
 *
 * And this is how we would like it:
 *
 * +--------------------+ 0xffffffff
 * | Xen private        |
 * +--------------------+ 0xfc000000
 * | Unmapped           |
 * +--------------------+ 0x02001000
 * | Shared page        |
 * +--------------------+ 0x02000000 XenSharedPage
 * | Page dir/tables    |
 * +--------------------+ 0x01ff7000 XenPageDir
 * | Start info         |
 * +--------------------+ 0x01ff6000 XenStartInfo
 * | Mapped unused      |
 * +--------------------+ 0x00041000
 * | FreeLdr image      |
 * +--------------------+ 0x00008000
 * | Stack              |
 * +--------------------+ 0x00000000
 *
 * This way, we can let the FreeLdr memory manager handle the address space
 * between the end of the FreeLdr image and the information near the end of
 * the physical memory (starting at XenStartInfo).
 * XenMemInit sets up the memory the way we like it, except for switching the
 * stack. That is done by our caller.
 */
void
XenMemInit(start_info_t *StartInfo)
{
  extern void *start;               /* Where is freeldr loaded? */
  static char ErrMsg[] = "XenMemInit failed\n";
  unsigned long StartPfn;           /* (Virtual) page frame number of beginning
                                       of freeldr */
  PPDE PageDir;                     /* Virtual address of page directory */
  PPTE PageTableForPageDir;         /* Virtual address of page table which
                                       contains entry for the page directory */
  unsigned long PageDirMachineAddr; /* Machine address of page directory */
  unsigned PageTablesRequired;      /* Number of page tables we require */
  unsigned PageTableNumber;         /* Index of current page table */
  mmu_update_t MmuReq;              /* Request info for mmu update */
  unsigned long MfnIndex;           /* Index into mfn_list */
  unsigned long HighAddr;           /* Virtual address after reloc to high
                                       memory */
  unsigned long PageNumber;         /* Index of current page */
  PPTE PageTable;                   /* Page table containing current page */
#ifdef CONFIG_XEN_BLKDEV_GRANT
  gnttab_setup_table_t Setup;       /* Grant table setup request */
  unsigned long Frame;              /* Grant table frame */
#endif /* CONFIG_XEN_BLKDEV_GRANT */

  PageDir = (PPDE) StartInfo->pt_base;
  PageTableForPageDir = (PPTE)((char *) StartInfo->pt_base
                               + PAGE_SIZE * (PD_IDX(StartInfo->pt_base) + 1));
  PageDirMachineAddr = PageTableForPageDir[PT_IDX(StartInfo->pt_base)].Pte
                       & PAGE_MASK;

  /* Determine pfn of first allocated memory */
  StartPfn = ROUND_DOWN((unsigned long) &start, PAGE_SIZE) / PAGE_SIZE;

  /* First, lets connect all our page tables */
#ifdef CONFIG_XEN_BLKDEV_GRANT
  PageTablesRequired = ROUND_UP(StartInfo->nr_pages + 2, PTRS_PER_PT)
                       / PTRS_PER_PT;
#else /* CONFIG_XEN_BLKDEV_GRANT */
  PageTablesRequired = ROUND_UP(StartInfo->nr_pages + 1, PTRS_PER_PT)
                       / PTRS_PER_PT;
#endif /* CONFIG_XEN_BLKDEV_GRANT */
  for (PageTableNumber = 0; PageTableNumber < PageTablesRequired;
       PageTableNumber++)
    {
      MfnIndex = StartInfo->nr_pages - (StartPfn + PageTablesRequired)
                 + PageTableNumber;
      if (0 == PageDir[PageTableNumber].Pde)
        {
          MmuReq.ptr = PageDirMachineAddr + PageTableNumber * sizeof(PTE);
        }
      else
        {
          /* There's already a page table connected at this position. That
           * means that it's also mapped in the area following pt_base. We
           * don't want it there, so let's map another page at that pos.
           * This page table will be mapped near the top of memory later */
          PageTable = (PPTE)((char *) PageDir
                             + (PageTableNumber + 1) * PAGE_SIZE);
          MmuReq.ptr = (PageDir[PD_IDX((unsigned long) PageTable)].Pde
                        & PAGE_MASK) +
                       PT_IDX((unsigned long) PageTable) * sizeof(PTE);
        }
      MmuReq.val = (((u32*)StartInfo->mfn_list)[MfnIndex] << PAGE_SHIFT)
                   | (PA_PRESENT | PA_READWRITE | PA_USER);
      if (0 != XEN_MMU_UPDATE(&MmuReq, 1, NULL, DOMID_SELF))
        {
          HYPERVISOR_console_io(CONSOLEIO_write, sizeof(ErrMsg), ErrMsg);
          XenDie();
        }
    }

  /* Now, let's make the page directory visible near the top of mem */
  HighAddr = (StartInfo->nr_pages - (PageTablesRequired + 1)) * PAGE_SIZE;
  MmuReq.ptr = (PageDir[PD_IDX(HighAddr)].Pde & PAGE_MASK)
               + PT_IDX(HighAddr) * sizeof(PTE);
  MmuReq.val = PageDirMachineAddr
               | (PA_PRESENT | PA_USER);
  if (0 != XEN_MMU_UPDATE(&MmuReq, 1, NULL, DOMID_SELF))
    {
      HYPERVISOR_console_io(CONSOLEIO_write, sizeof(ErrMsg), ErrMsg);
      XenDie();
    }
  XenPageDir = (PPDE) HighAddr;

  /* We don't need the page directory mapped at the low address (pt_base)
   * anymore, so we'll map another page there */
  MmuReq.ptr = (PageDir[PD_IDX((unsigned long) PageDir)].Pde & PAGE_MASK)
               + PT_IDX((unsigned long) PageDir) * sizeof(PTE);
  MfnIndex = StartInfo->nr_pages - (StartPfn + PageTablesRequired + 1);
  MmuReq.val = (((u32*)StartInfo->mfn_list)[MfnIndex] << PAGE_SHIFT)
               | (PA_PRESENT | PA_READWRITE | PA_USER);
  if (0 != XEN_MMU_UPDATE(&MmuReq, 1, NULL, DOMID_SELF))
    {
      HYPERVISOR_console_io(CONSOLEIO_write, sizeof(ErrMsg), ErrMsg);
      XenDie();
    }

  /* Map the page tables near the top of mem */
  for (PageTableNumber = 0; PageTableNumber < PageTablesRequired;
       PageTableNumber++)
    {
      HighAddr = (StartInfo->nr_pages - PageTablesRequired + PageTableNumber)
                 * PAGE_SIZE;
      MmuReq.ptr = (XenPageDir[PD_IDX(HighAddr)].Pde & PAGE_MASK)
                    + PT_IDX(HighAddr) * sizeof(PTE);
      MmuReq.val = (XenPageDir[PageTableNumber].Pde & PAGE_MASK)
                   | (PA_PRESENT | PA_USER);
      if (0 != XEN_MMU_UPDATE(&MmuReq, 1, NULL, DOMID_SELF))
        {
          HYPERVISOR_console_io(CONSOLEIO_write, sizeof(ErrMsg), ErrMsg);
          XenDie();
        }
    }

  /* Fill in gaps */
  for (PageNumber = 0;
       PageNumber < StartInfo->nr_pages - (PageTablesRequired + 1);
       PageNumber++)
    {
      PageTable = (PPTE)((char *) XenPageDir +
                         (PD_IDX(PageNumber * PAGE_SIZE) + 1) * PAGE_SIZE);
      if (0 == PageTable[PT_IDX(PageNumber * PAGE_SIZE)].Pte)
        {
          if (PageNumber < StartPfn)
            {
              MfnIndex = StartInfo->nr_pages - StartPfn + PageNumber;
            }
          else
            {
              MfnIndex = PageNumber - StartPfn;
            }
          MmuReq.ptr = (XenPageDir[PD_IDX(PageNumber * PAGE_SIZE)].Pde
                        & PAGE_MASK)
                        + PT_IDX(PageNumber * PAGE_SIZE) * sizeof(PTE);
          MmuReq.val = (((u32 *) StartInfo->mfn_list)[MfnIndex] << PAGE_SHIFT)
                       | (PA_PRESENT | PA_READWRITE | PA_USER);
          if (0 != XEN_MMU_UPDATE(&MmuReq, 1, NULL, DOMID_SELF))
            {
              HYPERVISOR_console_io(CONSOLEIO_write, sizeof(ErrMsg), ErrMsg);
              XenDie();
            }
        }
    }

  /* Need to move the start info out of the way */
  XenStartInfo = (start_info_t *)((char *) XenPageDir - PAGE_SIZE);
  memcpy(XenStartInfo, StartInfo, PAGE_SIZE);

  /* We don't own the shared_info page, map it as an extra page just after
   * our "normal" memory */
  XenSharedInfo = (shared_info_t *)(XenStartInfo->nr_pages * PAGE_SIZE);
  MmuReq.ptr = (XenPageDir[PD_IDX(XenStartInfo->nr_pages * PAGE_SIZE)].Pde
                & PAGE_MASK)
               + PT_IDX(XenStartInfo->nr_pages * PAGE_SIZE) * sizeof(PTE);
  MmuReq.val = XenStartInfo->shared_info
               | (PA_PRESENT | PA_READWRITE | PA_USER);
  if (0 != XEN_MMU_UPDATE(&MmuReq, 1, NULL, DOMID_SELF))
    {
      HYPERVISOR_console_io(CONSOLEIO_write, sizeof(ErrMsg), ErrMsg);
      XenDie();
    }

#ifdef CONFIG_XEN_BLKDEV_GRANT
  /* If we're using grant tables, setup a single shared grant table page
   * following the shared_info page */
  Setup.dom = DOMID_SELF;
  Setup.nr_frames = 1;
  Setup.frame_list = &Frame;

  if (0 != HYPERVISOR_grant_table_op(GNTTABOP_setup_table, &Setup, 1)
      || 0 != Setup.status)
    {
      HYPERVISOR_console_io(CONSOLEIO_write, sizeof(ErrMsg), ErrMsg);
      XenDie();
    }
  XenMemBlkdevGrantShared = (grant_entry_t *)((XenStartInfo->nr_pages + 1)
                                              * PAGE_SIZE);
  MmuReq.ptr = (XenPageDir[PD_IDX(XenMemBlkdevGrantShared)].Pde
                & PAGE_MASK)
               + PT_IDX(XenMemBlkdevGrantShared) * sizeof(PTE);
  MmuReq.val = (Frame << PAGE_SHIFT)
               | (PA_PRESENT | PA_READWRITE | PA_USER);
  if (0 != XEN_MMU_UPDATE(&MmuReq, 1, NULL, DOMID_SELF))
    {
      HYPERVISOR_console_io(CONSOLEIO_write, sizeof(ErrMsg), ErrMsg);
      XenDie();
    }
#endif /* CONFIG_XEN_BLKDEV_GRANT */
}

u32
XenMemVirtualToMachine(void *VirtualAddress)
{
  PPTE PageTable;

  PageTable = (PPTE)((char *) XenPageDir +
                     (PD_IDX((ULONG_PTR) VirtualAddress) + 1) * PAGE_SIZE);
  return PageTable[PT_IDX((ULONG_PTR) VirtualAddress)].Pte
         & PAGE_MASK;
}

#ifdef CONFIG_XEN_BLKDEV_GRANT
int
XenMemGrantForeignAccess(domid_t DomId, void *VirtualAddress, BOOL ReadOnly)
{
  int Ref = 0; /* We only do 1 page at the moment */

  XenMemBlkdevGrantShared[Ref].frame = (XenMemVirtualToMachine(VirtualAddress)
                                        >> PAGE_SHIFT);
  XenMemBlkdevGrantShared[Ref].domid = DomId;
  wmb();
  XenMemBlkdevGrantShared[Ref].flags = GTF_permit_access
                                       | (ReadOnly ? GTF_readonly : 0);

  return Ref;
}
#endif /* CONFIG_XEN_BLKDEV_GRANT */

/* EOF */
