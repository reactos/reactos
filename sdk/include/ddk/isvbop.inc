/*
 * isvbop.inc
 *
 * Windows NT Device Driver Kit
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/*
 * This is the corresponding ASM header for isvbop.h.
 * Please refer to isvbop.h for information about these interfaces.
 */

#include <asm.inc>

BOP_3RDPARTY    = HEX(58)
BOP_UNSIMULATE  = HEX(FE)

MACRO(RegisterModule)
    .byte HEX(C4), HEX(C4), BOP_3RDPARTY, 0
ENDM

MACRO(UnRegisterModule)
    .byte HEX(C4), HEX(C4), BOP_3RDPARTY, 1
ENDM

MACRO(DispatchCall)
    .byte HEX(C4), HEX(C4), BOP_3RDPARTY, 2
ENDM

MACRO(VDDUnSimulate16)
    .byte HEX(C4), HEX(C4), BOP_UNSIMULATE
ENDM

/* EOF */
