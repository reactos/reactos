/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/fpu.c
 * PURPOSE:         Handles the FPU
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Gregor Anich
 */

/* INCLUDES *****************************************************************/

#include <roscfg.h>
#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* DEFINES *******************************************************************/

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

extern ULONG KeI386NpxPresent;
extern ULONG KeI386XMMIPresent;
extern ULONG KeI386FxsrPresent;

static ULONG MxcsrFeatureMask = 0;

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
                    Tag = 1;  /* Zero */
                else
                    Tag = 2;  /* Special */
                break;

            case 0x7fff:
                Tag = 2;      /* Special */
                break;

            default:
                if (FpReg->Significand[3] & 0x00008000)
                    Tag = 0;  /* Valid */
                else
                    Tag = 2;  /* Special */
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
KiFnsaveToFxsaveFormat(PFXSAVE_FORMAT FxSave, PFNSAVE_FORMAT FnSave)
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
    if (KeI386XMMIPresent)
        FxSave->MXCsr = 0x00001f80 & MxcsrFeatureMask;
    else
        FxSave->MXCsr = 0;
    FxSave->MXCsrMask = MxcsrFeatureMask;
    memset(FxSave->Reserved3, 0, sizeof(FxSave->Reserved3) +
           sizeof(FxSave->Reserved4)); /* Don't zero Align16Byte because Context->ExtendedRegisters
                                          is only 512 bytes, not 520 */
    for (i = 0; i < 8; i++)
    {
        memcpy(FxSave->RegisterArea + (i * 16), FnSave->RegisterArea + (i * 10), 10);
        memset(FxSave->RegisterArea + (i * 16) + 10, 0, 6);
    }
}

STATIC VOID
KiFxsaveToFnsaveFormat(PFNSAVE_FORMAT FnSave, PFXSAVE_FORMAT FxSave)
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


STATIC VOID
KiFloatingSaveAreaToFxSaveArea(PFX_SAVE_AREA FxSaveArea, FLOATING_SAVE_AREA *FloatingSaveArea)
{
    if (KeI386FxsrPresent)
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


VOID
KiFxSaveAreaToFloatingSaveArea(FLOATING_SAVE_AREA *FloatingSaveArea, CONST PFX_SAVE_AREA FxSaveArea)
{
    if (KeI386FxsrPresent)
    {
        KiFxsaveToFnsaveFormat((PFNSAVE_FORMAT)FloatingSaveArea, &FxSaveArea->U.FxArea);
    }
    else
    {
        memcpy(FloatingSaveArea, &FxSaveArea->U.FnArea, sizeof(FxSaveArea->U.FnArea));
    }
    FloatingSaveArea->Cr0NpxState = FxSaveArea->Cr0NpxState;
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
        if (KeI386FxsrPresent)
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
    PKPRCB Prcb = KeGetCurrentPrcb();

    Ke386SaveFlags(Flags);
    Ke386DisableInterrupts();

    KeI386NpxPresent = 0;
    KeI386FxsrPresent = 0;
    KeI386XMMIPresent = 0;

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

    KeI386NpxPresent = 1;

    /* check for and enable MMX/SSE support if possible */
    if ((Prcb->FeatureBits & X86_FEATURE_FXSR) != 0)
    {
        BYTE DummyArea[sizeof(FX_SAVE_AREA) + 15];
        PFX_SAVE_AREA FxSaveArea;

        /* enable FXSR */
        KeI386FxsrPresent = 1;

        /* we need a 16 byte aligned FX_SAVE_AREA */
        FxSaveArea = (PFX_SAVE_AREA)(((ULONG_PTR)DummyArea + 0xf) & (~0x0f));

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
    if (Prcb->FeatureBits & (X86_FEATURE_SSE | X86_FEATURE_SSE2))
    {
        Ke386SetCr4(Ke386GetCr4() | X86_CR4_OSXMMEXCPT);

        /* enable SSE */
        KeI386XMMIPresent = 1;
    }

    Ke386SetCr0(Ke386GetCr0() | X86_CR0_TS);
    Ke386RestoreFlags(Flags);
}


PFX_SAVE_AREA
KiGetFpuState(PKTHREAD Thread)
{
    PFX_SAVE_AREA FxSaveArea = NULL;
    KIRQL OldIrql;
    ULONG Cr0;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    if (Thread->NpxState & NPX_STATE_VALID)
    {
        FxSaveArea = (PFX_SAVE_AREA)((ULONG_PTR)Thread->InitialStack - sizeof (FX_SAVE_AREA));
        if (Thread->NpxState & NPX_STATE_DIRTY)
        {
            ASSERT(KeGetCurrentPrcb()->NpxThread == Thread);

            Cr0 = Ke386GetCr0();
            asm volatile("clts");
            if (KeI386FxsrPresent)
                asm volatile("fxsave %0" : : "m"(FxSaveArea->U.FxArea));
            else
            {
                asm volatile("fnsave %0" : : "m"(FxSaveArea->U.FnArea));
                /* FPU state has to be reloaded because fnsave changes it. */
                Cr0 |= X86_CR0_TS;
                KeGetCurrentPrcb()->NpxThread = NULL;
            }
            Ke386SetCr0(Cr0);
            Thread->NpxState = NPX_STATE_VALID;
        }
    }
    KeLowerIrql(OldIrql);

    return FxSaveArea;
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
#ifndef CONFIG_SMP
        PKTHREAD NpxThread;
#endif

        (void) cr0;
        ASSERT((cr0 & X86_CR0_TS) == X86_CR0_TS);
        ASSERT((Tf->EFlags & X86_EFLAGS_VM) == 0);
        ASSERT((cr0 & X86_CR0_EM) == 0);

        /* disable scheduler, clear TS in cr0 */
        ASSERT_IRQL(DISPATCH_LEVEL);
        KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
        asm volatile("clts");

        CurrentThread = KeGetCurrentThread();
#ifndef CONFIG_SMP
        NpxThread = KeGetCurrentPrcb()->NpxThread;
#endif

        ASSERT(CurrentThread != NULL);
        DPRINT("Device not present exception happened! (Cr0 = 0x%x, NpxState = 0x%x)\n", cr0, CurrentThread->NpxState);

#ifndef CONFIG_SMP
        /* check if the current thread already owns the FPU */
        if (NpxThread != CurrentThread) /* FIXME: maybe this could be an assertation */
        {
            /* save the FPU state into the owner's save area */
            if (NpxThread != NULL)
            {
                KeGetCurrentPrcb()->NpxThread = NULL;
                FxSaveArea = (PFX_SAVE_AREA)((ULONG_PTR)NpxThread->InitialStack - sizeof (FX_SAVE_AREA));
                /* the fnsave might raise a delayed #MF exception */
                if (KeI386FxsrPresent)
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
#endif /* !CONFIG_SMP */

            /* restore the state of the current thread */
            ASSERT((CurrentThread->NpxState & NPX_STATE_DIRTY) == 0);
            FxSaveArea = (PFX_SAVE_AREA)((ULONG_PTR)CurrentThread->InitialStack - sizeof (FX_SAVE_AREA));
            if (CurrentThread->NpxState & NPX_STATE_VALID)
            {
                if (KeI386FxsrPresent)
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
                if (KeI386FxsrPresent)
                {
                    memset(&FxSaveArea->U.FxArea, 0, sizeof(FxSaveArea->U.FxArea));
                    FxSaveArea->U.FxArea.ControlWord = 0x037f;
                    if (KeI386XMMIPresent)
                    {
                        FxSaveArea->U.FxArea.MXCsr = 0x00001f80 & MxcsrFeatureMask;
                    }
                    asm volatile("fxrstor %0" : : "m"(FxSaveArea->U.FxArea));
                }
                else if (!FpuInitialized)
                {
                    asm volatile("fninit");
                }
            }
            KeGetCurrentPrcb()->NpxThread = CurrentThread;
#ifndef CONFIG_SMP
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
        KPROCESSOR_MODE PreviousMode;
        PKTHREAD CurrentThread, NpxThread;
        KIRQL OldIrql;
        ULONG FpuEnvBuffer[7];
        PFNSAVE_FORMAT FpuEnv = (PFNSAVE_FORMAT)FpuEnvBuffer;

        ASSERT(ExceptionNr == 16 || ExceptionNr == 19); /* math fault or XMM fault*/

        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

        NpxThread = KeGetCurrentPrcb()->NpxThread;
        CurrentThread = KeGetCurrentThread();
        if (NpxThread == NULL)
        {
            KeLowerIrql(OldIrql);
            DPRINT("Math/Xmm fault ignored! (NpxThread == NULL)\n");
            return STATUS_SUCCESS;
        }
        if (ExceptionNr == 16)
        {
            asm volatile("fnstenv %0" : : "m"(*FpuEnv));
            asm volatile("fldenv %0" : : "m"(*FpuEnv)); /* Stupid x87... */
            FpuEnv->StatusWord &= 0xffff;
        }
        KeLowerIrql(OldIrql);

        PreviousMode = ((Tf->SegCs & 0xffff) == (KGDT_R3_CODE | RPL_MASK)) ? (UserMode) : (KernelMode);
        DPRINT("Math/Xmm fault happened! (PreviousMode = %s)\n",
               (PreviousMode != KernelMode) ? ("UserMode") : ("KernelMode"));

        ASSERT(NpxThread == CurrentThread); /* FIXME: Is not always true I think */

        /* Get FPU/XMM state */
        KeLowerIrql(OldIrql);

        /* Determine exception code */
        if (ExceptionNr == 16)
        {
            DPRINT("FpuStatusWord = 0x%04x\n", FpuStatusWord);

            if (FpuEnv->StatusWord & X87_SW_IE)
                Er.ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            else if (FpuEnv->StatusWord & X87_SW_DE)
                Er.ExceptionCode = STATUS_FLOAT_DENORMAL_OPERAND;
            else if (FpuEnv->StatusWord & X87_SW_ZE)
                Er.ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
            else if (FpuEnv->StatusWord & X87_SW_OE)
                Er.ExceptionCode = STATUS_FLOAT_OVERFLOW;
            else if (FpuEnv->StatusWord & X87_SW_UE)
                Er.ExceptionCode = STATUS_FLOAT_UNDERFLOW;
            else if (FpuEnv->StatusWord & X87_SW_PE)
                Er.ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            else if (FpuEnv->StatusWord & X87_SW_SE)
                Er.ExceptionCode = STATUS_FLOAT_STACK_CHECK;
            else
                ASSERT(0); /* not reached */
            Er.ExceptionAddress = (PVOID)FpuEnv->ErrorOffset;
        }
        else /* ExceptionNr == 19 */
        {
            Er.ExceptionCode = STATUS_FLOAT_MULTIPLE_TRAPS;
            Er.ExceptionAddress = (PVOID)Tf->Eip;
        }

        Er.ExceptionFlags = 0;
        Er.ExceptionRecord = NULL;
        Er.NumberParameters = 0;

        /* Dispatch exception */
        DPRINT("Dispatching exception (ExceptionCode = 0x%08x)\n", Er.ExceptionCode);
        KiDispatchException(&Er, NULL, Tf, PreviousMode, TRUE);

        DPRINT("Math-fault handled!\n");
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}


/* This is a rather naive implementation of Ke(Save/Restore)FloatingPointState
   which will not work for WDM drivers. Please feel free to improve */

NTSTATUS STDCALL
KeSaveFloatingPointState(OUT PKFLOATING_SAVE Save)
{
    PFNSAVE_FORMAT FpState;

    ASSERT_IRQL(DISPATCH_LEVEL);

    /* check if we are doing software emulation */
    if (!KeI386NpxPresent)
    {
        return STATUS_ILLEGAL_FLOAT_CONTEXT;
    }

    FpState = ExAllocatePool(NonPagedPool, sizeof (FNSAVE_FORMAT));
    if (NULL == FpState)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    *((PVOID *) Save) = FpState;

#if defined(__GNUC__)
    asm volatile("fnsave %0\n\t" : "=m" (*FpState));
#elif defined(_MSC_VER)
    __asm mov eax, FpState;
    __asm fsave [eax];
#else
#error Unknown compiler for inline assembler
#endif

    KeGetCurrentThread()->DispatcherHeader.NpxIrql = KeGetCurrentIrql();

    return STATUS_SUCCESS;
}


NTSTATUS STDCALL
KeRestoreFloatingPointState(IN PKFLOATING_SAVE Save)
{
    PFNSAVE_FORMAT FpState = *((PVOID *) Save);

    if (KeGetCurrentThread()->DispatcherHeader.NpxIrql != KeGetCurrentIrql())
    {
        KEBUGCHECK(UNDEFINED_BUG_CODE);
    }

#if defined(__GNUC__)
    asm volatile("fnclex\n\t");
    asm volatile("frstor %0\n\t" : "=m" (*FpState));
#elif defined(_MSC_VER)
    __asm mov eax, FpState;
    __asm frstor [eax];
#else
#error Unknown compiler for inline assembler
#endif

    ExFreePool(FpState);

    return STATUS_SUCCESS;
}
