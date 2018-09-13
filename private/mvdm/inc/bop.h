/*++ BUILD Version: 0001

Copyright (c) 1990  Microsoft Corporation

Module Name:

    BOP.H

Abstract:

    This module contains macro support for use of Bops in C code.

Author:

    Dave Hastings (daveh) 25-Apr-1991

Revision History:

--*/

//
// Assigned Bop Numbers
//

#define BOP_DOS              0x50
#define BOP_WOW              0x51
#define BOP_XMS              0x52
#define BOP_DPMI             0x53
#define BOP_CMD              0x54
#define BOP_DEBUGGER         0x56
#define BOP_REDIR            0x57    // used to be 55, now goes to MS_bop_7()
#define BOP_NOSUPPORT        0x59    // host warning dialog box
#define BOP_WAITIFIDLE       0x5A    // idle bop
#define BOP_DBGBREAKPOINT    0x5B    // does a 32 bit DbgBreakPoint
#define BOP_KBD              0x5C    // BUGBUG temporary
#define BOP_VIDEO            0x5D    // BUGBUG temporary
#define BOP_NOTIFICATION     0x5E    // 16bits to 32 bits notification
#define BOP_UNIMPINT         0x5F    // BUGBUG temporary
#define BOP_SWITCHTOREALMODE 0xFD
#define BOP_UNSIMULATE       0xFE    // end execution of code in a vdm

#define BOP_SIZE         3       // # of bytes in a bop instruction
//
// Bop Macro
//

/* XLATOFF */

#define BOP(BopNumber) _asm db 0xC4, 0xC4, BopNumber

/* XLATON */

/* ASM
BOP macro BopNumber
    db  0C4h, 0C4h, BopNumber
        endm

IFNDEF WOW_x86
FBOP macro BopNumber,BopMinorNumber,FastBopEntry
    BOP BopNumber
ifnb <BopMinorNumber>
    db  BopMinorNumber
endif
    endm
ELSE
FBOP macro BopNumber,BopMinorNumber,FastBopEntry
    local fb10,fb20
    test    word ptr [FastBopEntry + 4],0FFFFh
    jz  fb10
.386p
    push    ds
    push    40h
    pop     ds
    test    ds:[FIXED_NTVDMSTATE_REL40],RM_BIT_MASK
    pop     ds
    jnz     short fb10
	call fword ptr [FastBopEntry]

    db BopNumber            ; indicates which bop
ifnb <BopMinorNumber>
    db BopMinorNumber
endif
	jmp short fb20

.286p
fb10:   BOP BopNumber
ifnb <BopMinorNumber>
    db  BopMinorNumber
endif
fb20:
    endm
endif
 */
