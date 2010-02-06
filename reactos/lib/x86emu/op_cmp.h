
VOID
FORCEINLINE
Opcode_3D_CMP(PX86_VM_STATE VmState, PCHAR IntructionPointer)
{
    USHORT Value;

    Value = *(USHORT*)(IntructionPointer + 1);
    VmState_CMP16(VmState, VmState->Registers.Eax, Value);
    VmState_AdvanceIp(VmState, 3);
    DPRINT("CMP AX, 0x%x\n", Value);
}
