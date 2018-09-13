/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmnpx.c

Abstract:

    This module contains the support for Vdm use of the npx.

Author:

    Dave Hastings (daveh) 02-Feb-1992


Revision History:
    18-Dec-1992 sudeepb Tuned all the routines for performance

--*/

#include <ntos.h>
#include <vdmntos.h>
#include "vdmp.h"

#ifdef ALLOC_PRAGMA
#pragma  alloc_text(PAGE, VdmDispatchIRQ13)
#pragma  alloc_text(PAGE, VdmSkipNpxInstruction)
#endif

static UCHAR MOD16[] = { 0, 1, 2, 0 };
static UCHAR MOD32[] = { 0, 1, 4, 0 };
UCHAR VdmUserCr0MapIn[] = {
    /* !EM !MP */       0,
    /* !EM  MP */       CR0_PE,             // Don't set MP, but shadow users MP setting
    /*  EM !MP */       CR0_EM,
    /*  EM  MP */       CR0_EM | CR0_MP
    };

UCHAR VdmUserCr0MapOut[] = {
    /* !EM !MP !PE */   0,
    /* !EM !MP  PE */   CR0_MP,
    /* !EM  MP !PE */   CR0_MP,             // setting not valid
    /* !EM  MP  PE */   CR0_MP,             // setting not valid
    /*  EM !MP !PE */   CR0_EM,
    /*  EM !MP  PE */   CR0_EM | CR0_MP,    // setting not valid
    /*  EM  MP !PE */   CR0_EM | CR0_MP,
    /*  EM  MP  PE */   CR0_EM | CR0_MP     // setting not valid
    };


BOOLEAN
VdmDispatchIRQ13(
    PKTRAP_FRAME TrapFrame
    )
/*++

aRoutine Description:

    This routine reflects an IRQ 13 event to the usermode monitor for this
    vdm.  The IRQ 13 must be reflected to usermode, so that it can properly
    be raised as an interrupt through the virtual PIC.

Arguments:

    none

Return Value:

    TRUE if the event was reflected
    FALSE if not

--*/
{
    EXCEPTION_RECORD ExceptionRecord;
    PVDM_TIB VdmTib;
    BOOLEAN Success;
    NTSTATUS Status;

    PAGED_CODE();

    Success = TRUE;

    Status = VdmpGetVdmTib(&VdmTib, VDMTIB_KPROBE);
    if (!NT_SUCCESS(Status)) {
       return(FALSE);
    }

    try {
        VdmTib->EventInfo.Event = VdmIrq13;
        VdmTib->EventInfo.InstructionSize = 0L;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ExceptionRecord.ExceptionCode = GetExceptionCode();
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.NumberParameters = 0;
        ExRaiseException(&ExceptionRecord);
        Success = FALSE;
    }

    if (Success)  {             // insure that we do not redispatch an exception
        VdmEndExecution(TrapFrame,VdmTib);
        }


    return TRUE;
}

BOOLEAN
VdmSkipNpxInstruction(
    PKTRAP_FRAME TrapFrame,
    ULONG        Address32Bits,
    PUCHAR       istream,
    ULONG        InstructionSize
    )
/*++

Routine Description:

    This functions gains control when the system has no installed
    NPX support, but the thread has cleared it's EM bit in CR0.

    The purpose of this function is to move the instruction
    pointer forward over the current NPX instruction.

Enviroment:

    V86 MODE ONLY, first opcode byte already verified to be 0xD8 - 0xDF.

Arguments:

Return Value:

    TRUE if trap frame was modified to skip the NPX instruction

--*/
{
    UCHAR       ibyte, Mod, rm;

    ASSERT (!KeI386NpxPresent);

    //
    // This NPX instruction should be skipped
    //

    try {
        //
        // Get ModR/M byte for NPX opcode
        //

        istream += 1;
        ibyte = *istream;
        InstructionSize += 1;

        if (ibyte > 0xbf) {
            //
            // Outside of ModR/M range for addressing, all done
            //

            goto try_exit;
        }

        Mod = ibyte >> 6;
        rm  = ibyte & 0x7;
        if (Address32Bits) {
            InstructionSize += MOD32 [Mod];
            if (Mod == 0  &&  rm == 5) {
                // disp 32
                InstructionSize += 4;
            }

            //
            // If SIB byte, read it
            //

            if (rm == 4) {
                istream += 1;
                ibyte = *istream;
                InstructionSize += 1;

                if (Mod == 0  &&  (ibyte & 7) == 5) {
                    // disp 32
                    InstructionSize += 4;
                }
            }

        } else {
            InstructionSize += MOD16 [Mod];
            if (Mod == 0  &&  rm == 6) {
                // disp 16
                InstructionSize += 2;
            }
        }

try_exit:  ;
    } except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // Some sort of fault !?
        //

#if DBG
        DbgPrint ("P5_FPU_PATCH: V86: Fault occured\n");
#endif
        return FALSE;
    }

    //
    // Adjust Eip to skip NPX instruction
    //

    TrapFrame->Eip += InstructionSize;
    TrapFrame->Eip &= 0xffff;

    return TRUE;
}
