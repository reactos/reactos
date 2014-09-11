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
    DBGKM_EXCEPTION64* Exception = NULL;

    if (CurrentStateChange.NewState == DbgKdExceptionStateChange)
        Exception = &CurrentStateChange.u.Exception;

    /* Report to GDB */
    *ptr++ = 'T';
    if (Exception)
        ptr = exception_code_to_gdb(Exception->ExceptionRecord.ExceptionCode, ptr);
    else
        ptr += sprintf(ptr, "05");
    ptr += sprintf(ptr, "thread:p%p.%p;",
        PsGetThreadProcessId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread),
        PsGetThreadId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread));
    ptr += sprintf(ptr, "core:%x;", CurrentStateChange.Processor);
    send_gdb_packet(gdb_out);
}

#ifdef KDDEBUG
ULONG KdpDbgPrint(const char* Format, ...)
{
    va_list ap;
    CHAR Buffer[512];
    struct _STRING Str;
    int Length;

    va_start(ap, Format);
    Length = _vsnprintf(Buffer, sizeof(Buffer), Format, ap);
    va_end(ap);

    /* Check if we went past the buffer */
    if (Length == -1)
    {
        /* Terminate it if we went over-board */
        Buffer[sizeof(Buffer) - 1] = '\n';

        /* Put maximum */
        Length = sizeof(Buffer);
    }

    Str.Buffer = Buffer;
    Str.Length = Length;
    Str.MaximumLength = sizeof(Buffer);

    gdb_send_debug_io(&Str);

    return 0;
}
#endif

