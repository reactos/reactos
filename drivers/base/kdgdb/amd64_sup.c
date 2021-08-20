/*
 * PROJECT:     ReactOS KD dll - GDB stub
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Base functions for the kernel debugger
 * COPYRIGHT:   Copyright 2021 Jérôme Gardou
 */

#include "kdgdb.h"

enum reg_name
{
    RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP,
    R8, R9, R10, R11, R12, R13, R14, R15,
    RIP,
    EFLAGS,
    CS, SS, DS, ES, FS, GS,
    ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7,
    FCTRL, FSTAT, FTAG, FISEG, FIOFF, FOSEG, FOOFF, FOP
};

static const unsigned char reg_size[] =
{
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8,
    4,
    4, 4, 4, 4, 4, 4,
    10, 10, 10, 10, 10, 10, 10, 10,
    8, 8, 8, 8, 8, 8, 8, 8
};

static
void*
ctx_to_reg(CONTEXT* ctx, enum reg_name name)
{
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
        case CS: return &ctx->SegCs;
        case DS: return &ctx->SegSs;
        case ES: return &ctx->SegEs;
        case FS: return &ctx->SegFs;
        case GS: return &ctx->SegGs;
        case SS: return &ctx->SegSs;
    }
#undef return_reg
    return 0;
}

static
void*
thread_to_reg(PETHREAD Thread, enum reg_name reg_name)
{
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
        /* Terminated thread ? */
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
        switch(reg_name)
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

KDSTATUS
gdb_send_registers(void)
{
    CHAR RegisterStr[17];
    UCHAR* RegisterPtr;
    unsigned short i;
    unsigned short size;

    start_gdb_packet();

    KDDBGPRINT("Sending registers of thread %" PRIxPTR ".\n", gdb_dbg_tid);
    KDDBGPRINT("Current thread_id: %p.\n", PsGetThreadId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread));
    if (((gdb_dbg_pid == 0) && (gdb_dbg_tid == 0)) ||
            gdb_tid_to_handle(gdb_dbg_tid) == PsGetThreadId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread))
    {
        for (i = 0; i < 24; i++)
        {
            RegisterPtr = ctx_to_reg(&CurrentContext, i);
            size = reg_size[i] * 2;
            RegisterStr[size] = 0;
            while (size)
            {
                size--;
                RegisterStr[size] = hex_chars[RegisterPtr[size/2] & 0xF];
                size--;
                RegisterStr[size] = hex_chars[RegisterPtr[size/2] >> 4];
            }

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

        for (i = 0; i < 24; i++)
        {
            RegisterPtr = thread_to_reg(DbgThread, i);
            size = reg_size[i] * 2;
            RegisterStr[size] = 0;

            while (size)
            {
                if (RegisterPtr)
                {
                    size--;
                    RegisterStr[size] = hex_chars[RegisterPtr[size/2] & 0xF];
                    size--;
                    RegisterStr[size] = hex_chars[RegisterPtr[size/2] >> 4];
                }
                else
                {
                    size--;
                    RegisterStr[size] = 'x';
                    size--;
                    RegisterStr[size] = 'x';
                }
            }

            send_gdb_partial_packet(RegisterStr);
        }
    }

    return finish_gdb_packet();
}

KDSTATUS
gdb_send_register(void)
{
    enum reg_name reg_name;
    void *ptr;

    /* Get the GDB register name (gdb_input = "pXX") */
    reg_name = (hex_value(gdb_input[1]) << 4) | hex_value(gdb_input[2]);

    if (((gdb_dbg_pid == 0) && (gdb_dbg_tid == 0)) ||
            gdb_tid_to_handle(gdb_dbg_tid) == PsGetThreadId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread))
    {
        /* We can get it from the context of the current exception */
        ptr = ctx_to_reg(&CurrentContext, reg_name);
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

        ptr = thread_to_reg(DbgThread, reg_name);
    }

    if (!ptr)
    {
        unsigned char size = reg_size[reg_name];
        start_gdb_packet();
        while (size--)
            send_gdb_partial_packet("xx");
        return finish_gdb_packet();
    }
    else
    {
        KDDBGPRINT("KDDBG : Sending registers as memory.\n");
        return send_gdb_memory(ptr, reg_size[reg_name]);
    }
}


