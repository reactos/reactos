/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/gdb_input.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

const CHAR gdb_target_xml[] =
    "<?xml version=\"1.0\"?><!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target version=\"1.0\"><architecture>i386</architecture>"
    "<feature name=\"org.gnu.gdb.i386.core\">"
    "<reg name=\"eax\" bitsize=\"32\" type=\"int32\"/><reg name=\"ecx\" bitsize=\"32\" type=\"int32\"/>"
    "<reg name=\"edx\" bitsize=\"32\" type=\"int32\"/><reg name=\"ebx\" bitsize=\"32\" type=\"int32\"/>"
    "<reg name=\"esp\" bitsize=\"32\" type=\"data_ptr\"/><reg name=\"ebp\" bitsize=\"32\" type=\"data_ptr\"/>"
    "<reg name=\"esi\" bitsize=\"32\" type=\"int32\"/><reg name=\"edi\" bitsize=\"32\" type=\"int32\"/>"
    "<reg name=\"eip\" bitsize=\"32\" type=\"code_ptr\"/><reg name=\"eflags\" bitsize=\"32\" type=\"uint32\"/>"
    "<reg name=\"cs\" bitsize=\"32\" type=\"uint32\"/><reg name=\"ss\" bitsize=\"32\" type=\"uint32\"/>"
    "<reg name=\"ds\" bitsize=\"32\" type=\"uint32\"/><reg name=\"es\" bitsize=\"32\" type=\"uint32\"/>"
    "<reg name=\"fs\" bitsize=\"32\" type=\"uint32\"/><reg name=\"gs\" bitsize=\"32\" type=\"uint32\"/>"
    "<reg name=\"st0\" bitsize=\"80\" type=\"i387_ext\"/><reg name=\"st1\" bitsize=\"80\" type=\"i387_ext\"/>"
    "<reg name=\"st2\" bitsize=\"80\" type=\"i387_ext\"/><reg name=\"st3\" bitsize=\"80\" type=\"i387_ext\"/>"
    "<reg name=\"st4\" bitsize=\"80\" type=\"i387_ext\"/><reg name=\"st5\" bitsize=\"80\" type=\"i387_ext\"/>"
    "<reg name=\"st6\" bitsize=\"80\" type=\"i387_ext\"/><reg name=\"st7\" bitsize=\"80\" type=\"i387_ext\"/>"
    "<reg name=\"fctrl\" bitsize=\"32\" type=\"uint32\"/><reg name=\"fstat\" bitsize=\"32\" type=\"uint32\"/>"
    "<reg name=\"ftag\" bitsize=\"32\" type=\"uint32\"/><reg name=\"fiseg\" bitsize=\"32\" type=\"uint32\"/>"
    "<reg name=\"fioff\" bitsize=\"32\" type=\"uint32\"/><reg name=\"foseg\" bitsize=\"32\" type=\"uint32\"/>"
    "<reg name=\"fooff\" bitsize=\"32\" type=\"uint32\"/><reg name=\"fop\" bitsize=\"32\" type=\"uint32\"/>"
    "</feature><feature name=\"org.gnu.gdb.i386.sse\">"
    "<reg name=\"xmm0\" bitsize=\"128\" type=\"uint128\"/><reg name=\"xmm1\" bitsize=\"128\" type=\"uint128\"/>"
    "<reg name=\"xmm2\" bitsize=\"128\" type=\"uint128\"/><reg name=\"xmm3\" bitsize=\"128\" type=\"uint128\"/>"
    "<reg name=\"xmm4\" bitsize=\"128\" type=\"uint128\"/><reg name=\"xmm5\" bitsize=\"128\" type=\"uint128\"/>"
    "<reg name=\"xmm6\" bitsize=\"128\" type=\"uint128\"/><reg name=\"xmm7\" bitsize=\"128\" type=\"uint128\"/>"
    "<reg name=\"mxcsr\" bitsize=\"32\" type=\"uint32\"/>"
    "</feature></target>";
const SIZE_T gdb_target_xml_length = sizeof(gdb_target_xml) - 1;

enum reg_name
{
    EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
    EIP,
    EFLAGS,
    CS, SS, DS, ES, FS, GS,
    ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7,
    FCTRL, FSTAT, FTAG, FISEG, FIOFF, FOSEG, FOOFF, FOP,
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
    MXCSR
};

const UCHAR gdb_reg_size[] =
{
    4, 4, 4, 4, 4, 4, 4, 4,
    4,
    4,
    4, 4, 4, 4, 4, 4,
    10, 10, 10, 10, 10, 10, 10, 10,
    4, 4, 4, 4, 4, 4, 4, 4,
    16, 16, 16, 16, 16, 16, 16, 16,
    4
};
const ULONG gdb_reg_count = RTL_NUMBER_OF(gdb_reg_size);

const void*
gdb_ctx_to_reg(
    _In_ CONTEXT* ctx,
    _In_ ULONG Register,
    _Out_ PULONG ScalarValue)
{
    enum reg_name name = (enum reg_name)Register;

    switch (name)
    {
    case EAX: return &ctx->Eax;
    case EBX: return &ctx->Ebx;
    case ECX: return &ctx->Ecx;
    case EDX: return &ctx->Edx;
    case ESP: return &ctx->Esp;
    case EBP: return &ctx->Ebp;
    case ESI: return &ctx->Esi;
    case EDI: return &ctx->Edi;
    case EIP: return &ctx->Eip;
    case EFLAGS: return &ctx->EFlags;
    case CS: return &ctx->SegCs;
    case DS: return &ctx->SegDs;
    case ES: return &ctx->SegEs;
    case FS: return &ctx->SegFs;
    case GS: return &ctx->SegGs;
    case SS: return &ctx->SegSs;
    /* 80 bits */
    case ST0:
    case ST1:
    case ST2:
    case ST3:
    case ST4:
    case ST5:
    case ST6:
    case ST7:
        return &ctx->FloatSave.RegisterArea[10 * (name - ST0)];
    /* X87 registers */
    case FCTRL: return &ctx->FloatSave.ControlWord;
    case FSTAT: return &ctx->FloatSave.StatusWord;
    case FTAG: return &ctx->FloatSave.TagWord;
    case FISEG: return &ctx->FloatSave.ErrorSelector;
    case FIOFF: return &ctx->FloatSave.ErrorOffset;
    case FOSEG: return &ctx->FloatSave.DataSelector;
    case FOOFF: return &ctx->FloatSave.DataOffset;
    case FOP:
        *ScalarValue = *(UNALIGNED USHORT*)&ctx->ExtendedRegisters[6];
        return ScalarValue;
    /* SSE */
    case XMM0:
    case XMM1:
    case XMM2:
    case XMM3:
    case XMM4:
    case XMM5:
    case XMM6:
    case XMM7:
        return &ctx->ExtendedRegisters[160 + (name - XMM0)*16];
    case MXCSR: return &ctx->ExtendedRegisters[24];
    }
    return NULL;
}
BOOLEAN
gdb_set_ctx_reg(
    _Inout_ CONTEXT* Context,
    _In_ ULONG Register,
    _In_reads_bytes_(Size) const UCHAR* Value,
    _In_ SIZE_T Size)
{
    ULONG ScalarValue;
    PVOID Storage;

    if (Register >= gdb_reg_count || Size != gdb_reg_size[Register])
        return FALSE;

    if ((enum reg_name)Register == FOP)
    {
        RtlCopyMemory(&ScalarValue, Value, sizeof(ScalarValue));
        *(UNALIGNED USHORT*)&Context->ExtendedRegisters[6] = (USHORT)ScalarValue;
        return TRUE;
    }

    Storage = (PVOID)gdb_ctx_to_reg(Context, Register, &ScalarValue);
    if (Storage == NULL)
        return FALSE;
    RtlCopyMemory(Storage, Value, Size);
    return TRUE;
}

const void*
gdb_thread_to_reg(
    _In_ PETHREAD Thread,
    _In_ ULONG Register)
{
    enum reg_name reg_name = (enum reg_name)Register;
    static const void* NullValue = NULL;

    if (!Thread->Tcb.InitialStack)
    {
        /* Terminated thread? */
        switch (reg_name)
        {
            case ESP:
            case EBP:
            case EIP:
                KDDBGPRINT("Returning NULL for register %d.\n", reg_name);
                return &NullValue;
            default:
                return NULL;
        }
    }
#if 0
    else if (Thread->Tcb.TrapFrame)
    {
        PKTRAP_FRAME TrapFrame = Thread->Tcb.TrapFrame;

        switch (reg_name)
        {
            case EAX: return &TrapFrame->Eax;
            case ECX: return &TrapFrame->Ecx;
            case EDX: return &TrapFrame->Edx;
            case EBX: return &TrapFrame->Ebx;
            case ESP: return (TrapFrame->PreviousPreviousMode == KernelMode) ?
                    &TrapFrame->TempEsp : &TrapFrame->HardwareEsp;
            case EBP: return &TrapFrame->Ebp;
            case ESI: return &TrapFrame->Esi;
            case EDI: return &TrapFrame->Edi;
            case EIP: return &TrapFrame->Eip;
            case EFLAGS: return &TrapFrame->EFlags;
            case CS: return &TrapFrame->SegCs;
            case SS: return &TrapFrame->HardwareSegSs;
            case DS: return &TrapFrame->SegDs;
            case ES: return &TrapFrame->SegEs;
            case FS: return &TrapFrame->SegFs;
            case GS: return &TrapFrame->SegGs;
            default:
                KDDBGPRINT("Unhandled regname: %d.\n", reg_name);
        }
    }
#endif
    else
    {
        static PULONG Esp;
        Esp = Thread->Tcb.KernelStack;
        switch (reg_name)
        {
            case EBP: return &Esp[3];
            case ESP: return &Esp;
            case EIP: return &NullValue;
            default:
                return NULL;
        }
    }

    return NULL;
}
