/*
 *  ReactOS kernel
 *  Copyright (C) 2000  ReactOS Team
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
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/tss.c
 * PURPOSE:         TSS managment
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <internal/i386/segment.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static KTSS* Ki386TssArray[MAXIMUM_PROCESSORS];
PVOID Ki386InitialStackArray[MAXIMUM_PROCESSORS];
static KTSS* Ki386TrapTssArray[MAXIMUM_PROCESSORS];
static PVOID Ki386TrapStackArray[MAXIMUM_PROCESSORS];

KTSS KiBootTss;
static KTSS KiBootTrapTss;

extern USHORT KiBootGdt[];

extern VOID KiTrap8(VOID);

/* FUNCTIONS *****************************************************************/

VOID
Ki386ApplicationProcessorInitializeTSS(VOID)
{
  ULONG cr3;
  KTSS* Tss;
  KTSS* TrapTss;
  PVOID TrapStack;
  ULONG Id;
  PUSHORT Gdt;
  ULONG base, length;

  Id = KeGetCurrentProcessorNumber();
  Gdt = KeGetCurrentKPCR()->GDT;

  __asm__("movl %%cr3,%0\n\t" : "=d" (cr3));

  Tss = ExAllocatePool(NonPagedPool, sizeof(KTSS));
  TrapTss = ExAllocatePool(NonPagedPool, sizeof(KTSS));
  TrapStack = ExAllocatePool(NonPagedPool, MM_STACK_SIZE);

  Ki386TssArray[Id] = Tss;
  Ki386TrapTssArray[Id] = TrapTss;
  Ki386TrapStackArray[Id] = TrapStack;
  KeGetCurrentKPCR()->TSS = Tss;

  /* Initialize the boot TSS. */
  Tss->Esp0 = (ULONG)Ki386InitialStackArray[Id];
  Tss->Ss0 = KERNEL_DS;
  //   Tss.IoMapBase = FIELD_OFFSET(KTSS, IoBitmap);
  Tss->IoMapBase = 0xFFFF; /* No i/o bitmap */
  Tss->IoBitmap[0] = 0xFF;   
  Tss->Ldt = LDT_SELECTOR;

  /*
   * Initialize a descriptor for the TSS
   */
  base = (ULONG)Tss;
  length = sizeof(KTSS) - 1;
  
  Gdt[(TSS_SELECTOR / 2) + 0] = (length & 0xFFFF);
  Gdt[(TSS_SELECTOR / 2) + 1] = (base & 0xFFFF);
  Gdt[(TSS_SELECTOR / 2) + 2] = ((base & 0xFF0000) >> 16) | 0x8900;
  Gdt[(TSS_SELECTOR / 2) + 3] = ((length & 0xF0000) >> 16) |
    ((base & 0xFF000000) >> 16);

  /* Initialize the TSS used for handling double faults. */
  TrapTss->Eflags = 0;
  TrapTss->Esp0 = ((ULONG)TrapStack + MM_STACK_SIZE);
  TrapTss->Ss0 = KERNEL_DS;
  TrapTss->Esp = ((ULONG)TrapStack + MM_STACK_SIZE);
  TrapTss->Cs = KERNEL_CS;
  TrapTss->Eip = (ULONG)KiTrap8;
  TrapTss->Ss = KERNEL_DS;
  TrapTss->Ds = KERNEL_DS;
  TrapTss->Es = KERNEL_DS;
  TrapTss->Fs = PCR_SELECTOR;
  TrapTss->IoMapBase = 0xFFFF; /* No i/o bitmap */
  TrapTss->IoBitmap[0] = 0xFF;   
  TrapTss->Ldt = LDT_SELECTOR;
  TrapTss->Cr3 = cr3;  

  /*
   * Initialize a descriptor for the trap TSS.
   */
  base = (ULONG)TrapTss;
  length = sizeof(KTSS) - 1;

  Gdt[(TRAP_TSS_SELECTOR / 2) + 0] = (length & 0xFFFF);
  Gdt[(TRAP_TSS_SELECTOR / 2) + 1] = (base & 0xFFFF);
  Gdt[(TRAP_TSS_SELECTOR / 2) + 2] = ((base & 0xFF0000) >> 16) | 0x8900;
  Gdt[(TRAP_TSS_SELECTOR / 2) + 3] = ((length & 0xF0000) >> 16) |
    ((base & 0xFF000000) >> 16);

  /*
   * Load the task register
   */
  __asm__("ltr %%ax" 
	  : /* no output */
	  : "a" (TSS_SELECTOR));
}

VOID
Ki386BootInitializeTSS(VOID)
{
  ULONG cr3;
  extern unsigned int init_stack, init_stack_top;
  extern unsigned int trap_stack, trap_stack_top;
  unsigned int base, length;

  __asm__("movl %%cr3,%0\n\t" : "=d" (cr3));

  Ki386TssArray[0] = &KiBootTss;
  Ki386TrapTssArray[0] = &KiBootTrapTss;
  Ki386TrapStackArray[0] = (PVOID)&trap_stack;
  Ki386InitialStackArray[0] = (PVOID)&init_stack;

  /* Initialize the boot TSS. */
  KiBootTss.Esp0 = (ULONG)&init_stack_top;
  KiBootTss.Ss0 = KERNEL_DS;
  //   KiBootTss.IoMapBase = FIELD_OFFSET(KTSS, IoBitmap);
  KiBootTss.IoMapBase = 0xFFFF; /* No i/o bitmap */
  KiBootTss.IoBitmap[0] = 0xFF;   
  KiBootTss.Ldt = LDT_SELECTOR;

  /*
   * Initialize a descriptor for the TSS
   */
  base = (unsigned int)&KiBootTss;
  length = sizeof(KiBootTss) - 1;
  
  KiBootGdt[(TSS_SELECTOR / 2) + 0] = (length & 0xFFFF);
  KiBootGdt[(TSS_SELECTOR / 2) + 1] = (base & 0xFFFF);
  KiBootGdt[(TSS_SELECTOR / 2) + 2] = ((base & 0xFF0000) >> 16) | 0x8900;
  KiBootGdt[(TSS_SELECTOR / 2) + 3] = ((length & 0xF0000) >> 16) |
    ((base & 0xFF000000) >> 16);

  /* Initialize the TSS used for handling double faults. */
  KiBootTrapTss.Eflags = 0;
  KiBootTrapTss.Esp0 = (ULONG)&trap_stack_top;
  KiBootTrapTss.Ss0 = KERNEL_DS;
  KiBootTrapTss.Esp = (ULONG)&trap_stack_top;
  KiBootTrapTss.Cs = KERNEL_CS;
  KiBootTrapTss.Eip = (ULONG)KiTrap8;
  KiBootTrapTss.Ss = KERNEL_DS;
  KiBootTrapTss.Ds = KERNEL_DS;
  KiBootTrapTss.Es = KERNEL_DS;
  KiBootTrapTss.Fs = PCR_SELECTOR;
  KiBootTrapTss.IoMapBase = 0xFFFF; /* No i/o bitmap */
  KiBootTrapTss.IoBitmap[0] = 0xFF;   
  KiBootTrapTss.Ldt = LDT_SELECTOR;
  KiBootTrapTss.Cr3 = cr3;
  
  /*
   * Initialize a descriptor for the trap TSS.
   */
  base = (unsigned int)&KiBootTrapTss;
  length = sizeof(KiBootTrapTss) - 1;

  KiBootGdt[(TRAP_TSS_SELECTOR / 2) + 0] = (length & 0xFFFF);
  KiBootGdt[(TRAP_TSS_SELECTOR / 2) + 1] = (base & 0xFFFF);
  KiBootGdt[(TRAP_TSS_SELECTOR / 2) + 2] = ((base & 0xFF0000) >> 16) | 0x8900;
  KiBootGdt[(TRAP_TSS_SELECTOR / 2) + 3] = ((length & 0xF0000) >> 16) |
    ((base & 0xFF000000) >> 16);

  /*
   * Load the task register
   */
  __asm__("ltr %%ax" 
	  : /* no output */
	  : "a" (TSS_SELECTOR));
}





