/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kdgdb/gdb_regs.c
 * PURPOSE:         Architecture independent register access for the GDB stub.
 *
 * The architecture specific files only describe the register file, through
 * gdb_reg_size/gdb_reg_count and the gdb_ctx_to_reg/gdb_set_ctx_reg/
 * gdb_thread_to_reg accessors. Everything below is common to all of them.
 */

#include "kdgdb.h"

/* PRIVATE FUNCTIONS **********************************************************/
static
BOOLEAN
debug_thread_is_current(VOID)
{
    UINT_PTR CurrentTid;

    CurrentTid = handle_to_gdb_tid(PsGetThreadId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread));
    return gdb_dbg_tid == 0 ||
           gdb_dbg_tid == (UINT_PTR)-1 ||
           gdb_dbg_tid == CurrentTid;
}

/* GDB expects a fixed size register block, so registers we cannot provide
 * must still be sent, as a run of 'x' characters. */
static
VOID
send_gdb_unavailable_register(
    _In_ ULONG Register)
{
    UCHAR Size = gdb_reg_size[Register];

    while (Size--)
        send_gdb_partial_packet("xx");
}

/* GLOBAL FUNCTIONS ***********************************************************/
KDSTATUS
gdb_send_registers(void)
{
    const UCHAR* RegisterPtr;
    ULONG ScalarValue;
    ULONG i;

    start_gdb_packet();

    KDDBGPRINT("Sending registers of thread %" PRIxPTR ".\n", gdb_dbg_tid);
    KDDBGPRINT("Current thread_id: %p.\n", PsGetThreadId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread));
    if (debug_thread_is_current())
    {
        for (i = 0; i < gdb_reg_count; i++)
        {
            RegisterPtr = gdb_ctx_to_reg(&CurrentContext, i, &ScalarValue);
            send_gdb_partial_memory(RegisterPtr, gdb_reg_size[i]);
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

        for (i = 0; i < gdb_reg_count; i++)
        {
            RegisterPtr = gdb_thread_to_reg(DbgThread, i);
            if (RegisterPtr)
                send_gdb_partial_memory(RegisterPtr, gdb_reg_size[i]);
            else
                send_gdb_unavailable_register(i);
        }
    }

    return finish_gdb_packet();
}

KDSTATUS
gdb_send_register(void)
{
    const char* End = &gdb_input[gdb_input_length];
    const char* Next;
    const void* ptr;
    ULONG ScalarValue;
    ULONG64 Register;

    /* Get the GDB register number (gdb_input = "pXX") */
    if (!parse_hex_value(&gdb_input[1], End, &Register, &Next) ||
        Next != End ||
        Register >= gdb_reg_count)
    {
        return send_gdb_packet("E01");
    }

    if (debug_thread_is_current())
    {
        /* We can get it from the context of the current exception */
        ptr = gdb_ctx_to_reg(&CurrentContext, (ULONG)Register, &ScalarValue);
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

        ptr = gdb_thread_to_reg(DbgThread, (ULONG)Register);
    }

    if (!ptr)
    {
        start_gdb_packet();
        send_gdb_unavailable_register((ULONG)Register);
        return finish_gdb_packet();
    }

    KDDBGPRINT("KDGDB: Sending registers as memory.\n");
    return send_gdb_memory(ptr, gdb_reg_size[Register]);
}

BOOLEAN
gdb_write_register(
    _In_ ULONG Register,
    _In_reads_(Length) const CHAR* Value,
    _In_ ULONG Length)
{
    CONTEXT Context;
    UCHAR RegisterValue[GDB_MAX_REGISTER_SIZE];

    if (!debug_thread_is_current() ||
        Register >= gdb_reg_count ||
        !gdb_decode_hex(Value, Length, RegisterValue, gdb_reg_size[Register]))
    {
        return FALSE;
    }

    /* Only commit once the whole write is known to be good */
    Context = CurrentContext;
    if (!gdb_set_ctx_reg(&Context, Register, RegisterValue, gdb_reg_size[Register]))
        return FALSE;

    CurrentContext = Context;
    return TRUE;
}

BOOLEAN
gdb_write_registers(
    _In_reads_(Length) const CHAR* Value,
    _In_ ULONG Length)
{
    CONTEXT Context;
    UCHAR RegisterValue[GDB_MAX_REGISTER_SIZE];
    ULONG ExpectedLength = 0;
    ULONG Offset = 0;
    ULONG Register;

    if (!debug_thread_is_current())
        return FALSE;

    for (Register = 0; Register < gdb_reg_count; Register++)
        ExpectedLength += gdb_reg_size[Register] * 2;
    if (Length != ExpectedLength)
        return FALSE;

    /* Only commit once the whole block is known to be good */
    Context = CurrentContext;
    for (Register = 0; Register < gdb_reg_count; Register++)
    {
        ULONG RegisterLength = gdb_reg_size[Register] * 2;

        if (!gdb_decode_hex(&Value[Offset],
                            RegisterLength,
                            RegisterValue,
                            gdb_reg_size[Register]) ||
            !gdb_set_ctx_reg(&Context,
                             Register,
                             RegisterValue,
                             gdb_reg_size[Register]))
        {
            return FALSE;
        }
        Offset += RegisterLength;
    }

    CurrentContext = Context;
    return TRUE;
}
