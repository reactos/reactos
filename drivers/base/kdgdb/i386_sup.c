/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/gdb_input.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

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

static
void*
ctx_to_reg(CONTEXT* ctx, enum reg_name name, unsigned short* size)
{
    /* For general registers: 32bits */
    *size = 4;
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
        *size = 10;
        return &ctx->FloatSave.RegisterArea[10 * (name - ST0)];
    /* X87 registers */
    case FCTRL: return &ctx->FloatSave.ControlWord;
    case FSTAT: return &ctx->FloatSave.StatusWord;
    case FTAG: return &ctx->FloatSave.TagWord;
    case FISEG: return &ctx->FloatSave.DataSelector;
    case FIOFF: return &ctx->FloatSave.DataOffset;
    case FOSEG: return &ctx->FloatSave.ErrorSelector;
    case FOOFF: return &ctx->FloatSave.ErrorOffset;
    case FOP: return &ctx->FloatSave.Cr0NpxState;
    /* SSE */
    case XMM0:
    case XMM1:
    case XMM2:
    case XMM3:
    case XMM4:
    case XMM5:
    case XMM6:
    case XMM7:
        *size = 16;
        return &ctx->ExtendedRegisters[160 + (name - XMM0)*16];
    case MXCSR: return &ctx->ExtendedRegisters[24];
    }
    return 0;
}

void
gdb_send_registers(void)
{
    CONTEXT* ctx;
    PKPRCB* ProcessorBlockLists;
    ULONG32 Registers[16];
    unsigned i;
    unsigned short size;

    ProcessorBlockLists = (PKPRCB*)KdDebuggerDataBlock->KiProcessorBlock.Pointer;
    ctx = (CONTEXT*)((char*)ProcessorBlockLists[CurrentStateChange.Processor] + KdDebuggerDataBlock->OffsetPrcbProcStateContext);

    for(i=0; i < 16; i++)
    {
        Registers[i] = *(ULONG32*)ctx_to_reg(ctx, i, &size);
    }
    send_gdb_memory(Registers, sizeof(Registers));
}
