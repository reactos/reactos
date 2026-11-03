/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 unwinding
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#include <arch/ketypes.h>

#define NDEBUG
#include <debug.h>

/* .pdata unwind data kinds, the low 2 bits of the second word */
#define PDATA_XDATA         0
#define PDATA_PACKED        1
#define PDATA_FRAGMENT      2
#define PDATA_LINK          3

/* Unwind codes that are not plain stack or register operations */
#define UWC_ALLOC_Z         0xDF
#define UWC_ALLOC_L         0xE0
#define UWC_SET_FP          0xE1
#define UWC_ADD_FP          0xE2
#define UWC_NOP             0xE3
#define UWC_END             0xE4
#define UWC_END_C           0xE5
#define UWC_SAVE_NEXT       0xE6
#define UWC_SAVE_ANY        0xE7
#define UWC_TRAP_FRAME      0xE8
#define UWC_MACHINE_FRAME   0xE9
#define UWC_CONTEXT         0xEA
#define UWC_EC_CONTEXT      0xEB
#define UWC_CLEAR_UNWOUND   0xEC
#define UWC_PAC_SIGN_LR     0xFC

/* No SVE support, so use the architectural minimum vector length */
#define SVE_VECTOR_LENGTH   16

/* Room for the codes of the longest packed prolog */
#define PACKED_CODES_MAX    40

typedef struct _UNWIND_STATE
{
    PCONTEXT Context;
    PKNONVOLATILE_CONTEXT_POINTERS ContextPointers;
    ULONG PendingPairs;
    BOOLEAN PcRestored;
} UNWIND_STATE, *PUNWIND_STATE;

/* FUNCTIONS *****************************************************************/

/*! RtlLookupFunctionTable
 * \brief Locates the table of RUNTIME_FUNCTION entries for a code address.
 */
PRUNTIME_FUNCTION
NTAPI
RtlLookupFunctionTable(
    _In_ DWORD64 ControlPc,
    _Out_ PDWORD64 ImageBase,
    _Out_ PULONG Length)
{
    PVOID Table;
    ULONG Size;

    if (!RtlPcToFileHeader((PVOID)ControlPc, (PVOID*)ImageBase))
        return NULL;

    Table = RtlImageDirectoryEntryToData((PVOID)*ImageBase,
                                         TRUE,
                                         IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                                         &Size);

    *Length = Size / sizeof(RUNTIME_FUNCTION);
    return Table;
}

PRUNTIME_FUNCTION
NTAPI
RtlpLookupDynamicFunctionEntry(
    _In_ DWORD64 ControlPc,
    _Out_ PDWORD64 ImageBase,
    _In_ PUNWIND_HISTORY_TABLE HistoryTable);

/* Length in bytes of the code a .pdata entry covers */
ULONG
NTAPI
RtlpGetFunctionLength(
    _In_ DWORD64 ImageBase,
    _In_ PRUNTIME_FUNCTION FunctionEntry)
{
    if (FunctionEntry->Flag == PDATA_XDATA)
        return (*(PULONG)(ImageBase + FunctionEntry->UnwindData) & 0x3FFFF) * 4;

    /* A link record marks a jump island, three instructions long */
    if ((FunctionEntry->UnwindData & 7) == PDATA_LINK)
        return 12;

    return FunctionEntry->FunctionLength * 4;
}

/*! RtlLookupFunctionEntry
 * \brief Locates the RUNTIME_FUNCTION entry corresponding to a code address.
 * \todo Implement HistoryTable
 */
PRUNTIME_FUNCTION
NTAPI
RtlLookupFunctionEntry(
    _In_ DWORD64 ControlPc,
    _Out_ PDWORD64 ImageBase,
    _Inout_opt_ PUNWIND_HISTORY_TABLE HistoryTable)
{
    PRUNTIME_FUNCTION FunctionTable, FunctionEntry;
    ULONG TableLength, Low, High, Middle, Rva, Length;

    FunctionTable = RtlLookupFunctionTable(ControlPc, ImageBase, &TableLength);
    if (!FunctionTable)
        return RtlpLookupDynamicFunctionEntry(ControlPc, ImageBase, HistoryTable);

    Rva = (ULONG)(ControlPc - *ImageBase);

    /* Find the last entry starting at or below the address */
    Low = 0;
    High = TableLength;
    while (Low < High)
    {
        Middle = Low + (High - Low) / 2;
        if (FunctionTable[Middle].BeginAddress <= Rva)
            Low = Middle + 1;
        else
            High = Middle;
    }

    if (Low == 0)
        return NULL;

    FunctionEntry = &FunctionTable[Low - 1];
    Length = RtlpGetFunctionLength(*ImageBase, FunctionEntry);
    if (Rva - FunctionEntry->BeginAddress >= Length)
        return NULL;

    return FunctionEntry;
}

/* Number of bytes an unwind code takes, decided by its first byte */
static
ULONG
RtlpUnwindCodeSize(
    _In_ UCHAR Code)
{
    if (Code < 0xC0)
        return 1;
    if (Code < 0xE0)
        return 2;

    switch (Code)
    {
        case UWC_ALLOC_L:
            return 4;
        case UWC_ADD_FP:
            return 2;
        case UWC_SAVE_ANY:
            return 3;
        case 0xF8:
        case 0xF9:
        case 0xFA:
        case 0xFB:
            return Code - 0xF6;
        default:
            return 1;
    }
}

/* Custom stack codes and the reserved codes around them are not prolog instructions */
static
BOOLEAN
RtlpIsFrameCode(
    _In_ UCHAR Code)
{
    return Code >= UWC_TRAP_FRAME && Code <= 0xF7;
}

/* Counts the instructions described from Index up to end or end_c */
static
ULONG
RtlpCountInstructions(
    _In_reads_(CodeSize) PUCHAR Codes,
    _In_ ULONG CodeSize,
    _In_ ULONG Index)
{
    ULONG Count = 0;

    while (Index < CodeSize && Codes[Index] != UWC_END && Codes[Index] != UWC_END_C)
    {
        if (!RtlpIsFrameCode(Codes[Index]))
            Count++;
        Index += RtlpUnwindCodeSize(Codes[Index]);
    }

    return Count;
}

/* Steps over Count codes, never past end */
static
ULONG
RtlpSkipCodes(
    _In_reads_(CodeSize) PUCHAR Codes,
    _In_ ULONG CodeSize,
    _In_ ULONG Index,
    _In_ ULONG Count)
{
    while (Count != 0 && Index < CodeSize && Codes[Index] != UWC_END)
    {
        Index += RtlpUnwindCodeSize(Codes[Index]);
        Count--;
    }

    return Index;
}

static
VOID
RtlpRestoreX(
    _Inout_ PUNWIND_STATE State,
    _In_ ULONG Reg,
    _In_ ULONG64 Address)
{
    PULONG64 Slot = (PULONG64)Address;
    PKNONVOLATILE_CONTEXT_POINTERS Pointers = State->ContextPointers;

    State->Context->X[Reg] = *Slot;

    if (Pointers == NULL || Reg < 19)
        return;

    if (Reg <= 28)
        (&Pointers->X19)[Reg - 19] = Slot;
    else if (Reg == 29)
        Pointers->Fp = Slot;
    else
        Pointers->Lr = Slot;
}

static
VOID
RtlpRestoreD(
    _Inout_ PUNWIND_STATE State,
    _In_ ULONG Reg,
    _In_ ULONG64 Address)
{
    PULONG64 Slot = (PULONG64)Address;

    State->Context->V[Reg].Low = *Slot;

    if (State->ContextPointers != NULL && Reg >= 8 && Reg <= 15)
        (&State->ContextPointers->D8)[Reg - 8] = Slot;
}

static
VOID
RtlpRestoreQ(
    _Inout_ PUNWIND_STATE State,
    _In_ ULONG Reg,
    _In_ ULONG64 Address)
{
    RtlpRestoreD(State, Reg, Address);
    State->Context->V[Reg].High = *(PLONG64)(Address + 8);
}

/* Restores Count registers plus the pairs save_next added, FALSE past the last register */
static
BOOLEAN
RtlpRestoreRange(
    _Inout_ PUNWIND_STATE State,
    _In_ UCHAR Kind,
    _In_ ULONG FirstReg,
    _In_ ULONG Count,
    _In_ ULONG64 Address)
{
    ULONG Size = (Kind == 'Q') ? 16 : 8;
    ULONG i;

    Count += 2 * State->PendingPairs;
    State->PendingPairs = 0;

    if (FirstReg + Count - 1 > ((Kind == 'X') ? 30u : 31u))
        return FALSE;

    for (i = 0; i < Count; i++)
    {
        if (Kind == 'X')
            RtlpRestoreX(State, FirstReg + i, Address + i * Size);
        else if (Kind == 'D')
            RtlpRestoreD(State, FirstReg + i, Address + i * Size);
        else
            RtlpRestoreQ(State, FirstReg + i, Address + i * Size);
    }

    return TRUE;
}

static
VOID
RtlpRestoreFromTrapFrame(
    _Inout_ PUNWIND_STATE State,
    _In_ PKTRAP_FRAME TrapFrame)
{
    PCONTEXT Context = State->Context;
    PKARM64_VFP_STATE VfpState = TrapFrame->VfpState;
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(TrapFrame->X); i++)
        Context->X[i] = TrapFrame->X[i];

    Context->Fp = TrapFrame->Fp;
    Context->Lr = TrapFrame->Lr;
    Context->Sp = TrapFrame->Sp;
    Context->Pc = TrapFrame->Pc;
    Context->Cpsr = TrapFrame->Spsr;

    if (VfpState == NULL)
        return;

    Context->Fpcr = VfpState->Fpcr;
    Context->Fpsr = VfpState->Fpsr;
    for (i = 0; i < 32; i++)
    {
        Context->V[i].Low = VfpState->V[i].Low;
        Context->V[i].High = VfpState->V[i].High;
    }

    if (State->ContextPointers != NULL)
    {
        for (i = 0; i < 8; i++)
            (&State->ContextPointers->D8)[i] = &VfpState->V[8 + i].Low;
    }
}

static
VOID
RtlpRestoreFromContext(
    _Inout_ PUNWIND_STATE State,
    _In_ PCONTEXT Saved)
{
    PCONTEXT Context = State->Context;
    ULONG i;

    for (i = 0; i < 31; i++)
        Context->X[i] = Saved->X[i];
    for (i = 0; i < 32; i++)
        Context->V[i] = Saved->V[i];

    Context->Cpsr = Saved->Cpsr;
    Context->Fpcr = Saved->Fpcr;
    Context->Fpsr = Saved->Fpsr;
    Context->Sp = Saved->Sp;
    Context->Pc = Saved->Pc;
    Context->ContextFlags = (Context->ContextFlags & ~CONTEXT_ARM64_UNWOUND_TO_CALL) |
                            (Saved->ContextFlags & CONTEXT_ARM64_UNWOUND_TO_CALL);

    if (State->ContextPointers != NULL)
    {
        for (i = 0; i < 10; i++)
            (&State->ContextPointers->X19)[i] = &Saved->X[19 + i];
        for (i = 0; i < 8; i++)
            (&State->ContextPointers->D8)[i] = &Saved->V[8 + i].Low;
    }
}

/* Converts the SSE control and status bits of an x64 context to Fpcr and Fpsr */
static
VOID
RtlpConvertMxCsr(
    _Inout_ PCONTEXT Context,
    _In_ ULONG MxCsr)
{
    static const UCHAR StatusBits[] = { 0, 7, 1, 2, 3, 4 };
    static const UCHAR EnableBits[] = { 8, 15, 9, 10, 11, 12 };
    ULONG Fpcr = 0, Fpsr = 0, i;

    for (i = 0; i < RTL_NUMBER_OF(StatusBits); i++)
    {
        if (MxCsr & (1 << i))
            Fpsr |= 1 << StatusBits[i];

        /* x64 masks exceptions, ARM64 enables them */
        if (!(MxCsr & (1 << (i + 7))))
            Fpcr |= 1 << EnableBits[i];
    }

    /* DAZ, the two rounding control bits swapped, FZ */
    if (MxCsr & (1 << 6))
        Fpcr |= 1 << 19;
    if (MxCsr & (1 << 13))
        Fpcr |= 1 << 23;
    if (MxCsr & (1 << 14))
        Fpcr |= 1 << 22;
    if (MxCsr & (1 << 15))
        Fpcr |= 1 << 24;

    Context->Fpcr = Fpcr;
    Context->Fpsr = Fpsr;
}

static
VOID
RtlpRestoreFromEcContext(
    _Inout_ PUNWIND_STATE State,
    _In_ PARM64EC_NT_CONTEXT Saved)
{
    PCONTEXT Context = State->Context;
    ULONG EFlags = Saved->AMD64_EFlags;
    ULONG i;

    for (i = 0; i < 31; i++)
        Context->X[i] = 0;
    for (i = 0; i < 32; i++)
    {
        Context->V[i].Low = 0;
        Context->V[i].High = 0;
    }

    Context->X0 = Saved->X0;
    Context->X1 = Saved->X1;
    Context->X2 = Saved->X2;
    Context->X3 = Saved->X3;
    Context->X4 = Saved->X4;
    Context->X5 = Saved->X5;
    Context->X6 = Saved->X6;
    Context->X7 = Saved->X7;
    Context->X8 = Saved->X8;
    Context->X9 = Saved->X9;
    Context->X10 = Saved->X10;
    Context->X11 = Saved->X11;
    Context->X12 = Saved->X12;
    Context->X15 = Saved->X15;
    Context->X16 = (ULONG64)Saved->X16_0 | ((ULONG64)Saved->X16_1 << 16) |
                   ((ULONG64)Saved->X16_2 << 32) | ((ULONG64)Saved->X16_3 << 48);
    Context->X17 = (ULONG64)Saved->X17_0 | ((ULONG64)Saved->X17_1 << 16) |
                   ((ULONG64)Saved->X17_2 << 32) | ((ULONG64)Saved->X17_3 << 48);
    Context->X19 = Saved->X19;
    Context->X20 = Saved->X20;
    Context->X21 = Saved->X21;
    Context->X22 = Saved->X22;
    Context->X25 = Saved->X25;
    Context->X26 = Saved->X26;
    Context->X27 = Saved->X27;
    Context->Fp = Saved->Fp;
    Context->Lr = Saved->Lr;
    Context->Sp = Saved->Sp;
    Context->Pc = Saved->Pc;

    for (i = 0; i < 16; i++)
    {
        Context->V[i].Low = Saved->V[i].Low;
        Context->V[i].High = Saved->V[i].High;
    }

    /* CF, ZF, SF and OF become NZCV, TF becomes SS */
    Context->Cpsr = 0;
    if (EFlags & 0x1)
        Context->Cpsr |= 1UL << 29;
    if (EFlags & 0x40)
        Context->Cpsr |= 1UL << 30;
    if (EFlags & 0x80)
        Context->Cpsr |= 1UL << 31;
    if (EFlags & 0x800)
        Context->Cpsr |= 1UL << 28;
    if (EFlags & 0x100)
        Context->Cpsr |= 1UL << 21;

    RtlpConvertMxCsr(Context, Saved->AMD64_MxCsr);

    Context->ContextFlags = (Context->ContextFlags & ~CONTEXT_ARM64_UNWOUND_TO_CALL) |
                            (Saved->ContextFlags & CONTEXT_ARM64_UNWOUND_TO_CALL);

    if (State->ContextPointers != NULL)
    {
        for (i = 0; i < 8; i++)
            (&State->ContextPointers->D8)[i] = &Saved->V[8 + i].Low;
    }
}

/* Codes a save_next may sit in front of: register pair saves */
static
BOOLEAN
RtlpTakesSaveNext(
    _In_reads_(3) PUCHAR Code)
{
    if (Code[0] >= 0x20 && Code[0] < 0x40)
        return TRUE;
    if (Code[0] >= 0xC8 && Code[0] < 0xD0)
        return TRUE;
    if (Code[0] >= 0xD8 && Code[0] < 0xDC)
        return TRUE;
    if (Code[0] == UWC_SAVE_ANY)
        return (Code[1] & 0xC0) == 0x40 && (Code[2] & 0xC0) != 0xC0;
    return Code[0] == UWC_SAVE_NEXT || Code[0] == UWC_END_C;
}

static
BOOLEAN
RtlpExecuteSaveAny(
    _Inout_ PUNWIND_STATE State,
    _In_ UCHAR Info,
    _In_ UCHAR Slot)
{
    PCONTEXT Context = State->Context;
    BOOLEAN Pair = (Info & 0x40) != 0;
    ULONG Reg = Info & 0x1F;
    ULONG Offset = Slot & 0x3F;
    UCHAR Kind;

    if (Info & 0x80)
        return FALSE;

    switch (Slot >> 6)
    {
        case 0:
            Kind = 'X';
            break;
        case 1:
            Kind = 'D';
            break;
        case 2:
            Kind = 'Q';
            break;
        default:
            /* Z registers keep their low 128 bits in V, P registers, VG and SM are not in CONTEXT */
            if (!(Info & 0x10))
            {
                Offset |= (Info & 0x60) << 1;
                Reg = 8 + (Info & 0x0F);
                Context->V[Reg].Low = *(PULONG64)(Context->Sp + Offset * SVE_VECTOR_LENGTH);
                Context->V[Reg].High = *(PLONG64)(Context->Sp + Offset * SVE_VECTOR_LENGTH + 8);
            }
            return TRUE;
    }

    if (Info & 0x20)
    {
        /* Pre-indexed */
        if (!RtlpRestoreRange(State, Kind, Reg, Pair ? 2 : 1, Context->Sp))
            return FALSE;
        Context->Sp += ((ULONG64)Offset + 1) * 16;
        return TRUE;
    }

    if (Pair || Kind == 'Q')
        return RtlpRestoreRange(State, Kind, Reg, Pair ? 2 : 1, Context->Sp + Offset * 16);

    return RtlpRestoreRange(State, Kind, Reg, 1, Context->Sp + Offset * 8);
}

/* Runs unwind codes from Index to end, FALSE when they are invalid */
static
BOOLEAN
RtlpExecuteUnwindCodes(
    _Inout_ PUNWIND_STATE State,
    _In_reads_(CodeSize) PUCHAR Codes,
    _In_ ULONG CodeSize,
    _In_ ULONG Index)
{
    PCONTEXT Context = State->Context;

    while (Index < CodeSize)
    {
        UCHAR Bytes[4] = { 0 };
        ULONG Size, Reg, Offset, i;
        UCHAR Code;

        Code = Codes[Index];
        Size = RtlpUnwindCodeSize(Code);
        for (i = 0; i < Size && Index + i < CodeSize; i++)
            Bytes[i] = Codes[Index + i];

        if (State->PendingPairs != 0 && !RtlpTakesSaveNext(Bytes))
            return FALSE;

        if (Code == UWC_END)
            return TRUE;

        Reg = ((Bytes[0] & 0x03) << 2) | (Bytes[1] >> 6);
        Offset = Bytes[1] & 0x3F;

        if (Code < 0x20)
        {
            /* alloc_s */
            Context->Sp += (ULONG64)Code * 16;
        }
        else if (Code < 0x40)
        {
            /* save_r19r20_x */
            if (!RtlpRestoreRange(State, 'X', 19, 2, Context->Sp))
                return FALSE;
            Context->Sp += (ULONG64)(Code & 0x1F) * 8;
        }
        else if (Code < 0x80)
        {
            /* save_fplr */
            if (!RtlpRestoreRange(State, 'X', 29, 2, Context->Sp + (Code & 0x3F) * 8))
                return FALSE;
        }
        else if (Code < 0xC0)
        {
            /* save_fplr_x */
            if (!RtlpRestoreRange(State, 'X', 29, 2, Context->Sp))
                return FALSE;
            Context->Sp += ((ULONG64)(Code & 0x3F) + 1) * 8;
        }
        else if (Code < 0xC8)
        {
            /* alloc_m */
            Context->Sp += ((((ULONG64)Code & 0x07) << 8) | Bytes[1]) * 16;
        }
        else if (Code < 0xCC)
        {
            /* save_regp */
            if (!RtlpRestoreRange(State, 'X', 19 + Reg, 2, Context->Sp + Offset * 8))
                return FALSE;
        }
        else if (Code < 0xD0)
        {
            /* save_regp_x */
            if (!RtlpRestoreRange(State, 'X', 19 + Reg, 2, Context->Sp))
                return FALSE;
            Context->Sp += ((ULONG64)Offset + 1) * 8;
        }
        else if (Code < 0xD4)
        {
            /* save_reg */
            if (!RtlpRestoreRange(State, 'X', 19 + Reg, 1, Context->Sp + Offset * 8))
                return FALSE;
        }
        else if (Code < 0xD6)
        {
            /* save_reg_x */
            Reg = ((Code & 0x01) << 3) | (Bytes[1] >> 5);
            if (!RtlpRestoreRange(State, 'X', 19 + Reg, 1, Context->Sp))
                return FALSE;
            Context->Sp += ((ULONG64)(Bytes[1] & 0x1F) + 1) * 8;
        }
        else if (Code < 0xD8)
        {
            /* save_lrpair */
            Reg = ((Code & 0x01) << 2) | (Bytes[1] >> 6);
            if (!RtlpRestoreRange(State, 'X', 19 + 2 * Reg, 1, Context->Sp + Offset * 8))
                return FALSE;
            RtlpRestoreRange(State, 'X', 30, 1, Context->Sp + Offset * 8 + 8);
        }
        else if (Code < 0xDA)
        {
            /* save_fregp */
            Reg = ((Code & 0x01) << 2) | (Bytes[1] >> 6);
            if (!RtlpRestoreRange(State, 'D', 8 + Reg, 2, Context->Sp + Offset * 8))
                return FALSE;
        }
        else if (Code < 0xDC)
        {
            /* save_fregp_x */
            Reg = ((Code & 0x01) << 2) | (Bytes[1] >> 6);
            if (!RtlpRestoreRange(State, 'D', 8 + Reg, 2, Context->Sp))
                return FALSE;
            Context->Sp += ((ULONG64)Offset + 1) * 8;
        }
        else if (Code < 0xDE)
        {
            /* save_freg */
            Reg = ((Code & 0x01) << 2) | (Bytes[1] >> 6);
            if (!RtlpRestoreRange(State, 'D', 8 + Reg, 1, Context->Sp + Offset * 8))
                return FALSE;
        }
        else if (Code == 0xDE)
        {
            /* save_freg_x */
            if (!RtlpRestoreRange(State, 'D', 8 + (Bytes[1] >> 5), 1, Context->Sp))
                return FALSE;
            Context->Sp += ((ULONG64)(Bytes[1] & 0x1F) + 1) * 8;
        }
        else if (Code == UWC_ALLOC_Z)
        {
            Context->Sp += (ULONG64)Bytes[1] * SVE_VECTOR_LENGTH;
        }
        else if (Code == UWC_ALLOC_L)
        {
            Context->Sp += (((ULONG64)Bytes[1] << 16) | ((ULONG64)Bytes[2] << 8) | Bytes[3]) * 16;
        }
        else if (Code == UWC_SET_FP)
        {
            Context->Sp = Context->Fp;
        }
        else if (Code == UWC_ADD_FP)
        {
            Context->Sp = Context->Fp - (ULONG64)Bytes[1] * 8;
        }
        else if (Code == UWC_SAVE_NEXT)
        {
            State->PendingPairs++;
        }
        else if (Code == UWC_SAVE_ANY)
        {
            if (!RtlpExecuteSaveAny(State, Bytes[1], Bytes[2]))
                return FALSE;
        }
        else if (Code == UWC_TRAP_FRAME)
        {
            RtlpRestoreFromTrapFrame(State, (PKTRAP_FRAME)Context->Sp);
            Context->ContextFlags &= ~CONTEXT_ARM64_UNWOUND_TO_CALL;
            State->PcRestored = TRUE;
        }
        else if (Code == UWC_MACHINE_FRAME)
        {
            Context->Pc = ((PULONG64)Context->Sp)[1];
            Context->Sp = ((PULONG64)Context->Sp)[0];
            Context->ContextFlags &= ~CONTEXT_ARM64_UNWOUND_TO_CALL;
            State->PcRestored = TRUE;
        }
        else if (Code == UWC_CONTEXT)
        {
            RtlpRestoreFromContext(State, (PCONTEXT)Context->Sp);
            State->PcRestored = TRUE;
        }
        else if (Code == UWC_EC_CONTEXT)
        {
            RtlpRestoreFromEcContext(State, (PARM64EC_NT_CONTEXT)Context->Sp);
            State->PcRestored = TRUE;
        }
        else if (Code == UWC_CLEAR_UNWOUND)
        {
            /* The return address is lr as it is at this point */
            Context->Pc = Context->Lr;
            Context->ContextFlags &= ~CONTEXT_ARM64_UNWOUND_TO_CALL;
            State->PcRestored = TRUE;
        }
        else if (Code >= 0xED && Code <= 0xF7)
        {
            /* Reserved */
            return FALSE;
        }

        /* nop, end_c, pac_sign_lr and the reserved F8-FF codes change nothing */
        Index += Size;
    }

    return State->PendingPairs == 0;
}

typedef struct _PACKED_PROLOG
{
    UCHAR Codes[PACKED_CODES_MAX];
    UCHAR Starts[PACKED_CODES_MAX];
    ULONG Size;
    ULONG Instructions;
} PACKED_PROLOG, *PPACKED_PROLOG;

/* Adds the unwind code of one prolog instruction, Second is -1 for one byte codes */
static
VOID
RtlpEmitCode(
    _Inout_ PPACKED_PROLOG Prolog,
    _In_ UCHAR First,
    _In_ LONG Second)
{
    Prolog->Starts[Prolog->Instructions++] = (UCHAR)Prolog->Size;
    Prolog->Codes[Prolog->Size++] = First;
    if (Second >= 0)
        Prolog->Codes[Prolog->Size++] = (UCHAR)Second;
}

static
VOID
RtlpEmitAlloc(
    _Inout_ PPACKED_PROLOG Prolog,
    _In_ ULONG Size)
{
    ULONG Units = Size / 16;

    if (Units < 0x20)
        RtlpEmitCode(Prolog, (UCHAR)Units, -1);
    else
        RtlpEmitCode(Prolog, (UCHAR)(0xC0 | (Units >> 8)), Units & 0xFF);
}

/*
 * Builds the unwind codes of the canonical prolog packed unwind data stands
 * for. The epilog form has no homing stores and no set_fp.
 */
static
ULONG
RtlpBuildPackedCodes(
    _In_ PRUNTIME_FUNCTION FunctionEntry,
    _In_ BOOLEAN Epilog,
    _Out_writes_(PACKED_CODES_MAX) PUCHAR Codes)
{
    PACKED_PROLOG Prolog;
    ULONG RegI = FunctionEntry->RegI;
    ULONG FpRegs = FunctionEntry->RegF ? FunctionEntry->RegF + 1 : 0;
    ULONG CR = FunctionEntry->CR;
    BOOLEAN Chained = (CR == 2 || CR == 3);
    BOOLEAN Homing = FunctionEntry->H && !Epilog;
    ULONG IntSize, SaveSize, LocalSize, Reg, Offset, Out = 0, i;
    BOOLEAN SavesFirst;

    IntSize = RegI * 8 + ((CR == 1) ? 8 : 0);
    SaveSize = (IntSize + FpRegs * 8 + 8 * 8 * FunctionEntry->H + 0xF) & ~0xF;
    LocalSize = FunctionEntry->FrameSize * 16 - SaveSize;

    /* Without a register save to predecrement sp, the frame allocation does it all */
    SavesFirst = (IntSize != 0 || FpRegs != 0);
    if (!SavesFirst)
        LocalSize += SaveSize;

    Prolog.Size = 0;
    Prolog.Instructions = 0;

    if (CR == 2)
        RtlpEmitCode(&Prolog, UWC_PAC_SIGN_LR, -1);

    for (Reg = 0; Reg < RegI; Reg += 2)
    {
        if (Reg + 1 == RegI && CR == 1)
        {
            /* lr pairs with the last register, an odd x19 needs its own allocation */
            if (Reg == 0)
                RtlpEmitAlloc(&Prolog, SaveSize);
            RtlpEmitCode(&Prolog, 0xD6 | (Reg >> 3), ((Reg >> 1) & 3) << 6 | Reg);
        }
        else if (Reg + 1 == RegI && Reg == 0)
        {
            RtlpEmitCode(&Prolog, 0xD4, SaveSize / 8 - 1);
        }
        else if (Reg + 1 == RegI)
        {
            RtlpEmitCode(&Prolog, 0xD0 | (Reg >> 2), (Reg & 3) << 6 | Reg);
        }
        else if (Reg == 0)
        {
            RtlpEmitCode(&Prolog, 0xCC, SaveSize / 8 - 1);
        }
        else
        {
            RtlpEmitCode(&Prolog, 0xC8 | (Reg >> 2), (Reg & 3) << 6 | Reg);
        }
    }

    /* lr on its own when no odd register took it */
    if (CR == 1 && !(RegI & 1))
    {
        if (RegI == 0)
            RtlpEmitCode(&Prolog, 0xD5, 0x60 | (SaveSize / 8 - 1));
        else
            RtlpEmitCode(&Prolog, 0xD2, 0xC0 | (IntSize / 8 - 1));
    }

    for (Reg = 0; Reg < FpRegs; Reg += 2)
    {
        Offset = IntSize / 8 + Reg;

        if (Reg == 0 && IntSize == 0)
            RtlpEmitCode(&Prolog, 0xDA, SaveSize / 8 - 1);
        else if (Reg + 1 == FpRegs)
            RtlpEmitCode(&Prolog, 0xDC | (Reg >> 2), (Reg & 3) << 6 | Offset);
        else
            RtlpEmitCode(&Prolog, 0xD8 | (Reg >> 2), (Reg & 3) << 6 | Offset);
    }

    if (Homing && SavesFirst)
    {
        for (i = 0; i < 4; i++)
            RtlpEmitCode(&Prolog, UWC_NOP, -1);
    }

    /* Locals, outgoing arguments and the frame record */
    if (Chained && LocalSize != 0)
    {
        if (LocalSize <= 512)
        {
            RtlpEmitCode(&Prolog, (UCHAR)(0x80 | (LocalSize / 8 - 1)), -1);
        }
        else
        {
            RtlpEmitAlloc(&Prolog, LocalSize);
            RtlpEmitCode(&Prolog, 0x40, -1);
        }

        if (!Epilog)
            RtlpEmitCode(&Prolog, UWC_SET_FP, -1);
    }
    else if (!Chained && LocalSize != 0)
    {
        RtlpEmitAlloc(&Prolog, LocalSize);
    }

    if (Homing && !SavesFirst)
    {
        for (i = 0; i < 4; i++)
            RtlpEmitCode(&Prolog, UWC_NOP, -1);
    }

    /* Unwind codes run in the reverse order */
    Prolog.Starts[Prolog.Instructions] = (UCHAR)Prolog.Size;
    while (Prolog.Instructions != 0)
    {
        Prolog.Instructions--;
        for (i = Prolog.Starts[Prolog.Instructions]; i < Prolog.Starts[Prolog.Instructions + 1]; i++)
            Codes[Out++] = Prolog.Codes[i];
    }

    Codes[Out++] = UWC_END;
    return Out;
}

/*! RtlVirtualUnwind
 * \brief Unwinds one frame of a function described by ARM64 unwind data.
 */
PEXCEPTION_ROUTINE
NTAPI
RtlVirtualUnwind(
    _In_ ULONG HandlerType,
    _In_ ULONG64 ImageBase,
    _In_ ULONG64 ControlPc,
    _In_ PRUNTIME_FUNCTION FunctionEntry,
    _Inout_ PCONTEXT Context,
    _Outptr_ PVOID *HandlerData,
    _Out_ PULONG64 EstablisherFrame,
    _Inout_opt_ PKNONVOLATILE_CONTEXT_POINTERS ContextPointers)
{
    UNWIND_STATE State;
    UCHAR PackedCodes[PACKED_CODES_MAX];
    PUCHAR Codes;
    PULONG Xdata = NULL;
    ULONG CodeSize, Start = 0, Offset, Length, Count, EpilogStart, i;
    BOOLEAN InBody = TRUE;

    UNREFERENCED_PARAMETER(HandlerType);

    State.Context = Context;
    State.ContextPointers = ContextPointers;
    State.PendingPairs = 0;
    State.PcRestored = FALSE;

    /* Leaf functions keep the return address in lr */
    if (FunctionEntry == NULL)
    {
        if (ControlPc == Context->Lr)
        {
            Context->Pc = 0;
            return NULL;
        }

        Context->Pc = Context->Lr;
        Context->ContextFlags |= CONTEXT_ARM64_UNWOUND_TO_CALL;
        *EstablisherFrame = Context->Sp;
        *HandlerData = NULL;
        return NULL;
    }

    Offset = (ULONG)(ControlPc - ImageBase - FunctionEntry->BeginAddress);

    /* A jump island unwinds like the start of the function it belongs to */
    if (FunctionEntry->Flag == PDATA_LINK)
    {
        if (FunctionEntry->UnwindData & 4)
        {
            Context->Pc = 0;
            return NULL;
        }

        FunctionEntry = (PRUNTIME_FUNCTION)(ImageBase + FunctionEntry->UnwindData - PDATA_LINK);
        Offset = 0;
    }

    if (FunctionEntry->Flag == PDATA_XDATA)
    {
        BOOLEAN SingleEpilog;
        ULONG EpilogCount, CodeWords;
        PULONG Scopes;

        Xdata = (PULONG)(ImageBase + FunctionEntry->UnwindData);
        Length = (Xdata[0] & 0x3FFFF) * 4;
        SingleEpilog = (Xdata[0] >> 21) & 1;
        EpilogCount = (Xdata[0] >> 22) & 0x1F;
        CodeWords = Xdata[0] >> 27;
        Scopes = &Xdata[1];

        if (EpilogCount == 0 && CodeWords == 0)
        {
            EpilogCount = Xdata[1] & 0xFFFF;
            CodeWords = (Xdata[1] >> 16) & 0xFF;
            Scopes++;
        }

        /* With E set the epilog count is the code index of the only epilog */
        Codes = (PUCHAR)(SingleEpilog ? Scopes : Scopes + EpilogCount);
        CodeSize = CodeWords * 4;

        /* Only the prolog instructions that already ran get undone */
        Count = RtlpCountInstructions(Codes, CodeSize, 0);
        if (Offset / 4 < Count)
        {
            Start = RtlpSkipCodes(Codes, CodeSize, 0, Count - Offset / 4);
            InBody = FALSE;
        }
        else if (SingleEpilog)
        {
            EpilogStart = Length - (RtlpCountInstructions(Codes, CodeSize, EpilogCount) + 1) * 4;
            if (Offset >= EpilogStart)
            {
                Start = RtlpSkipCodes(Codes, CodeSize, EpilogCount, (Offset - EpilogStart) / 4);
                InBody = FALSE;
            }
        }
        else
        {
            for (i = 0; i < EpilogCount; i++)
            {
                ULONG Index = Scopes[i] >> 22;

                EpilogStart = (Scopes[i] & 0x3FFFF) * 4;
                Count = RtlpCountInstructions(Codes, CodeSize, Index) + 1;
                if (Offset >= EpilogStart && Offset < EpilogStart + Count * 4)
                {
                    Start = RtlpSkipCodes(Codes, CodeSize, Index, (Offset - EpilogStart) / 4);
                    InBody = FALSE;
                    break;
                }
            }
        }
    }
    else
    {
        Length = FunctionEntry->FunctionLength * 4;
        CodeSize = RtlpBuildPackedCodes(FunctionEntry, FALSE, PackedCodes);
        Codes = PackedCodes;

        if (FunctionEntry->Flag == PDATA_PACKED)
        {
            Count = RtlpCountInstructions(Codes, CodeSize, 0);
            if (Offset / 4 < Count)
            {
                Start = RtlpSkipCodes(Codes, CodeSize, 0, Count - Offset / 4);
            }
            else
            {
                CodeSize = RtlpBuildPackedCodes(FunctionEntry, TRUE, PackedCodes);
                EpilogStart = Length - (RtlpCountInstructions(Codes, CodeSize, 0) + 1) * 4;
                if (Offset >= EpilogStart)
                    Start = RtlpSkipCodes(Codes, CodeSize, 0, (Offset - EpilogStart) / 4);
                else
                    CodeSize = RtlpBuildPackedCodes(FunctionEntry, FALSE, PackedCodes);
            }
        }
    }

    Context->ContextFlags |= CONTEXT_ARM64_UNWOUND_TO_CALL;

    if (!RtlpExecuteUnwindCodes(&State, Codes, CodeSize, Start))
    {
        Context->Pc = 0;
        return NULL;
    }

    if (!State.PcRestored)
        Context->Pc = Context->Lr;

    *EstablisherFrame = Context->Sp;
    *HandlerData = NULL;

    /* The language handler only covers the body */
    if (Xdata != NULL && InBody && (Xdata[0] & (1 << 20)))
    {
        PULONG Handler = (PULONG)(Codes + CodeSize);

        *HandlerData = Handler + 1;
        return (PEXCEPTION_ROUTINE)(ImageBase + *Handler);
    }

    return NULL;
}

static
BOOLEAN
RtlpIsStackPointerValid(
    _In_ ULONG64 StackPointer,
    _In_ ULONG64 LowLimit,
    _In_ ULONG64 HighLimit)
{
    return StackPointer >= LowLimit && StackPointer < HighLimit && !(StackPointer & 15);
}

/* Keeps the nonvolatile registers a language handler sees for the frame it runs in */
static
VOID
RtlpSaveNonVolatileRegisters(
    _Out_ DISPATCHER_CONTEXT_NONVOLREG_ARM64 *NonVolatiles,
    _In_ PCONTEXT Context)
{
    ULONG i;

    for (i = 0; i < NONVOL_INT_NUMREG_ARM64; i++)
        NonVolatiles->GpNvRegs[i] = Context->X[19 + i];
    for (i = 0; i < NONVOL_FP_NUMREG_ARM64; i++)
        NonVolatiles->FpNvRegs[i] = Context->V[8 + i].D[0];
}

/* Unwinds one frame, after a call the lookup uses the call instruction itself */
static
PEXCEPTION_ROUTINE
RtlpUnwindFrame(
    _In_ ULONG HandlerType,
    _Inout_ PDISPATCHER_CONTEXT DispatcherContext,
    _Inout_ PCONTEXT Context,
    _Out_ PULONG64 EstablisherFrame)
{
    ULONG64 ControlPc = Context->Pc;

    DispatcherContext->ControlPc = ControlPc;
    DispatcherContext->ControlPcIsUnwound = (Context->ContextFlags & CONTEXT_ARM64_UNWOUND_TO_CALL) != 0;
    if (DispatcherContext->ControlPcIsUnwound)
        ControlPc -= 4;

    DispatcherContext->FunctionEntry = RtlLookupFunctionEntry(ControlPc,
                                                              &DispatcherContext->ImageBase,
                                                              NULL);

    return RtlVirtualUnwind(HandlerType,
                            DispatcherContext->ImageBase,
                            ControlPc,
                            DispatcherContext->FunctionEntry,
                            Context,
                            &DispatcherContext->HandlerData,
                            EstablisherFrame,
                            NULL);
}

/*! RtlpExecuteHandlerForUnwindHandler
 * \brief Exception routine of RtlpExecuteHandlerForUnwind.
 */
EXCEPTION_DISPOSITION
NTAPI
RtlpExecuteHandlerForUnwindHandler(
    _Inout_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID EstablisherFrame,
    _Inout_ PCONTEXT ContextRecord,
    _In_ PDISPATCHER_CONTEXT DispatcherContext)
{
    /* RtlpExecuteHandlerForUnwind keeps these right below its caller's sp */
    PULONG64 Saved = (PULONG64)EstablisherFrame - 2;
    ULONG ExceptionFlags = (ULONG)Saved[0];
    PDISPATCHER_CONTEXT PreviousDispatcherContext = (PDISPATCHER_CONTEXT)Saved[1];

    UNREFERENCED_PARAMETER(ContextRecord);

    /*
     * An unwind reached the termination handler another unwind is running.
     * Hand back the dispatcher context of that unwind so it carries on past
     * the handler instead of running it twice.
     */
    if (IS_UNWINDING(ExceptionFlags) && IS_UNWINDING(ExceptionRecord->ExceptionFlags))
    {
        *DispatcherContext = *PreviousDispatcherContext;
        return ExceptionCollidedUnwind;
    }

    // TODO: properly handle nested exceptions

    return ExceptionContinueSearch;
}

/*! RtlpUnwindInternal
 * \brief Walks the frames calling their language handlers, for exception
 *        dispatch (UNW_FLAG_EHANDLER) or for an unwind (UNW_FLAG_UHANDLER).
 */
BOOLEAN
NTAPI
RtlpUnwindInternal(
    _In_opt_ PVOID TargetFrame,
    _In_opt_ PVOID TargetIp,
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue,
    _In_ PCONTEXT ContextRecord,
    _In_opt_ struct _UNWIND_HISTORY_TABLE *HistoryTable,
    _In_ ULONG HandlerType)
{
    DISPATCHER_CONTEXT DispatcherContext;
    DISPATCHER_CONTEXT_NONVOLREG_ARM64 NonVolatiles;
    PEXCEPTION_ROUTINE ExceptionRoutine;
    EXCEPTION_DISPOSITION Disposition;
    ULONG_PTR StackLow, StackHigh;
    ULONG64 EstablisherFrame;
    CONTEXT UnwindContext;

    RtlpGetStackLimits(&StackLow, &StackHigh);

    /* Nothing above the target frame gets unwound */
    if (TargetFrame != NULL)
        StackHigh = (ULONG64)TargetFrame + 1;

    UnwindContext = *ContextRecord;

    DispatcherContext.ContextRecord =
        (HandlerType == UNW_FLAG_UHANDLER) ? ContextRecord : &UnwindContext;
    DispatcherContext.HistoryTable = HistoryTable;
    DispatcherContext.TargetPc = (ULONG64)TargetIp;
    DispatcherContext.NonVolatileRegisters = NonVolatiles.Buffer;

    while (TRUE)
    {
        if (!RtlpIsStackPointerValid(UnwindContext.Sp, StackLow, StackHigh))
            return FALSE;

        RtlpSaveNonVolatileRegisters(&NonVolatiles, &UnwindContext);

        ExceptionRoutine = RtlpUnwindFrame(HandlerType,
                                           &DispatcherContext,
                                           &UnwindContext,
                                           &EstablisherFrame);

        /* Broken unwind data or a frame outside of the stack */
        if (UnwindContext.Pc == 0 ||
            EstablisherFrame < StackLow ||
            EstablisherFrame >= StackHigh ||
            (EstablisherFrame & 15))
        {
            // TODO: Handle the DPC stack

            if (HandlerType == UNW_FLAG_EHANDLER)
            {
                ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
                return FALSE;
            }

            RtlRaiseStatus(STATUS_BAD_STACK);
        }

        if (ExceptionRoutine != NULL)
        {
            /* Let the language handler know it owns the target frame */
            if (EstablisherFrame == (ULONG64)TargetFrame)
                ExceptionRecord->ExceptionFlags |= EXCEPTION_TARGET_UNWIND;

            RtlpCheckLogException(ExceptionRecord,
                                  &UnwindContext,
                                  &DispatcherContext,
                                  sizeof(DispatcherContext));

            DispatcherContext.LanguageHandler = ExceptionRoutine;
            DispatcherContext.EstablisherFrame = EstablisherFrame;
            DispatcherContext.ScopeIndex = 0;

            UnwindContext.X0 = (ULONG64)ReturnValue;

            do
            {
                Disposition = RtlpExecuteHandlerForUnwind(ExceptionRecord,
                                                          (PVOID)EstablisherFrame,
                                                          ContextRecord,
                                                          &DispatcherContext);

                ExceptionRecord->ExceptionFlags &= ~(EXCEPTION_TARGET_UNWIND |
                                                     EXCEPTION_COLLIDED_UNWIND);

                if (HandlerType == UNW_FLAG_EHANDLER)
                {
                    if (Disposition == ExceptionContinueExecution)
                    {
                        if (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
                            RtlRaiseStatus(EXCEPTION_NONCONTINUABLE_EXCEPTION);

                        return TRUE;
                    }

                    if (Disposition == ExceptionNestedException)
                    {
                        // TODO: Handle nested exceptions
                        __debugbreak();
                    }
                }

                if (Disposition == ExceptionCollidedUnwind)
                {
                    /*
                     * The handler copied in the dispatcher context of the
                     * unwind it collided with. Pick up that unwind's context
                     * and unwind its frame again to continue from there.
                     */
                    UnwindContext = *ContextRecord = *DispatcherContext.ContextRecord;
                    RtlpSaveNonVolatileRegisters(&NonVolatiles, &UnwindContext);
                    RtlpUnwindFrame(HandlerType,
                                    &DispatcherContext,
                                    &UnwindContext,
                                    &EstablisherFrame);

                    DispatcherContext.ContextRecord = &UnwindContext;
                    DispatcherContext.NonVolatileRegisters = NonVolatiles.Buffer;
                    EstablisherFrame = DispatcherContext.EstablisherFrame;

                    ExceptionRecord->ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                    continue;
                }

                if (Disposition != ExceptionContinueSearch)
                    RtlRaiseStatus(STATUS_INVALID_DISPOSITION);

            } while (ExceptionRecord->ExceptionFlags & EXCEPTION_COLLIDED_UNWIND);
        }

        if (EstablisherFrame == (ULONG64)TargetFrame)
            break;

        /* The frame is done, the caller's context carries on */
        if (HandlerType == UNW_FLAG_UHANDLER)
            *ContextRecord = UnwindContext;
    }

    if (ExceptionRecord->ExceptionCode != STATUS_UNWIND_CONSOLIDATE)
        ContextRecord->Pc = (ULONG64)TargetIp;

    ContextRecord->X0 = (ULONG64)ReturnValue;

    RtlRestoreContext(ContextRecord, ExceptionRecord);

    /* Never returns */
    ASSERT(FALSE);
    return FALSE;
}

/*! RtlUnwindEx
 * \brief Unwinds the stack up to TargetFrame and continues at TargetIp.
 */
VOID
NTAPI
RtlUnwindEx(
    _In_opt_ PVOID TargetFrame,
    _In_opt_ PVOID TargetIp,
    _In_opt_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue,
    _In_ PCONTEXT ContextRecord,
    _In_opt_ struct _UNWIND_HISTORY_TABLE *HistoryTable)
{
    EXCEPTION_RECORD LocalExceptionRecord;

    RtlCaptureContext(ContextRecord);

    if (ExceptionRecord == NULL)
    {
        LocalExceptionRecord.ExceptionCode = STATUS_UNWIND;
        LocalExceptionRecord.ExceptionAddress = (PVOID)ContextRecord->Pc;
        LocalExceptionRecord.ExceptionRecord = NULL;
        LocalExceptionRecord.NumberParameters = 0;
        ExceptionRecord = &LocalExceptionRecord;
    }

    ExceptionRecord->ExceptionFlags = EXCEPTION_UNWINDING;
    if (TargetFrame == NULL)
        ExceptionRecord->ExceptionFlags |= EXCEPTION_EXIT_UNWIND;

    RtlpUnwindInternal(TargetFrame,
                       TargetIp,
                       ExceptionRecord,
                       ReturnValue,
                       ContextRecord,
                       HistoryTable,
                       UNW_FLAG_UHANDLER);
}

VOID
NTAPI
RtlUnwind(
    _In_opt_ PVOID TargetFrame,
    _In_opt_ PVOID TargetIp,
    _In_opt_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue)
{
    CONTEXT Context;

    RtlUnwindEx(TargetFrame,
                TargetIp,
                ExceptionRecord,
                ReturnValue,
                &Context,
                NULL);
}

/*! RtlWalkFrameChain
 * \brief Collects the return addresses of the callers of the current function.
 */
ULONG
NTAPI
RtlWalkFrameChain(
    _Out_writes_to_(Count, return) PVOID *Callers,
    _In_ ULONG Count,
    _In_ ULONG Flags)
{
    DISPATCHER_CONTEXT DispatcherContext;
    CONTEXT Context;
    ULONG_PTR StackLow, StackHigh;
    ULONG64 EstablisherFrame;
    ULONG i, FramesToSkip;
    MODE CurrentMode = RtlpGetMode();

    /* The upper bits of Flags tell how many frames to skip */
    FramesToSkip = Flags >> RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT;

    RtlCaptureContext(&Context);
    RtlpGetStackLimits(&StackLow, &StackHigh);

    _SEH2_TRY
    {
        for (i = 0; i < FramesToSkip + Count; i++)
        {
            RtlpUnwindFrame(UNW_FLAG_NHANDLER, &DispatcherContext, &Context, &EstablisherFrame);

            if (CurrentMode == KernelMode && Context.Pc < KSEG0_BASE)
            {
                /* Only continue into user mode when asked to */
                if (!(Flags & 1))
                    break;

                CurrentMode = UserMode;
                StackLow = (ULONG_PTR)NtCurrentTeb()->NtTib.StackLimit;
                StackHigh = (ULONG_PTR)NtCurrentTeb()->NtTib.StackBase;
            }

            if (CurrentMode == UserMode &&
                (Context.Pc < 0x10000 || Context.Pc > 0x00007FFFFFFEFFFFULL))
            {
                break;
            }

            if (Context.Sp <= StackLow || Context.Sp >= StackHigh)
                break;

            if (i >= FramesToSkip)
                Callers[i - FramesToSkip] = (PVOID)Context.Pc;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Exception while getting callers!\n");
        i = 0;
    }
    _SEH2_END;

    return (i > FramesToSkip) ? i - FramesToSkip : 0;
}

/*! RtlGetCallersAddress
 * \ref http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/Debug/RtlGetCallersAddress.html
 */
#undef RtlGetCallersAddress
VOID
NTAPI
RtlGetCallersAddress(
    _Out_ PVOID *CallersAddress,
    _Out_ PVOID *CallersCaller)
{
    PVOID Callers[4];
    ULONG Number;

    /* RtlWalkFrameChain -> RtlGetCallersAddress -> x -> y */
    Number = RtlWalkFrameChain(Callers, 4, 0);

    *CallersAddress = (Number >= 3) ? Callers[2] : NULL;
    *CallersCaller = (Number == 4) ? Callers[3] : NULL;
}

VOID
NTAPI
RtlpRestoreContextInternal(
    _In_ PCONTEXT ContextRecord);

/*! RtlRestoreContext
 * \brief Continues execution with ContextRecord, after a consolidation
 *        callback or with a longjmp target when ExceptionRecord asks for it.
 */
VOID
NTAPI
RtlRestoreContext(
    _In_ PCONTEXT ContextRecord,
    _In_opt_ PEXCEPTION_RECORD ExceptionRecord)
{
    ULONG i;

    if (ExceptionRecord != NULL)
    {
        if (ExceptionRecord->ExceptionCode == STATUS_UNWIND_CONSOLIDATE &&
            ExceptionRecord->NumberParameters >= 1)
        {
            PVOID (*Consolidate)(PEXCEPTION_RECORD) = (PVOID)ExceptionRecord->ExceptionInformation[0];

            // FIXME: This should be called through an asm wrapper to allow handling recursive unwinding
            ContextRecord->Pc = (ULONG64)Consolidate(ExceptionRecord);
        }
        else if (ExceptionRecord->ExceptionCode == STATUS_LONGJUMP)
        {
            _JUMP_BUFFER *JumpBuffer = (_JUMP_BUFFER *)ExceptionRecord->ExceptionInformation[0];

            for (i = 0; i < 10; i++)
                ContextRecord->X[19 + i] = (&JumpBuffer->X19)[i];

            ContextRecord->Fp = JumpBuffer->Fp;
            ContextRecord->Sp = JumpBuffer->Sp;
            ContextRecord->Pc = JumpBuffer->Lr;
            ContextRecord->Fpcr = JumpBuffer->Fpcr;
            ContextRecord->Fpsr = JumpBuffer->Fpsr;

            /* setjmp only keeps the low halves of v8-v15 */
            for (i = 0; i < 8; i++)
            {
                ContextRecord->V[8 + i].D[0] = JumpBuffer->D[i];
                ContextRecord->V[8 + i].D[1] = 0;
            }
        }
    }

    RtlpRestoreContextInternal(ContextRecord);
}
