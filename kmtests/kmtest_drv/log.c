/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver logging functions
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <ntddk.h>
#include <ntstrsafe.h>

#include <kmt_log.h>

#define LOGBUFFER_MAX (1024UL * 1024)
static PCHAR LogBuffer;
static SIZE_T LogOffset;

#define LOG_TAG 'LtmK'

/* TODO: allow concurrent log buffer access */

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

    LogBuffer = ExAllocatePoolWithTag(NonPagedPool, LOGBUFFER_MAX, LOG_TAG);

    if (!LogBuffer)
        Status = STATUS_INSUFFICIENT_RESOURCES;

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

    ExFreePoolWithTag(LogBuffer, LOG_TAG);
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
    SIZE_T MessageLength = strlen(Message);
    ASSERT(LogOffset + MessageLength + 1 < LOGBUFFER_MAX);
    RtlCopyMemory(&LogBuffer[LogOffset], Message, MessageLength + 1);
    LogOffset += MessageLength;
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
    CHAR Buffer[1024];
    SIZE_T BufferLength;
    /* TODO: make this work from any IRQL */
    PAGED_CODE();

    RtlStringCbVPrintfA(Buffer, sizeof Buffer, Format, Arguments);

    BufferLength = strlen(Buffer);
    ASSERT(LogOffset + BufferLength + 1 < LOGBUFFER_MAX);
    RtlCopyMemory(&LogBuffer[LogOffset], Buffer, BufferLength + 1);
    LogOffset += BufferLength;
}

/**
 * @name LogRead
 *
 * Retrieve data from the log buffer.
 *
 * @param Buffer
 *        Buffer to copy log data to
 * @param BufferSize
 *        Maximum number of bytes to copy
 *
 * @return Number of bytes copied
 */
SIZE_T LogRead(OUT PVOID Buffer, IN SIZE_T BufferSize)
{
    SIZE_T Size;
    PAGED_CODE();

    Size = min(BufferSize, LogOffset);
    RtlCopyMemory(Buffer, LogBuffer, Size);

    if (BufferSize < LogOffset)
    {
        SIZE_T SizeLeft = LogOffset - BufferSize;
        RtlMoveMemory(LogBuffer, &LogBuffer[LogOffset], SizeLeft);
        LogOffset = SizeLeft;
    }
    else
        LogOffset = 0;

    return Size;
}
