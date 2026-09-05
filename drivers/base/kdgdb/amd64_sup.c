/*
 * PROJECT:     ReactOS KD dll - GDB stub
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Base functions for the kernel debugger
 * COPYRIGHT:   Copyright 2021 Jérôme Gardou
 */

#include "kdgdb.h"

const CHAR gdb_target_xml[] =
    "<?xml version=\"1.0\"?><!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target version=\"1.0\"><architecture>i386:x86-64</architecture>"
    "<feature name=\"org.gnu.gdb.i386.core\">"
    "<reg name=\"rax\" bitsize=\"64\" type=\"int64\"/><reg name=\"rbx\" bitsize=\"64\" type=\"int64\"/>"
    "<reg name=\"rcx\" bitsize=\"64\" type=\"int64\"/><reg name=\"rdx\" bitsize=\"64\" type=\"int64\"/>"
    "<reg name=\"rsi\" bitsize=\"64\" type=\"int64\"/><reg name=\"rdi\" bitsize=\"64\" type=\"int64\"/>"
    "<reg name=\"rbp\" bitsize=\"64\" type=\"data_ptr\"/><reg name=\"rsp\" bitsize=\"64\" type=\"data_ptr\"/>"
    "<reg name=\"r8\" bitsize=\"64\" type=\"int64\"/><reg name=\"r9\" bitsize=\"64\" type=\"int64\"/>"
    "<reg name=\"r10\" bitsize=\"64\" type=\"int64\"/><reg name=\"r11\" bitsize=\"64\" type=\"int64\"/>"
    "<reg name=\"r12\" bitsize=\"64\" type=\"int64\"/><reg name=\"r13\" bitsize=\"64\" type=\"int64\"/>"
    "<reg name=\"r14\" bitsize=\"64\" type=\"int64\"/><reg name=\"r15\" bitsize=\"64\" type=\"int64\"/>"
    "<reg name=\"rip\" bitsize=\"64\" type=\"code_ptr\"/><reg name=\"eflags\" bitsize=\"32\" type=\"uint32\"/>"
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
    "<reg name=\"xmm8\" bitsize=\"128\" type=\"uint128\"/><reg name=\"xmm9\" bitsize=\"128\" type=\"uint128\"/>"
    "<reg name=\"xmm10\" bitsize=\"128\" type=\"uint128\"/><reg name=\"xmm11\" bitsize=\"128\" type=\"uint128\"/>"
    "<reg name=\"xmm12\" bitsize=\"128\" type=\"uint128\"/><reg name=\"xmm13\" bitsize=\"128\" type=\"uint128\"/>"
    "<reg name=\"xmm14\" bitsize=\"128\" type=\"uint128\"/><reg name=\"xmm15\" bitsize=\"128\" type=\"uint128\"/>"
    "<reg name=\"mxcsr\" bitsize=\"32\" type=\"uint32\"/>"
    "</feature></target>";
const SIZE_T gdb_target_xml_length = sizeof(gdb_target_xml) - 1;

enum reg_name
{
    RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP,
    R8, R9, R10, R11, R12, R13, R14, R15,
    RIP,
    EFLAGS,
    CS, SS, DS, ES, FS, GS,
    ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7,
    FCTRL, FSTAT, FTAG, FISEG, FIOFF, FOSEG, FOOFF, FOP,
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
    XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
    MXCSR
};

const UCHAR gdb_reg_size[] =
{
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8,
    4,
    4, 4, 4, 4, 4, 4,
    10, 10, 10, 10, 10, 10, 10, 10,
    4, 4, 4, 4, 4, 4, 4, 4,
    16, 16, 16, 16, 16, 16, 16, 16,
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
        case RAX: return &ctx->Rax;
        case RBX: return &ctx->Rbx;
        case RCX: return &ctx->Rcx;
        case RDX: return &ctx->Rdx;
        case RSP: return &ctx->Rsp;
        case RBP: return &ctx->Rbp;
        case RSI: return &ctx->Rsi;
        case RDI: return &ctx->Rdi;
        case RIP: return &ctx->Rip;
        case R8: return &ctx->R8;
        case R9: return &ctx->R9;
        case R10: return &ctx->R10;
        case R11: return &ctx->R11;
        case R12: return &ctx->R12;
        case R13: return &ctx->R13;
        case R14: return &ctx->R14;
        case R15: return &ctx->R15;
        case EFLAGS: return &ctx->EFlags;
        case CS: *ScalarValue = ctx->SegCs; return ScalarValue;
        case SS: *ScalarValue = ctx->SegSs; return ScalarValue;
        case DS: *ScalarValue = ctx->SegDs; return ScalarValue;
        case ES: *ScalarValue = ctx->SegEs; return ScalarValue;
        case FS: *ScalarValue = ctx->SegFs; return ScalarValue;
        case GS: *ScalarValue = ctx->SegGs; return ScalarValue;
        case ST0:
        case ST1:
        case ST2:
        case ST3:
        case ST4:
        case ST5:
        case ST6:
        case ST7:
            return &ctx->FltSave.FloatRegisters[name - ST0];
        case FCTRL: *ScalarValue = ctx->FltSave.ControlWord; return ScalarValue;
        case FSTAT: *ScalarValue = ctx->FltSave.StatusWord; return ScalarValue;
        case FTAG: *ScalarValue = ctx->FltSave.TagWord; return ScalarValue;
        case FISEG: *ScalarValue = ctx->FltSave.ErrorSelector; return ScalarValue;
        case FIOFF: *ScalarValue = ctx->FltSave.ErrorOffset; return ScalarValue;
        case FOSEG: *ScalarValue = ctx->FltSave.DataSelector; return ScalarValue;
        case FOOFF: *ScalarValue = ctx->FltSave.DataOffset; return ScalarValue;
        case FOP: *ScalarValue = ctx->FltSave.ErrorOpcode; return ScalarValue;
        case XMM0:
        case XMM1:
        case XMM2:
        case XMM3:
        case XMM4:
        case XMM5:
        case XMM6:
        case XMM7:
        case XMM8:
        case XMM9:
        case XMM10:
        case XMM11:
        case XMM12:
        case XMM13:
        case XMM14:
        case XMM15:
            return &ctx->FltSave.XmmRegisters[name - XMM0];
        case MXCSR: return &ctx->MxCsr;
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
    enum reg_name Name = (enum reg_name)Register;
    ULONG ScalarValue;
    PVOID Storage;

    if (Register >= gdb_reg_count || Size != gdb_reg_size[Register])
        return FALSE;

    if (Size == sizeof(ScalarValue))
        RtlCopyMemory(&ScalarValue, Value, sizeof(ScalarValue));

    switch (Name)
    {
        case CS: Context->SegCs = (USHORT)ScalarValue; return TRUE;
        case SS: Context->SegSs = (USHORT)ScalarValue; return TRUE;
        case DS: Context->SegDs = (USHORT)ScalarValue; return TRUE;
        case ES: Context->SegEs = (USHORT)ScalarValue; return TRUE;
        case FS: Context->SegFs = (USHORT)ScalarValue; return TRUE;
        case GS: Context->SegGs = (USHORT)ScalarValue; return TRUE;
        case FCTRL: Context->FltSave.ControlWord = (USHORT)ScalarValue; return TRUE;
        case FSTAT: Context->FltSave.StatusWord = (USHORT)ScalarValue; return TRUE;
        case FTAG: Context->FltSave.TagWord = (UCHAR)ScalarValue; return TRUE;
        case FISEG: Context->FltSave.ErrorSelector = (USHORT)ScalarValue; return TRUE;
        case FIOFF: Context->FltSave.ErrorOffset = ScalarValue; return TRUE;
        case FOSEG: Context->FltSave.DataSelector = (USHORT)ScalarValue; return TRUE;
        case FOOFF: Context->FltSave.DataOffset = ScalarValue; return TRUE;
        case FOP: Context->FltSave.ErrorOpcode = (USHORT)ScalarValue; return TRUE;
        default:
            Storage = (PVOID)gdb_ctx_to_reg(Context, Register, &ScalarValue);
            if (Storage == NULL)
                return FALSE;
            RtlCopyMemory(Storage, Value, Size);
            return TRUE;
    }
}

const void*
gdb_thread_to_reg(
    _In_ PETHREAD Thread,
    _In_ ULONG Register)
{
    enum reg_name reg_name = (enum reg_name)Register;
    static const void* NullValue = NULL;

#if 0
    if (Thread->Tcb.TrapFrame)
    {
        PKTRAP_FRAME TrapFrame = Thread->Tcb.TrapFrame;

        switch (reg_name)
        {
            case RAX: return &TrapFrame->Rax;
            case RBX: return &TrapFrame->Rbx;
            case RCX: return &TrapFrame->Rcx;
            case RDX: return &TrapFrame->Rdx;
            case RSP: return &TrapFrame->Rsp;
            case RBP: return &TrapFrame->Rbp;
            case RSI: return &TrapFrame->Rsi;
            case RDI: return &TrapFrame->Rdi;
            case RIP: return &TrapFrame->Rip;
            case R8: return &TrapFrame->R8;
            case R9: return &TrapFrame->R9;
            case R10: return &TrapFrame->R10;
            case R11: return &TrapFrame->R11;
            case EFLAGS: return &TrapFrame->EFlags;
            case CS: return &TrapFrame->SegCs;
            case DS: return &TrapFrame->SegSs;
            case ES: return &TrapFrame->SegEs;
            case FS: return &TrapFrame->SegFs;
            case GS: return &TrapFrame->SegGs;
            case SS: return &TrapFrame->SegSs;
            default:
                KDDBGPRINT("Unhandled regname: %d.\n", reg_name);
        }
    }
    else
#endif
    if (!Thread->Tcb.InitialStack)
    {
        /* Terminated thread? */
        switch (reg_name)
        {
            case RSP:
            case RBP:
            case RIP:
                KDDBGPRINT("Returning NULL for register %d.\n", reg_name);
                return &NullValue;
            default:
                return NULL;
        }
    }
    else
    {
        switch (reg_name)
        {
            case RSP: return &Thread->Tcb.KernelStack;
            case RIP:
            {
                PULONG_PTR Rsp = Thread->Tcb.KernelStack;
                return &Rsp[3];
            }
            case RBP:
            {
                PULONG_PTR Rsp = Thread->Tcb.KernelStack;
                return &Rsp[4];
            }
            default:
                return NULL;
        }
    }

    return NULL;
}
