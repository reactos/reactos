/* $Id: fpu.c,v 1.19 2004/11/27 23:50:26 hbirr Exp $
 *
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 * FILE:            ntoskrnl/ke/i386/fpu.c
 * PURPOSE:         Handles the FPU
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <roscfg.h>
#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* DEFINES *******************************************************************/

#define EXCEPTION_FLT_DENORMAL_OPERAND          (0xc000008dL)
#define EXCEPTION_FLT_DIVIDE_BY_ZERO            (0xc000008eL)
#define EXCEPTION_FLT_INEXACT_RESULT            (0xc000008fL)
#define EXCEPTION_FLT_INVALID_OPERATION         (0xc0000090L)
#define EXCEPTION_FLT_OVERFLOW                  (0xc0000091L)
#define EXCEPTION_FLT_STACK_CHECK               (0xc0000092L)
#define EXCEPTION_FLT_UNDERFLOW                 (0xc0000093L)
#define EXCEPTION_FLT_MULTIPLE_FAULTS           (0xC00002B4L)
#define EXCEPTION_FLT_MULTIPLE_TRAPS            (0xC00002B5L)

/* x87 Status Word exception flags */
#define X87_SW_IE       (1<<0)   /* Invalid Operation */
#define X87_SW_DE       (1<<1)   /* Denormalized Operand */
#define X87_SW_ZE       (1<<2)   /* Zero Devide */
#define X87_SW_OE       (1<<3)   /* Overflow */
#define X87_SW_UE       (1<<4)   /* Underflow */
#define X87_SW_PE       (1<<5)   /* Precision */
#define X87_SW_SE       (1<<6)   /* Stack Fault */

#define X87_SW_ES       (1<<7)   /* Error Summary */

/* MXCSR exception flags */
#define MXCSR_IE        (1<<0)   /* Invalid Operation */
#define MXCSR_DE        (1<<1)   /* Denormalized Operand */
#define MXCSR_ZE        (1<<2)   /* Zero Devide */
#define MXCSR_OE        (1<<3)   /* Overflow */
#define MXCSR_UE        (1<<4)   /* Underflow */
#define MXCSR_PE        (1<<5)   /* Precision */
#define MXCSR_DAZ       (1<<6)   /* Denormals Are Zeros (P4 only) */

/* GLOBALS *******************************************************************/

ULONG HardwareMathSupport = 0;
static ULONG MxcsrFeatureMask = 0, XmmSupport = 0;
ULONG FxsrSupport = 0; /* used by Ki386ContextSwitch for MP */

/* FUNCTIONS *****************************************************************/

STATIC USHORT
KiTagWordFnsaveToFxsave(USHORT TagWord)
{
  INT tmp;

  /*
   * Converts the tag-word. 11 (Empty) is converted into 0, everything else into 1
   */
  tmp = ~TagWord; /* Empty is now 00, any 2 bits containing 1 mean valid */
  tmp = (tmp | (tmp >> 1)) & 0x5555; /* 0V0V0V0V0V0V0V0V */
  tmp = (tmp | (tmp >> 1)) & 0x3333; /* 00VV00VV00VV00VV */
  tmp = (tmp | (tmp >> 2)) & 0x0f0f; /* 0000VVVV0000VVVV */
  tmp = (tmp | (tmp >> 4)) & 0x00ff; /* 00000000VVVVVVVV */

  return tmp;
}

STATIC USHORT
KiTagWordFxsaveToFnsave(PFXSAVE_FORMAT FxSave)
{
  USHORT TagWord = 0;
  UCHAR Tag;
  INT i;
  struct FPREG { USHORT Significand[4]; USHORT Exponent; } *FpReg;
  
  for (i = 0; i < 8; i++)
    {
      if (FxSave->TagWord & (1 << i)) /* valid */
        {
          FpReg = (struct FPREG *)(FxSave->RegisterArea + (i * 16));
          switch (FpReg->Exponent & 0x00007fff)
            {
              case 0x0000:
                if (FpReg->Significand[0] == 0 && FpReg->Significand[1] == 0 &&
                    FpReg->Significand[2] == 0 && FpReg->Significand[3] == 0)
                  {
                    Tag = 1;  /* Zero */
                  }
                else
                  {
                    Tag = 2;  /* Special */
                  }
                break;
            
              case 0x7fff:
                Tag = 2;      /* Special */
                break;
                
              default:
                if (FpReg->Significand[3] & 0x00008000)
                  {
                    Tag = 0;  /* Valid */
                  }
                else
                  {
                    Tag = 2;  /* Special */
                  }
                break;
            }
        }
      else /* empty */
        {
          Tag = 3;
        }
      TagWord |= Tag << (i * 2);
    }

  return TagWord;
}

STATIC VOID
KiFnsaveToFxsaveFormat(PFXSAVE_FORMAT FxSave, CONST PFNSAVE_FORMAT FnSave)
{
  INT i;

  FxSave->ControlWord = (USHORT)FnSave->ControlWord;
  FxSave->StatusWord = (USHORT)FnSave->StatusWord;
  FxSave->TagWord = KiTagWordFnsaveToFxsave((USHORT)FnSave->TagWord);
  FxSave->ErrorOpcode = (USHORT)(FnSave->ErrorSelector >> 16);
  FxSave->ErrorOffset = FnSave->ErrorOffset;
  FxSave->ErrorSelector = FnSave->ErrorSelector & 0x0000ffff;
  FxSave->DataOffset = FnSave->DataOffset;
  FxSave->DataSelector = FnSave->DataSelector & 0x0000ffff;
  if (XmmSupport)
    FxSave->MXCsr = 0x00001f80 & MxcsrFeatureMask;
  else
    FxSave->MXCsr = 0;
  FxSave->MXCsrMask = MxcsrFeatureMask;
  memset(FxSave->Reserved3, 0, sizeof(FxSave->Reserved3) +
         sizeof(FxSave->Reserved4)); /* XXX - doesnt zero Align16Byte because
                                        Context->ExtendedRegisters is only 512 bytes, not 520 */
  for (i = 0; i < 8; i++)
    {
      memcpy(FxSave->RegisterArea + (i * 16), FnSave->RegisterArea + (i * 10), 10);
      memset(FxSave->RegisterArea + (i * 16) + 10, 0, 6);
    }
}

STATIC VOID
KiFxsaveToFnsaveFormat(PFNSAVE_FORMAT FnSave, CONST PFXSAVE_FORMAT FxSave)
{
  INT i;

  FnSave->ControlWord = 0xffff0000 | FxSave->ControlWord;
  FnSave->StatusWord = 0xffff0000 | FxSave->StatusWord;
  FnSave->TagWord = 0xffff0000 | KiTagWordFxsaveToFnsave(FxSave);
  FnSave->ErrorOffset = FxSave->ErrorOffset;
  FnSave->ErrorSelector = FxSave->ErrorSelector & 0x0000ffff;
  FnSave->ErrorSelector |= FxSave->ErrorOpcode << 16;
  FnSave->DataOffset = FxSave->DataOffset;
  FnSave->DataSelector = FxSave->DataSelector | 0xffff0000;
  for (i = 0; i < 8; i++)
    {
      memcpy(FnSave->RegisterArea + (i * 10), FxSave->RegisterArea + (i * 16), 10);
    }
}

VOID
KiFloatingSaveAreaToFxSaveArea(PFX_SAVE_AREA FxSaveArea, CONST FLOATING_SAVE_AREA *FloatingSaveArea)
{
  if (FxsrSupport)
  {
    KiFnsaveToFxsaveFormat(&FxSaveArea->U.FxArea, (PFNSAVE_FORMAT)FloatingSaveArea);
  }
  else
  {
    memcpy(&FxSaveArea->U.FnArea, FloatingSaveArea, sizeof(FxSaveArea->U.FnArea));
  }
  FxSaveArea->NpxSavedCpu = 0;
  FxSaveArea->Cr0NpxState = FloatingSaveArea->Cr0NpxState;
}

BOOL
KiContextToFxSaveArea(PFX_SAVE_AREA FxSaveArea, PCONTEXT Context)
{
  BOOL FpuContextChanged = FALSE;

  /* First of all convert the FLOATING_SAVE_AREA into the FX_SAVE_AREA */
  if ((Context->ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT)
    {
      KiFloatingSaveAreaToFxSaveArea(FxSaveArea, &Context->FloatSave);
      FpuContextChanged = TRUE;
    }

  /* Now merge the FX_SAVE_AREA from the context with the destination area */
  if ((Context->ContextFlags & CONTEXT_EXTENDED_REGISTERS) == CONTEXT_EXTENDED_REGISTERS)
    {
      if (FxsrSupport)
        {
          PFXSAVE_FORMAT src = (PFXSAVE_FORMAT)Context->ExtendedRegisters;
          PFXSAVE_FORMAT dst = &FxSaveArea->U.FxArea;
          dst->MXCsr = src->MXCsr & MxcsrFeatureMask;
          memcpy(dst->Reserved3, src->Reserved3,
                 sizeof(src->Reserved3) + sizeof(src->Reserved4));

          if ((Context->ContextFlags & CONTEXT_FLOATING_POINT) != CONTEXT_FLOATING_POINT)
            {
              dst->ControlWord = src->ControlWord;
              dst->StatusWord = src->StatusWord;
              dst->TagWord = src->TagWord;
              dst->ErrorOpcode = src->ErrorOpcode;
              dst->ErrorOffset = src->ErrorOffset;
              dst->ErrorSelector = src->ErrorSelector;
              dst->DataOffset = src->DataOffset;
              dst->DataSelector = src->DataSelector;
              memcpy(dst->RegisterArea, src->RegisterArea, sizeof(src->RegisterArea));

              FxSaveArea->NpxSavedCpu = 0;
              FxSaveArea->Cr0NpxState = 0;
            }
          FpuContextChanged = TRUE;
        }
    }

  return FpuContextChanged;
}

VOID INIT_FUNCTION
KiCheckFPU(VOID)
{
   unsigned short int status;
   int cr0;
   ULONG Flags;
   PKPCR Pcr = KeGetCurrentKPCR();

   Ke386SaveFlags(Flags);
   Ke386DisableInterrupts();

   HardwareMathSupport = 0;
   FxsrSupport = 0;
   XmmSupport = 0;

   cr0 = Ke386GetCr0();
   cr0 |= X86_CR0_NE | X86_CR0_MP;
   cr0 &= ~(X86_CR0_EM | X86_CR0_TS);
   Ke386SetCr0(cr0);

#if defined(__GNUC__)
   asm volatile("fninit\n\t");
   asm volatile("fstsw %0\n\t" : "=a" (status));
#elif defined(_MSC_VER)
   __asm
   {
	   fninit;
	   fstsw status
   }
#else
#error Unknown compiler for inline assembler
#endif

   if (status != 0)
     {
	/* Set the EM flag in CR0 so any FPU instructions cause a trap. */
	Ke386SetCr0(Ke386GetCr0() | X86_CR0_EM);
        Ke386RestoreFlags(Flags);
	return;
     }

   /* fsetpm for i287, ignored by i387 */
#if defined(__GNUC__)
   asm volatile(".byte 0xDB, 0xE4\n\t");
#elif defined(_MSC_VER)
   __asm _emit 0xDB __asm _emit 0xe4
#else
#error Unknown compiler for inline assembler
#endif

   HardwareMathSupport = 1;

   /* check for and enable MMX/SSE support if possible */
   if ((Pcr->PrcbData.FeatureBits & X86_FEATURE_FXSR) != 0)
     {
        BYTE DummyArea[sizeof(FX_SAVE_AREA) + 15];
        PFX_SAVE_AREA FxSaveArea;

        /* enable FXSR */
        FxsrSupport = 1;

        /* we need a 16 byte aligned FX_SAVE_AREA */
        FxSaveArea = (PFX_SAVE_AREA)DummyArea;
        if ((ULONG_PTR)FxSaveArea & 0x0f)
          {
            FxSaveArea = (PFX_SAVE_AREA)(((ULONG_PTR)FxSaveArea + 0x10) & (~0x0f));
          }

        Ke386SetCr4(Ke386GetCr4() | X86_CR4_OSFXSR);
        memset(&FxSaveArea->U.FxArea, 0, sizeof(FxSaveArea->U.FxArea));
        asm volatile("fxsave %0" : : "m"(FxSaveArea->U.FxArea));
        MxcsrFeatureMask = FxSaveArea->U.FxArea.MXCsrMask;
        if (MxcsrFeatureMask == 0)
          {
             MxcsrFeatureMask = 0x0000ffbf;
          }
     }
   /* FIXME: Check for SSE3 in Ke386CpuidFlags2! */
   if (Pcr->PrcbData.FeatureBits & (X86_FEATURE_SSE | X86_FEATURE_SSE2))
     {
        Ke386SetCr4(Ke386GetCr4() | X86_CR4_OSXMMEXCPT);
        
        /* enable SSE */
        XmmSupport = 1;
     }
     
   Ke386SetCr0(Ke386GetCr0() | X86_CR0_TS);
   Ke386RestoreFlags(Flags);
}

/* This is a rather naive implementation of Ke(Save/Restore)FloatingPointState
   which will not work for WDM drivers. Please feel free to improve */

#define FPU_STATE_SIZE 108

NTSTATUS STDCALL
KeSaveFloatingPointState(OUT PKFLOATING_SAVE Save)
{
  char *FpState;

  ASSERT_IRQL(DISPATCH_LEVEL); /* FIXME: is this removed for non-debug builds? I hope not! */

  /* check if we are doing software emulation */
  if (!HardwareMathSupport)
    {
      return STATUS_ILLEGAL_FLOAT_CONTEXT;
    }

  FpState = ExAllocatePool(PagedPool, FPU_STATE_SIZE);
  if (NULL == FpState)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  *((PVOID *) Save) = FpState;

#if defined(__GNUC__)
  asm volatile("fsave %0\n\t" : "=m" (*FpState));
#elif defined(_MSC_VER)
  __asm mov eax, FpState;
  __asm fsave [eax];
#else
#error Unknown compiler for inline assembler
#endif

  KeGetCurrentThread()->NpxIrql = KeGetCurrentIrql();

  return STATUS_SUCCESS;
}

NTSTATUS STDCALL
KeRestoreFloatingPointState(IN PKFLOATING_SAVE Save)
{
  char *FpState = *((PVOID *) Save);

  if (KeGetCurrentThread()->NpxIrql != KeGetCurrentIrql())
    {
      KEBUGCHECK(UNDEFINED_BUG_CODE);
    }

#if defined(__GNUC__)
  __asm__("frstor %0\n\t" : "=m" (*FpState));
#elif defined(_MSC_VER)
  __asm mov eax, FpState;
  __asm frstor [eax];
#else
#error Unknown compiler for inline assembler
#endif

  ExFreePool(FpState);

  return STATUS_SUCCESS;
}

NTSTATUS
KiHandleFpuFault(PKTRAP_FRAME Tf, ULONG ExceptionNr)
{
  if (ExceptionNr == 7) /* device not present */
    {
      BOOL FpuInitialized = FALSE;
      unsigned int cr0 = Ke386GetCr0();
      PKTHREAD CurrentThread;
      PFX_SAVE_AREA FxSaveArea;
      KIRQL oldIrql;
#ifndef MP
      PKTHREAD NpxThread;
#endif
      
      (void) cr0;
      ASSERT((cr0 & X86_CR0_TS) == X86_CR0_TS);
      ASSERT((Tf->Eflags & X86_EFLAGS_VM) == 0);
      ASSERT((cr0 & X86_CR0_EM) == 0);

      /* disable scheduler, clear TS in cr0 */
      ASSERT_IRQL(DISPATCH_LEVEL);
      KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
      asm volatile("clts");

      CurrentThread = KeGetCurrentThread();
#ifndef MP
      NpxThread = KeGetCurrentKPCR()->PrcbData.NpxThread;
#endif

      ASSERT(CurrentThread != NULL);
      DPRINT("Device not present exception happened! (Cr0 = 0x%x, NpxState = 0x%x)\n", cr0, CurrentThread->NpxState);

#ifndef MP
      /* check if the current thread already owns the FPU */
      if (NpxThread != CurrentThread) /* FIXME: maybe this could be an assertation */
        {
          /* save the FPU state into the owner's save area */
          if (NpxThread != NULL)
            {
              KeGetCurrentKPCR()->PrcbData.NpxThread = NULL;
              FxSaveArea = (PFX_SAVE_AREA)((char *)NpxThread->InitialStack - sizeof (FX_SAVE_AREA));
              /* the fnsave might raise a delayed #MF exception */
              if (FxsrSupport)
                {
                  asm volatile("fxsave %0" : : "m"(FxSaveArea->U.FxArea));
                }
              else
                {
                  asm volatile("fnsave %0" : : "m"(FxSaveArea->U.FnArea));
                  FpuInitialized = TRUE;
                }
              NpxThread->NpxState = NPX_STATE_VALID;
            }
#endif /* !MP */

          /* restore the state of the current thread */
          ASSERT((CurrentThread->NpxState & NPX_STATE_DIRTY) == 0);
          FxSaveArea = (PFX_SAVE_AREA)((char *)CurrentThread->InitialStack - sizeof (FX_SAVE_AREA));
          if (CurrentThread->NpxState & NPX_STATE_VALID)
            {
              if (FxsrSupport)
                {
                  FxSaveArea->U.FxArea.MXCsr &= MxcsrFeatureMask;
                  asm volatile("fxrstor %0" : : "m"(FxSaveArea->U.FxArea));
                }
              else
                {
                  asm volatile("frstor %0" : : "m"(FxSaveArea->U.FnArea));
                }
            }
          else /* NpxState & NPX_STATE_INVALID */
            {
              DPRINT("Setting up clean FPU state\n");
              if (FxsrSupport)
                {
                  memset(&FxSaveArea->U.FxArea, 0, sizeof(FxSaveArea->U.FxArea));
                  FxSaveArea->U.FxArea.ControlWord = 0x037f;
                  if (XmmSupport)
                    {
                      FxSaveArea->U.FxArea.MXCsr = 0x00001f80 & MxcsrFeatureMask;
                    }
                  asm volatile("fxrstor %0" : : "m"(FxSaveArea->U.FxArea));
                }
              else if (!FpuInitialized)
                {
                  asm volatile("finit");
                }
            }
          KeGetCurrentKPCR()->PrcbData.NpxThread = CurrentThread;
#ifndef MP
        }
#endif

      CurrentThread->NpxState |= NPX_STATE_DIRTY;
      KeLowerIrql(oldIrql);
      DPRINT("Device not present exception handled!\n");

      return STATUS_SUCCESS;
    }
  else /* ExceptionNr == 16 || ExceptionNr == 19 */
    {
      EXCEPTION_RECORD Er;
      UCHAR DummyContext[sizeof(CONTEXT) + 16];
      PCONTEXT Context;
      KPROCESSOR_MODE PreviousMode;
      PKTHREAD CurrentThread, NpxThread;
      KIRQL oldIrql;

      ASSERT(ExceptionNr == 16 || ExceptionNr == 19); /* math fault or XMM fault*/

      KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

      NpxThread = KeGetCurrentKPCR()->PrcbData.NpxThread;
      CurrentThread = KeGetCurrentThread();
      if (NpxThread == NULL)
        {
          KeLowerIrql(oldIrql);
          DPRINT1("!!! Math/Xmm fault ignored! (NpxThread == NULL)\n");
          return STATUS_SUCCESS;
        }

      PreviousMode = ((Tf->Cs & 0xffff) == USER_CS) ? (UserMode) : (KernelMode);
      DPRINT("Math/Xmm fault happened! (PreviousMode = %s)\n",
              (PreviousMode == UserMode) ? ("UserMode") : ("KernelMode"));

      ASSERT(NpxThread == CurrentThread); /* FIXME: Is not always true I think */

      /* For fxsave we have to align Context->ExtendedRegisters on 16 bytes */
      Context = (PCONTEXT)DummyContext;
      Context = (PCONTEXT)((ULONG_PTR)Context + 0x10 - ((ULONG_PTR)Context->ExtendedRegisters & 0x0f));

      /* Get FPU/XMM state */
      Context->FloatSave.Cr0NpxState = 0;
      if (FxsrSupport)
        {
          PFXSAVE_FORMAT FxSave = (PFXSAVE_FORMAT)Context->ExtendedRegisters;
          FxSave->MXCsrMask = MxcsrFeatureMask;
          memset(FxSave->RegisterArea, 0, sizeof(FxSave->RegisterArea) +
                 sizeof(FxSave->Reserved3) + sizeof(FxSave->Reserved4));
          asm volatile("fxsave %0" : : "m"(*FxSave));
          KeLowerIrql(oldIrql);
          KiFxsaveToFnsaveFormat((PFNSAVE_FORMAT)&Context->FloatSave, FxSave);
        }
      else
        {
          PFNSAVE_FORMAT FnSave = (PFNSAVE_FORMAT)&Context->FloatSave;
          asm volatile("fnsave %0" : : "m"(*FnSave));
          KeLowerIrql(oldIrql);
          KiFnsaveToFxsaveFormat((PFXSAVE_FORMAT)Context->ExtendedRegisters, FnSave);
        }

      /* Fill the rest of the context */
      Context->ContextFlags = CONTEXT_FULL;
      KeTrapFrameToContext(Tf, Context);
      Context->ContextFlags |= CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;

      /* Determine exception code */
      if (ExceptionNr == 16)
        {
          USHORT FpuStatusWord = Context->FloatSave.StatusWord & 0xffff;
          DPRINT("FpuStatusWord = 0x%04x\n", FpuStatusWord);

          if (FpuStatusWord & X87_SW_IE)
            Er.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
          else if (FpuStatusWord & X87_SW_DE)
            Er.ExceptionCode = EXCEPTION_FLT_DENORMAL_OPERAND;
          else if (FpuStatusWord & X87_SW_ZE)
            Er.ExceptionCode = EXCEPTION_FLT_DIVIDE_BY_ZERO;
          else if (FpuStatusWord & X87_SW_OE)
            Er.ExceptionCode = EXCEPTION_FLT_OVERFLOW;
          else if (FpuStatusWord & X87_SW_UE)
            Er.ExceptionCode = EXCEPTION_FLT_UNDERFLOW;
          else if (FpuStatusWord & X87_SW_PE)
            Er.ExceptionCode = EXCEPTION_FLT_INEXACT_RESULT;
          else if (FpuStatusWord & X87_SW_SE)
            Er.ExceptionCode = EXCEPTION_FLT_STACK_CHECK;
          else
            ASSERT(0); /* not reached */
          /* FIXME: is this the right way to get the correct EIP of the faulting instruction? */
          Er.ExceptionAddress = (PVOID)Context->FloatSave.ErrorOffset;
        }
      else /* ExceptionNr == 19 */
        {
          /* FIXME: When should we use EXCEPTION_FLT_MULTIPLE_FAULTS? */
          Er.ExceptionCode = EXCEPTION_FLT_MULTIPLE_TRAPS;
          Er.ExceptionAddress = (PVOID)Tf->Eip;
        }

      Er.ExceptionFlags = 0;
      Er.ExceptionRecord = NULL;
      Er.NumberParameters = 0;

      /* Dispatch exception */
      DPRINT("Dispatching exception (ExceptionCode = 0x%08x)\n", Er.ExceptionCode);
      KiDispatchException(&Er, Context, Tf, PreviousMode, TRUE);
      
      DPRINT("Math-fault handled!\n");
      return STATUS_SUCCESS;
    }

  return STATUS_UNSUCCESSFUL;
}

