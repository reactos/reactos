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

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

#define SOFT386_NUM_OPCODE_HANDLERS 256
#define SOFT386_OPCODE_WRITE_REG (1 << 1)
#define SOFT386_OPCODE_HANDLER(x) BOOLEAN\
                                  FASTCALL\
                                  (x)(PSOFT386_STATE State, UCHAR Opcode)

typedef BOOLEAN (FASTCALL *SOFT386_OPCODE_HANDLER_PROC)(PSOFT386_STATE, UCHAR);

extern
SOFT386_OPCODE_HANDLER_PROC
Soft386OpcodeHandlers[SOFT386_NUM_OPCODE_HANDLERS];

SOFT386_OPCODE_HANDLER(Soft386OpcodePrefix);
SOFT386_OPCODE_HANDLER(Soft386OpcodeIncrement);
SOFT386_OPCODE_HANDLER(Soft386OpcodeDecrement);
SOFT386_OPCODE_HANDLER(Soft386OpcodePushReg);
SOFT386_OPCODE_HANDLER(Soft386OpcodePopReg);
SOFT386_OPCODE_HANDLER(Soft386OpcodeNop);
SOFT386_OPCODE_HANDLER(Soft386OpcodeExchangeEax);
SOFT386_OPCODE_HANDLER(Soft386OpcodeShortConditionalJmp);
SOFT386_OPCODE_HANDLER(Soft386OpcodeClearCarry);
SOFT386_OPCODE_HANDLER(Soft386OpcodeSetCarry);
SOFT386_OPCODE_HANDLER(Soft386OpcodeComplCarry);
SOFT386_OPCODE_HANDLER(Soft386OpcodeClearInt);
SOFT386_OPCODE_HANDLER(Soft386OpcodeSetInt);
SOFT386_OPCODE_HANDLER(Soft386OpcodeClearDir);
SOFT386_OPCODE_HANDLER(Soft386OpcodeSetDir);
SOFT386_OPCODE_HANDLER(Soft386OpcodeHalt);
SOFT386_OPCODE_HANDLER(Soft386OpcodeInByte);
SOFT386_OPCODE_HANDLER(Soft386OpcodeIn);
SOFT386_OPCODE_HANDLER(Soft386OpcodeOutByte);
SOFT386_OPCODE_HANDLER(Soft386OpcodeOut);
SOFT386_OPCODE_HANDLER(Soft386OpcodeShortJump);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovRegImm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovByteRegImm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAddByteModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAddModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAddAl);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAddEax);

#endif // _OPCODES_H_
