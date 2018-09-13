/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    iafptrap.c

Abstract:

    This is based on the i386 trapc.c module with very minor changes.
    It would be nice if there wasn't so much duplicate code so that
    fixes to that file would carry over to this one...
    
    This module contains some trap handling code written in C.
    Only by the kernel.

Author:

    Ken Reneris     6-9-93

Revision History:

--*/

#include    "ki.h"
#include    "ia32def.h"

NTSTATUS
Ki386CheckDivideByZeroTrap (
    IN  PKTRAP_FRAME    UserFrame
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ki386CheckDivideByZeroTrap)
#endif


#define REG(field)          ((ULONG_PTR)(&((KTRAP_FRAME *)0)->field))
#define GETREG(frame,reg)   ((PULONG) (((ULONG_PTR) frame)+reg))[0]

typedef struct {
    UCHAR   RmDisplaceOnly;     // RM of displacment only, no base reg
    UCHAR   RmSib;              // RM of SIB
    UCHAR   RmDisplace;         // bit mask of RMs which have a displacement
    UCHAR   Disp;               // sizeof displacement (in bytes)
} KMOD, *PKMOD;

static ULONG_PTR RM32[] = {
    /* 000 */   REG(IntV0),         // EAX
    /* 001 */   REG(IntT2),         // ECX
    /* 010 */   REG(IntT3),         // EDX
    /* 011 */   REG(IntT4),         // EBX
    /* 100 */   REG(IntSp),         // ESP
    /* 101 */   REG(IntTeb),        // EBP
    /* 110 */   REG(IntT5),         // ESI
    /* 111 */   REG(IntT6)          // EDI
};

static ULONG_PTR RM8[] = {
    /* 000 */   REG(IntV0),       // al
    /* 001 */   REG(IntT2),       // cl
    /* 010 */   REG(IntT3),       // dl
    /* 011 */   REG(IntT4),       // bl
    /* 100 */   REG(IntV0) + 1,   // ah
    /* 101 */   REG(IntT2) + 1,   // ch
    /* 110 */   REG(IntT3) + 1,   // dh
    /* 111 */   REG(IntT4) + 1    // bh
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
    ULONG       operandsize, operandmask, i;
    ULONG_PTR   accum;
    PUCHAR      istream;
    ULONG_PTR   *pRM;
    UCHAR       ibyte, rm;
    PKMOD       Mod;
    BOOLEAN     fPrefix;
    NTSTATUS    status;

    status = STATUS_INTEGER_DIVIDE_BY_ZERO;

    try {

        //
        // read instruction prefixes
        //

        fPrefix = TRUE;
        pRM = RM32;
        operandsize = 4;
        operandmask = 0xffffffff;
        istream = (PUCHAR) EIP(UserFrame);
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
                //
                // BUGBUG: Seems like this could be a problem if rm points
                // to the 8 bit registers (see line 200 above (pRm = RM8))
                // since the GETREG routine gets a ULONG's worth of data
                // 1) The rm8 means there is only 8 significant bits, not 32
                // 2) If grabbing from the high half, pointer is to an odd
                //    address when grab 32-bits - a misalign fault...
                // 
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
