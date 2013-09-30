/*
 * Soft386 386/486 CPU Emulation Library
 * opgroups.h
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

#ifndef _OPGROUPS_H_
#define _OPGROUPS_H_

/* DEFINES ********************************************************************/

SOFT386_OPCODE_HANDLER(Soft386OpcodeGroup8082);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroup81);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroup83);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroup8F);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupC0);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupC1);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupC6);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupC7);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupD0);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupD1);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupD2);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupD3);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupF6);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupF7);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupFE);
SOFT386_OPCODE_HANDLER(Soft386OpcodeGroupFF);

#endif // _OPGROUPS_H_

/* EOF */

