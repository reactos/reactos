
VOID
FORCEINLINE
Opcode_BB_MOV(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    USHORT Value;

    Value = *(USHORT*)(IntructionPointer + 1);
    VmState->Registers.Bx = Value;
    VmState_AdvanceIp(VmState, 3);
    DPRINT("MOV BX, 0x%x\n", Value);
}

VOID
FORCEINLINE
Opcode_8E_MOV(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    MODRM ModRm;
    UCHAR Value;

    ModRm.Byte = IntructionPointer[1];

    if (ModRm.mod == 3)
    {
        DPRINT("MOV ??, ??\n", IntructionPointer[2]);
        Value = VmState_GetVal8(VmState, ModRm);
        // ...
        VmState_AdvanceIp(VmState, 2);
        return;
    }
    DPRINT1("UNKNOWN\n");
}

VOID
FORCEINLINE
Opcode_89_MOV(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    MODRM ModRm;
    USHORT Value;

    ModRm.Byte = IntructionPointer[1];
    Value = VmState_GetRegVal16(VmState, ModRm);

    VmState_AdvanceIp(VmState, 2);
}
