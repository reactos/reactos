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

VOID
INIT_FUNCTION
NTAPI
KeInitExceptions(VOID)
{
    ULONG i;
    USHORT FlippedSelector;

    /* Loop the IDT */
    for (i = 0; i <= MAXIMUM_IDTVECTOR; i++)
    {
        /* Save the current Selector */
        FlippedSelector = KiIdt[i].Selector;

        /* Flip Selector and Extended Offset */
        KiIdt[i].Selector = KiIdt[i].ExtendedOffset;
        KiIdt[i].ExtendedOffset = FlippedSelector;
    }
}

ULONG
FASTCALL
KiUpdateDr7(IN ULONG Dr7)
{
    ULONG DebugMask = KeGetCurrentThread()->Header.DebugActive;

    /* Check if debugging is enabled */
    if (DebugMask & DR_MASK(DR7_OVERRIDE_V))
    {
        /* Sanity checks */
        ASSERT((DebugMask & DR_REG_MASK) != 0);
        ASSERT((Dr7 & ~DR7_RESERVED_MASK) == DR7_OVERRIDE_MASK);
        return 0;
    }

    /* Return DR7 itself */
    return Dr7;
}

BOOLEAN
FASTCALL
KiRecordDr7(OUT PULONG Dr7Ptr,
            OUT PULONG DrMask)
{
    ULONG NewMask, Mask;
    UCHAR Result;

    /* Check if the caller gave us a mask */
    if (!DrMask)
    {
        /* He didn't, use the one from the thread */
        Mask = KeGetCurrentThread()->Header.DebugActive;
    }
    else
    {
        /* He did, read it */
        Mask = *DrMask;
    }

    /* Sanity check */
    ASSERT((*Dr7Ptr & DR7_RESERVED_MASK) == 0);

    /* Check if DR7 is empty */
    NewMask = Mask;
    if (!(*Dr7Ptr))
    {
        /* Assume failure */
        Result = FALSE;

        /* Check the DR mask */
        NewMask &= ~(DR_MASK(7));
        if (NewMask & DR_REG_MASK)
        {
            /* Set the active mask */
            NewMask |= DR_MASK(DR7_OVERRIDE_V);

            /* Set DR7 override */
            *Dr7Ptr |= DR7_OVERRIDE_MASK;
        }
        else
        {
            /* Sanity check */
            ASSERT(NewMask == 0);
        }
    }
    else
    {
        /* Check if we have a mask or not */
        Result = NewMask ? TRUE: FALSE;

        /* Update the mask to disable debugging */
        NewMask &= ~(DR_MASK(DR7_OVERRIDE_V));
        NewMask |= DR_MASK(7);
    }

    /* Check if caller wants the new mask */
    if (DrMask)
    {
        /* Update it */
        *DrMask = NewMask;
    }
    else
    {
        /* Check if the mask changed */
        if (Mask != NewMask)
        {
            /* Update it */
            KeGetCurrentThread()->Header.DebugActive = (UCHAR)NewMask;
        }
    }

    /* Return the result */
    return Result;
}

ULONG
NTAPI
KiEspFromTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    /* Check if this is user-mode or V86 */
    if (KiUserTrap(TrapFrame) ||
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
    KIRQL OldIrql;
    ULONG Previous;

    /* Raise to APC_LEVEL if needed */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < APC_LEVEL) KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Get the old ESP */
    Previous = KiEspFromTrapFrame(TrapFrame);

    /* Check if this is user-mode or V86 */
    if (KiUserTrap(TrapFrame) ||
        (TrapFrame->EFlags & EFLAGS_V86_MASK))
    {
        /* Write it directly */
        TrapFrame->HardwareEsp = Esp;
    }
    else
    {
        /* Don't allow ESP to be lowered, this is illegal */
        if (Esp < Previous) KeBugCheckEx(SET_OF_INVALID_CONTEXT,
                                         Esp,
                                         Previous,
                                         (ULONG_PTR)TrapFrame,
                                         0);

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

    /* Restore IRQL */
    if (OldIrql < APC_LEVEL) KeLowerIrql(OldIrql);
}

ULONG
NTAPI
KiSsFromTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    /* Check if this was V86 Mode */
    if (TrapFrame->EFlags & EFLAGS_V86_MASK)
    {
        /* Just return it */
        return TrapFrame->HardwareSegSs;
    }
    else if (KiUserTrap(TrapFrame))
    {
        /* User mode, return the User SS */
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
    else if (KiUserTrap(TrapFrame))
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
Ki386AdjustEsp0(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    ULONG_PTR Stack;
    ULONG EFlags;

    /* Get the current thread's stack */
    Thread = KeGetCurrentThread();
    Stack = (ULONG_PTR)Thread->InitialStack;

    /* Check if we are in V8086 mode */
    if (!(TrapFrame->EFlags & EFLAGS_V86_MASK))
    {
        /* Bias the stack for the V86 segments */
        Stack -= (FIELD_OFFSET(KTRAP_FRAME, V86Gs) -
                  FIELD_OFFSET(KTRAP_FRAME, HardwareSegSs));
    }

    /* Bias the stack for the FPU area */
    Stack -= sizeof(FX_SAVE_AREA);

    /* Disable interrupts */
    EFlags = __readeflags();
    _disable();

    /* Set new ESP0 value in the TSS */
    KeGetPcr()->TSS->Esp0 = Stack;

    /* Restore old interrupt state */
    __writeeflags(EFlags);
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
    KIRQL OldIrql;
    ULONG DrMask = 0;

    /* Do this at APC_LEVEL */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < APC_LEVEL) KeRaiseIrql(APC_LEVEL, &OldIrql);

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

        /* Copy EFLAGS and sanitize them*/
        TrapFrame->EFlags = Ke386SanitizeFlags(Context->EFlags, PreviousMode);

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
            /* We weren't in V86, so sanitize the CS */
            TrapFrame->SegCs = Ke386SanitizeSeg(Context->SegCs, PreviousMode);

            /* Don't let it under 8, that's invalid */
            if ((PreviousMode != KernelMode) && (TrapFrame->SegCs < 8))
            {
                /* Force it to User CS */
                TrapFrame->SegCs = KGDT_R3_CODE | RPL_MASK;
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
            /* Copy the V86 Segments directly */
            TrapFrame->V86Ds = Context->SegDs;
            TrapFrame->V86Es = Context->SegEs;
            TrapFrame->V86Fs = Context->SegFs;
            TrapFrame->V86Gs = Context->SegGs;
        }
        else if (!KiUserTrap(TrapFrame))
        {
            /* For kernel mode, write the standard values */
            TrapFrame->SegDs = KGDT_R3_DATA | RPL_MASK;
            TrapFrame->SegEs = KGDT_R3_DATA | RPL_MASK;
            TrapFrame->SegFs = Ke386SanitizeSeg(Context->SegFs, PreviousMode);
            TrapFrame->SegGs = 0;
        }
        else
        {
            /* For user mode, return the values directly */
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
        CONTEXT_EXTENDED_REGISTERS) && KiUserTrap(TrapFrame))
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
            FxSaveArea->U.FxArea.MXCsr &= KiMXCsrMask;

            /* Mask out any invalid flags */
            FxSaveArea->Cr0NpxState &= ~(CR0_EM | CR0_MP | CR0_TS);

            /* Check if this is a VDM app */
            if (PsGetCurrentProcess()->VdmObjects)
            {
                /* Allow the EM flag */
                FxSaveArea->Cr0NpxState |= Context->FloatSave.Cr0NpxState &
                                           (CR0_EM | CR0_MP);
            }
        }
    }

    /* Handle the floating point state */
    if (((ContextFlags & CONTEXT_FLOATING_POINT) ==
        CONTEXT_FLOATING_POINT) && KiUserTrap(TrapFrame))
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
                /* Copy the structure */
                FxSaveArea->U.FnArea.ControlWord = Context->FloatSave.
                                                   ControlWord;
                FxSaveArea->U.FnArea.StatusWord = Context->FloatSave.
                                                  StatusWord;
                FxSaveArea->U.FnArea.TagWord = Context->FloatSave.TagWord;
                FxSaveArea->U.FnArea.ErrorOffset = Context->FloatSave.
                                                   ErrorOffset;
                FxSaveArea->U.FnArea.ErrorSelector = Context->FloatSave.
                                                     ErrorSelector;
                FxSaveArea->U.FnArea.DataOffset = Context->FloatSave.
                                                  DataOffset;
                FxSaveArea->U.FnArea.DataSelector = Context->FloatSave.
                                                    DataSelector;

                /* Loop registers */
                for (i = 0; i < SIZE_OF_80387_REGISTERS; i++)
                {
                    /* Copy registers */
                    FxSaveArea->U.FnArea.RegisterArea[i] =
                        Context->FloatSave.RegisterArea[i];
                }
            }

            /* Mask out any invalid flags */
            FxSaveArea->Cr0NpxState &= ~(CR0_EM | CR0_MP | CR0_TS);

            /* Check if this is a VDM app */
            if (PsGetCurrentProcess()->VdmObjects)
            {
                /* Allow the EM flag */
                FxSaveArea->Cr0NpxState |= Context->FloatSave.Cr0NpxState &
                    (CR0_EM | CR0_MP);
            }
        }
        else
        {
            /* FIXME: Handle FPU Emulation */
            //ASSERT(FALSE);
            UNIMPLEMENTED;
        }
    }

    /* Handle the Debug Registers */
    if ((ContextFlags & CONTEXT_DEBUG_REGISTERS) == CONTEXT_DEBUG_REGISTERS)
    {
        /* Copy Dr0 - Dr4 */
        TrapFrame->Dr0 = Context->Dr0;
        TrapFrame->Dr1 = Context->Dr1;
        TrapFrame->Dr2 = Context->Dr2;
        TrapFrame->Dr3 = Context->Dr3;

        /* If we're in user-mode */
        if (PreviousMode != KernelMode)
        {
            /* Make sure, no Dr address is above user space */
            if (Context->Dr0 > (ULONG)MmHighestUserAddress) TrapFrame->Dr0 = 0;
            if (Context->Dr1 > (ULONG)MmHighestUserAddress) TrapFrame->Dr1 = 0;
            if (Context->Dr2 > (ULONG)MmHighestUserAddress) TrapFrame->Dr2 = 0;
            if (Context->Dr3 > (ULONG)MmHighestUserAddress) TrapFrame->Dr3 = 0;
        }

        /* Now sanitize and save DR6 */
        TrapFrame->Dr6 = Context->Dr6 & DR6_LEGAL;

        /* Update the Dr active mask */
        if (TrapFrame->Dr0) DrMask |= DR_MASK(0);
        if (TrapFrame->Dr1) DrMask |= DR_MASK(1);
        if (TrapFrame->Dr2) DrMask |= DR_MASK(2);
        if (TrapFrame->Dr3) DrMask |= DR_MASK(3);
        if (TrapFrame->Dr6) DrMask |= DR_MASK(6);

        /* Sanitize and save DR7 */
        TrapFrame->Dr7 = Context->Dr7 & DR7_LEGAL;
        KiRecordDr7(&TrapFrame->Dr7, &DrMask);

        /* If we're in user-mode */
        if (PreviousMode != KernelMode)
        {
            /* Save the mask */
            KeGetCurrentThread()->Header.DebugActive = (UCHAR)DrMask;
        }
    }

    /* Check if thread has IOPL and force it enabled if so */
    if (KeGetCurrentThread()->Iopl) TrapFrame->EFlags |= EFLAGS_IOPL;

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
    KIRQL OldIrql;
    ULONG i;

    /* Do this at APC_LEVEL */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < APC_LEVEL) KeRaiseIrql(APC_LEVEL, &OldIrql);

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
        CONTEXT_EXTENDED_REGISTERS) && KiUserTrap(TrapFrame))
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
        CONTEXT_FLOATING_POINT) && KiUserTrap(TrapFrame))
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

            /* Copy structure */
            Context->FloatSave.ControlWord = FloatSaveArea->ControlWord;
            Context->FloatSave.StatusWord = FloatSaveArea->StatusWord;
            Context->FloatSave.TagWord = FloatSaveArea->TagWord;
            Context->FloatSave.ErrorOffset = FloatSaveArea->ErrorOffset;
            Context->FloatSave.ErrorSelector = FloatSaveArea->ErrorSelector;
            Context->FloatSave.DataOffset = FloatSaveArea->DataOffset;
            Context->FloatSave.DataSelector = FloatSaveArea->DataSelector;
            Context->FloatSave.Cr0NpxState = FxSaveArea->Cr0NpxState;

            /* Loop registers */
            for (i = 0; i < SIZE_OF_80387_REGISTERS; i++)
            {
                /* Copy them */
                Context->FloatSave.RegisterArea[i] =
                    FloatSaveArea->RegisterArea[i];
            }
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
        /* Make sure DR7 is valid */
        if (TrapFrame->Dr7 & ~DR7_RESERVED_MASK)
        {
            /* Copy the debug registers */
            Context->Dr0 = TrapFrame->Dr0;
            Context->Dr1 = TrapFrame->Dr1;
            Context->Dr2 = TrapFrame->Dr2;
            Context->Dr3 = TrapFrame->Dr3;
            Context->Dr6 = TrapFrame->Dr6;

            /* Update DR7 */
            Context->Dr7 = KiUpdateDr7(TrapFrame->Dr7);
        }
        else
        {
            /* Otherwise clear DR registers */
            Context->Dr0 =
            Context->Dr1 =
            Context->Dr2 =
            Context->Dr3 =
            Context->Dr6 =
            Context->Dr7 = 0;
        }
    }

    /* Restore IRQL */
    if (OldIrql < APC_LEVEL) KeLowerIrql(OldIrql);
}

BOOLEAN
FASTCALL
KeInvalidAccessAllowed(IN PVOID TrapInformation OPTIONAL)
{
    ULONG Eip;
    PKTRAP_FRAME TrapFrame = TrapInformation;
    VOID NTAPI ExpInterlockedPopEntrySListFault(VOID);

    /* Don't do anything if we didn't get a trap frame */
    if (!TrapInformation) return FALSE;

    /* Check where we came from */
    switch (TrapFrame->SegCs)
    {
        /* Kernel mode */
        case KGDT_R0_CODE:

            /* Allow S-LIST Routine to fail */
            Eip = (ULONG)&ExpInterlockedPopEntrySListFault;
            break;

        /* User code */
        case KGDT_R3_CODE | RPL_MASK:

            /* Allow S-LIST Routine to fail */
            //Eip = (ULONG)KeUserPopEntrySListFault;
            Eip = 0;
            break;

        default:

            /* Anything else gets a bugcheck */
            Eip = 0;
    }

    /* Return TRUE if we want to keep the system up */
    return (TrapFrame->Eip == Eip) ? TRUE : FALSE;
}

VOID
NTAPI
KiDispatchException(IN PEXCEPTION_RECORD ExceptionRecord,
                    IN PKEXCEPTION_FRAME ExceptionFrame,
                    IN PKTRAP_FRAME TrapFrame,
                    IN KPROCESSOR_MODE PreviousMode,
                    IN BOOLEAN FirstChance)
{
    CONTEXT Context;
    EXCEPTION_RECORD LocalExceptRecord;

    /* Increase number of Exception Dispatches */
    KeGetCurrentPrcb()->KeExceptionDispatchCount++;

    /* Set the context flags */
    Context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;

    /* Check if User Mode or if the kernel debugger is enabled */
    if ((PreviousMode == UserMode) || (KeGetPcr()->KdVersionBlock))
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

    /* Look at our exception code */
    switch (ExceptionRecord->ExceptionCode)
    {
        /* Breakpoint */
        case STATUS_BREAKPOINT:

            /* Decrement EIP by one */
            Context.Eip--;
            break;

        /* Internal exception */
        case KI_EXCEPTION_ACCESS_VIOLATION:

            /* Set correct code */
            ExceptionRecord->ExceptionCode = STATUS_ACCESS_VIOLATION;
            if (PreviousMode == UserMode)
            {
                /* FIXME: Handle no execute */
            }
            break;
    }

    /* Sanity check */
    ASSERT(!((PreviousMode == KernelMode) &&
             (Context.EFlags & EFLAGS_V86_MASK)));

    /* Handle kernel-mode first, it's simpler */
    if (PreviousMode == KernelMode)
    {
        /* Check if this is a first-chance exception */
        if (FirstChance != FALSE)
        {
            /* Break into the debugger for the first time */
            if (KiDebugRoutine(TrapFrame,
                               ExceptionFrame,
                               ExceptionRecord,
                               &Context,
                               PreviousMode,
                               FALSE))
            {
                /* Exception was handled */
                goto Handled;
            }

            /* If the Debugger couldn't handle it, dispatch the exception */
            if (RtlDispatchException(ExceptionRecord, &Context)) goto Handled;
        }

        /* This is a second-chance exception, only for the debugger */
        if (KiDebugRoutine(TrapFrame,
                           ExceptionFrame,
                           ExceptionRecord,
                           &Context,
                           PreviousMode,
                           TRUE))
        {
            /* Exception was handled */
            goto Handled;
        }

        /* Third strike; you're out */
        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     (ULONG_PTR)TrapFrame,
                     0);
    }
    else
    {
        /* User mode exception, was it first-chance? */
        if (FirstChance)
        {
            /*
             * Break into the kernel debugger unless a user mode debugger
             * is present or user mode exceptions are ignored, except if this
             * is a debug service which we must always pass to KD
             */
            if ((!(PsGetCurrentProcess()->DebugPort) &&
                 !(KdIgnoreUmExceptions)) ||
                 (KdIsThisAKdTrap(ExceptionRecord,
                                  &Context,
                                  PreviousMode)))
            {
                /* Call the kernel debugger */
                if (KiDebugRoutine(TrapFrame,
                                   ExceptionFrame,
                                   ExceptionRecord,
                                   &Context,
                                   PreviousMode,
                                   FALSE))
                {
                    /* Exception was handled */
                    goto Handled;
                }
            }

            /* Forward exception to user mode debugger */
            if (DbgkForwardException(ExceptionRecord, TRUE, FALSE)) return;

            /* Set up the user-stack */
DispatchToUser:
            _SEH2_TRY
            {
                ULONG Size;
                ULONG_PTR Stack, NewStack;

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
                TrapFrame->SegCs = Ke386SanitizeSeg(KGDT_R3_CODE, PreviousMode);
                TrapFrame->SegDs = Ke386SanitizeSeg(KGDT_R3_DATA, PreviousMode);
                TrapFrame->SegEs = Ke386SanitizeSeg(KGDT_R3_DATA, PreviousMode);
                TrapFrame->SegFs = Ke386SanitizeSeg(KGDT_R3_TEB,  PreviousMode);
                TrapFrame->SegGs = 0;

                /* Set EIP to the User-mode Dispatcher */
                TrapFrame->Eip = (ULONG)KeUserExceptionDispatcher;

                /* Dispatch exception to user-mode */
                _SEH2_YIELD(return);
            }
            _SEH2_EXCEPT((RtlCopyMemory(&LocalExceptRecord, _SEH2_GetExceptionInformation()->ExceptionRecord, sizeof(EXCEPTION_RECORD)), EXCEPTION_EXECUTE_HANDLER))
            {
                /* Check if we got a stack overflow and raise that instead */
                if ((NTSTATUS)LocalExceptRecord.ExceptionCode ==
                    STATUS_STACK_OVERFLOW)
                {
                    /* Copy the exception address and record */
                    LocalExceptRecord.ExceptionAddress =
                        ExceptionRecord->ExceptionAddress;
                    RtlCopyMemory(ExceptionRecord,
                                  (PVOID)&LocalExceptRecord,
                                  sizeof(EXCEPTION_RECORD));

                    /* Do the exception again */
                    _SEH2_YIELD(goto DispatchToUser);
                }
            }
            _SEH2_END;

            DPRINT("First chance exception in %.16s, ExceptionCode: %lx, ExceptionAddress: %p, P0: %lx, P1: %lx\n",
                   PsGetCurrentProcess()->ImageFileName,
                   ExceptionRecord->ExceptionCode,
                   ExceptionRecord->ExceptionAddress,
                   ExceptionRecord->ExceptionInformation[0],
                   ExceptionRecord->ExceptionInformation[1]);
        }

        /* Try second chance */
        if (DbgkForwardException(ExceptionRecord, TRUE, TRUE))
        {
            /* Handled, get out */
            return;
        }
        else if (DbgkForwardException(ExceptionRecord, FALSE, TRUE))
        {
            /* Handled, get out */
            return;
        }

        /* 3rd strike, kill the process */
        DPRINT1("Kill %.16s, ExceptionCode: %lx, ExceptionAddress: %p, BaseAddress: %p, P0: %lx, P1: %lx\n",
                PsGetCurrentProcess()->ImageFileName,
                ExceptionRecord->ExceptionCode,
                ExceptionRecord->ExceptionAddress,
                PsGetCurrentProcess()->SectionBaseAddress,
                ExceptionRecord->ExceptionInformation[0],
                ExceptionRecord->ExceptionInformation[1]);

        ZwTerminateProcess(NtCurrentProcess(), ExceptionRecord->ExceptionCode);
        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     (ULONG_PTR)TrapFrame,
                     0);
    }

Handled:
    /* Convert the context back into Trap/Exception Frames */
    KeContextToTrapFrame(&Context,
                         ExceptionFrame,
                         TrapFrame,
                         Context.ContextFlags,
                         PreviousMode);
    return;
}

DECLSPEC_NORETURN
VOID
NTAPI
KiDispatchExceptionFromTrapFrame(IN NTSTATUS Code,
                                 IN ULONG Flags,
                                 IN ULONG_PTR Address,
                                 IN ULONG ParameterCount,
                                 IN ULONG_PTR Parameter1,
                                 IN ULONG_PTR Parameter2,
                                 IN ULONG_PTR Parameter3,
                                 IN PKTRAP_FRAME TrapFrame)
{
    EXCEPTION_RECORD ExceptionRecord;

    /* Build the exception record */
    ExceptionRecord.ExceptionCode = Code;
    ExceptionRecord.ExceptionFlags = Flags;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.ExceptionAddress = (PVOID)Address;
    ExceptionRecord.NumberParameters = ParameterCount;
    if (ParameterCount)
    {
        /* Copy extra parameters */
        ExceptionRecord.ExceptionInformation[0] = Parameter1;
        ExceptionRecord.ExceptionInformation[1] = Parameter2;
        ExceptionRecord.ExceptionInformation[2] = Parameter3;
    }

    /* Now go dispatch the exception */
    KiDispatchException(&ExceptionRecord,
                        NULL,
                        TrapFrame,
                        TrapFrame->EFlags & EFLAGS_V86_MASK ?
                        -1 : KiUserTrap(TrapFrame),
                        TRUE);

    /* Return from this trap */
    KiEoiHelper(TrapFrame);
}

DECLSPEC_NORETURN
VOID
FASTCALL
KiSystemFatalException(IN ULONG ExceptionCode,
                       IN PKTRAP_FRAME TrapFrame)
{
    /* Bugcheck the system */
    KeBugCheckWithTf(UNEXPECTED_KERNEL_MODE_TRAP,
                     ExceptionCode,
                     0,
                     0,
                     0,
                     TrapFrame);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeRaiseUserException(IN NTSTATUS ExceptionCode)
{
    ULONG OldEip;
    PTEB Teb = KeGetCurrentThread()->Teb;
    PKTRAP_FRAME TrapFrame = KeGetCurrentThread()->TrapFrame;

    /* Make sure we can access the TEB */
    _SEH2_TRY
    {
        /* Set the exception code */
        Teb->ExceptionCode = ExceptionCode;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return the exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Get the old EIP */
    OldEip = TrapFrame->Eip;

    /* Change it to the user-mode dispatcher */
    TrapFrame->Eip = (ULONG_PTR)KeRaiseUserExceptionDispatcher;

    /* Return the old EIP */
    return (NTSTATUS)OldEip;
}
