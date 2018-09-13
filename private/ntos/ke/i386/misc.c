/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module implements machine dependent miscellaneous kernel functions.

Author:

    Ken Reneris     7-5-95

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

extern BOOLEAN KeI386FxsrPresent;
extern BOOLEAN KeI386XMMIPresent;

//
// Internal format of the floating_save structure which is passed
//
typedef struct _CONTROL_WORD {
    USHORT      ControlWord;
    ULONG       MXCsr;
} CONTROL_WORD, *PCONTROL_WORD;

typedef struct {
    UCHAR       Flags;
    KIRQL       Irql;
    KIRQL       PreviousNpxIrql;
    UCHAR       Spare[2];

    union {
        CONTROL_WORD    Fcw;
        PFX_SAVE_AREA   Context;
    } u;
    ULONG       Cr0NpxState;

    PKTHREAD    Thread;         // debug

} FLOAT_SAVE, *PFLOAT_SAVE;


#define FLOAT_SAVE_COMPLETE_CONTEXT     0x01
#define FLOAT_SAVE_FREE_CONTEXT_HEAP    0x02
#define FLOAT_SAVE_VALID                0x04
#define FLOAT_SAVE_RESERVED             0xF8

#pragma warning(disable:4035)               // re-enable below

// notes:
// Kix86FxSave(NpxFame) - performs an FxSave to the address specificied
//                   - no other action occurs
__inline
KIRQL
Kix86FxSave(
    PULONG NpxFrame
    )
{
    _asm {
        mov eax, NpxFrame
        ;fxsave [eax]
        _emit  0fh
        _emit  0aeh
        _emit   0
    }
}

// Kix86FnSave(NpxFame) - performs an FxSave to the address specificied
//                   - no other action occurs
__inline
KIRQL
Kix86FnSave(
    PULONG NpxFrame
    )
{
    __asm {
        mov eax, NpxFrame
        fnsave [eax]
    }
}

//
// Load Katmai New Instruction Technology Control/Status
//
__inline
KIRQL
Kix86LdMXCsr(
    PULONG MXCsr
    )
{
    _asm {
        mov eax, MXCsr
        ;LDMXCSR [eax]
        _emit  0fh
        _emit  0aeh
        _emit  10h
    }
}

//
// Store Katmai New Instruction Technology Control/Status
//
__inline
KIRQL
Kix86StMXCsr(
    PULONG MXCsr
    )
{
    _asm {
        mov eax, MXCsr
        ;STMXCSR [eax]
        _emit  0fh
        _emit  0aeh
        _emit  18h
    }
}
#pragma warning(default:4035)


NTSTATUS
KeSaveFloatingPointState (
    OUT PKFLOATING_SAVE     PublicFloatSave
    )
/*++

Routine Description:

    This routine saves the thread's current non-volatile NPX state,
    and sets a new initial floating point state for the caller.

Arguments:

    FloatSave - receives the current non-volatile npx state for the thread

Return Value:

--*/
{
    PKTHREAD Thread;
    PFX_SAVE_AREA NpxFrame;
    KIRQL                   Irql;
    USHORT                  ControlWord;
    ULONG                   MXCsr;
    PKPRCB                  Prcb;
    PFLOAT_SAVE             FloatSave;

    //
    // If the system is using floating point emulation, then
    // return an error
    //

    if (!KeI386NpxPresent) {
        return STATUS_ILLEGAL_FLOAT_CONTEXT;
    }

    //
    // Get the current irql and thread
    //

    FloatSave = (PFLOAT_SAVE) PublicFloatSave;

    Irql = KeGetCurrentIrql();
    Thread = KeGetCurrentThread();

    ASSERT (Thread->NpxIrql <= Irql);

    FloatSave->Flags           = 0;
    FloatSave->Irql            = Irql;
    FloatSave->PreviousNpxIrql = Thread->NpxIrql;
    FloatSave->Thread          = Thread;

    //
    // If the irql has changed we need to save the complete floating
    // state context as the prior level has been interrupted.
    //

    if (Thread->NpxIrql != Irql) {

        //
        // If this is apc level we don't have anyplace to hold this
        // context, allocate some heap.
        //

        if (Irql == APC_LEVEL) {
            FloatSave->u.Context = ExAllocatePoolWithTag (
                                        NonPagedPool,
                                        sizeof (FX_SAVE_AREA),
                                        ' XPN'
                                        );

            if (!FloatSave->u.Context) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            FloatSave->Flags |= FLOAT_SAVE_FREE_CONTEXT_HEAP;

        } else {

            ASSERT (Irql == DISPATCH_LEVEL);
            FloatSave->u.Context = &KeGetCurrentPrcb()->NpxSaveArea;

        }

        FloatSave->Flags |= FLOAT_SAVE_COMPLETE_CONTEXT;
    }

    //
    // Stop context switching and allow access to the local fp unit
    //

    _asm {
        cli
        mov     eax, cr0
        mov     ecx, eax
        and     eax, not (CR0_MP|CR0_EM|CR0_TS)
        cmp     eax, ecx
        je      short sav10

        mov     cr0, eax
sav10:
    }

    Prcb = KeGetCurrentPrcb();

    //
    // Get ownership of npx register set for this context
    //

    if (Prcb->NpxThread != Thread) {

        //
        // If the other context is loaded in the npx registers, flush
        // it to that threads save area
        //
        if (Prcb->NpxThread) {

            NpxFrame = (PFX_SAVE_AREA)(((ULONG)(Prcb->NpxThread->InitialStack) -
                        sizeof(FX_SAVE_AREA)));

            if (KeI386FxsrPresent) {
                Kix86FxSave((PULONG)NpxFrame);
            } else {
                Kix86FnSave((PULONG)NpxFrame);
            }

            NpxFrame->NpxSavedCpu = 0;
            Prcb->NpxThread->NpxState = NPX_STATE_NOT_LOADED;

        }

        Prcb->NpxThread = Thread;
    }

    NpxFrame = (PFX_SAVE_AREA)(((ULONG)(Thread->InitialStack) -
                sizeof(FX_SAVE_AREA)));


    //
    // Save the previous state as required
    //

    if (FloatSave->Flags & FLOAT_SAVE_COMPLETE_CONTEXT) {

        //
        // Need to save the entire context
        //

        if (Thread->NpxState == NPX_STATE_LOADED) {
            if (KeI386FxsrPresent) {
                Kix86FxSave ((PULONG)(FloatSave->u.Context));
            } else {
                Kix86FnSave ((PULONG)(FloatSave->u.Context));
            }

            FloatSave->u.Context->NpxSavedCpu = 0;
            FloatSave->u.Context->Cr0NpxState = NpxFrame->Cr0NpxState;

        } else {
            RtlCopyMemory (FloatSave->u.Context, NpxFrame, sizeof(FX_SAVE_AREA));
            FloatSave->u.Context->NpxSavedCpu = 0;

        }

    } else {

        //
        // Save only the non-volatile state
        //

        if (Thread->NpxState == NPX_STATE_LOADED) {

            _asm {
                mov     eax, FloatSave
                fnstcw  [eax] FLOAT_SAVE.u.Fcw.ControlWord
            }

            if ((KeI386FxsrPresent) && (KeI386XMMIPresent)) {
                Kix86StMXCsr(&FloatSave->u.Fcw.MXCsr);
            }

        } else {
            //
            // Save the control word from the npx frame.
            //

            if (KeI386FxsrPresent) {
                FloatSave->u.Fcw.ControlWord = (USHORT) NpxFrame->U.FxArea.ControlWord;
                FloatSave->u.Fcw.MXCsr = NpxFrame->U.FxArea.MXCsr;
        
            } else {
                FloatSave->u.Fcw.ControlWord = (USHORT) NpxFrame->U.FnArea.ControlWord;
            }
        }


        //
        // Save Cr0NpxState, but clear CR0_TS as there's not non-volatile
        // pending fp exceptions
        //

        FloatSave->Cr0NpxState = NpxFrame->Cr0NpxState & ~CR0_TS;
    }

    //
    // The previous state is saved.  Set an initial default
    // FP state for the caller
    //

    NpxFrame->Cr0NpxState = 0;
    Thread->NpxState = NPX_STATE_LOADED;
    Thread->NpxIrql  = Irql;
    ControlWord = 0x27f;    // 64bit mode
    MXCsr = 0x1f80;

    _asm {
        fninit
        fldcw       ControlWord
    }

    if ((KeI386FxsrPresent) && (KeI386XMMIPresent)) {
        Kix86LdMXCsr(&MXCsr);
    }

    _asm {
        sti
    }

    FloatSave->Flags |= FLOAT_SAVE_VALID;
    return STATUS_SUCCESS;
}


NTSTATUS
KeRestoreFloatingPointState (
    IN PKFLOATING_SAVE      PublicFloatSave
    )
/*++

Routine Description:

    This routine retores the thread's current non-volatile NPX state,
    to the passed in state.

Arguments:

    FloatSave - the non-volatile npx state for the thread to restore

Return Value:

--*/
{
    PKTHREAD Thread;
    PFX_SAVE_AREA NpxFrame;
    ULONG                   Cr0State;
    PFLOAT_SAVE             FloatSave;

    ASSERT (KeI386NpxPresent);

    FloatSave = (PFLOAT_SAVE) PublicFloatSave;
    Thread = FloatSave->Thread;
    
    NpxFrame = (PFX_SAVE_AREA)(((ULONG)(Thread->InitialStack) -
                sizeof(FX_SAVE_AREA)));

    
    //
    // Verify float save looks like it's from the right context
    //

    if ((FloatSave->Flags & (FLOAT_SAVE_VALID | FLOAT_SAVE_RESERVED)) != FLOAT_SAVE_VALID) {

        // BUGBUG: need to pick a good bugcheck code
        // KeBugCheck (...);
        DbgPrint("KeRestoreFloatingPointState: Invalid float save area\n");
        DbgBreakPoint();
    }

    if (FloatSave->Irql != KeGetCurrentIrql()) {
        // BUGBUG: need to pick a good bugcheck code
        // KeBugCheck (...);
        DbgPrint("KeRestoreFloatingPointState: Invalid IRQL\n");
        DbgBreakPoint();
    }

    if (Thread != KeGetCurrentThread()) {
        // BUGBUG: need to pick a good bugcheck code
        // KeBugCheck (...);
        DbgPrint("KeRestoreFloatingPointState: Invalid thread\n");
        DbgBreakPoint();
    }


    //
    // Synchronize with context switches and the npx trap handlers
    //

    _asm {
        cli
    }

    //
    // Restore the required state
    //

    if (FloatSave->Flags & FLOAT_SAVE_COMPLETE_CONTEXT) {

        //
        // Restore the entire fp state to the threads save area
        //

        if (Thread->NpxState == NPX_STATE_LOADED) {

            //
            // This state in the fp unit is no longer needed, just disregard it
            //

            Thread->NpxState = NPX_STATE_NOT_LOADED;
            KeGetCurrentPrcb()->NpxThread = NULL;
        }

        //
        // Copy restored state to npx frame
        //

        RtlCopyMemory (NpxFrame, FloatSave->u.Context, sizeof(FX_SAVE_AREA));

    } else {

        //
        // Restore the non-volatile state
        //

        if (Thread->NpxState == NPX_STATE_LOADED) {

            //
            // Init fp state and restore control word
            //

            _asm {
                fninit
                mov     eax, FloatSave
                fldcw   [eax] FLOAT_SAVE.u.Fcw.ControlWord
            }

            
            if ((KeI386FxsrPresent) && (KeI386XMMIPresent)) {
                Kix86LdMXCsr(&FloatSave->u.Fcw.MXCsr);
            }
            

        } else {

            //
            // Fp state not loaded.  Restore control word in npx frame
            //

            if (KeI386FxsrPresent) {
                NpxFrame->U.FxArea.ControlWord = FloatSave->u.Fcw.ControlWord;
                NpxFrame->U.FxArea.StatusWord = 0;
                NpxFrame->U.FxArea.TagWord = 0;
                NpxFrame->NpxSavedCpu = 0;
                NpxFrame->U.FxArea.MXCsr = FloatSave->u.Fcw.MXCsr;
                 
            } else {
                NpxFrame->U.FnArea.ControlWord = FloatSave->u.Fcw.ControlWord;
                NpxFrame->U.FnArea.StatusWord = 0;
                NpxFrame->U.FnArea.TagWord = 0xffff;
            }

        }

        NpxFrame->Cr0NpxState = FloatSave->Cr0NpxState;
    }

    //
    // Restore NpxIrql and Cr0
    //

    Thread->NpxIrql = FloatSave->PreviousNpxIrql;
    Cr0State = Thread->NpxState | NpxFrame->Cr0NpxState;

    _asm {
        mov     eax, cr0
        mov     ecx, eax
        and     eax, not (CR0_MP|CR0_EM|CR0_TS)
        or      eax, Cr0State
        cmp     eax, ecx
        je      short res10
        mov     cr0, eax
res10:
        sti
    }

    //
    // Done
    //

    if (FloatSave->Flags & FLOAT_SAVE_FREE_CONTEXT_HEAP) {
        ExFreePool (FloatSave->u.Context);
    }

    FloatSave->Flags = 0;
    return STATUS_SUCCESS;
}

#pragma optimize( "y", off )    // disable frame pointer omission (FPO)

VOID
__cdecl
KeSaveStateForHibernate(
    IN PKPROCESSOR_STATE ProcessorState
    )
/*++

Routine Description:

    Saves all processor-specific state that must be preserved
    across an S4 state (hibernation).

    N.B. #pragma surrounding this function is required in order
         to create the frame pointer than RtlCaptureContext relies
         on.
    N.B. _CRTAPI1 (__cdecl) decoration is also required so that
         RtlCaptureContext can compute the correct ESP.

Arguments:

    ProcessorState - Supplies the KPROCESSOR_STATE where the
        current CPU's state is to be saved.

Return Value:

    None.

--*/

{
    RtlCaptureContext(&ProcessorState->ContextFrame);
    KiSaveProcessorControlState(ProcessorState);
}
#pragma optimize( "y", off )    // disable frame pointer omission (FPO)
