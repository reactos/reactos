/*
 * Fast486 386/486 CPU Emulation Library
 * fpu.h
 *
 * Copyright (C) 2014 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
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

#include "opcodes.h"

/* DEFINES ********************************************************************/

#define FPU_CHECK() if (State->ControlRegisters[FAST486_REG_CR0] & (FAST486_CR0_EM | FAST486_CR0_TS)) \
                    { \
                        Fast486Exception(State, FAST486_EXCEPTION_NM); \
                        return; \
                    }
#define FPU_ST(i) State->FpuRegisters[(State->FpuStatus.Top + (i)) % FAST486_NUM_FPU_REGS]
#define FPU_GET_TAG(i)  ((State->FpuTag >> ((i) * 2)) & 3)
#define FPU_SET_TAG(i, t)   { \
                                State->FpuTag &= ~((1 << ((i) * 2)) | (1 << (((i) * 2) + 1))); \
                                State->FpuTag |= ((t) & 3) << ((i) * 2); \
                            }

#define FPU_REAL4_BIAS 0x7F
#define FPU_REAL8_BIAS 0x3FF
#define FPU_REAL10_BIAS 0x3FFF
#define FPU_MAX_EXPONENT 0x7FFE
#define FPU_MANTISSA_HIGH_BIT 0x8000000000000000ULL

#define FPU_IS_NORMALIZED(x) (!FPU_IS_ZERO(x) && (((x)->Mantissa & FPU_MANTISSA_HIGH_BIT) != 0ULL))
#define FPU_IS_ZERO(x) ((x)->Mantissa == 0ULL)
#define FPU_IS_NAN(x) ((x)->Exponent == (FPU_MAX_EXPONENT + 1))
#define FPU_IS_INFINITY(x) (FPU_IS_NAN(x) && (x)->Mantissa & FPU_MANTISSA_HIGH_BIT)
#define FPU_IS_POS_INF(x) (FPU_IS_INFINITY(x) && !(x)->Sign)
#define FPU_IS_NEG_INF(x) (FPU_IS_INFINITY(x) && (x)->Sign)

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

enum
{
    FPU_ROUND_NEAREST = 0,
    FPU_ROUND_DOWN = 1,
    FPU_ROUND_UP = 2,
    FPU_ROUND_TRUNCATE = 3
};

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeD8DC);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeD9);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDA);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDB);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDD);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDE);
FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDF);

#endif // _FPU_H_

/* EOF */
