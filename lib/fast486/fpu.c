/*
 * Fast486 386/486 CPU Emulation Library
 * fpu.c
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

/* INCLUDES *******************************************************************/

#include <windef.h>

// #define NDEBUG
#include <debug.h>

#include <fast486.h>
#include "common.h"
#include "opcodes.h"
#include "fpu.h"

/* PUBLIC FUNCTIONS ***********************************************************/

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeD8)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;
    Fast486Exception(State, FAST486_EXCEPTION_UD);
    return FALSE;
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeD9)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;
    Fast486Exception(State, FAST486_EXCEPTION_UD);
    return FALSE;
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDA)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;
    Fast486Exception(State, FAST486_EXCEPTION_UD);
    return FALSE;
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDB)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;
    Fast486Exception(State, FAST486_EXCEPTION_UD);
    return FALSE;
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDC)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;
    Fast486Exception(State, FAST486_EXCEPTION_UD);
    return FALSE;
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDD)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;
    Fast486Exception(State, FAST486_EXCEPTION_UD);
    return FALSE;
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDE)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;
    Fast486Exception(State, FAST486_EXCEPTION_UD);
    return FALSE;
}

FAST486_OPCODE_HANDLER(Fast486FpuOpcodeDF)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;
    Fast486Exception(State, FAST486_EXCEPTION_UD);
    return FALSE;
}

/* EOF */
