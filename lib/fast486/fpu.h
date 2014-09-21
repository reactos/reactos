/*
 * Fast486 386/486 CPU Emulation Library
 * fpu.h
 *
 * Copyright (C) 2013 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _FPU_H_
#define _FPU_H_

#pragma once

/* DEFINES ********************************************************************/

#define FPU_CHECK() if (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_EM) \
                    { \
                        Fast486Exception(State, FAST486_EXCEPTION_NM); \
                        return FALSE; \
                    }
#define FPU_ST(i) State->FpuRegisters[(State->FpuStatus.Top + (i)) % FAST486_NUM_FPU_REGS]

enum
{
    FPU_SINGLE_PRECISION = 0,
    FPU_DOUBLE_PRECISION = 2,
    FPU_DOUBLE_EXT_PRECISION = 3
};

enum
{
    FPU_TAG_VALID = 0,
    FPU_TAG_ZERO = 1,
    FPU_TAG_SPECIAL = 2,
    FPU_TAG_EMPTY = 3
};

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeD8);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeD9);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDA);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDB);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDC);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDD);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDE);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDF);

#endif // _FPU_H_

/* EOF */
