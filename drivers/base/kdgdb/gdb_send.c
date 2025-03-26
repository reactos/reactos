/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/gdb_send.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

/* LOCALS *********************************************************************/
const char hex_chars[] = "0123456789abcdef";
static CHAR currentChecksum = 0;

/* PRIVATE FUNCTIONS **********************************************************/
static
char*
exception_code_to_gdb(NTSTATUS code, char* out)
{
    unsigned char SigVal;

    switch (code)
    {
    case STATUS_INTEGER_DIVIDE_BY_ZERO:
        SigVal = 8; /* divide by zero */
        break;
    case STATUS_SINGLE_STEP:
    case STATUS_BREAKPOINT:
        SigVal = 5; /* breakpoint */
        break;
    case STATUS_INTEGER_OVERFLOW:
    case STATUS_ARRAY_BOUNDS_EXCEEDED:
        SigVal = 16; /* bound instruction */
        break;
    case STATUS_ILLEGAL_INSTRUCTION:
        SigVal = 4; /* Invalid opcode */
        break;
    case STATUS_STACK_OVERFLOW:
    case STATUS_DATATYPE_MISALIGNMENT:
    case STATUS_ACCESS_VIOLATION:
        SigVal = 11; /* access violation */
        break;
    default:
        SigVal = 7; /* "software generated" */
    }
    *out++ = hex_chars[(SigVal >> 4) & 0xf];
    *out++ = hex_chars[SigVal & 0xf];
    return out;
}

/* GLOBAL FUNCTIONS ***********************************************************/
void
start_gdb_packet(void)
{
    /* Start the start byte and begin checksum calculation */
    KdpSendByte('$');
    currentChecksum = 0;
}

void
send_gdb_partial_packet(_In_ const CHAR* Buffer)
{
    const CHAR* ptr = Buffer;

    /* Update check sum and send */
    while (*ptr)
    {
        currentChecksum += *ptr;
        KdpSendByte(*ptr++);
    }
}


KDSTATUS
finish_gdb_packet(void)
{
    UCHAR ack;
    KDSTATUS Status;

    /* Send finish byte and append checksum */
    KdpSendByte('#');
    KdpSendByte(hex_chars[(currentChecksum >> 4) & 0xf]);
    KdpSendByte(hex_chars[currentChecksum & 0xf]);

    /* Wait for acknowledgement */
    Status = KdpReceiveByte(&ack);

    if (Status != KdPacketReceived)
    {
        KD_DEBUGGER_NOT_PRESENT = TRUE;
        return Status;
    }

    if (ack != '+')
        return KdPacketNeedsResend;

    return KdPacketReceived;
}

KDSTATUS
send_gdb_packet(_In_ const CHAR* Buffer)
{
    start_gdb_packet();
    send_gdb_partial_packet(Buffer);
    return finish_gdb_packet();
}

ULONG
send_gdb_partial_binary(
    _In_ const VOID* Buffer,
    _In_ size_t Length)
{
    const UCHAR* ptr = Buffer;
    ULONG Sent = Length;

    while(Length--)
    {
        UCHAR Byte = *ptr++;

        switch (Byte)
        {
            case 0x7d:
            case 0x23:
            case 0x24:
            case 0x2a:
                currentChecksum += 0x7d;
                KdpSendByte(0x7d);
                Byte ^= 0x20;
                Sent++;
            /* Fall-through */
            default:
                currentChecksum += Byte;
                KdpSendByte(Byte);
        }
    }

    return Sent;
}

void
send_gdb_partial_memory(
    _In_ const VOID* Buffer,
    _In_ size_t Length)
{
    const UCHAR* ptr = Buffer;
    CHAR gdb_out[3];

    gdb_out[2] = '\0';

    while(Length--)
    {
        gdb_out[0] = hex_chars[(*ptr >> 4) & 0xf];
        gdb_out[1] = hex_chars[*ptr++ & 0xf];
        send_gdb_partial_packet(gdb_out);
    }
}

KDSTATUS
send_gdb_memory(
    _In_ const VOID* Buffer,
    _In_ size_t Length)
{
    start_gdb_packet();
    send_gdb_partial_memory(Buffer, Length);
    return finish_gdb_packet();
}

KDSTATUS
gdb_send_debug_io(
    _In_ PSTRING String,
    _In_ BOOLEAN WithPrefix)
{
    CHAR gdb_out[3];
    CHAR* ptr = String->Buffer;
    USHORT Length = String->Length;

    gdb_out[2] = '\0';

    start_gdb_packet();

    if (WithPrefix)
    {
        send_gdb_partial_packet("O");
    }

    /* Send the data */
    while (Length--)
    {
        gdb_out[0] = hex_chars[(*ptr >> 4) & 0xf];
        gdb_out[1] = hex_chars[*ptr++ & 0xf];
        send_gdb_partial_packet(gdb_out);
    }

    return finish_gdb_packet();
}

KDSTATUS
gdb_send_exception()
{
    char gdb_out[1024];
    char* ptr = gdb_out;
    PETHREAD Thread = (PETHREAD)(ULONG_PTR)CurrentStateChange.Thread;

    /* Report to GDB */
    *ptr++ = 'T';

    if (CurrentStateChange.NewState == DbgKdExceptionStateChange)
    {
        EXCEPTION_RECORD64* ExceptionRecord = &CurrentStateChange.u.Exception.ExceptionRecord;
        ptr = exception_code_to_gdb(ExceptionRecord->ExceptionCode, ptr);
    }
    else
        ptr += sprintf(ptr, "05");

    if (CurrentStateChange.NewState == DbgKdLoadSymbolsStateChange)
        ptr += sprintf(ptr, "library:");

#if MONOPROCESS
    ptr += sprintf(ptr, "thread:%" PRIxPTR ";",
        handle_to_gdb_tid(PsGetThreadId(Thread)));
#else
    ptr += sprintf(ptr, "thread:p%" PRIxPTR ".%" PRIxPTR ";",
        handle_to_gdb_pid(PsGetThreadProcessId(Thread)),
        handle_to_gdb_tid(PsGetThreadId(Thread)));
#endif

    ptr += sprintf(ptr, "core:%x;", CurrentStateChange.Processor);
    return send_gdb_packet(gdb_out);
}

void
send_gdb_ntstatus(
    _In_ NTSTATUS Status)
{
    /* Just build a EXX packet and send it */
    char gdb_out[4];
    gdb_out[0] = 'E';
    exception_code_to_gdb(Status, &gdb_out[1]);
    gdb_out[3] = '\0';
    send_gdb_packet(gdb_out);
}
