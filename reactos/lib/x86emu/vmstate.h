
UCHAR
FORCEINLINE
VmState_GetRegVal8(PX86_VM_STATE VmState, MODRM ModRm)
{
    PUCHAR Pointer;

    Pointer = (PUCHAR)&VmState->Registers + RegisterTable[0][ModRm.reg];
    return *Pointer;
}

UCHAR
FORCEINLINE
VmState_GetRegVal16(PX86_VM_STATE VmState, MODRM ModRm)
{
    return VmState->IndexedRegisters[ModRm.reg].Word;
}


VOID
FORCEINLINE
VmState_EnablePrefix(PX86_VM_STATE VmState, ULONG Prefix)
{
    //DPRINT("_EnablePrefix\n");
}

VOID
FORCEINLINE
VmState_ClearPrefixes(PX86_VM_STATE VmState)
{
    //DPRINT1("_ClearPrefixes\n");
}

FORCEINLINE
VOID
VmState_AdvanceIp(PX86_VM_STATE VmState, SHORT Advance)
{
    VmState->Registers.Ip += Advance;
}

FORCEINLINE
VOID
VmState_SetIp(PX86_VM_STATE VmState, USHORT Value)
{
    VmState->Registers.Ip = Value;
}

FORCEINLINE
PCHAR
VmState_GetIp(PX86_VM_STATE VmState)
{
    return (PCHAR)VmState->MemBuffer + 
           VmState->Registers.ShiftedCs +
           VmState->Registers.Eip;
}

FORCEINLINE
VOID
VmState_Push(PX86_VM_STATE VmState, USHORT Value)
{
    PUSHORT StackPointer;
    //DPRINT1("Pushing %x %x %x\n", Value, VmState->Registers.ShiftedSs, VmState->Registers.Sp);
    StackPointer = (PVOID)((PCHAR)VmState->MemBuffer + 
                           VmState->Registers.ShiftedSs +
                           VmState->Registers.Sp); // FIXME: overflow
    *StackPointer = Value;
    VmState->Registers.Sp--;
}

FORCEINLINE
USHORT
VmState_Pop(PX86_VM_STATE VmState)
{
    PUSHORT StackPointer;
    //DPRINT1("Popping %x\n", Value);
    StackPointer = (PVOID)((PCHAR)VmState->MemBuffer + 
                           VmState->Registers.ShiftedSs +
                           VmState->Registers.Sp);
    VmState->Registers.Sp--;
    return *StackPointer;
}

UCHAR
FORCEINLINE
VmState_GetVal8(PX86_VM_STATE VmState, MODRM ModRm)
{
    return 0;
}

FORCEINLINE
VOID
VmState_CMP8(PX86_VM_STATE VmState, UCHAR Value1, UCHAR Value2)
{
    VmState->Registers.Eflags.Zf = ((Value1 - Value2) == 0);
    VmState->Registers.Eflags.Cf = ((Value1 - Value2) > Value1);
    VmState->Registers.Eflags.Sf = ((CHAR)(Value1 - Value2) < 0);
    VmState->Registers.Eflags.Of = ((CHAR)(Value1 - Value2) > (CHAR)Value1);
}

FORCEINLINE
VOID
VmState_CMP16(PX86_VM_STATE VmState, USHORT Value1, USHORT Value2)
{
    VmState->Registers.Eflags.Zf = ((Value1 - Value2) == 0);
    VmState->Registers.Eflags.Cf = ((Value1 - Value2) > Value1);
    VmState->Registers.Eflags.Sf = ((SHORT)(Value1 - Value2) < 0);
    VmState->Registers.Eflags.Of = ((SHORT)(Value1 - Value2) > (SHORT)Value1);
}
