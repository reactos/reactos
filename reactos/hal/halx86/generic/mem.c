/* $Id: mem.c,v 1.1 2004/12/04 17:22:46 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            hal/halx86/generic/mem.c
 * PURPOSE:         Memory mapping functions
 * PROGRAMMER:      Ge van Geldorp (gvg@reactos.com)
 * UPDATE HISTORY:
 *                  Created 2004/12/03
 */

#include <ddk/ntddk.h>

#define PAGETABLE_MAP     (0xf0000000)

#define ADDR_TO_PTE(v) (PULONG)(PAGETABLE_MAP + ((((ULONG)(v) / 1024))&(~0x3)))

#define VIRT_ADDR 0xff400000

PVOID
HalpMapPhysMemory(ULONG PhysAddr, ULONG Size)
{
  PULONG PageTable;
  unsigned i;

  PageTable = (PULONG)PAGE_ROUND_DOWN((PVOID)ADDR_TO_PTE(VIRT_ADDR));
  for (i = 0; i < PAGE_ROUND_UP(Size) / PAGE_SIZE; i++)
    {
      PageTable[i] = (PhysAddr | 0x3);
      PhysAddr += PAGE_SIZE;
    }

  /* Flush TLB */
  __asm__ __volatile__(
    "movl %%cr3,%%eax\n\t"
    "movl %%eax,%%cr3\n\t"
    : : : "eax" );

  return (PVOID) VIRT_ADDR;
}

/* EOF */
