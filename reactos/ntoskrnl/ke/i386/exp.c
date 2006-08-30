/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/exp.c
 * PURPOSE:         Exception Dispatching and Context<->Trap Frame Conversion
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Gregor Anich
 *                  Skywing (skywing@valhallalegends.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

_SEH_DEFINE_LOCALS(KiCopyInfo)
{
    volatile EXCEPTION_RECORD SehExceptRecord;
};

_SEH_FILTER(KiCopyInformation)
{
    _SEH_ACCESS_LOCALS(KiCopyInfo);

    /* Copy the exception records and return to the handler */
    RtlMoveMemory((PVOID)&_SEH_VAR(SehExceptRecord),
                  _SEH_GetExceptionPointers()->ExceptionRecord,
                  sizeof(EXCEPTION_RECORD));
    return EXCEPTION_EXECUTE_HANDLER;
}

VOID
INIT_FUNCTION
NTAPI
KeInitExceptions(VOID)
{
    ULONG i;
    USHORT FlippedSelector;
    extern KIDTENTRY KiIdt[];

    /* Loop the IDT */
    for (i = 0; i <= MAXIMUM_IDTVECTOR; i ++)
    {
        /* Save the current Selector */
        FlippedSelector = KiIdt[i].Selector;

        /* Flip Selector and Extended Offset */
        KiIdt[i].Selector = KiIdt[i].ExtendedOffset;
        KiIdt[i].ExtendedOffset = FlippedSelector;
    }
}

ULONG
NTAPI
KiEspFromTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    /* Check if this is user-mode or V86 */
    if ((TrapFrame->SegCs & MODE_MASK) ||
        (TrapFrame->EFlags & EFLAGS_V86_MASK))
    {
        /* Return it directly */
        return TrapFrame->HardwareEsp;
    }
    else
    {
        /* Edited frame */
        if (!(TrapFrame->SegCs & FRAME_EDITED))
        {
            /* Return edited value */
            return TrapFrame->TempEsp;
        }
        else
        {
            /* Virgin frame, calculate */
            return (ULONG)&TrapFrame->HardwareEsp;
        }
    }
}

VOID
NTAPI
KiEspToTrapFrame(IN PKTRAP_FRAME TrapFrame,
                 IN ULONG Esp)
{
    ULONG Previous = KiEspFromTrapFrame(TrapFrame);

    /* Check if this is user-mode or V86 */
    if ((TrapFrame->SegCs & MODE_MASK) || (TrapFrame->EFlags & EFLAGS_V86_MASK))
    {
        /* Write it directly */
        TrapFrame->HardwareEsp = Esp;
    }
    else
    {
        /* Don't allow ESP to be lowered, this is illegal */
        if (Esp < Previous) KeBugCheck(SET_OF_INVALID_CONTEXT);

        /* Create an edit frame, check if it was alrady */
        if (!(TrapFrame->SegCs & FRAME_EDITED))
        {
            /* Update the value */
            TrapFrame->TempEsp = Esp;
        }
        else
        {
            /* Check if ESP changed */
            if (Previous != Esp)
            {
                /* Save CS */
                TrapFrame->TempSegCs = TrapFrame->SegCs;
                TrapFrame->SegCs &= ~FRAME_EDITED;

                /* Save ESP */
                TrapFrame->TempEsp = Esp;
            }
        }
    }
}

ULONG
NTAPI
KiSsFromTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    /* If this was V86 Mode */
    if (TrapFrame->EFlags & EFLAGS_V86_MASK)
    {
        /* Just return it */
        return TrapFrame->HardwareSegSs;
    }
    else if (TrapFrame->SegCs & MODE_MASK)
    {
        /* Usermode, return the User SS */
        return TrapFrame->HardwareSegSs | RPL_MASK;
    }
    else
    {
        /* Kernel mode */
        return KGDT_R0_DATA;
    }
}

VOID
NTAPI
KiSsToTrapFrame(IN PKTRAP_FRAME TrapFrame,
                IN ULONG Ss)
{
    /* Remove the high-bits */
    Ss &= 0xFFFF;

    /* If this was V86 Mode */
    if (TrapFrame->EFlags & EFLAGS_V86_MASK)
    {
        /* Just write it */
        TrapFrame->HardwareSegSs = Ss;
    }
    else if (TrapFrame->SegCs & MODE_MASK)
    {
        /* Usermode, save the User SS */
        TrapFrame->HardwareSegSs = Ss | RPL_MASK;
    }
}

USHORT
NTAPI
KiTagWordFnsaveToFxsave(USHORT TagWord)
{
    INT FxTagWord = ~TagWord; 

    /* 
     * Empty is now 00, any 2 bits containing 1 mean valid
     * Now convert the rest (11->0 and the rest to 1)
     */
    FxTagWord = (FxTagWord | (FxTagWord >> 1)) & 0x5555; /* 0V0V0V0V0V0V0V0V */
    FxTagWord = (FxTagWord | (FxTagWord >> 1)) & 0x3333; /* 00VV00VV00VV00VV */
    FxTagWord = (FxTagWord | (FxTagWord >> 2)) & 0x0f0f; /* 0000VVVV0000VVVV */
    FxTagWord = (FxTagWord | (FxTagWord >> 4)) & 0x00ff; /* 00000000VVVVVVVV */
    return FxTagWord;
}

VOID
NTAPI
KeContextToTrapFrame(IN PCONTEXT Context,
                     IN OUT PKEXCEPTION_FRAME ExceptionFrame,
                     IN OUT PKTRAP_FRAME TrapFrame,
                     IN ULONG ContextFlags,
                     IN KPROCESSOR_MODE PreviousMode)
{
    PFX_SAVE_AREA FxSaveArea;
    ULONG i;
    BOOLEAN V86Switch = FALSE;
    KIRQL OldIrql = APC_LEVEL;

    /* Do this at APC_LEVEL */
    if (KeGetCurrentIrql() < APC_LEVEL) KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Start with the basic Registers */
    if ((ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL)
    {
        /* Check if we went through a V86 switch */
        if ((Context->EFlags & EFLAGS_V86_MASK) !=
            (TrapFrame->EFlags & EFLAGS_V86_MASK))
        {
            /* We did, remember this for later */
            V86Switch = TRUE;
        }

        /* Copy EFLAGS. FIXME: Needs to be sanitized */
        TrapFrame->EFlags = Context->EFlags;

        /* Copy EBP and EIP */
        TrapFrame->Ebp = Context->Ebp;
        TrapFrame->Eip = Context->Eip;

        /* Check if we were in V86 Mode */
        if (TrapFrame->EFlags & EFLAGS_V86_MASK)
        {
            /* Simply copy the CS value */
            TrapFrame->SegCs = Context->SegCs;
        }
        else
        {
            /* We weren't in V86, so sanitize the CS (FIXME!) */
            TrapFrame->SegCs = Context->SegCs;

            /* Don't let it under 8, that's invalid */
            if ((PreviousMode != KernelMode) && (TrapFrame->SegCs < 8))
            {
                /* Force it to User CS */
                TrapFrame->SegCs = (KGDT_R3_CODE | RPL_MASK);
            }
        }

        /* Handle SS Specially for validation */
        KiSsToTrapFrame(TrapFrame, Context->SegSs);

        /* Write ESP back; take into account Edited Trap Frames */
        KiEspToTrapFrame(TrapFrame, Context->Esp);

        /* Handle our V86 Bias if we went through a switch */
        if (V86Switch) Ki386AdjustEsp0(TrapFrame);
    }

    /* Process the Integer Registers */
    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
    {
        /* Copy them manually */
        TrapFrame->Eax = Context->Eax;
        TrapFrame->Ebx = Context->Ebx;
        TrapFrame->Ecx = Context->Ecx;
        TrapFrame->Edx = Context->Edx;
        TrapFrame->Esi = Context->Esi;
        TrapFrame->Edi = Context->Edi;
    }

    /* Process the Context Segments */
    if ((ContextFlags & CONTEXT_SEGMENTS) == CONTEXT_SEGMENTS)
    {
        /* Check if we were in V86 Mode */
        if (TrapFrame->EFlags & EFLAGS_V86_MASK)
        {
            /* Copy the V86 Segments directlry */
            TrapFrame->V86Ds = Context->SegDs;
            TrapFrame->V86Es = Context->SegEs;
            TrapFrame->V86Fs = Context->SegFs;
            TrapFrame->V86Gs = Context->SegGs;
        }
        else if (!(TrapFrame->SegCs & MODE_MASK))
        {
            /* For kernel mode, write the standard values */
            TrapFrame->SegDs = KGDT_R3_DATA | RPL_MASK;
            TrapFrame->SegEs = KGDT_R3_DATA | RPL_MASK;
            TrapFrame->SegFs = Context->SegFs;
            TrapFrame->SegGs = 0;
        }
        else
        {
            /* For user mode, return the values directlry */
            TrapFrame->SegDs = Context->SegDs;
            TrapFrame->SegEs = Context->SegEs;
            TrapFrame->SegFs = Context->SegFs;

            /* Handle GS specially */
            if (TrapFrame->SegCs == (KGDT_R3_CODE | RPL_MASK))
            {
                /* Don't use it, if user */
                TrapFrame->SegGs = 0;
            }
            else
            {
                /* Copy it if kernel */
                TrapFrame->SegGs = Context->SegGs;
            }
        }
    }

    /* Handle the extended registers */
    if (((ContextFlags & CONTEXT_EXTENDED_REGISTERS) ==
        CONTEXT_EXTENDED_REGISTERS) && (TrapFrame->SegCs & MODE_MASK))
    {
        /* Get the FX Area */
        FxSaveArea = (PFX_SAVE_AREA)(TrapFrame + 1);

        /* Check if NPX is present */
        if (KeI386NpxPresent)
        {
            /* Flush the NPX State */
            KiFlushNPXState(NULL);

            /* Copy the FX State */
            RtlCopyMemory(&FxSaveArea->U.FxArea,
                          &Context->ExtendedRegisters[0],
                          MAXIMUM_SUPPORTED_EXTENSION);

            /* Remove reserved bits from MXCSR */
            FxSaveArea->U.FxArea.MXCsr &= ~0xFFBF;

            /* Mask out any invalid flags */
            FxSaveArea->Cr0NpxState &= ~(CR0_EM | CR0_MP | CR0_TS);

            /* FIXME: Check if this is a VDM app */
        }
    }

    /* Handle the floating point state */
    if (((ContextFlags & CONTEXT_FLOATING_POINT) ==
        CONTEXT_FLOATING_POINT) && (TrapFrame->SegCs & MODE_MASK))
    {
        /* Get the FX Area */
        FxSaveArea = (PFX_SAVE_AREA)(TrapFrame + 1);

        /* Check if NPX is present */
        if (KeI386NpxPresent)
        {
            /* Flush the NPX State */
            KiFlushNPXState(NULL);

            /* Check if we have Fxsr support */
            if (KeI386FxsrPresent)
            {
                /* Convert the Fn Floating Point state to Fx */
                FxSaveArea->U.FxArea.ControlWord =
                    (USHORT)Context->FloatSave.ControlWord;
                FxSaveArea->U.FxArea.StatusWord =
                    (USHORT)Context->FloatSave.StatusWord;
                FxSaveArea->U.FxArea.TagWord =
                    KiTagWordFnsaveToFxsave((USHORT)Context->FloatSave.TagWord);
                FxSaveArea->U.FxArea.ErrorOpcode =
                    (USHORT)((Context->FloatSave.ErrorSelector >> 16) & 0xFFFF);
                FxSaveArea->U.FxArea.ErrorOffset =
                    Context->FloatSave.ErrorOffset;
                FxSaveArea->U.FxArea.ErrorSelector =
                    Context->FloatSave.ErrorSelector & 0xFFFF;
                FxSaveArea->U.FxArea.DataOffset =
                    Context->FloatSave.DataOffset;
                FxSaveArea->U.FxArea.DataSelector =
                    Context->FloatSave.DataSelector;

                /* Clear out the Register Area */
                RtlZeroMemory(&FxSaveArea->U.FxArea.RegisterArea[0],
                              SIZE_OF_FX_REGISTERS);

                /* Loop the 8 floating point registers */
                for (i = 0; i < 8; i++)
                {
                    /* Copy from Fn to Fx */
                    RtlCopyMemory(FxSaveArea->U.FxArea.RegisterArea + (i * 16),
                                  Context->FloatSave.RegisterArea + (i * 10),
                                  10);
                }
            }
            else
            {
                /* Just dump the Fn state in */
                RtlCopyMemory(&FxSaveArea->U.FnArea,
                              &Context->FloatSave,
                              sizeof(FNSAVE_FORMAT));
            }

            /* Mask out any invalid flags */
            FxSaveArea->Cr0NpxState &= ~(CR0_EM | CR0_MP | CR0_TS);

            /* FIXME: Check if this is a VDM app */
        }
        else
        {
            /* FIXME: Handle FPU Emulation */
            ASSERT(FALSE);
        }
    }

    /* Handle the Debug Registers */
    if ((ContextFlags & CONTEXT_DEBUG_REGISTERS) == CONTEXT_DEBUG_REGISTERS)
    {
        /* FIXME: All these should be sanitized */
        TrapFrame->Dr0 = Context->Dr0;
        TrapFrame->Dr1 = Context->Dr1;
        TrapFrame->Dr2 = Context->Dr2;
        TrapFrame->Dr3 = Context->Dr3;
        TrapFrame->Dr6 = Context->Dr6;
        TrapFrame->Dr7 = Context->Dr7;

        /* Check if usermode */
        if (PreviousMode != KernelMode)
        {
            /* Set the Debug Flag */
            KeGetCurrentThread()->DispatcherHeader.DebugActive =
                (Context->Dr7 & DR7_ACTIVE);
        }
    }

    /* Restore IRQL */
    if (OldIrql < APC_LEVEL) KeLowerIrql(OldIrql);
}

VOID
NTAPI
KeTrapFrameToContext(IN PKTRAP_FRAME TrapFrame,
                     IN PKEXCEPTION_FRAME ExceptionFrame,
                     IN OUT PCONTEXT Context)
{
    PFX_SAVE_AREA FxSaveArea;
    struct _AlignHack
    {
        UCHAR Hack[15];
        FLOATING_SAVE_AREA UnalignedArea;
    } FloatSaveBuffer;
    FLOATING_SAVE_AREA *FloatSaveArea;
    KIRQL OldIrql = APC_LEVEL;

    /* Do this at APC_LEVEL */
    if (KeGetCurrentIrql() < APC_LEVEL) KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Start with the Control flags */
    if ((Context->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL)
    {
        /* EBP, EIP and EFLAGS */
        Context->Ebp = TrapFrame->Ebp;
        Context->Eip = TrapFrame->Eip;
        Context->EFlags = TrapFrame->EFlags;

        /* Return the correct CS */
        if (!(TrapFrame->SegCs & FRAME_EDITED) &&
            !(TrapFrame->EFlags & EFLAGS_V86_MASK))
        {
            /* Get it from the Temp location */
            Context->SegCs = TrapFrame->TempSegCs & 0xFFFF;
        }
        else
        {
            /* Return it directly */
            Context->SegCs = TrapFrame->SegCs & 0xFFFF;
        }

        /* Get the Ss and ESP */
        Context->SegSs = KiSsFromTrapFrame(TrapFrame);
        Context->Esp = KiEspFromTrapFrame(TrapFrame);
    }

    /* Handle the Segments */
    if ((Context->ContextFlags & CONTEXT_SEGMENTS) == CONTEXT_SEGMENTS)
    {
        /* Do V86 Mode first */
        if (TrapFrame->EFlags & EFLAGS_V86_MASK)
        {
            /* Return from the V86 location */
            Context->SegGs = TrapFrame->V86Gs & 0xFFFF;
            Context->SegFs = TrapFrame->V86Fs & 0xFFFF;
            Context->SegEs = TrapFrame->V86Es & 0xFFFF;
            Context->SegDs = TrapFrame->V86Ds & 0xFFFF;
        }
        else
        {
            /* Check if this was a Kernel Trap */
            if (TrapFrame->SegCs == KGDT_R0_CODE)
            {
                /* Set valid selectors */
                TrapFrame->SegGs = 0;
                TrapFrame->SegFs = KGDT_R0_PCR;
                TrapFrame->SegEs = KGDT_R3_DATA | RPL_MASK;
                TrapFrame->SegDs = KGDT_R3_DATA | RPL_MASK;
            }

            /* Return the segments */
            Context->SegGs = TrapFrame->SegGs & 0xFFFF;
            Context->SegFs = TrapFrame->SegFs & 0xFFFF;
            Context->SegEs = TrapFrame->SegEs & 0xFFFF;
            Context->SegDs = TrapFrame->SegDs & 0xFFFF;
        }
    }

    /* Handle the simple registers */
    if ((Context->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
    {
        /* Return them directly */
        Context->Eax = TrapFrame->Eax;
        Context->Ebx = TrapFrame->Ebx;
        Context->Ecx = TrapFrame->Ecx;
        Context->Edx = TrapFrame->Edx;
        Context->Esi = TrapFrame->Esi;
        Context->Edi = TrapFrame->Edi;
    }

    /* Handle extended registers */
    if (((Context->ContextFlags & CONTEXT_EXTENDED_REGISTERS) ==
        CONTEXT_EXTENDED_REGISTERS) && (TrapFrame->SegCs & MODE_MASK))
    {
        /* Get the FX Save Area */
        FxSaveArea = (PFX_SAVE_AREA)(TrapFrame + 1);

        /* Make sure NPX is present */
        if (KeI386NpxPresent)
        {
            /* Flush the NPX State */
            KiFlushNPXState(NULL);

            /* Copy the registers */
            RtlCopyMemory(&Context->ExtendedRegisters[0],
                          &FxSaveArea->U.FxArea,
                          MAXIMUM_SUPPORTED_EXTENSION);
        }
    }

    /* Handle Floating Point */
    if (((Context->ContextFlags & CONTEXT_FLOATING_POINT) ==
        CONTEXT_FLOATING_POINT) && (TrapFrame->SegCs & MODE_MASK))
    {
        /* Get the FX Save Area */
        FxSaveArea = (PFX_SAVE_AREA)(TrapFrame + 1);

        /* Make sure we have an NPX */
        if (KeI386NpxPresent)
         {
            /* Check if we have Fxsr support */
            if (KeI386FxsrPresent)
            {
                /* Align the floating area to 16-bytes */
                FloatSaveArea = (FLOATING_SAVE_AREA*)
                                ((ULONG_PTR)&FloatSaveBuffer.UnalignedArea &~ 0xF);

                /* Get the State */
                KiFlushNPXState(FloatSaveArea);
            }
            else
            {
                /* We don't, use the FN area and flush the NPX State */
                FloatSaveArea = (FLOATING_SAVE_AREA*)&FxSaveArea->U.FnArea;
                KiFlushNPXState(NULL);
            }

            /* Copy into the Context */
            RtlCopyMemory(&Context->FloatSave,
                          FloatSaveArea,
                          sizeof(FNSAVE_FORMAT));
         }
         else
         {
            /* FIXME: Handle Emulation */
            ASSERT(FALSE);
         }
    }

    /* Handle debug registers */
    if ((Context->ContextFlags & CONTEXT_DEBUG_REGISTERS) ==
        CONTEXT_DEBUG_REGISTERS)
    {
        /* Copy the debug registers */
        Context->Dr0 = TrapFrame->Dr0;
        Context->Dr1 = TrapFrame->Dr1;
        Context->Dr2 = TrapFrame->Dr2;
        Context->Dr3 = TrapFrame->Dr3;
        Context->Dr6 = TrapFrame->Dr6;

        /* For user-mode, only set DR7 if a debugger is active */
        if (((TrapFrame->SegCs & MODE_MASK) ||
            (TrapFrame->EFlags & EFLAGS_V86_MASK)) &&
            (KeGetCurrentThread()->DispatcherHeader.DebugActive))
        {
            /* Copy it over */
            Context->Dr7 = TrapFrame->Dr7;
        }
        else
        {
            /* Clear it */
            Context->Dr7 = 0;
        }
    }

    /* Restore IRQL */
    if (OldIrql < APC_LEVEL) KeLowerIrql(OldIrql);
}

VOID
NTAPI
KiDispatchException(PEXCEPTION_RECORD ExceptionRecord,
                    PKEXCEPTION_FRAME ExceptionFrame,
                    PKTRAP_FRAME TrapFrame,
                    KPROCESSOR_MODE PreviousMode,
                    BOOLEAN FirstChance)
{
    CONTEXT Context;
    KD_CONTINUE_TYPE Action;
    ULONG_PTR Stack, NewStack;
    ULONG Size;
    EXCEPTION_RECORD LocalExceptRecord;
    _SEH_DECLARE_LOCALS(KiCopyInfo);

    /* Increase number of Exception Dispatches */
    KeGetCurrentPrcb()->KeExceptionDispatchCount++;

    /* Set the context flags */
    Context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;

    /* Check if User Mode or if the debugger isenabled */
    if ((PreviousMode == UserMode) || (KdDebuggerEnabled))
    {
        /* Add the FPU Flag */
        Context.ContextFlags |= CONTEXT_FLOATING_POINT;

        /* Check for NPX Support */
        if (KeI386FxsrPresent)
        {
            /* Save those too */
            Context.ContextFlags |= CONTEXT_EXTENDED_REGISTERS;
        }
    }

    /* Get a Context */
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Context);

    /* Fix up EIP */
    if (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT)
    {
        /* Decrement EIP by one */
        Context.Eip--;
    }

    ASSERT(!((PreviousMode == KernelMode) &&
             (Context.EFlags & EFLAGS_V86_MASK)));

    /* Handle kernel-mode first, it's simpler */
    if (PreviousMode == KernelMode)
    {
        /* Check if this is a first-chance exception */
        if (FirstChance == TRUE)
        {
            /* Break into the debugger for the first time */
            Action = KdpEnterDebuggerException(ExceptionRecord,
                                               PreviousMode,
                                               &Context,
                                               TrapFrame,
                                               TRUE,
                                               TRUE);

            /* If the debugger said continue, then continue */
            if (Action == kdContinue) goto Handled;

            /* If the Debugger couldn't handle it, dispatch the exception */
            if (RtlDispatchException(ExceptionRecord, &Context)) goto Handled;
        }

        /* This is a second-chance exception, only for the debugger */
        Action = KdpEnterDebuggerException(ExceptionRecord,
                                           PreviousMode,
                                           &Context,
                                           TrapFrame,
                                           FALSE,
                                           FALSE);

        /* If the debugger said continue, then continue */
        if (Action == kdContinue) goto Handled;

        /* Third strike; you're out */
        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     ExceptionRecord->ExceptionInformation[0],
                     ExceptionRecord->ExceptionInformation[1]);
    }
    else
    {
        /* User mode exception, was it first-chance? */
        if (FirstChance)
        {
            /* Enter Debugger if available */
            Action = KdpEnterDebuggerException(ExceptionRecord,
                                               PreviousMode,
                                               &Context,
                                               TrapFrame,
                                               TRUE,
                                               TRUE);

            /* Exit if we're continuing */
            if (Action == kdContinue) goto Handled;

            /* FIXME: Forward exception to user mode debugger */
            if (DbgkForwardException(ExceptionRecord, TRUE, FALSE)) goto Exit;

            /* Set up the user-stack */
DispatchToUser:
            _SEH_TRY
            {
                /* Make sure we have a valid SS and that this isn't V86 mode */
                if ((TrapFrame->HardwareSegSs != (KGDT_R3_DATA | RPL_MASK)) ||
                    (TrapFrame->EFlags & EFLAGS_V86_MASK))
                {
                    /* Raise an exception instead */
                    LocalExceptRecord.ExceptionCode = STATUS_ACCESS_VIOLATION;
                    LocalExceptRecord.ExceptionFlags = 0;
                    LocalExceptRecord.NumberParameters = 0;
                    RtlRaiseException(&LocalExceptRecord);
                }

                /* Align context size and get stack pointer */
                Size = (sizeof(CONTEXT) + 3) & ~3;
                Stack = (Context.Esp & ~3) - Size;

                /* Probe stack and copy Context */
                ProbeForWrite((PVOID)Stack, Size, sizeof(ULONG));
                RtlCopyMemory((PVOID)Stack, &Context, sizeof(CONTEXT));

                /* Align exception record size and get stack pointer */
                Size = (sizeof(EXCEPTION_RECORD) -
                       (EXCEPTION_MAXIMUM_PARAMETERS -
                        ExceptionRecord->NumberParameters) *
                       sizeof(ULONG) + 3) & ~3;
                NewStack = Stack - Size;

                /* Probe stack and copy exception record */
                ProbeForWrite((PVOID)(NewStack - 2 * sizeof(ULONG_PTR)),
                              Size +  2 * sizeof(ULONG_PTR),
                              sizeof(ULONG));
                RtlCopyMemory((PVOID)NewStack, ExceptionRecord, Size);

                /* Now write the two params for the user-mode dispatcher */
                *(PULONG_PTR)(NewStack - 1 * sizeof(ULONG_PTR)) = Stack;
                *(PULONG_PTR)(NewStack - 2 * sizeof(ULONG_PTR)) = NewStack;

                /* Set new Stack Pointer */
                KiSsToTrapFrame(TrapFrame, KGDT_R3_DATA);
                KiEspToTrapFrame(TrapFrame, NewStack - 2 * sizeof(ULONG_PTR));

                /* Force correct segments */
                TrapFrame->SegCs = KGDT_R3_CODE | RPL_MASK;
                TrapFrame->SegDs = KGDT_R3_DATA | RPL_MASK;
                TrapFrame->SegEs = KGDT_R3_DATA | RPL_MASK;
                TrapFrame->SegFs = KGDT_R3_TEB | RPL_MASK;
                TrapFrame->SegGs = 0;

                /* Set EIP to the User-mode Dispathcer */
                TrapFrame->Eip = (ULONG)KeUserExceptionDispatcher;
                _SEH_LEAVE;
            }
            _SEH_EXCEPT(KiCopyInformation)
            {
                /* Check if we got a stack overflow and raise that instead */
                if (_SEH_VAR(SehExceptRecord).ExceptionCode ==
                    STATUS_STACK_OVERFLOW)
                {
                    /* Copy the exception address and record */
                    _SEH_VAR(SehExceptRecord).ExceptionAddress =
                        ExceptionRecord->ExceptionAddress;
                    RtlMoveMemory(ExceptionRecord,
                                  (PVOID)&_SEH_VAR(SehExceptRecord),
                                  sizeof(EXCEPTION_RECORD));

                    /* Do the exception again */
                    goto DispatchToUser;
                }
            }
            _SEH_END;
        }

        /* Try second chance */
        if (DbgkForwardException(ExceptionRecord, TRUE, FALSE))
        {
            /* Handled, get out */
            goto Exit;
        }
        else if (DbgkForwardException(ExceptionRecord, FALSE, TRUE))
        {
            /* Handled, get out */
            goto Exit;
        }

        /* 3rd strike, kill the process */
        ZwTerminateProcess(NtCurrentProcess(), ExceptionRecord->ExceptionCode);
        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     ExceptionRecord->ExceptionInformation[0],
                     ExceptionRecord->ExceptionInformation[1]);
    }

Handled:
    /* Convert the context back into Trap/Exception Frames */
    KeContextToTrapFrame(&Context,
                         NULL,
                         TrapFrame,
                         Context.ContextFlags,
                         PreviousMode);
Exit:
    return;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeRaiseUserException(IN NTSTATUS ExceptionCode)
{
    ULONG OldEip;
    PKTHREAD Thread = KeGetCurrentThread();

    /* Make sure we can access the TEB */
    _SEH_TRY
    {
        Thread->Teb->ExceptionCode = ExceptionCode;
    }
    _SEH_HANDLE
    {
        return(ExceptionCode);
    }
    _SEH_END;

    /* Get the old EIP */
    OldEip = Thread->TrapFrame->Eip;

    /* Change it to the user-mode dispatcher */
    Thread->TrapFrame->Eip = (ULONG_PTR)KeRaiseUserExceptionDispatcher;

    /* Return the old EIP */
    return((NTSTATUS)OldEip);
}

