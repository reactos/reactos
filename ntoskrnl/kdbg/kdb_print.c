/*
 * PROJECT:     ReactOS KDBG Kernel Debugger
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     IO interface for KDBG. Provides local KDBG versions
 *              of KdpPrintString, KdpPromptString and KdpDprintf.
 * COPYRIGHT:   Copyright 2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "kdb.h"

/* FUNCTIONS *****************************************************************/

static VOID
KdbPrintStringWorker(
    _In_ const CSTRING* Output,
    _In_ ULONG ApiNumber,
    _Inout_ PDBGKD_DEBUG_IO DebugIo,
    _Inout_ PSTRING Header,
    _Inout_ PSTRING Data)
{
    USHORT Length;
    C_ASSERT(PACKET_MAX_SIZE >= sizeof(*DebugIo));

    ASSERT((ApiNumber == DbgKdPrintStringApi) ||
           (ApiNumber == DbgKdGetStringApi));

    /* Make sure we don't exceed the KD Packet size */
    Length = min(Output->Length, PACKET_MAX_SIZE - sizeof(*DebugIo));

    /* Build the packet header */
    DebugIo->ApiNumber = ApiNumber;
    DebugIo->ProcessorLevel = 0; // (USHORT)KeProcessorLevel;
    DebugIo->Processor = KeGetCurrentPrcb()->Number;

    if (ApiNumber == DbgKdPrintStringApi)
        DebugIo->u.PrintString.LengthOfString = Length;
    else // if (ApiNumber == DbgKdGetStringApi)
        DebugIo->u.GetString.LengthOfPromptString = Length;

    Header->Length = sizeof(*DebugIo);
    Header->Buffer = (PCHAR)DebugIo;

    /* Build the data */
    Data->Length = Length;
    Data->Buffer = (PCHAR)Output->Buffer;

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_DEBUG_IO, Header, Data, &KdpContext);
}

VOID
KdbPrintString(
    _In_ const CSTRING* Output)
{
    DBGKD_DEBUG_IO DebugIo;
    STRING Header, Data;

    KdbPrintStringWorker(Output, DbgKdPrintStringApi,
                         &DebugIo, &Header, &Data);
}

static BOOLEAN
KdbPromptStringWorker(
    _In_ const CSTRING* PromptString,
    _Inout_ PSTRING ResponseString)
{
    DBGKD_DEBUG_IO DebugIo;
    STRING Header, Data;
    ULONG Length;
    KDSTATUS Status;

    /* Print the prompt */
    // DebugIo.u.GetString.LengthOfPromptString = Length;
    DebugIo.u.GetString.LengthOfStringRead = ResponseString->MaximumLength;
    KdbPrintStringWorker(PromptString, DbgKdGetStringApi,
                         &DebugIo, &Header, &Data);

    /* Set the maximum lengths for the receive */
    Header.MaximumLength = sizeof(DebugIo);
    Data.MaximumLength = ResponseString->MaximumLength;
    /* Build the data */
    Data.Buffer = ResponseString->Buffer;

    /* Enter receive loop */
    do
    {
        /* Get our reply */
        Status = KdReceivePacket(PACKET_TYPE_KD_DEBUG_IO,
                                 &Header,
                                 &Data,
                                 &Length,
                                 &KdpContext);

        /* Return TRUE if we need to resend */
        if (Status == KdPacketNeedsResend)
            return TRUE;

    /* Loop until we succeed */
    } while (Status != KdPacketReceived);

    /* Don't copy back a larger response than there is room for */
    Length = min(Length, ResponseString->MaximumLength);

    /* We've got the string back; return the length */
    ResponseString->Length = (USHORT)Length;

    /* Success; we don't need to resend */
    return FALSE;
}

USHORT
KdbPromptString(
    _In_ const CSTRING* PromptString,
    _Inout_ PSTRING ResponseString)
{
    /* Enter prompt loop: send the prompt and receive the response */
    ResponseString->Length = 0;
    while (KdbPromptStringWorker(PromptString, ResponseString))
    {
        /* Loop while we need to resend */
    }
    return ResponseString->Length;
}


VOID
KdbPutsN(
    _In_ PCCH String,
    _In_ USHORT Length)
{
    CSTRING Output;

    Output.Buffer = String;
    Output.Length = Output.MaximumLength = Length;
    KdbPrintString(&Output);
}

VOID
KdbPuts(
    _In_ PCSTR String)
{
    KdbPutsN(String, (USHORT)strnlen(String, MAXUSHORT - sizeof(ANSI_NULL)));
}

VOID
__cdecl
KdbPrintf(
    _In_ PCSTR Format,
    ...)
{
    va_list ap;
    SIZE_T Length;
    CHAR Buffer[1024];

    /* Format the string */
    va_start(ap, Format);
    Length = _vsnprintf(Buffer,
                        sizeof(Buffer),
                        Format,
                        ap);
    Length = min(Length, MAXUSHORT - sizeof(ANSI_NULL));
    va_end(ap);

    /* Send it to the debugger directly */
    KdbPutsN(Buffer, (USHORT)Length);
}

SIZE_T
KdbPrompt(
    _In_ PCSTR Prompt,
    _Out_ PCHAR Buffer,
    _In_ SIZE_T Size)
{
    CSTRING PromptString;
    STRING ResponseBuffer;

    PromptString.Buffer = Prompt;
    PromptString.Length = PromptString.MaximumLength =
        (USHORT)strnlen(Prompt, MAXUSHORT - sizeof(ANSI_NULL));

    ResponseBuffer.Buffer = Buffer;
    ResponseBuffer.Length = 0;
    ResponseBuffer.MaximumLength = (USHORT)min(Size, MAXUSHORT);

    return KdbPromptString(&PromptString, &ResponseBuffer);
}

/* EOF */
