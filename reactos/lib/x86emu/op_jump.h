
VOID
FORCEINLINE
Opcode_E9_JMP16(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    SHORT Offset;

    Offset = *(PSHORT)(IntructionPointer + 1);
    DPRINT("JMP %04x\n", VmState->Registers.Ip + Offset + 3);
    VmState_AdvanceIp(VmState, Offset + 3);
}

VOID
FORCEINLINE
Opcode_75_JNZ8(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    DPRINT("JNZ %04x\n", VmState->Registers.Ip + IntructionPointer[1] + 2);

    if (!VmState->Registers.Eflags.Zf)
    {
        VmState_AdvanceIp(VmState, IntructionPointer[1] + 2);
    }
    else
        VmState_AdvanceIp(VmState, 2);
}

VOID
FORCEINLINE
Opcode_74_JZ8(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    DPRINT("JZ %04x\n", VmState->Registers.Ip + IntructionPointer[1] + 2);

    if (VmState->Registers.Eflags.Zf)
    {
        VmState_AdvanceIp(VmState, IntructionPointer[1] + 2);
    }
    else
        VmState_AdvanceIp(VmState, 2);
}
