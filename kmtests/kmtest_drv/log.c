/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver logging functions
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <ntddk.h>
#include <ntstrsafe.h>

#include <kmt_log.h>
#define KMT_DEFINE_TEST_FUNCTIONS
#include <kmt_test.h>

/**
 * @name LogInit
 *
 * Initialize logging mechanism. Call from DriverEntry.
 *
 * @return Status
 */
NTSTATUS LogInit(VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    return Status;
}

/**
 * @name LogFree
 *
 * Clean up logging mechanism. Call from Unload.
 *
 * @return None
 */
VOID LogFree(VOID)
{
    PAGED_CODE();
}

/**
 * @name LogPrint
 *
 * Print a log message.
 *
 * @param Message
 *        Ansi string to be logged
 *
 * @return None
 */
VOID LogPrint(IN PCSTR Message)
{
    size_t MessageLength;
    ASSERT(NT_SUCCESS(RtlStringCbLengthA(Message, 512, &MessageLength)));

    KmtAddToLogBuffer(ResultBuffer, Message, MessageLength);
}

/**
 * @name LogPrintF
 *
 * Print a formatted log message.
 *
 * @param Format
 *        printf-like format string
 * @param ...
 *        Arguments corresponding to the format
 *
 * @return None
 */
VOID LogPrintF(IN PCSTR Format, ...)
{
    va_list Arguments;
    PAGED_CODE();
    va_start(Arguments, Format);
    LogVPrintF(Format, Arguments);
    va_end(Arguments);
}

/**
 * @name LogVPrintF
 *
 * Print a formatted log message.
 *
 * @param Format
 *        printf-like format string
 * @param Arguments
 *        Arguments corresponding to the format
 *
 * @return None
 */
VOID LogVPrintF(IN PCSTR Format, va_list Arguments)
{
    CHAR Buffer[512];
    /* TODO: make this work from any IRQL */
    PAGED_CODE();

    RtlStringCbVPrintfA(Buffer, sizeof Buffer, Format, Arguments);

    LogPrint(Buffer);
}
