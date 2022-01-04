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

static
void*
thread_to_reg(PETHREAD Thread, enum reg_name reg_name, unsigned short* size)
{
    static const void* NullValue = NULL;

    if (!Thread->Tcb.InitialStack)
    {
        /* Terminated thread ? */
        switch (reg_name)
        {
            case ESP:
            case EBP:
            case EIP:
                KDDBGPRINT("Returning NULL for register %d.\n", reg_name);
                *size = 4;
                return &NullValue;
            default:
                return NULL;
        }
    }
#if 0
    else if (Thread->Tcb.TrapFrame)
    {
        PKTRAP_FRAME TrapFrame = Thread->Tcb.TrapFrame;

        *size = 4;
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
        *size = 4;
        switch(reg_name)
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

KDSTATUS
gdb_send_registers(void)
{
    CHAR RegisterStr[9];
    UCHAR* RegisterPtr;
    unsigned i;
    unsigned short size;

    RegisterStr[8] = '\0';

    start_gdb_packet();

    KDDBGPRINT("Sending registers of thread %" PRIxPTR ".\n", gdb_dbg_tid);
    KDDBGPRINT("Current thread_id: %p.\n", PsGetThreadId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread));
    if (((gdb_dbg_pid == 0) && (gdb_dbg_tid == 0)) ||
            gdb_tid_to_handle(gdb_dbg_tid) == PsGetThreadId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread))
    {
        for(i=0; i < 16; i++)
        {
            RegisterPtr = ctx_to_reg(&CurrentContext, i, &size);
            RegisterStr[0] = hex_chars[RegisterPtr[0] >> 4];
            RegisterStr[1] = hex_chars[RegisterPtr[0] & 0xF];
            RegisterStr[2] = hex_chars[RegisterPtr[1] >> 4];
            RegisterStr[3] = hex_chars[RegisterPtr[1] & 0xF];
            RegisterStr[4] = hex_chars[RegisterPtr[2] >> 4];
            RegisterStr[5] = hex_chars[RegisterPtr[2] & 0xF];
            RegisterStr[6] = hex_chars[RegisterPtr[3] >> 4];
            RegisterStr[7] = hex_chars[RegisterPtr[3] & 0xF];

            send_gdb_partial_packet(RegisterStr);
        }
    }
    else
    {
        PETHREAD DbgThread;

        DbgThread = find_thread(gdb_dbg_pid, gdb_dbg_tid);

        if (DbgThread == NULL)
        {
            /* Thread is dead */
            send_gdb_partial_packet("E03");
            return finish_gdb_packet();
        }

        for(i=0; i < 16; i++)
        {
            RegisterPtr = thread_to_reg(DbgThread, i, &size);
            if (RegisterPtr)
            {
                RegisterStr[0] = hex_chars[RegisterPtr[0] >> 4];
                RegisterStr[1] = hex_chars[RegisterPtr[0] & 0xF];
                RegisterStr[2] = hex_chars[RegisterPtr[1] >> 4];
                RegisterStr[3] = hex_chars[RegisterPtr[1] & 0xF];
                RegisterStr[4] = hex_chars[RegisterPtr[2] >> 4];
                RegisterStr[5] = hex_chars[RegisterPtr[2] & 0xF];
                RegisterStr[6] = hex_chars[RegisterPtr[3] >> 4];
                RegisterStr[7] = hex_chars[RegisterPtr[3] & 0xF];

                send_gdb_partial_packet(RegisterStr);
            }
            else
            {
                send_gdb_partial_packet("xxxxxxxx");
            }
        }
    }

    return finish_gdb_packet();
}

KDSTATUS
gdb_send_register(void)
{
    enum reg_name reg_name;
    void *ptr;
    unsigned short size;

    /* Get the GDB register name (gdb_input = "pXX") */
    reg_name = (hex_value(gdb_input[1]) << 4) | hex_value(gdb_input[2]);

    if (((gdb_dbg_pid == 0) && (gdb_dbg_tid == 0)) ||
            gdb_tid_to_handle(gdb_dbg_tid) == PsGetThreadId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread))
    {
        /* We can get it from the context of the current exception */
        ptr = ctx_to_reg(&CurrentContext, reg_name, &size);
    }
    else
    {
        PETHREAD DbgThread;

        DbgThread = find_thread(gdb_dbg_pid, gdb_dbg_tid);

        if (DbgThread == NULL)
        {
            /* Thread is dead */
            return send_gdb_packet("E03");
        }

        ptr = thread_to_reg(DbgThread, reg_name, &size);
    }

    if (!ptr)
    {
        /* Undefined. Let's assume 32 bit register */
        return send_gdb_packet("xxxxxxxx");
    }
    else
    {
        KDDBGPRINT("KDDBG : Sending registers as memory.\n");
        return send_gdb_memory(ptr, size);
    }
}


