/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/tss.c
 * PURPOSE:         TSS managment
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, Ki386BootInitializeTSS)
#endif

/* GLOBALS *******************************************************************/

typedef struct _KTSSNOIOPM
{
    UCHAR TssData[KTSS_IO_MAPS];
} KTSSNOIOPM;

static KTSS* Ki386TssArray[MAXIMUM_PROCESSORS];
PVOID Ki386InitialStackArray[MAXIMUM_PROCESSORS];
static KTSSNOIOPM* Ki386TrapTssArray[MAXIMUM_PROCESSORS];
static PVOID Ki386TrapStackArray[MAXIMUM_PROCESSORS];

KTSS KiBootTss;
static KTSSNOIOPM KiBootTrapTss;

extern USHORT KiBootGdt[];
extern ULONG init_stack;
extern ULONG init_stack_top;
extern VOID KiTrap8(VOID);

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION
Ki386BootInitializeTSS(VOID)
{
  ULONG cr3_;
  extern unsigned int trap_stack, trap_stack_top;
  unsigned int base, length;
  PKTSS Tss;

  Ke386GetPageTableDirectory(cr3_);

  Ki386TssArray[0] = &KiBootTss;
  Ki386TrapTssArray[0] = &KiBootTrapTss;
  Ki386TrapStackArray[0] = (PVOID)trap_stack;
  Ki386InitialStackArray[0] = (PVOID)init_stack;

  /* Initialize the boot TSS. */
  KiBootTss.Esp0 = (ULONG)init_stack_top - sizeof(FX_SAVE_AREA);
  KiBootTss.Ss0 = KGDT_R0_DATA;
  KiBootTss.IoMapBase = 0xFFFF; /* No i/o bitmap */
  KiBootTss.LDT = KGDT_LDT;

  /*
   * Initialize a descriptor for the TSS
   */
  base = (unsigned int)&KiBootTss;
  length = sizeof(KiBootTss) - 1;

  KiBootGdt[(KGDT_TSS / 2) + 0] = (length & 0xFFFF);
  KiBootGdt[(KGDT_TSS / 2) + 1] = (base & 0xFFFF);
  KiBootGdt[(KGDT_TSS / 2) + 2] = ((base & 0xFF0000) >> 16) | 0x8900;
  KiBootGdt[(KGDT_TSS / 2) + 3] = ((length & 0xF0000) >> 16) |
    ((base & 0xFF000000) >> 16);

  /* Initialize the TSS used for handling double faults. */
  Tss = (PKTSS)&KiBootTrapTss;
  Tss->Flags = 0;
  Tss->Esp0 = (ULONG)trap_stack_top; /* FIXME: - sizeof(FX_SAVE_AREA)? */
  Tss->Ss0 = KGDT_R0_DATA;
  Tss->Cs = KGDT_R0_CODE;
  Tss->Eip = (ULONG)KiTrap8;
  Tss->Ds = KGDT_R0_DATA;
  Tss->Es = KGDT_R0_DATA;
  Tss->Fs = KGDT_R0_PCR;
  Tss->IoMapBase = 0xFFFF; /* No i/o bitmap */
  Tss->LDT = 0x0;

  /*
   * Initialize a descriptor for the trap TSS.
   */
  base = (unsigned int)&KiBootTrapTss;
  length = sizeof(KiBootTrapTss) - 1;

  KiBootGdt[(KGDT_DF_TSS / 2) + 0] = (length & 0xFFFF);
  KiBootGdt[(KGDT_DF_TSS / 2) + 1] = (base & 0xFFFF);
  KiBootGdt[(KGDT_DF_TSS / 2) + 2] = ((base & 0xFF0000) >> 16) | 0x8900;
  KiBootGdt[(KGDT_DF_TSS / 2) + 3] = ((length & 0xF0000) >> 16) |
    ((base & 0xFF000000) >> 16);

  /*
   * Load the task register
   */
#if defined(__GNUC__)
  __asm__("ltr %%ax"
	  : /* no output */
	  : "a" (KGDT_TSS));
#elif defined(_MSC_VER)
  __asm mov ax, KGDT_TSS
  __asm ltr ax
#else
#error Unknown compiler for inline assembler
#endif
}





