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
#include "i386boot.h"
#include "i386mem.h"
#include "machxen.h"

#include <rosxen.h>
#include <xen.h>
#include <hypervisor.h>

#ifdef CONFIG_XEN_BLKDEV_GRANT
#include <grant_table.h>
#endif /* CONFIG_XEN_BLKDEV_GRANT */

start_info_t *XenStartInfo;
shared_info_t *XenSharedInfo;

static PPAGE_DIRECTORY_X86 XenPageDir;

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
  PPAGE_DIRECTORY_X86 PageDir;      /* Virtual address of page directory */
  PPAGE_TABLE_X86 PageTableForPageDir; /* Virtual address of page table which
                                       contains entry for the page directory */
  unsigned long PageDirMachineAddr; /* Machine address of page directory */
  unsigned PageTablesRequired;      /* Number of page tables we require */
  unsigned PageTableNumber;         /* Index of current page table */
  mmu_update_t MmuReq;              /* Request info for mmu update */
  unsigned long MfnIndex;           /* Index into mfn_list */
  unsigned long HighAddr;           /* Virtual address after reloc to high
                                       memory */
  unsigned long PageNumber;         /* Index of current page */
  PPAGE_TABLE_X86 PageTable;        /* Page table containing current page */
  HARDWARE_PTE_X86 Pte;             /* Page table entry */
#ifdef CONFIG_XEN_BLKDEV_GRANT
  gnttab_setup_table_t Setup;       /* Grant table setup request */
  unsigned long Frame;              /* Grant table frame */
#endif /* CONFIG_XEN_BLKDEV_GRANT */

  PageDir = (PPAGE_DIRECTORY_X86) StartInfo->pt_base;
  PageTableForPageDir = (PPAGE_TABLE_X86)((char *) StartInfo->pt_base
                                          + PAGE_SIZE
                                            * (PD_IDX(StartInfo->pt_base) + 1));
  PageDirMachineAddr = PageTableForPageDir->Pte[PT_IDX(StartInfo->pt_base)].Val
                       & PAGE_MASK;

  /* Determine pfn of first allocated memory */
  StartPfn = ROUND_DOWN((unsigned long) &start, PAGE_SIZE) / PAGE_SIZE;

  /* First, lets connect all our page tables */
#ifdef CONFIG_XEN_BLKDEV_GRANT
  PageTablesRequired = ROUND_UP(StartInfo->nr_pages + 2, PTRS_PER_PT_X86)
                       / PTRS_PER_PT_X86;
#else /* CONFIG_XEN_BLKDEV_GRANT */
  PageTablesRequired = ROUND_UP(StartInfo->nr_pages + 1, PTRS_PER_PT_X86)
                       / PTRS_PER_PT_X86;
#endif /* CONFIG_XEN_BLKDEV_GRANT */
  for (PageTableNumber = 0; PageTableNumber < PageTablesRequired;
       PageTableNumber++)
    {
      MfnIndex = StartInfo->nr_pages - (StartPfn + PageTablesRequired)
                 + PageTableNumber;
      if (0 == PageDir->Pde[PageTableNumber].Val)
        {
          MmuReq.ptr = PageDirMachineAddr
                       + PageTableNumber * sizeof(HARDWARE_PTE_X86);
        }
      else
        {
          /* There's already a page table connected at this position. That
           * means that it's also mapped in the area following pt_base. We
           * don't want it there, so let's map another page at that pos.
           * This page table will be mapped near the top of memory later */
          PageTable = (PPAGE_TABLE_X86)((char *) PageDir
                                        + (PageTableNumber + 1) * PAGE_SIZE);
          MmuReq.ptr = (PageDir->Pde[PD_IDX(PageTable)].Val
                        & PAGE_MASK) +
                       PT_IDX(PageTable) * sizeof(HARDWARE_PTE_X86);
        }
      Pte.Val = 0;
      Pte.PageFrameNumber = ((u32*)StartInfo->mfn_list)[MfnIndex];
      Pte.Valid = 1;
      Pte.Write = 1;
      Pte.Owner = 1;
      MmuReq.val = Pte.Val;
      if (0 != XEN_MMU_UPDATE(&MmuReq, 1, NULL, DOMID_SELF))
        {
          HYPERVISOR_console_io(CONSOLEIO_write, sizeof(ErrMsg), ErrMsg);
          XenDie();
        }
    }

  /* Now, let's make the page directory visible near the top of mem */
  HighAddr = (StartInfo->nr_pages - (PageTablesRequired + 1)) * PAGE_SIZE;
  MmuReq.ptr = (PageDir->Pde[PD_IDX(HighAddr)].Val & PAGE_MASK)
               + PT_IDX(HighAddr) * sizeof(HARDWARE_PTE_X86);
  Pte.Val = 0;
  Pte.PageFrameNumber = PageDirMachineAddr >> PFN_SHIFT;
  Pte.Valid = 1;
  Pte.Owner = 1;
  MmuReq.val = Pte.Val;
  if (0 != XEN_MMU_UPDATE(&MmuReq, 1, NULL, DOMID_SELF))
    {
      HYPERVISOR_console_io(CONSOLEIO_write, sizeof(ErrMsg), ErrMsg);
      XenDie();
    }
  XenPageDir = (PPAGE_DIRECTORY_X86) HighAddr;

  /* We don't need the page directory mapped at the low address (pt_base)
   * anymore, so we'll map another page there */
  MmuReq.ptr = (PageDir->Pde[PD_IDX((unsigned long) PageDir)].Val & PAGE_MASK)
               + PT_IDX((unsigned long) PageDir) * sizeof(HARDWARE_PTE_X86);
  MfnIndex = StartInfo->nr_pages - (StartPfn + PageTablesRequired + 1);
  Pte.Val = 0;
  Pte.PageFrameNumber = ((u32*)StartInfo->mfn_list)[MfnIndex];
  Pte.Valid = 1;
  Pte.Write = 1;
  Pte.Owner = 1;
  MmuReq.val = Pte.Val;
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
      MmuReq.ptr = (XenPageDir->Pde[PD_IDX(HighAddr)].Val & PAGE_MASK)
                    + PT_IDX(HighAddr) * sizeof(HARDWARE_PTE_X86);
      Pte.Val = 0;
      Pte.PageFrameNumber = XenPageDir->Pde[PageTableNumber].Val >> PFN_SHIFT;
      Pte.Valid = 1;
      Pte.Owner = 1;
      MmuReq.val = Pte.Val;
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
      PageTable = (PPAGE_TABLE_X86)((char *) XenPageDir
                                    + (PD_IDX(PageNumber * PAGE_SIZE) + 1)
                                      * PAGE_SIZE);
      if (0 == PageTable->Pte[PT_IDX(PageNumber * PAGE_SIZE)].Val)
        {
          if (PageNumber < StartPfn)
            {
              MfnIndex = StartInfo->nr_pages - StartPfn + PageNumber;
            }
          else
            {
              MfnIndex = PageNumber - StartPfn;
            }
          MmuReq.ptr = (XenPageDir->Pde[PD_IDX(PageNumber * PAGE_SIZE)].Val
                        & PAGE_MASK)
                        + PT_IDX(PageNumber * PAGE_SIZE)
                          * sizeof(HARDWARE_PTE_X86);
          Pte.Val = 0;
          Pte.PageFrameNumber = ((u32 *) StartInfo->mfn_list)[MfnIndex];
          Pte.Valid = 1;
          Pte.Write = 1;
          Pte.Owner = 1;
          MmuReq.val = Pte.Val;
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
  MmuReq.ptr = (XenPageDir->Pde[PD_IDX(XenStartInfo->nr_pages * PAGE_SIZE)].Val
                & PAGE_MASK)
               + PT_IDX(XenStartInfo->nr_pages * PAGE_SIZE)
                 * sizeof(HARDWARE_PTE_X86);
  Pte.Val = 0;
  Pte.PageFrameNumber = XenStartInfo->shared_info >> PFN_SHIFT;
  Pte.Valid = 1;
  Pte.Write = 1;
  Pte.Owner = 1;
  MmuReq.val = Pte.Val;
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
  MmuReq.ptr = (XenPageDir->Pde[PD_IDX(XenMemBlkdevGrantShared)].Val
                & PAGE_MASK)
               + PT_IDX(XenMemBlkdevGrantShared) * sizeof(HARDWARE_PTE_X86);
  Pte.Val = 0;
  Pte.PageFrameNumber = Frame;
  Pte.Valid = 1;
  Pte.Write = 1;
  Pte.Owner = 1;
  MmuReq.val = Pte.Val;
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
  PPAGE_TABLE_X86 PageTable;

  PageTable = (PPAGE_TABLE_X86)((char *) XenPageDir
                                + (PD_IDX((ULONG_PTR) VirtualAddress) + 1)
                                  * PAGE_SIZE);
  return PageTable->Pte[PT_IDX(VirtualAddress)].Val & PAGE_MASK;
}

static void *
XenMemMachineToVirtual(ULONG Mfn)
{
  PPAGE_TABLE_X86 PageTable;
  ULONG Index;

  PageTable = (PPAGE_TABLE_X86)(XenPageDir + 1);
  Index = 0;
  while (TRUE)
    {
      if ((PageTable->Pte[Index].PageFrameNumber) == Mfn)
        {
          return (void *)(Index * PAGE_SIZE);
        }
      Index++;
    }
}

#ifdef CONFIG_XEN_BLKDEV_GRANT
int
XenMemGrantForeignAccess(domid_t DomId, void *VirtualAddress, BOOL ReadOnly)
{
  int Ref = 0; /* We only do 1 page at the moment */

  XenMemBlkdevGrantShared[Ref].frame = (XenMemVirtualToMachine(VirtualAddress)
                                        >> PFN_SHIFT);
  XenMemBlkdevGrantShared[Ref].domid = DomId;
  wmb();
  XenMemBlkdevGrantShared[Ref].flags = GTF_permit_access
                                       | (ReadOnly ? GTF_readonly : 0);

  return Ref;
}
#endif /* CONFIG_XEN_BLKDEV_GRANT */

static VOID
XenMemSetNewPageReadonly(PPAGE_DIRECTORY_X86 NewPageDir, ULONG Mfn)
{
  PPAGE_TABLE_X86 PageTable;
  unsigned PdIndex;
  unsigned PtIndex;

  for (PdIndex = 0; PdIndex < PTRS_PER_PD_X86; PdIndex++)
    {
      if (NewPageDir->Pde[PdIndex].Valid)
        {
          PageTable = (PPAGE_TABLE_X86)
                      XenMemMachineToVirtual(NewPageDir->Pde[PdIndex].PageFrameNumber);
          for (PtIndex = 0; PtIndex < PTRS_PER_PT_X86; PtIndex++)
            {
              if (PageTable->Pte[PtIndex].PageFrameNumber == Mfn)
                {
                  PageTable->Pte[PtIndex].Write = 0;
                }
            }
        }
    }
}

static VOID
XenMemSetOldPageReadonly(ULONG Mfn)
{
  PPAGE_TABLE_X86 PageTable;
  ULONG Index;
  mmu_update_t MmuReq;
  ULONG_PTR Address;
  HARDWARE_PTE_X86 Pte;

  PageTable = (PPAGE_TABLE_X86)(XenPageDir + 1);
  Index = 0;
  while (TRUE)
    {
      if ((PageTable->Pte[Index].PageFrameNumber) == Mfn)
        {
          Address = Index * PAGE_SIZE;
          MmuReq.ptr = (XenPageDir->Pde[PD_IDX(Address)].Val & PAGE_MASK)
                       + PT_IDX(Address) * sizeof(HARDWARE_PTE_X86);
          Pte.Val = PageTable->Pte[Index].Val;
          Pte.Write = 0;
          MmuReq.val = Pte.Val;
          if (0 != XEN_MMU_UPDATE(&MmuReq, 1, NULL, DOMID_SELF))
            {
              printf("Failed to map new page dir/table read-only\n");
              XenDie();
            }
          return;
        }
      Index++;
    }
}

VOID
XenMemInstallPageDir(PPAGE_DIRECTORY_X86 NewPageDir)
{
#if 2 == XEN_VER
  mmu_update_t MmuReq;
#else /* XEN_VER */
  struct mmuext_op MmuExtReq;
#endif /* XEN_VER */
  ULONG NewPageDirMfn;
  ULONG PageIndex;

  NewPageDirMfn = PaToPfn(XenMemVirtualToMachine((void *) NewPageDir));

  for (PageIndex = PTRS_PER_PD_X86; 0 < PageIndex; PageIndex--)
    {
      if (NewPageDir->Pde[PageIndex - 1].PageFrameNumber == NewPageDirMfn)
        {
        /* A machine page cannot be a page directory and a page table at
           the same time. We're trying to map the page dir as page table here,
           undo the mapping and deal with it later. */
        NewPageDir->Pde[PageIndex - 1].Val = 0;
        }
    }

  XenMemSetNewPageReadonly(NewPageDir, NewPageDirMfn);
  for (PageIndex = PTRS_PER_PD_X86; 0 < PageIndex; PageIndex--)
    {
      if (NewPageDir->Pde[PageIndex - 1].Valid)
        {
          XenMemSetNewPageReadonly(NewPageDir,
                                   NewPageDir->Pde[PageIndex - 1].Val
                                   >> PFN_SHIFT);
        }
    }

  XenMemSetOldPageReadonly(NewPageDirMfn);
  for (PageIndex = PTRS_PER_PD_X86; 0 < PageIndex; PageIndex--)
    {
      if (NewPageDir->Pde[PageIndex - 1].Valid)
        {
          XenMemSetOldPageReadonly(NewPageDir->Pde[PageIndex - 1].Val
                                   >> PFN_SHIFT);
        }
    }

  /* Set the PDBR */
#if 2 == XEN_VER
  MmuReq.ptr = (NewPageDirMfn << PFN_SHIFT) | MMU_EXTENDED_COMMAND;
  MmuReq.val = MMUEXT_NEW_BASEPTR;
  if (0 != XEN_MMU_UPDATE(&MmuReq, 1, NULL, DOMID_SELF))
#else /* XEN_VER */
  MmuExtReq.cmd = MMUEXT_NEW_BASEPTR;
  MmuExtReq.mfn = NewPageDirMfn;
  if (0 != HYPERVISOR_mmuext_op(&MmuExtReq, 1, NULL, DOMID_SELF))
#endif /* XEN_VER */
    {
      printf("Failed to set new page dir\n");
      XenDie();
    }
}

/* EOF */
