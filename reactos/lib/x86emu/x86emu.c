/*
 * PROJECT:         x86 CPU emulator
 * LICENSE:         GPL, See COPYING in the top level directory
 * FILE:            lib/x86emu/x86emu.c
 * PURPOSE:         
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <x86emu.h>
//#define NDEBUG
#include <debug.h>

#undef DPRINT
#define DPRINT DbgPrint

/* GLOBALS *******************************************************************/

static const ULONG RegisterTable[3][8] =
{
    {
        FIELD_OFFSET(X86_REGISTERS, Al),
        FIELD_OFFSET(X86_REGISTERS, Cl),
        FIELD_OFFSET(X86_REGISTERS, Dl),
        FIELD_OFFSET(X86_REGISTERS, Bl),
        FIELD_OFFSET(X86_REGISTERS, Ah),
        FIELD_OFFSET(X86_REGISTERS, Ch),
        FIELD_OFFSET(X86_REGISTERS, Dh),
        FIELD_OFFSET(X86_REGISTERS, Bh),
    },
    {
        FIELD_OFFSET(X86_REGISTERS, Ax),
        FIELD_OFFSET(X86_REGISTERS, Cx),
        FIELD_OFFSET(X86_REGISTERS, Dx),
        FIELD_OFFSET(X86_REGISTERS, Bx),
        FIELD_OFFSET(X86_REGISTERS, Sp),
        FIELD_OFFSET(X86_REGISTERS, Bp),
        FIELD_OFFSET(X86_REGISTERS, Si),
        FIELD_OFFSET(X86_REGISTERS, Di),
    },
    {
        FIELD_OFFSET(X86_REGISTERS, Eax),
        FIELD_OFFSET(X86_REGISTERS, Ecx),
        FIELD_OFFSET(X86_REGISTERS, Edx),
        FIELD_OFFSET(X86_REGISTERS, Ebx),
        FIELD_OFFSET(X86_REGISTERS, Esp),
        FIELD_OFFSET(X86_REGISTERS, Ebp),
        FIELD_OFFSET(X86_REGISTERS, Esi),
        FIELD_OFFSET(X86_REGISTERS, Edi),
    }
};

/* INLINE FUNCTONS ***********************************************************/

#include "vmstate.h"
#include "op_cmp.h"
#include "op_stack.h"
#include "op_jump.h"
#include "op_mov.h"

VOID
FORCEINLINE
Opcode_E8_CALL16(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    SHORT Offset;

    Offset = *(PSHORT)(IntructionPointer + 1) + 3;
    DPRINT("CALL %x\n", VmState->Registers.Ip + Offset);
    VmState_Push(VmState, VmState->Registers.Ip + 3);
    VmState_AdvanceIp(VmState, Offset);
}

VOID
FORCEINLINE
Opcode_80(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    MODRM ModRm;
    UCHAR Value;
    
    ModRm.Byte = IntructionPointer[1];

    if (ModRm.reg == 7)
    {
        /* We have an 8 bit CMP */
        DPRINT("CMP ??. 0x%x\n", IntructionPointer[2]);
        Value = VmState_GetVal8(VmState, ModRm);
        VmState_CMP8(VmState, Value, IntructionPointer[2]);
        VmState_AdvanceIp(VmState, 3);
        return;
    }
    DPRINT1("UNKNOWN\n");
}

VOID
FORCEINLINE
Opcode_F3_REP(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    UCHAR ByteVal;
    
    ByteVal = IntructionPointer[1];
    switch (ByteVal)
    {
        case 0x6C: /* REP INSB */
        case 0x6D: /* REP INSW/D */
        case 0xAC: /* REP LODSB */
        case 0xAD: /* REP LODSWDQ */
        case 0xA4: /* REP MOVSB */
        case 0xA5: /* REP MOVSW/D/Q */
        case 0x6E: /* REP OUTSB */
        case 0x6F: /* REP OUTSW/D */
        case 0xAA: /* REP STOSB */
        case 0xAB: /* REP STOSW/D/Q */
            break;
    }

}





VOID
NTAPI
x86Emulator(PX86_VM_STATE VmState)
{
    PCHAR InstructionPointer;
    UCHAR ByteVal;

//    x86EmuInitializeState();
    
    for (;;)
    {
        InstructionPointer = VmState_GetIp(VmState);
        ByteVal = *InstructionPointer;
        
        DPRINT("%04x:%04x  %02x  ", VmState->Registers.SegCs, VmState->Registers.Eip, ByteVal);

        switch (ByteVal)
        {
            case 0x06: /* PUSH ES */
                Opcode_06_PUSH_ES(VmState, InstructionPointer);
                break;

            case 0x07: /* POP ES */
                Opcode_07_POP_ES(VmState, InstructionPointer);
                break;

//            case 0x14: /* ADC AL, imm8 */
//                VmState_AdvanceIp(VmState, 2);
//                break;

            case 0x15: /* ADC AX, imm16 */
                //_OpcodeADC16(&VmState->Registers.Al, WORD(&InstructionPointer[1]));
                VmState_AdvanceIp(VmState, 3);
                break;

            case 0x1E: /* PUSH DS */
                Opcode_1E_PUSH_DS(VmState, InstructionPointer);
                break;

            case 0x1F: /* POP DS */
                Opcode_1F_POP_DS(VmState, InstructionPointer);
                break;

            case 0x26: /* Force ES segment */
            case 0x2e: /* Force CS segment */
                VmState_EnablePrefix(VmState, PREFIX_SEGMENT_CS);
                VmState_AdvanceIp(VmState, 1);
                continue;

            case 0x36: /* Force SS segment */
                VmState->Registers.ShiftedMs = VmState->Registers.ShiftedSs;
                VmState_AdvanceIp(VmState, 1);
                continue;

            case 0x37: /* AAA */

            case 0x3D: /* CMP (E)AX, imm16/32 */
                Opcode_3D_CMP(VmState, InstructionPointer);
                break;

            case 0x3E: /* Force DS segment */
                VmState->Registers.ShiftedMs = VmState->Registers.ShiftedDs;
                VmState_AdvanceIp(VmState, 1);
                continue;

            case 0x3F: /* AAS */
                VmState_AdvanceIp(VmState, 1);
                break;

            /* 0x40 ... 0x4F are REX prefixes */

            case 0x55: /* PUSH BP */
                Opcode_55_PUSH_BP(VmState, InstructionPointer);
                break;

            case 0x60: /* PUSHA */
                Opcode_60_PUSHA(VmState, InstructionPointer);
                break;

            case 0x61: /* POPA */
                Opcode_61_POPA(VmState, InstructionPointer);
                break;

            case 0x64: /* Force FS segment */
            case 0x65: /* Force GS segment */

            case 0x66: /* Operand size override */
                VmState_EnablePrefix(VmState, PREFIX_SIZE_OVERRIDE);
                VmState_AdvanceIp(VmState, 1);
                continue;

            case 0x67: /* Address size prefix */
                VmState_EnablePrefix(VmState, PREFIX_ADDRESS_OVERRIDE);
                VmState_AdvanceIp(VmState, 1);
                continue;

            case 0x74: /* JZ */
                Opcode_74_JZ8(VmState, InstructionPointer);
                break;

            case 0x75: /* JNZ */
                Opcode_75_JNZ8(VmState, InstructionPointer);
                break;

            case 0x80: /* ... */
                Opcode_80(VmState, InstructionPointer);
                break;

            case 0x89: /* MOV regmem16, reg16 */
                Opcode_89_MOV(VmState, InstructionPointer);
                break;

            case 0x8E: /* MOV seg, reg16 */
                Opcode_8E_MOV(VmState, InstructionPointer);
                break;

            case 0x9C: /* PUSHF */
                Opcode_9C_PUSHF(VmState, InstructionPointer);
                break;

            case 0x9D: /* POPF */
                Opcode_9D_POPF(VmState, InstructionPointer);
                break;

            case 0xBB: /* MOV BX, imm16 */
                Opcode_BB_MOV(VmState, InstructionPointer);
                break;

            case 0xCF: /* IRET */
                DPRINT("IRET\n");
                return;

            case 0xD4: /* AAM */
                /* Check for D4 0A */

            case 0xD5: /* AAD */
                /* Check for D5 0A */

            case 0xE8:
                Opcode_E8_CALL16(VmState, InstructionPointer);
                break;

            case 0xE9: /* JMP off16 */
                Opcode_E9_JMP16(VmState, InstructionPointer);
                break;

            case 0xF0: /* LOCK (ignored) */
                DPRINT("LOCK ");
                VmState_AdvanceIp(VmState, 1);
                continue;

            case 0xF2: /* REPNZ/REPNE */
                DPRINT("REPNE ");
                VmState_EnablePrefix(VmState, PREFIX_REP);
                VmState_AdvanceIp(VmState, 1);
                continue;

            case 0xF3: /* REP */
                Opcode_F3_REP(VmState, InstructionPointer);
                break;


            default:
                DPRINT("Unknown opcode 0x%x\n", ByteVal);
                VmState_AdvanceIp(VmState, 1);
                //x86EmuRaiseException(EXCEPTION_INVALID_OPCODE, ByteVal);
                //return;
        }

        /* Clear prefixes and continue with next intruction */
        VmState_ClearPrefixes(VmState);

//ResetMs:
        VmState->Registers.ShiftedMs = VmState->Registers.ShiftedDs;

    }
}
