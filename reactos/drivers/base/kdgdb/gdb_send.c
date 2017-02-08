/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/gdb_send.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

/* LOCALS *********************************************************************/
const char hex_chars[] = "0123456789abcdef";

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
send_gdb_packet(_In_ CHAR* Buffer)
{
    UCHAR ack;

    do {
        CHAR* ptr = Buffer;
        CHAR check_sum = 0;

        KdpSendByte('$');

        /* Calculate checksum */
        check_sum = 0;
        while (*ptr)
        {
            check_sum += *ptr;
            KdpSendByte(*ptr++);
        }

        /* append it */
        KdpSendByte('#');
        KdpSendByte(hex_chars[(check_sum >> 4) & 0xf]);
        KdpSendByte(hex_chars[check_sum & 0xf]);

        /* Wait for acknowledgement */
        if (KdpReceiveByte(&ack) != KdPacketReceived)
        {
            KD_DEBUGGER_NOT_PRESENT = TRUE;
            break;
        }
    } while (ack != '+');
}

void
send_gdb_memory(
    _In_ VOID* Buffer,
    _In_ size_t Length)
{
    UCHAR ack;

    do {
        CHAR* ptr = Buffer;
        CHAR check_sum = 0;
        size_t len = Length;
        CHAR Byte;

        KdpSendByte('$');

        /* Send the data */
        check_sum = 0;
        while (len--)
        {
            Byte = hex_chars[(*ptr >> 4) & 0xf];
            KdpSendByte(Byte);
            check_sum += Byte;
            Byte = hex_chars[*ptr++ & 0xf];
            KdpSendByte(Byte);
            check_sum += Byte;
        }

        /* append check sum */
        KdpSendByte('#');
        KdpSendByte(hex_chars[(check_sum >> 4) & 0xf]);
        KdpSendByte(hex_chars[check_sum & 0xf]);

        /* Wait for acknowledgement */
        if (KdpReceiveByte(&ack) != KdPacketReceived)
        {
            KD_DEBUGGER_NOT_PRESENT = TRUE;
            break;
        }
    } while (ack != '+');
}

void
gdb_send_debug_io(
    _In_ PSTRING String)
{
    UCHAR ack;

    do {
        CHAR* ptr = String->Buffer;
        CHAR check_sum;
        USHORT Length = String->Length;
        CHAR Byte;

        KdpSendByte('$');

        KdpSendByte('O');

        /* Send the data */
        check_sum = 'O';
        while (Length--)
        {
            Byte = hex_chars[(*ptr >> 4) & 0xf];
            KdpSendByte(Byte);
            check_sum += Byte;
            Byte = hex_chars[*ptr++ & 0xf];
            KdpSendByte(Byte);
            check_sum += Byte;
        }

        /* append check sum */
        KdpSendByte('#');
        KdpSendByte(hex_chars[(check_sum >> 4) & 0xf]);
        KdpSendByte(hex_chars[check_sum & 0xf]);

        /* Wait for acknowledgement */
        if (KdpReceiveByte(&ack) != KdPacketReceived)
        {
            KD_DEBUGGER_NOT_PRESENT = TRUE;
            break;
        }
    } while (ack != '+');
}

void
gdb_send_exception(void)
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

    ptr += sprintf(ptr, "thread:p%" PRIxPTR ".%" PRIxPTR ";",
        handle_to_gdb_pid(PsGetThreadProcessId(Thread)),
        handle_to_gdb_tid(PsGetThreadId(Thread)));
    ptr += sprintf(ptr, "core:%x;", CurrentStateChange.Processor);
    send_gdb_packet(gdb_out);
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
