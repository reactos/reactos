/*
 *  ReactOS kernel
 *  Copyright (C) 2004, 2005 ReactOS Team
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
/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS kernel
 * FILE:        hal/halx86/apic.c
 * PURPOSE:     
 * PROGRAMMER:  
 */

/* INCLUDE ***********************************************************************/

#include <hal.h>
#include <halfuncs.h> /* Not in PCH because only used for MP HAL */
#include <rtlfuncs.h> /* Not in PCH because only used for MP HAL */
#define NDEBUG
#include <debug.h>

/* GLOBALS ***********************************************************************/

ULONG CPUCount;					/* Total number of CPUs */
ULONG BootCPU;					/* Bootstrap processor */
ULONG OnlineCPUs;				/* Bitmask of online CPUs */
CPU_INFO CPUMap[MAX_CPU];			/* Map of all CPUs in the system */

#ifdef CONFIG_SMP
PULONG BIOSBase;				/* Virtual address of BIOS data segment */
PULONG CommonBase;				/* Virtual address of common area */
#endif

PULONG APICBase = (PULONG)APIC_DEFAULT_BASE;	/* Virtual address of local APIC */

ULONG APICMode;					/* APIC mode at startup */

/* For debugging */
ULONG lastregr[MAX_CPU];
ULONG lastvalr[MAX_CPU];
ULONG lastregw[MAX_CPU];
ULONG lastvalw[MAX_CPU];

#ifdef CONFIG_SMP
#include <pshpack1.h>
typedef struct _COMMON_AREA_INFO
{
   ULONG Stack;		    /* Location of AP stack */
   ULONG PageDirectory;	    /* Page directory for an AP */
   ULONG NtProcessStartup;  /* Kernel entry point for an AP */
   ULONG PaeModeEnabled;    /* PAE mode is enabled */
   ULONG Debug[16];	    /* For debugging */
} COMMON_AREA_INFO, *PCOMMON_AREA_INFO;
#include <poppack.h>
#endif

CHAR *APstart, *APend;

#define BIOS_AREA	0x0
#define COMMON_AREA	0x2000

#define HZ		(100)
#define APIC_DIVISOR	(16)

#define CMOS_READ(address) { \
   WRITE_PORT_UCHAR((PUCHAR)0x70, address)); \
   READ_PORT_UCHAR((PUCHAR)0x71)); \
}

#define CMOS_WRITE(address, value) { \
   WRITE_PORT_UCHAR((PUCHAR)0x70, address); \
   WRITE_PORT_UCHAR((PUCHAR)0x71, value); \
}

extern ULONG_PTR KernelBase;

/* FUNCTIONS *********************************************************************/

extern ULONG Read8254Timer(VOID);
extern VOID WaitFor8254Wraparound(VOID);
extern VOID MpsTimerInterrupt(VOID);
extern VOID MpsErrorInterrupt(VOID);
extern VOID MpsSpuriousInterrupt(VOID);
extern VOID MpsIpiInterrupt(VOID);

ULONG APICGetMaxLVT(VOID)
{
  ULONG tmp, ver, maxlvt;

  tmp = APICRead(APIC_VER);
  ver = GET_APIC_VERSION(tmp);
  /* 82489DXs do not report # of LVT entries. */
  maxlvt = APIC_INTEGRATED(ver) ? GET_APIC_MAXLVT(tmp) : 2;

  return maxlvt;
}

VOID APICClear(VOID)
{
  ULONG tmp, maxlvt;

  maxlvt = APICGetMaxLVT();

  /*
   * Careful: we have to set masks only first to deassert
   * any level-triggered sources.
   */

  if (maxlvt >= 3) 
  {
    tmp = ERROR_VECTOR;
    APICWrite(APIC_LVT3, tmp | APIC_LVT3_MASKED);
  }

  tmp = APICRead(APIC_LVTT);
  APICWrite(APIC_LVTT, tmp | APIC_LVT_MASKED);

  tmp = APICRead(APIC_LINT0);
  APICWrite(APIC_LINT0, tmp | APIC_LVT_MASKED);

  tmp = APICRead(APIC_LINT1);
  APICWrite(APIC_LINT1, tmp | APIC_LVT_MASKED);

  if (maxlvt >= 4) 
  {
    tmp = APICRead(APIC_LVTPC);
    APICWrite(APIC_LVTPC, tmp | APIC_LVT_MASKED);
  }
#if 0
  if (maxlvt >= 5)
  {
    tmp = APICRead(APIC_LVTTHMR);
    APICWrite(APIC_LVTTHMR, tmp | APIC_LVT_MASKED);
  }
#endif
  /*
   * Clean APIC state for other OSs:
   */
  APICWrite(APIC_LVTT, APIC_LVT_MASKED);
  APICWrite(APIC_LINT0, APIC_LVT_MASKED);
  APICWrite(APIC_LINT1, APIC_LVT_MASKED);

  if (maxlvt >= 3) 
  {
    APICWrite(APIC_LVT3, APIC_LVT3_MASKED);
  }

  if (maxlvt >= 4) 
  {
    APICWrite(APIC_LVTPC, APIC_LVT_MASKED);
  }
#if 0
  if (maxlvt >= 5) 
  {
    APICWrite(APIC_LVTTHMR, APIC_LVT_MASKED);
  }
#endif
}

/* Enable symetric I/O mode ie. connect the BSP's local APIC to INT and NMI lines */
VOID EnableApicMode(VOID)
{
   /*
    * Do not trust the local APIC being empty at bootup.
    */
   APICClear();

   WRITE_PORT_UCHAR((PUCHAR)0x22, 0x70);
   WRITE_PORT_UCHAR((PUCHAR)0x23, 0x01);
}

/* Disable symetric I/O mode ie. go to PIC mode */
__inline VOID DisableSMPMode(VOID)
{
   /*
    * Put the board back into PIC mode (has an effect
    * only on certain older boards).  Note that APIC
    * interrupts, including IPIs, won't work beyond
    * this point!  The only exception are INIT IPIs.
    */
   WRITE_PORT_UCHAR((PUCHAR)0x22, 0x70);
   WRITE_PORT_UCHAR((PUCHAR)0x23, 0x00);
}

VOID DumpESR(VOID)
{
  ULONG tmp;

  if (APICGetMaxLVT() > 3)
  {
    APICWrite(APIC_ESR, 0);
  }
  tmp = APICRead(APIC_ESR);
  DbgPrint("ESR %08x\n", tmp);
}


VOID APICDisable(VOID)
{
  ULONG tmp;

  APICClear();

  /*
   * Disable APIC (implies clearing of registers for 82489DX!).
   */
  tmp = APICRead(APIC_SIVR);
  tmp &= ~APIC_SIVR_ENABLE;
  APICWrite(APIC_SIVR, tmp);
}

static VOID APICDumpBit(ULONG base)
{
	ULONG v, i, j;

	DbgPrint("0123456789abcdef0123456789abcdef\n");
	for (i = 0; i < 8; i++) 
	{
		v = APICRead(base + i*0x10);
		for (j = 0; j < 32; j++) 
		{
			if (v & (1<<j))
				DbgPrint("1");
			else
				DbgPrint("0");
		}
		DbgPrint("\n");
	}
}


VOID APICDump(VOID)
/*
 * Dump the contents of the local APIC registers
 */
{
  ULONG v, ver, maxlvt;
  ULONG r1, r2, w1, w2;
  ULONG CPU = ThisCPU();


  r1 = lastregr[CPU];
  r2 = lastvalr[CPU];
  w1 = lastregw[CPU];
  w2 = lastvalw[CPU];

  DbgPrint("\nPrinting local APIC contents on CPU(%d):\n", ThisCPU());
  v = APICRead(APIC_ID);
  DbgPrint("... ID     : %08x (%01x) ", v, GET_APIC_ID(v));
  v = APICRead(APIC_VER);
  DbgPrint("... VERSION: %08x\n", v);
  ver = GET_APIC_VERSION(v);
  maxlvt = APICGetMaxLVT();

  v = APICRead(APIC_TPR);
  DbgPrint("... TPR    : %08x (%02x)", v, v & ~0);

  if (APIC_INTEGRATED(ver)) 
  {
     /* !82489DX */
     v = APICRead(APIC_APR);
     DbgPrint("... APR    : %08x (%02x)\n", v, v & ~0);
     v = APICRead(APIC_PPR);
     DbgPrint("... PPR    : %08x\n", v);
  }

  v = APICRead(APIC_EOI);
  DbgPrint("... EOI    : %08x  !  ", v);
  v = APICRead(APIC_LDR);
  DbgPrint("... LDR    : %08x\n", v);
  v = APICRead(APIC_DFR);
  DbgPrint("... DFR    : %08x  !  ", v);
  v = APICRead(APIC_SIVR);
  DbgPrint("... SIVR   : %08x\n", v);

  if (0)
  {
  	DbgPrint("... ISR field:\n");
  	APICDumpBit(APIC_ISR);
  	DbgPrint("... TMR field:\n");
  	APICDumpBit(APIC_TMR);
  	DbgPrint("... IRR field:\n");
  	APICDumpBit(APIC_IRR);
  }

  if (APIC_INTEGRATED(ver)) 
  {
     /* !82489DX */
     if (maxlvt > 3)		
     {
	/* Due to the Pentium erratum 3AP. */
	APICWrite(APIC_ESR, 0);
     }
     v = APICRead(APIC_ESR);
     DbgPrint("... ESR    : %08x\n", v);
  }
 
  v = APICRead(APIC_ICR0);
  DbgPrint("... ICR0   : %08x  !  ", v);
  v = APICRead(APIC_ICR1);
  DbgPrint("... ICR1   : %08x  !  ", v);

  v = APICRead(APIC_LVTT);
  DbgPrint("... LVTT   : %08x\n", v);

  if (maxlvt > 3) 
  {
     /* PC is LVT#4. */
     v = APICRead(APIC_LVTPC);
     DbgPrint("... LVTPC  : %08x  !  ", v);
  }
  v = APICRead(APIC_LINT0);
  DbgPrint("... LINT0  : %08x  !  ", v);
  v = APICRead(APIC_LINT1);
  DbgPrint("... LINT1  : %08x\n", v);

  if (maxlvt > 2) 
  {
     v = APICRead(APIC_LVT3);
     DbgPrint("... LVT3   : %08x\n", v);
  }

  v = APICRead(APIC_ICRT);
  DbgPrint("... ICRT   : %08x  !  ", v);
  v = APICRead(APIC_CCRT);
  DbgPrint("... CCCT   : %08x  !  ", v);
  v = APICRead(APIC_TDCR);
  DbgPrint("... TDCR   : %08x\n", v);
  DbgPrint("\n");
  DbgPrint("Last register read (offset): 0x%08X\n", r1);
  DbgPrint("Last register read (value): 0x%08X\n", r2);
  DbgPrint("Last register written (offset): 0x%08X\n", w1);
  DbgPrint("Last register written (value): 0x%08X\n", w2);
  DbgPrint("\n");
}

BOOLEAN VerifyLocalAPIC(VOID)
{
   SIZE_T reg0, reg1;
   ULONG l, h;
   /* The version register is read-only in a real APIC */
   reg0 = APICRead(APIC_VER);
   DPRINT1("Getting VERSION: %x\n", reg0);
   APICWrite(APIC_VER, reg0 ^ APIC_VER_MASK);
   reg1 = APICRead(APIC_VER);
   DPRINT1("Getting VERSION: %x\n", reg1);

   /*
    * The two version reads above should print the same
    * numbers.  If the second one is different, then we
    * poke at a non-APIC.
    */

   if (reg1 != reg0)
   {
      return FALSE;
   }

   /*
    * Check if the version looks reasonably.
    */
   reg1 = GET_APIC_VERSION(reg0);
   if (reg1 == 0x00 || reg1 == 0xff)
   {
      return FALSE;
   }
   reg1 = APICGetMaxLVT();
   if (reg1 < 0x02 || reg1 == 0xff)
   {
      return FALSE;
   }

   /*
    * The ID register is read/write in a real APIC.
    */
   reg0 = APICRead(APIC_ID);
   DPRINT1("Getting ID: %x\n", reg0);
   APICWrite(APIC_ID, reg0 ^ APIC_ID_MASK);
   reg1 = APICRead(APIC_ID);
   DPRINT1("Getting ID: %x\n", reg1);
   APICWrite(APIC_ID, reg0);
   if (reg1 != (reg0 ^ APIC_ID_MASK))
   {
      return FALSE;
   }

   Ke386Rdmsr(0x1b /*MSR_IA32_APICBASE*/, l, h);

   if (!(l & /*MSR_IA32_APICBASE_ENABLE*/(1<<11))) 
   {
      DPRINT1("Local APIC disabled by BIOS -- reenabling.\n");
      l &= ~/*MSR_IA32_APICBASE_BASE*/(1<<11);
      l |= /*MSR_IA32_APICBASE_ENABLE | APIC_DEFAULT_PHYS_BASE*/(1<<11)|0xfee00000;
      Ke386Wrmsr(0x1b/*MSR_IA32_APICBASE*/, l, h);
   }

    

   return TRUE;
}

#ifdef CONFIG_SMP
VOID APICSendIPI(ULONG Target, ULONG Mode)
{
   ULONG tmp, i, flags;

   /* save flags and disable interrupts */
   Ke386SaveFlags(flags);
   _disable();

   /* Wait up to 100ms for the APIC to become ready */
   for (i = 0; i < 10000; i++) 
   {
      tmp = APICRead(APIC_ICR0);
      /* Check Delivery Status */
      if ((tmp & APIC_ICR0_DS) == 0)
         break;
      KeStallExecutionProcessor(10);
   }

   if (i == 10000) 
   {
      DPRINT1("CPU(%d) Previous IPI was not delivered after 100ms.\n", ThisCPU());
   }

   /* Setup the APIC to deliver the IPI */
   DPRINT("%08x %x\n", SET_APIC_DEST_FIELD(Target), Target);
   APICWrite(APIC_ICR1, SET_APIC_DEST_FIELD(Target));

   if (Target == APIC_TARGET_SELF) 
   {
      Mode |= APIC_ICR0_DESTS_SELF;
   } 
   else if (Target == APIC_TARGET_ALL) 
   {
      Mode |= APIC_ICR0_DESTS_ALL;
   } 
   else if (Target == APIC_TARGET_ALL_BUT_SELF) 
   {
      Mode |= APIC_ICR0_DESTS_ALL_BUT_SELF;
   } 
   else 
   {
      Mode |= APIC_ICR0_DESTS_FIELD;
   }

   /* Now, fire off the IPI */
   APICWrite(APIC_ICR0, Mode);

   /* Wait up to 100ms for the APIC to become ready */
   for (i = 0; i < 10000; i++) 
   {
      tmp = APICRead(APIC_ICR0);
      /* Check Delivery Status */
      if ((tmp & APIC_ICR0_DS) == 0)
         break;
      KeStallExecutionProcessor(10);
   }

   if (i == 10000) 
   {
      DPRINT1("CPU(%d) Current IPI was not delivered after 100ms.\n", ThisCPU());
   }
   Ke386RestoreFlags(flags);
}
#endif

VOID APICSetup(VOID)
{
   ULONG CPU, tmp;

   CPU = ThisCPU();

//   APICDump();
  
   DPRINT1("CPU%d:\n", CPU);
   DPRINT1("  Physical APIC id: %d\n", GET_APIC_ID(APICRead(APIC_ID)));
   DPRINT1("  Logical APIC id:  %d\n", GET_APIC_LOGICAL_ID(APICRead(APIC_LDR)));
   DPRINT1("%08x %08x %08x\n", APICRead(APIC_ID), APICRead(APIC_LDR), APICRead(APIC_DFR));

   /*
    * Intel recommends to set DFR, LDR and TPR before enabling
    * an APIC.  See e.g. "AP-388 82489DX User's Manual" (Intel
    * document number 292116).  So here it goes...
    */

   /*
    * Put the APIC into flat delivery mode.
    * Must be "all ones" explicitly for 82489DX.
    */
   APICWrite(APIC_DFR, 0xFFFFFFFF);

   /*
    * Set up the logical destination ID.
    */
   tmp = APICRead(APIC_LDR);
   tmp &= ~APIC_LDR_MASK;
   /* 
    * FIXME:
    *   This works only up to 8 CPU's
    */
   tmp |= (1 << (KeGetCurrentProcessorNumber() + 24));
   APICWrite(APIC_LDR, tmp);


   DPRINT1("CPU%d:\n", CPU);
   DPRINT1("  Physical APIC id: %d\n", GET_APIC_ID(APICRead(APIC_ID)));
   DPRINT1("  Logical APIC id:  %d\n", GET_APIC_LOGICAL_ID(APICRead(APIC_LDR)));
   DPRINT1("%08x %08x %08x\n", APICRead(APIC_ID), APICRead(APIC_LDR), APICRead(APIC_DFR));
   DPRINT1("%d\n", CPUMap[CPU].APICId);

   /* Accept only higher interrupts */
   APICWrite(APIC_TPR, 0xef);

   /* Enable local APIC */
   tmp = APICRead(APIC_SIVR);
   tmp &= ~0xff;
   tmp |= APIC_SIVR_ENABLE;

#if 0
   tmp &= ~APIC_SIVR_FOCUS;
#else
   tmp |= APIC_SIVR_FOCUS;
#endif

  /* Set spurious interrupt vector */
  tmp |= SPURIOUS_VECTOR;
  APICWrite(APIC_SIVR, tmp);

  /*
   * Set up LVT0, LVT1:
   *
   * set up through-local-APIC on the BP's LINT0. This is not
   * strictly necessery in pure symmetric-IO mode, but sometimes
   * we delegate interrupts to the 8259A.
   */
  tmp = APICRead(APIC_LINT0) & APIC_LVT_MASKED;
  if (CPU == BootCPU && (APICMode == amPIC || !tmp)) 
  {
     tmp = APIC_DM_EXTINT;
     DPRINT1("enabled ExtINT on CPU#%d\n", CPU);
  } 
  else 
  {
     tmp = APIC_DM_EXTINT | APIC_LVT_MASKED;
     DPRINT1("masked ExtINT on CPU#%d\n", CPU);
  }
  APICWrite(APIC_LINT0, tmp);

  /*
   * Only the BSP should see the LINT1 NMI signal, obviously.
   */
  if (CPU == BootCPU)
  {
     tmp = APIC_DM_NMI;
  }
  else
  {
     tmp = APIC_DM_NMI | APIC_LVT_MASKED;
  }
  if (!APIC_INTEGRATED(CPUMap[CPU].APICVersion)) 
  {
     /* 82489DX */
     tmp |= APIC_LVT_LEVEL_TRIGGER;
  }
  APICWrite(APIC_LINT1, tmp);

  if (APIC_INTEGRATED(CPUMap[CPU].APICVersion)) 
  {	
     /* !82489DX */
     if (APICGetMaxLVT() > 3) 
     {
	/* Due to the Pentium erratum 3AP */
	APICWrite(APIC_ESR, 0);
     }

     tmp = APICRead(APIC_ESR);
     DPRINT("ESR value before enabling vector: 0x%X\n", tmp);

     /* Enable sending errors */
     tmp = ERROR_VECTOR;
     APICWrite(APIC_LVT3, tmp);

     /*
      * Spec says clear errors after enabling vector
      */
     if (APICGetMaxLVT() > 3)
     {
        APICWrite(APIC_ESR, 0);
     }
     tmp = APICRead(APIC_ESR);
     DPRINT("ESR value after enabling vector: 0x%X\n", tmp);
  }
}
#ifdef CONFIG_SMP
VOID APICSyncArbIDs(VOID)
{
   ULONG i, tmp;

   /* Wait up to 100ms for the APIC to become ready */
   for (i = 0; i < 10000; i++) 
   {
      tmp = APICRead(APIC_ICR0);
      /* Check Delivery Status */
      if ((tmp & APIC_ICR0_DS) == 0)
         break;
      KeStallExecutionProcessor(10);
   }

   if (i == 10000) 
   {
      DPRINT("CPU(%d) APIC busy for 100ms.\n", ThisCPU());
   }

   DPRINT("Synchronizing Arb IDs.\n");
   APICWrite(APIC_ICR0, APIC_ICR0_DESTS_ALL | APIC_ICR0_LEVEL | APIC_DM_INIT);
}
#endif

VOID MpsErrorHandler(VOID)
{
  ULONG tmp1, tmp2;

  APICDump();

  tmp1 = APICRead(APIC_ESR);
  APICWrite(APIC_ESR, 0);
  tmp2 = APICRead(APIC_ESR);
  DPRINT1("APIC error on CPU(%d) ESR(%x)(%x)\n", ThisCPU(), tmp1, tmp2);

  /*
   * Acknowledge the interrupt
   */
  APICSendEOI();

  /* Here is what the APIC error bits mean:
   *   0: Send CS error
   *   1: Receive CS error
   *   2: Send accept error
   *   3: Receive accept error
   *   4: Reserved
   *   5: Send illegal vector
   *   6: Received illegal vector
   *   7: Illegal register address
   */
  DPRINT1("APIC error on CPU(%d) ESR(%x)(%x)\n", ThisCPU(), tmp1, tmp2);
  for (;;);
}

VOID MpsSpuriousHandler(VOID)
{
  ULONG tmp;

  DPRINT("Spurious interrupt on CPU(%d)\n", ThisCPU());
  
  tmp = APICRead(APIC_ISR + ((SPURIOUS_VECTOR & ~0x1f) >> 1));
  if (tmp & (1 << (SPURIOUS_VECTOR & 0x1f)))
  {
      APICSendEOI();
      return;
  }
#if 0
  /* No need to send EOI here */
  APICDump();
#endif
}

#ifdef CONFIG_SMP
VOID MpsIpiHandler(VOID)
{
   KIRQL oldIrql;

   HalBeginSystemInterrupt(IPI_LEVEL,
                           IPI_VECTOR, 
			   &oldIrql);
   _enable();
#if 0
   DbgPrint("(%s:%d) MpsIpiHandler on CPU%d, current irql is %d\n", 
            __FILE__,__LINE__, KeGetCurrentProcessorNumber(), KeGetCurrentIrql());
#endif   

   KiIpiServiceRoutine(NULL, NULL);

#if 0
   DbgPrint("(%s:%d) MpsIpiHandler on CPU%d done\n", __FILE__,__LINE__, KeGetCurrentProcessorNumber());
#endif

   _disable();
   HalEndSystemInterrupt(oldIrql, 0);
}
#endif

VOID
MpsIRQTrapFrameToTrapFrame(PKIRQ_TRAPFRAME IrqTrapFrame,
			   PKTRAP_FRAME TrapFrame)
{
#ifdef _M_AMD64
    UNIMPLEMENTED;
#else
   TrapFrame->SegGs     = (USHORT)IrqTrapFrame->Gs;
   TrapFrame->SegFs     = (USHORT)IrqTrapFrame->Fs;
   TrapFrame->SegEs     = (USHORT)IrqTrapFrame->Es;
   TrapFrame->SegDs     = (USHORT)IrqTrapFrame->Ds;
   TrapFrame->Eax    = IrqTrapFrame->Eax;
   TrapFrame->Ecx    = IrqTrapFrame->Ecx;
   TrapFrame->Edx    = IrqTrapFrame->Edx;
   TrapFrame->Ebx    = IrqTrapFrame->Ebx;
   TrapFrame->HardwareEsp    = IrqTrapFrame->Esp;
   TrapFrame->Ebp    = IrqTrapFrame->Ebp;
   TrapFrame->Esi    = IrqTrapFrame->Esi;
   TrapFrame->Edi    = IrqTrapFrame->Edi;
   TrapFrame->Eip    = IrqTrapFrame->Eip;
   TrapFrame->SegCs     = IrqTrapFrame->Cs;
   TrapFrame->EFlags = IrqTrapFrame->Eflags;
#endif
}

VOID
MpsTimerHandler(ULONG Vector, PKIRQ_TRAPFRAME Trapframe)
{
   KIRQL oldIrql;
   KTRAP_FRAME KernelTrapFrame;
#if 0
   ULONG CPU;
   static ULONG Count[MAX_CPU] = {0,};
#endif
   HalBeginSystemInterrupt(LOCAL_TIMER_VECTOR, 
                           PROFILE_LEVEL, 
			   &oldIrql);
   _enable();

#if 0
   CPU = ThisCPU();
   if ((Count[CPU] % 100) == 0)
   {
     DbgPrint("(%s:%d) MpsTimerHandler on CPU%d, irql = %d, epi = %x, KPCR = %x\n", __FILE__, __LINE__, CPU, oldIrql,Trapframe->Eip, KeGetPcr());
   }
   Count[CPU]++;
#endif

   /* FIXME: SMP is totally broken */
   MpsIRQTrapFrameToTrapFrame(Trapframe, &KernelTrapFrame);
   if (KeGetCurrentProcessorNumber() == 0)
   {
      //KeUpdateSystemTime(&KernelTrapFrame, oldIrql);
   }
   else
   {
      //KeUpdateRunTime(&KernelTrapFrame, oldIrql);
   }

   _disable();
   HalEndSystemInterrupt (oldIrql, 0);
}

VOID APICSetupLVTT(ULONG ClockTicks)
{
   ULONG tmp;

   tmp = GET_APIC_VERSION(APICRead(APIC_VER));
   if (!APIC_INTEGRATED(tmp))
   {
      tmp = SET_APIC_TIMER_BASE(APIC_TIMER_BASE_DIV) | APIC_LVT_PERIODIC | LOCAL_TIMER_VECTOR;
   }
   else
   {
      /* Periodic timer */
      tmp = APIC_LVT_PERIODIC | LOCAL_TIMER_VECTOR;
   }
   APICWrite(APIC_LVTT, tmp);

   tmp = APICRead(APIC_TDCR);
   tmp &= ~(APIC_TDCR_1 | APIC_TIMER_BASE_DIV);
   tmp |= APIC_TDCR_16;
   APICWrite(APIC_TDCR, tmp);
   APICWrite(APIC_ICRT, ClockTicks / APIC_DIVISOR);
}

VOID 
APICCalibrateTimer(ULONG CPU)
{
   ULARGE_INTEGER t1, t2;
   LONG tt1, tt2;
   BOOLEAN TSCPresent;

   DPRINT("Calibrating APIC timer for CPU %d\n", CPU);

   APICSetupLVTT(1000000000);

   TSCPresent = KeGetCurrentPrcb()->FeatureBits & KF_RDTSC ? TRUE : FALSE;

   /*
    * The timer chip counts down to zero. Let's wait
    * for a wraparound to start exact measurement:
    * (the current tick might have been already half done)
    */
   //WaitFor8254Wraparound();

   /*
    * We wrapped around just now. Let's start
    */
   if (TSCPresent)
   {
      t1.QuadPart = (LONGLONG)__rdtsc();
   }
   tt1 = APICRead(APIC_CCRT);

   //WaitFor8254Wraparound();


   tt2 = APICRead(APIC_CCRT);
   if (TSCPresent)
   {
      t2.QuadPart = (LONGLONG)__rdtsc();
      CPUMap[CPU].CoreSpeed = (HZ * (t2.QuadPart - t1.QuadPart));
      DPRINT("CPU clock speed is %ld.%04ld MHz.\n",
	     CPUMap[CPU].CoreSpeed/1000000,
	     CPUMap[CPU].CoreSpeed%1000000);
      KeGetCurrentPrcb()->MHz = CPUMap[CPU].CoreSpeed/1000000;
   }

   CPUMap[CPU].BusSpeed = (HZ * (long)(tt1 - tt2) * APIC_DIVISOR);

   /* Setup timer for normal operation */
// APICSetupLVTT((CPUMap[CPU].BusSpeed / 1000000) * 100);    // 100ns
   APICSetupLVTT((CPUMap[CPU].BusSpeed / 1000000) * 10000);  // 10ms
// APICSetupLVTT((CPUMap[CPU].BusSpeed / 1000000) * 100000); // 100ms

   DPRINT("Host bus clock speed is %ld.%04ld MHz.\n",
	  CPUMap[CPU].BusSpeed/1000000,
	  CPUMap[CPU].BusSpeed%1000000);
}

VOID 
SetInterruptGate(ULONG index, ULONG_PTR address)
{
#ifdef _M_AMD64
  KIDTENTRY64 *idt;

  idt = &KeGetPcr()->IdtBase[index];

  idt->OffsetLow = address & 0xffff;
  idt->Selector = KGDT_64_R0_CODE;
  idt->IstIndex = 0;
  idt->Reserved0 = 0;
  idt->Type = 0x0e;
  idt->Dpl = 0;
  idt->Present = 1;
  idt->OffsetMiddle = (address >> 16) & 0xffff;
  idt->OffsetHigh = address >> 32;
  idt->Reserved1 = 0;
  idt->Alignment = 0;
#else
  KIDTENTRY *idt;
  KIDT_ACCESS Access;

  /* Set the IDT Access Bits */
  Access.Reserved = 0;
  Access.Present = 1;
  Access.Dpl = 0; /* Kernel-Mode */
  Access.SystemSegmentFlag = 0;
  Access.SegmentType = I386_INTERRUPT_GATE;
  
  idt = (KIDTENTRY*)((ULONG)KeGetPcr()->IDT + index * sizeof(KIDTENTRY));
  idt->Offset = address & 0xffff;
  idt->Selector = KGDT_R0_CODE;
  idt->Access = Access.Value;
  idt->ExtendedOffset = address >> 16;
#endif
}

VOID HaliInitBSP(VOID)
{
#ifdef CONFIG_SMP
   PUSHORT ps;
#endif

   static BOOLEAN BSPInitialized = FALSE;

   /* Only initialize the BSP once */
   if (BSPInitialized)
   {
      ASSERT(FALSE);
      return;
   }

   BSPInitialized = TRUE;

   /* Setup interrupt handlers */
   SetInterruptGate(LOCAL_TIMER_VECTOR, (ULONG_PTR)MpsTimerInterrupt);
   SetInterruptGate(ERROR_VECTOR, (ULONG_PTR)MpsErrorInterrupt);
   SetInterruptGate(SPURIOUS_VECTOR, (ULONG_PTR)MpsSpuriousInterrupt);
#ifdef CONFIG_SMP
   SetInterruptGate(IPI_VECTOR, (ULONG_PTR)MpsIpiInterrupt);
#endif
   DPRINT1("APIC is mapped at 0x%p\n", (PVOID)APICBase);

   if (VerifyLocalAPIC()) 
   {
      DPRINT("APIC found\n");
   } 
   else 
   {
      DPRINT1("No APIC found\n");
      ASSERT(FALSE);
   }

   if (APICMode == amPIC) 
   {
      EnableApicMode();
   }

   APICSetup();

#ifdef CONFIG_SMP
   /* BIOS data segment */
   BIOSBase = (PULONG)BIOS_AREA;
   
   /* Area for communicating with the APs */
   CommonBase = (PULONG)COMMON_AREA;
 
   /* Copy bootstrap code to common area */
   memcpy((PVOID)((ULONG)CommonBase + PAGE_SIZE),
	  &APstart,
	  (ULONG)&APend - (ULONG)&APstart + 1);

   /* Set shutdown code */
   CMOS_WRITE(0xF, 0xA);

   /* Set warm reset vector */
   ps = (PUSHORT)((ULONG)BIOSBase + 0x467);
   *ps = (COMMON_AREA + PAGE_SIZE) & 0xF;
 
   ps = (PUSHORT)((ULONG)BIOSBase + 0x469);
   *ps = (COMMON_AREA + PAGE_SIZE) >> 4;
#endif

   /* Calibrate APIC timer */
   APICCalibrateTimer(BootCPU);
}

#ifdef CONFIG_SMP
VOID
HaliStartApplicationProcessor(ULONG Cpu, ULONG Stack)
{
   ULONG tmp, maxlvt;
   PCOMMON_AREA_INFO Common;
   ULONG StartupCount;
   ULONG i, j;
   ULONG DeliveryStatus = 0;
   ULONG AcceptStatus = 0;

   if (Cpu >= MAX_CPU ||
       Cpu >= CPUCount ||
       OnlineCPUs & (1 << Cpu))
   {
     ASSERT(FALSE);
   }
   DPRINT1("Attempting to boot CPU %d\n", Cpu);

   /* Send INIT IPI */

   APICSendIPI(Cpu, APIC_DM_INIT|APIC_ICR0_LEVEL_ASSERT);
 
   KeStallExecutionProcessor(200);

   /* Deassert INIT */

   APICSendIPI(Cpu, APIC_DM_INIT|APIC_ICR0_LEVEL_DEASSERT);

   if (APIC_INTEGRATED(CPUMap[Cpu].APICVersion)) 
   {
      /* Clear APIC errors */
      APICWrite(APIC_ESR, 0);
      tmp = (APICRead(APIC_ESR) & APIC_ESR_MASK);
   }

   Common = (PCOMMON_AREA_INFO)CommonBase;

   /* Write the location of the AP stack */
   Common->Stack = (ULONG)Stack;
   /* Write the page directory page */
   Common->PageDirectory = __readcr3();
   /* Write the kernel entry point */
   Common->NtProcessStartup = (ULONG_PTR)RtlImageNtHeader((PVOID)KernelBase)->OptionalHeader.AddressOfEntryPoint + KernelBase;
   /* Write the state of the mae mode */
   Common->PaeModeEnabled = __readcr4() & CR4_PAE ? 1 : 0;

   DPRINT1("%x %x %x %x\n", Common->Stack, Common->PageDirectory, Common->NtProcessStartup, Common->PaeModeEnabled);

   DPRINT("Cpu %d got stack at 0x%X\n", Cpu, Common->Stack);
#if 0
   for (j = 0; j < 16; j++) 
   {
      Common->Debug[j] = 0;
   }
#endif

   maxlvt = APICGetMaxLVT();

   /* Is this a local APIC or an 82489DX? */
   StartupCount = (APIC_INTEGRATED(CPUMap[Cpu].APICVersion)) ? 2 : 0;

   for (i = 1; i <= StartupCount; i++)
   {
      /* It's a local APIC, so send STARTUP IPI */
      DPRINT("Sending startup signal %d\n", i);
      /* Clear errors */
      APICWrite(APIC_ESR, 0);
      APICRead(APIC_ESR);

      APICSendIPI(Cpu, APIC_DM_STARTUP | ((COMMON_AREA + PAGE_SIZE) >> 12)|APIC_ICR0_LEVEL_DEASSERT);

      /* Wait up to 10ms for IPI to be delivered */
      j = 0;
      do 
      {
         KeStallExecutionProcessor(10);

         /* Check Delivery Status */
         DeliveryStatus = APICRead(APIC_ICR0) & APIC_ICR0_DS;

         j++;
      } while ((DeliveryStatus) && (j < 1000));

      KeStallExecutionProcessor(200);

      /*
       * Due to the Pentium erratum 3AP.
       */
      if (maxlvt > 3) 
      {
        APICRead(APIC_SIVR);
        APICWrite(APIC_ESR, 0);
      }

      AcceptStatus = APICRead(APIC_ESR) & APIC_ESR_MASK;

      if (DeliveryStatus || AcceptStatus) 
      {
         break;
      }
   }

   if (DeliveryStatus) 
   {
      DPRINT("STARTUP IPI for CPU %d was never delivered.\n", Cpu);
   }

   if (AcceptStatus) 
   {
      DPRINT("STARTUP IPI for CPU %d was never accepted.\n", Cpu);
   }

   if (!(DeliveryStatus || AcceptStatus)) 
   {

      /* Wait no more than 5 seconds for processor to boot */
      DPRINT("Waiting for 5 seconds for CPU %d to boot\n", Cpu);

      /* Wait no more than 5 seconds */
      for (j = 0; j < 50000; j++) 
      {
         if (CPUMap[Cpu].Flags & CPU_ENABLED)
	 {
            break;
	 }
         KeStallExecutionProcessor(100);
      }
   }

   if (CPUMap[Cpu].Flags & CPU_ENABLED) 
   {
      DbgPrint("CPU %d is now running\n", Cpu);
   } 
   else 
   {
      DbgPrint("Initialization of CPU %d failed\n", Cpu);
   }

#if 0
   DPRINT("Debug bytes are:\n");

   for (j = 0; j < 4; j++) 
   {
      DPRINT("0x%08X 0x%08X 0x%08X 0x%08X.\n",
             Common->Debug[j*4+0],
             Common->Debug[j*4+1],
             Common->Debug[j*4+2],
             Common->Debug[j*4+3]);
   }

#endif
}

#endif

/* EOF */
