/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    trapc.c

Abstract:

    This module contains some trap handling code written in C.
    Only by the kernel.

Author:

    Ken Reneris     6-9-93

Revision History:

--*/

#include    "ki.h"

NTSTATUS
Ki386CheckDivideByZeroTrap (
    IN  PKTRAP_FRAME    UserFrame
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ki386CheckDivideByZeroTrap)
#endif


#define REG(field)          ((ULONG)(&((KTRAP_FRAME *)0)->field))
#define GETREG(frame,reg)   ((PULONG) (((ULONG) frame)+reg))[0]

typedef struct {
    UCHAR   RmDisplaceOnly;     // RM of displacment only, no base reg
    UCHAR   RmSib;              // RM of SIB
    UCHAR   RmDisplace;         // bit mask of RMs which have a displacement
    UCHAR   Disp;               // sizeof displacement (in bytes)
} KMOD, *PKMOD;

static UCHAR RM32[] = {
    /* 000 */   REG(Eax),
    /* 001 */   REG(Ecx),
    /* 010 */   REG(Edx),
    /* 011 */   REG(Ebx),
    /* 100 */   REG(HardwareEsp),
    /* 101 */   REG(Ebp),       // SIB
    /* 110 */   REG(Esi),
    /* 111 */   REG(Edi)
};

static UCHAR RM8[] = {
    /* 000 */   REG(Eax),       // al
    /* 001 */   REG(Ecx),       // cl
    /* 010 */   REG(Edx),       // dl
    /* 011 */   REG(Ebx),       // bl
    /* 100 */   REG(Eax) + 1,   // ah
    /* 101 */   REG(Ecx) + 1,   // ch
    /* 110 */   REG(Edx) + 1,   // dh
    /* 111 */   REG(Ebx) + 1    // bh
};

static KMOD MOD32[] = {
    /* 00 */     5,     4,   0x20,   4,
    /* 01 */  0xff,     4,   0xff,   1,
    /* 10 */  0xff,     4,   0xff,   4,
    /* 11 */  0xff,  0xff,   0x00,   0
} ;

static struct {
    UCHAR   Opcode1, Opcode2;   // instruction opcode
    UCHAR   ModRm, type;        // if 2nd part of opcode is encoded in ModRm
} NoWaitNpxInstructions[] = {
    /* FNINIT   */  0xDB, 0xE3, 0,  1,
    /* FNCLEX   */  0xDB, 0xE2, 0,  1,
    /* FNSTENV  */  0xD9, 0x06, 1,  1,
    /* FNSAVE   */  0xDD, 0x06, 1,  1,
    /* FNSTCW   */  0xD9, 0x07, 1,  2,
    /* FNSTSW   */  0xDD, 0x07, 1,  3,
    /* FNSTSW AX*/  0xDF, 0xE0, 0,  4,
                    0x00, 0x00, 0,  1
};


NTSTATUS
Ki386CheckDivideByZeroTrap (
    IN  PKTRAP_FRAME    UserFrame
    )
/*++

Routine Description:

    This function gains control when the x86 processor generates a
    divide by zero trap.  The x86 design generates such a trap on
    divide by zero and on division overflows.  In order to determine
    which expection code to dispatch, the divisor of the "div" or "idiv"
    instruction needs to be inspected.

Arguments:

    UserFrame - Trap frame of the divide by zero trap

Return Value:

    exception code dispatch

--*/
{
    ULONG       operandsize, operandmask, i, accum;
    PUCHAR      istream, pRM;
    UCHAR       ibyte, rm;
    PKMOD       Mod;
    BOOLEAN     fPrefix;
    NTSTATUS    status;

    status = STATUS_INTEGER_DIVIDE_BY_ZERO;

    if (UserFrame->SegCs == KGDT_R0_CODE) {
        //
        // Divide by zero exception from Kernel Mode?
        // Likely bad hardware interrupt and the device or vector table is corrupt.
        // Bugcheck NOW so we can figure out what went wrong. If we try and
        // proceed, then we'll likely fault in reading the top of user space, and then
        // double fault (page fault in the div zero handler.) -- This is a debugging
        // consideration. You can't put breakpoints on the trap labels  so this is hard to debug.
        // We cannot recover
        //
        //
        KeBugCheck (UNEXPECTED_KERNEL_MODE_TRAP);

    }


    try {

        //
        // read instruction prefixes
        //

        fPrefix = TRUE;
        pRM = RM32;
        operandsize = 4;
        operandmask = 0xffffffff;
        istream = (PUCHAR) UserFrame->Eip;
        while (fPrefix) {
            ibyte = ProbeAndReadUchar(istream);
            istream++;
            switch (ibyte) {
                case 0x2e:  // cs override
                case 0x36:  // ss override
                case 0x3e:  // ds override
                case 0x26:  // es override
                case 0x64:  // fs override
                case 0x65:  // gs override
                case 0xF3:  // rep
                case 0xF2:  // rep
                case 0xF0:  // lock
                    break;

                case 0x66:
                    // 16 bit operand override
                    operandsize = 2;
                    operandmask = 0xffff;
                    break;

                case 0x67:
                    // 16 bit address size override
                    // this is some non-flat code
                    goto try_exit;

                default:
                    fPrefix = FALSE;
                    break;
            }
        }

        //
        // Check instruction opcode
        //

        if (ibyte != 0xf7  &&  ibyte != 0xf6) {
            // this is not a DIV or IDIV opcode
            goto try_exit;
        }

        if (ibyte == 0xf6) {
            // this is a byte div or idiv
            operandsize = 1;
            operandmask = 0xff;
        }

        //
        // Get Mod R/M
        //

        ibyte = ProbeAndReadUchar (istream);
        istream++;
        Mod = MOD32 + (ibyte >> 6);
        rm  = ibyte & 7;

        //
        // put register values into accum
        //

        if (operandsize == 1  &&  (ibyte & 0xc0) == 0xc0) {
            pRM = RM8;
        }

        accum = 0;
        if (rm != Mod->RmDisplaceOnly) {
            if (rm == Mod->RmSib) {
                // get SIB
                ibyte = ProbeAndReadUchar(istream);
                istream++;
                i = (ibyte >> 3) & 7;
                if (i != 4) {
                    accum = GETREG(UserFrame, RM32[i]);
                    accum = accum << (ibyte >> 6);    // apply scaler
                }
                i = ibyte & 7;
                accum = accum + GETREG(UserFrame, RM32[i]);
            } else {
                // get register's value
                accum = GETREG(UserFrame, pRM[rm]);
            }
        }

        //
        // apply displacement to accum
        //

        if (Mod->RmDisplace & (1 << rm)) {
            if (Mod->Disp == 4) {
                i = ProbeAndReadUlong ((PULONG) istream);
            } else {
                ibyte = ProbeAndReadChar (istream);
                i = (signed long) ((signed char) ibyte);    // sign extend
            }
            accum += i;
        }

        //
        // if this is an effective address, go get the data value
        //

        if (Mod->Disp) {
            switch (operandsize) {
                case 1:  accum = ProbeAndReadUchar((PUCHAR) accum);    break;
                case 2:  accum = ProbeAndReadUshort((PUSHORT) accum);  break;
                case 4:  accum = ProbeAndReadUlong((PULONG) accum);    break;
            }
        }

        //
        // accum now contains the instruction operand, see if the
        // operand was really a zero
        //

        if (accum & operandmask) {
            // operand was non-zero, must be an overflow
            status = STATUS_INTEGER_OVERFLOW;
        }

try_exit: ;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        // do nothing...
    }

    return status;
}

UCHAR
KiNextIStreamByte (
    IN  PKTRAP_FRAME UserFrame,
    IN  PUCHAR  *istream
    )
/*++

Routine Description:

    Reads the next byte from the istream pointed to by the UserFrame, and
    advances the EIP.


    Note: this function works for 32 bit code only

--*/
{
    UCHAR   ibyte;

    if (UserFrame->SegCs == KGDT_R0_CODE) {
        ibyte = **istream;
    } else {
        ibyte = ProbeAndReadUchar (*istream);
    }

    *istream += 1;
    return ibyte;
}




BOOLEAN
Ki386CheckDelayedNpxTrap (
    IN  PKTRAP_FRAME UserFrame,
    IN  PFX_SAVE_AREA NpxFrame
    )

/*++

Routine Description:

    This function gains control from the Trap07 handler.  It examines
    the user mode instruction to see if it's a NoWait NPX instruction.
    Such instructions do not generate floating point exceptions - this
    check needs to be done due to the way 80386/80387 systems are
    implemented.  Such machines will generate a floating point exception
    interrupt when the kernel performs an FRSTOR to reload the thread's
    NPX context.  If the thread's next instruction is a NoWait style
    instruction, then we clear the exception or emulate the instruction.

    AND... due to a different 80386/80387 "feature" the kernel needs
    to use FWAIT at times which can causes 80487's to generate delayed
    exceptions that can lead to the same problem described above.

Arguments:

    UserFrame - Trap frame of the exception
    NpxFrame - Thread's NpxFrame  (WARNING: does not have NpxState)

    Interrupts are disabled

Return Value:

    FALSE - Dispatch NPX exception to user mode
    TRUE - Exception handled, continue

--*/

{
    EXCEPTION_RECORD ExceptionRecord;
    UCHAR       ibyte1, ibyte2, inmodrm, status;
    USHORT      StatusWord, ControlWord, UsersWord;
    PUCHAR      istream;
    BOOLEAN     fPrefix;
    UCHAR       rm;
    PKMOD       Mod;
    ULONG       accum, i;

    status = 0;
    try {

        //
        // read instruction prefixes
        //

        fPrefix = TRUE;
        istream = (PUCHAR) UserFrame->Eip;
        while (fPrefix) {
            ibyte1 = KiNextIStreamByte (UserFrame, &istream);
            switch (ibyte1) {
                case 0x2e:  // cs override
                case 0x36:  // ss override
                case 0x3e:  // ds override
                case 0x26:  // es override
                case 0x64:  // fs override
                case 0x65:  // gs override
                    break;

                default:
                    fPrefix = FALSE;
                    break;
            }
        }

        //
        // Check for coprocessor NoWait NPX instruction
        //

        ibyte2 = KiNextIStreamByte (UserFrame, &istream);
        inmodrm = (ibyte2 >> 3) & 0x7;

        for (i=0; NoWaitNpxInstructions[i].Opcode1; i++) {
            if (NoWaitNpxInstructions[i].Opcode1 == ibyte1) {

                //
                // first opcode byte matched - check second part of opcode
                //

                if (NoWaitNpxInstructions[i].ModRm) {

                    //
                    // modrm only applies for opcode in range 0-0xbf
                    //

                    if (((ibyte2 & 0xc0) != 0xc0) &&
                        (NoWaitNpxInstructions[i].Opcode2 == inmodrm)) {

                        //
                        // This is a no-wait NPX instruction
                        //

                        status = NoWaitNpxInstructions[i].type;
                        break;
                    }

                } else {
                    if (NoWaitNpxInstructions[i].Opcode2 == ibyte2) {

                        //
                        // This is a no-wait NPX instruction
                        //

                        status = NoWaitNpxInstructions[i].type;
                        break;
                    }
                }
            }
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        // do nothing...
    }

    if (status == 0) {
        //
        // Dispatch coprocessor exception to user mode
        //

        return FALSE;
    }

    if (status == 1) {
        //
        // Ignore pending exception, user mode instruction does not trap
        // on pending execptions and it will clear/mask the pending exceptions
        //

        _asm {
            mov     eax, cr0
            and     eax, NOT (CR0_MP+CR0_EM+CR0_TS)
            mov     cr0, eax
        }

        NpxFrame->Cr0NpxState &= ~CR0_TS;
        return TRUE;
    }

    //
    // This is either FNSTSW or FNSTCW.  Both of these instructions get
    // a value from the coprocessor without effecting the pending exception
    // state.  To do this we emulate the instructions.
    //

    //
    // Read the coprocessors Status & Control word state, then re-enable
    // interrupts.  (it's safe to context switch after that point)
    //

    _asm {
        mov     eax, cr0
        mov     ecx, eax
        and     eax, NOT (CR0_MP+CR0_EM+CR0_TS)
        mov     cr0, eax

        fnstsw  StatusWord
        fnstcw  ControlWord

        mov     cr0, ecx
        sti
    }

    if (status == 4) {
        //
        // Emulate FNSTSW AX
        //

        UserFrame->Eip = (ULONG)istream;
        UserFrame->Eax = (UserFrame->Eax & 0xFFFF0000) | StatusWord;
        return TRUE;
    }

    if (status == 2) {
        UsersWord = ControlWord;
    } else {
        UsersWord = StatusWord;
    }

    try {

        //
        // (PERFNOTE: the operand decode code should really share code with
        // KiCheckDivideByZeroTrap, but this is a late change therefore the
        // code was copied to keep the impact of the change localized)
        //

        //
        // decode Mod/RM byte
        //

        Mod = MOD32 + (ibyte2 >> 6);
        rm  = ibyte2 & 7;

        //
        // Decode the instruction's word pointer into accum
        //

        accum = 0;
        if (rm != Mod->RmDisplaceOnly) {
            if (rm == Mod->RmSib) {
                // get SIB
                ibyte1 = KiNextIStreamByte (UserFrame, &istream);
                i = (ibyte1 >> 3) & 7;
                if (i != 4) {
                    accum = GETREG(UserFrame, RM32[i]);
                    accum = accum << (ibyte1 >> 6);    // apply scaler
                }
                i = ibyte1 & 7;
                accum = accum + GETREG(UserFrame, RM32[i]);
            } else {
                // get register's value
                accum = GETREG(UserFrame, RM32[rm]);
            }
        }

        //
        // apply displacement to accum
        //

        if (Mod->RmDisplace & (1 << rm)) {
            if (Mod->Disp == 4) {
                i = (KiNextIStreamByte (UserFrame, &istream) << 0) |
                    (KiNextIStreamByte (UserFrame, &istream) << 8) |
                    (KiNextIStreamByte (UserFrame, &istream) << 16) |
                    (KiNextIStreamByte (UserFrame, &istream) << 24);
            } else {
                ibyte1 = KiNextIStreamByte (UserFrame, &istream);
                i = (signed long) ((signed char) ibyte1);    // sign extend
            }
            accum += i;
        }

        //
        // Set the word pointer
        //

        if (UserFrame->SegCs == KGDT_R0_CODE) {
            *((PUSHORT) accum) = UsersWord;
        } else {
            ProbeAndWriteUshort ((PUSHORT) accum, UsersWord);
        }
        UserFrame->Eip = (ULONG)istream;

    } except (KiCopyInformation(&ExceptionRecord,
                (GetExceptionInformation())->ExceptionRecord)) {
        //
        // Faulted addressing user's memory.
        // Set the address of the exception to the current program address
        // and raise the exception by calling the exception dispatcher.
        //

        ExceptionRecord.ExceptionAddress = (PVOID)(UserFrame->Eip);
        KiDispatchException(
            &ExceptionRecord,
            NULL,                // ExceptionFrame
            UserFrame,
            UserMode,
            TRUE
        );
    }

    return TRUE;
}
