/*
 * Fast486 386/486 CPU Emulation Library
 * opgroups.h
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

#ifndef _OPGROUPS_H_
#define _OPGROUPS_H_

#pragma once

/* DEFINES ********************************************************************/

FAST486_OPCODE_HANDLER(Fast486OpcodeGroup8082);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroup81);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroup83);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroup8F);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroupC0);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroupC1);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroupC6);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroupC7);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroupD0);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroupD1);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroupD2);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroupD3);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroupF6);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroupF7);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroupFE);
FAST486_OPCODE_HANDLER(Fast486OpcodeGroupFF);

FAST486_OPCODE_HANDLER(Fast486ExtOpcodeGroup0F00);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeGroup0F01);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeGroup0FB9);
FAST486_OPCODE_HANDLER(Fast486ExtOpcodeGroup0FBA);

#endif // _OPGROUPS_H_

/* EOF */
