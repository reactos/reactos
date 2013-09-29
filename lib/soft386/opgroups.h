/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            opgroups.h
 * PURPOSE:         Opcode group handlers. (header file)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
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

