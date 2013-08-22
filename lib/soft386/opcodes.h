/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            opcodes.h
 * PURPOSE:         Opcode handlers. (header file)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _OPCODES_H_
#define _OPCODES_H_

/* DEFINES ********************************************************************/

#define SOFT386_NUM_OPCODE_HANDLERS 256

typedef BOOLEAN (__fastcall *SOFT386_OPCODE_HANDLER_PROC)(PSOFT386_STATE);

extern
SOFT386_OPCODE_HANDLER_PROC
Soft386OpcodeHandlers[SOFT386_NUM_OPCODE_HANDLERS];

#endif // _OPCODES_H_
