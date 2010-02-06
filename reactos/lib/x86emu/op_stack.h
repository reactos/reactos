

VOID
FORCEINLINE
Opcode_9C_PUSHF(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    DPRINT("PUSHF\n");
    VmState_Push(VmState, VmState->Registers.Eflags.Short);
    VmState_AdvanceIp(VmState, 1);
}

VOID
FORCEINLINE
Opcode_9D_POPF(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    DPRINT("POPF\n");
    VmState->Registers.Eflags.Short = VmState_Pop(VmState);
    VmState_AdvanceIp(VmState, 1);
}

VOID
FORCEINLINE
Opcode_1E_PUSH_DS(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    DPRINT("PUSH DS\n");
    VmState_Push(VmState, VmState->Registers.SegDs);
    VmState_AdvanceIp(VmState, 1);
}

VOID
FORCEINLINE
Opcode_1F_POP_DS(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    DPRINT("POP DS\n");
    VmState->Registers.SegDs = VmState_Pop(VmState);
    VmState_AdvanceIp(VmState, 1);
}

VOID
FORCEINLINE
Opcode_06_PUSH_ES(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    DPRINT("PUSH ES\n");
    VmState_Push(VmState, VmState->Registers.SegEs);
    VmState_AdvanceIp(VmState, 1);
}

VOID
FORCEINLINE
Opcode_07_POP_ES(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    DPRINT("POP ES\n");
    VmState->Registers.SegEs = VmState_Pop(VmState);
    VmState_AdvanceIp(VmState, 1);
}

VOID
FORCEINLINE
Opcode_60_PUSHA(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    USHORT OrigSp;
    DPRINT("PUSHA\n");

    OrigSp = VmState->Registers.Sp;
    VmState_Push(VmState, VmState->Registers.Ax);
    VmState_Push(VmState, VmState->Registers.Cx);
    VmState_Push(VmState, VmState->Registers.Dx);
    VmState_Push(VmState, VmState->Registers.Bx);
    VmState_Push(VmState, OrigSp);
    VmState_Push(VmState, VmState->Registers.Bp);
    VmState_Push(VmState, VmState->Registers.Si);
    VmState_Push(VmState, VmState->Registers.Di);
    VmState_AdvanceIp(VmState, 1);
}

VOID
FORCEINLINE
Opcode_61_POPA(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    USHORT OrigSp;
    DPRINT("POPA\n");

    VmState->Registers.Di = VmState_Pop(VmState);
    VmState->Registers.Si = VmState_Pop(VmState);
    VmState->Registers.Bp = VmState_Pop(VmState);
    OrigSp = VmState_Pop(VmState);
    VmState->Registers.Bx = VmState_Pop(VmState);
    VmState->Registers.Dx = VmState_Pop(VmState);
    VmState->Registers.Cx = VmState_Pop(VmState);
    VmState->Registers.Ax = VmState_Pop(VmState);
    VmState_AdvanceIp(VmState, 1);
}

VOID
FORCEINLINE
Opcode_55_PUSH_BP(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    DPRINT("PUSH BP\n");
    VmState_Push(VmState, VmState->Registers.Bp);
    VmState_AdvanceIp(VmState, 1);
}

VOID
FORCEINLINE
Opcode_xx_POP_BP(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    DPRINT("POP BP\n");
    VmState->Registers.Bp = VmState_Pop(VmState);
    VmState_AdvanceIp(VmState, 1);
}
