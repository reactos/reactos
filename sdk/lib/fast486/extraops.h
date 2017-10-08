/*
 * Fast486 386/486 CPU Emulation Library
 * extraops.h
 *
 * Copyright (C) 2015 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
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

#ifndef _EXTRAOPS_H_
#define _EXTRAOPS_H_

#pragma once

/* DEFINES ********************************************************************/

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeInvalid);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeUnimplemented);
FAST486_OPCODE_HANDLER(Fast486ExtOpcode0F0B);

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeLar);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeLsl);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeClts);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeStoreControlReg);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeStoreDebugReg);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeLoadControlReg);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeLoadDebugReg);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodePushFs);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodePopFs);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBitTest);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeShld);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodePushGs);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodePopGs);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBts);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeShrd);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeImul);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeCmpXchgByte);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeCmpXchg);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeLss);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBtr);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeLfsLgs);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeMovzxByte);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeMovzxWord);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBtc);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBsf);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBsr);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeMovsxByte);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeMovsxWord);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeConditionalJmp);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeConditionalSet);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeXaddByte);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeXadd);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeBswap);
FAST486_OPCODE_HANDLER(Fast486OpcodeExtended);

#endif // _EXTRAOPS_H_

/* EOF */
