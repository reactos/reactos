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

/* GLOBALS *******************************************************************/

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

BOOL STDCALL
Ke386IoSetAccessProcess(PEPROCESS Process, BOOL EnableDisable)
{
  KIRQL oldIrql;
  USHORT Offset;

  if(EnableDisable > 1) return FALSE;
  Offset = (EnableDisable) ? (USHORT) FIELD_OFFSET(KTSS, IoBitmap) : 0xffff;

  oldIrql = KeRaiseIrqlToSynchLevel();
  Process->Pcb.IopmOffset = Offset;

  if(PsGetCurrentProcess() == Process)
  {
    KeGetCurrentKPCR()->TSS->IoMapBase = Offset;
  }

  KeLowerIrql(oldIrql);
  return TRUE;
}


BOOL STDCALL
Ke386SetIoAccessMap(DWORD MapNumber, PULONG IOMapStart)
{
  KIRQL oldIrql;

  if(MapNumber != 1) return FALSE;

  oldIrql = KeRaiseIrqlToSynchLevel();

  memcpy(&KeGetCurrentKPCR()->TSS->IoBitmap[0],
         IOMapStart,
         0x2000);

  KeGetCurrentKPCR()->TSS->IoMapBase = KeGetCurrentProcess()->IopmOffset;
  KeLowerIrql(oldIrql);
  return TRUE;
}

BOOL STDCALL
Ke386QueryIoAccessMap(DWORD MapNumber, PULONG IOMapStart)
{
  KIRQL oldIrql;

  if(MapNumber == 0x0)
  {
    memset(IOMapStart, 0xff, 0x2000);
    return TRUE;
  } else if(MapNumber != 1) return FALSE;

  oldIrql = KeRaiseIrqlToSynchLevel();

  memcpy(IOMapStart,
         &KeGetCurrentKPCR()->TSS->IoBitmap[0],
         0x2000);

  KeLowerIrql(oldIrql);
  return TRUE;
}

VOID
Ki386ApplicationProcessorInitializeTSS(VOID)
{
  ULONG cr3_;
  KTSS* Tss;
  KTSSNOIOPM* TrapTss;
  PVOID TrapStack;
  ULONG Id;
  PUSHORT Gdt;
  ULONG base, length;

  Id = KeGetCurrentProcessorNumber();
  Gdt = KeGetCurrentKPCR()->GDT;

  Ke386GetPageTableDirectory(cr3_);

  Tss = ExAllocatePool(NonPagedPool, sizeof(KTSS));
  TrapTss = ExAllocatePool(NonPagedPool, sizeof(KTSSNOIOPM));
  TrapStack = ExAllocatePool(NonPagedPool, MM_STACK_SIZE);

  Ki386TssArray[Id] = Tss;
  Ki386TrapTssArray[Id] = TrapTss;
  Ki386TrapStackArray[Id] = TrapStack;
  KeGetCurrentKPCR()->TSS = Tss;

  /* Initialize the boot TSS. */
  Tss->Esp0 = (ULONG)Ki386InitialStackArray[Id] + MM_STACK_SIZE; /* FIXME: - sizeof(FX_SAVE_AREA)? */
  Tss->Ss0 = KGDT_R0_DATA;
  Tss->IoMapBase = 0xFFFF; /* No i/o bitmap */
  Tss->IoBitmap[8192] = 0xFF;
  Tss->Ldt = 0;

  /*
   * Initialize a descriptor for the TSS
   */
  base = (ULONG)Tss;
  length = sizeof(KTSS) - 1;

  Gdt[(KGDT_TSS / 2) + 0] = (USHORT)(length & 0xFFFF);
  Gdt[(KGDT_TSS / 2) + 1] = (USHORT)(base & 0xFFFF);
  Gdt[(KGDT_TSS / 2) + 2] = (USHORT)(((base & 0xFF0000) >> 16) | 0x8900);
  Gdt[(KGDT_TSS / 2) + 3] = (USHORT)(((length & 0xF0000) >> 16) |
    ((base & 0xFF000000) >> 16));

  /* Initialize the TSS used for handling double faults. */
  TrapTss->Eflags = 0;
  TrapTss->Esp0 = ((ULONG)TrapStack + MM_STACK_SIZE); /* FIXME: - sizeof(FX_SAVE_AREA)? */
  TrapTss->Ss0 = KGDT_R0_DATA;
  TrapTss->Esp = ((ULONG)TrapStack + MM_STACK_SIZE); /* FIXME: - sizeof(FX_SAVE_AREA)? */
  TrapTss->Cs = KGDT_R0_CODE;
  TrapTss->Eip = (ULONG)KiTrap8;
  TrapTss->Ss = KGDT_R0_DATA;
  TrapTss->Ds = KGDT_R0_DATA;
  TrapTss->Es = KGDT_R0_DATA;
  TrapTss->Fs = KGDT_R0_PCR;
  TrapTss->IoMapBase = 0xFFFF; /* No i/o bitmap */
  TrapTss->IoBitmap[0] = 0xFF;
  TrapTss->Ldt = 0;
  TrapTss->Cr3 = cr3_;

  /*
   * Initialize a descriptor for the trap TSS.
   */
  base = (ULONG)TrapTss;
  length = sizeof(KTSSNOIOPM) - 1;

  Gdt[(KGDT_DF_TSS / 2) + 0] = (USHORT)(length & 0xFFFF);
  Gdt[(KGDT_DF_TSS / 2) + 1] = (USHORT)(base & 0xFFFF);
  Gdt[(KGDT_DF_TSS / 2) + 2] = (USHORT)(((base & 0xFF0000) >> 16) | 0x8900);
  Gdt[(KGDT_DF_TSS / 2) + 3] = (USHORT)(((length & 0xF0000) >> 16) |
    ((base & 0xFF000000) >> 16));

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

VOID INIT_FUNCTION
Ki386BootInitializeTSS(VOID)
{
  ULONG cr3_;
  extern unsigned int trap_stack, trap_stack_top;
  unsigned int base, length;

  Ke386GetPageTableDirectory(cr3_);

  Ki386TssArray[0] = &KiBootTss;
  Ki386TrapTssArray[0] = &KiBootTrapTss;
  Ki386TrapStackArray[0] = (PVOID)trap_stack;
  Ki386InitialStackArray[0] = (PVOID)init_stack;

  /* Initialize the boot TSS. */
  KiBootTss.Esp0 = (ULONG)init_stack_top - sizeof(FX_SAVE_AREA);
  KiBootTss.Ss0 = KGDT_R0_DATA;
  //   KiBootTss.IoMapBase = FIELD_OFFSET(KTSS, IoBitmap);
  KiBootTss.IoMapBase = 0xFFFF; /* No i/o bitmap */
  KiBootTss.IoBitmap[8192] = 0xFF;
  KiBootTss.Ldt = KGDT_LDT;

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
  KiBootTrapTss.Eflags = 0;
  KiBootTrapTss.Esp0 = (ULONG)trap_stack_top; /* FIXME: - sizeof(FX_SAVE_AREA)? */
  KiBootTrapTss.Ss0 = KGDT_R0_DATA;
  KiBootTrapTss.Esp = (ULONG)trap_stack_top; /* FIXME: - sizeof(FX_SAVE_AREA)? */
  KiBootTrapTss.Cs = KGDT_R0_CODE;
  KiBootTrapTss.Eip = (ULONG)KiTrap8;
  KiBootTrapTss.Ss = KGDT_R0_DATA;
  KiBootTrapTss.Ds = KGDT_R0_DATA;
  KiBootTrapTss.Es = KGDT_R0_DATA;
  KiBootTrapTss.Fs = KGDT_R0_PCR;
  KiBootTrapTss.IoMapBase = 0xFFFF; /* No i/o bitmap */
  KiBootTrapTss.IoBitmap[0] = 0xFF;
  KiBootTrapTss.Ldt = 0x0;
  KiBootTrapTss.Cr3 = cr3_;

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





