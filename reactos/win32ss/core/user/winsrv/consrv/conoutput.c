/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            consrv/conoutput.c
 * PURPOSE:         General Console Output Functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"

#define NDEBUG
#include <debug.h>

/* PUBLIC SERVER APIS *********************************************************/

/*
 * FIXME: This function MUST be moved fro condrv/conoutput.c because only
 * consrv knows how to manipulate VDM screenbuffers.
 */
NTSTATUS NTAPI
ConDrvWriteConsoleOutputVDM(IN PCONSOLE Console,
                            IN PTEXTMODE_SCREEN_BUFFER Buffer,
                            IN PCHAR_CELL CharInfo/*Buffer*/,
                            IN COORD CharInfoSize,
                            IN PSMALL_RECT WriteRegion);
NTSTATUS NTAPI
ConDrvInvalidateBitMapRect(IN PCONSOLE Console,
                           IN PCONSOLE_SCREEN_BUFFER Buffer,
                           IN PSMALL_RECT Region);
CSR_API(SrvInvalidateBitMapRect)
{
    NTSTATUS Status;
    PCONSOLE_INVALIDATEDIBITS InvalidateDIBitsRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.InvalidateDIBitsRequest;
    PCONSOLE_SCREEN_BUFFER Buffer;

    DPRINT("SrvInvalidateBitMapRect\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   InvalidateDIBitsRequest->OutputHandle,
                                   &Buffer, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* In text-mode only, draw the VDM buffer if present */
    if (GetType(Buffer) == TEXTMODE_BUFFER && Buffer->Header.Console->VDMBuffer)
    {
        PTEXTMODE_SCREEN_BUFFER TextBuffer = (PTEXTMODE_SCREEN_BUFFER)Buffer;

        /*Status =*/ ConDrvWriteConsoleOutputVDM(Buffer->Header.Console,
                                                 TextBuffer,
                                                 Buffer->Header.Console->VDMBuffer,
                                                 Buffer->Header.Console->VDMBufferSize,
                                                 &InvalidateDIBitsRequest->Region);
    }

    Status = ConDrvInvalidateBitMapRect(Buffer->Header.Console,
                                        Buffer,
                                        &InvalidateDIBitsRequest->Region);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsolePalette(IN PCONSOLE Console,
                        // IN PGRAPHICS_SCREEN_BUFFER Buffer,
                        IN PCONSOLE_SCREEN_BUFFER Buffer,
                        IN HPALETTE PaletteHandle,
                        IN UINT PaletteUsage);
CSR_API(SrvSetConsolePalette)
{
    NTSTATUS Status;
    PCONSOLE_SETPALETTE SetPaletteRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetPaletteRequest;
    // PGRAPHICS_SCREEN_BUFFER Buffer;
    PCONSOLE_SCREEN_BUFFER Buffer;

    DPRINT("SrvSetConsolePalette\n");

    // NOTE: Tests show that this function is used only for graphics screen buffers
    // and otherwise it returns FALSE + sets last error to invalid handle.
    // I think it's ridiculous, because if you are in text mode, simulating
    // a change of VGA palette via DAC registers (done by a call to SetConsolePalette)
    // cannot be done... So I allow it in ReactOS !
    /*
    Status = ConSrvGetGraphicsBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     SetPaletteRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    */
    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   SetPaletteRequest->OutputHandle,
                                   &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /*
     * Make the palette handle public, so that it can be
     * used by other threads calling GDI functions on it.
     * Indeed, the palette handle comes from a console app
     * calling ourselves, running in CSRSS.
     */
    NtUserConsoleControl(ConsoleMakePalettePublic,
                         &SetPaletteRequest->PaletteHandle,
                         sizeof(SetPaletteRequest->PaletteHandle));

    Status = ConDrvSetConsolePalette(Buffer->Header.Console,
                                     Buffer,
                                     SetPaletteRequest->PaletteHandle,
                                     SetPaletteRequest->Usage);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvGetConsoleCursorInfo(IN PCONSOLE Console,
                           IN PTEXTMODE_SCREEN_BUFFER Buffer,
                           OUT PCONSOLE_CURSOR_INFO CursorInfo);
CSR_API(SrvGetConsoleCursorInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCURSORINFO CursorInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CursorInfoRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT("SrvGetConsoleCursorInfo\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     CursorInfoRequest->OutputHandle,
                                     &Buffer, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvGetConsoleCursorInfo(Buffer->Header.Console,
                                        Buffer,
                                        &CursorInfoRequest->Info);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleCursorInfo(IN PCONSOLE Console,
                           IN PTEXTMODE_SCREEN_BUFFER Buffer,
                           IN PCONSOLE_CURSOR_INFO CursorInfo);
CSR_API(SrvSetConsoleCursorInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSETCURSORINFO CursorInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CursorInfoRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT("SrvSetConsoleCursorInfo\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     CursorInfoRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleCursorInfo(Buffer->Header.Console,
                                        Buffer,
                                        &CursorInfoRequest->Info);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleCursorPosition(IN PCONSOLE Console,
                               IN PTEXTMODE_SCREEN_BUFFER Buffer,
                               IN PCOORD Position);
CSR_API(SrvSetConsoleCursorPosition)
{
    NTSTATUS Status;
    PCONSOLE_SETCURSORPOSITION SetCursorPositionRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetCursorPositionRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT("SrvSetConsoleCursorPosition\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     SetCursorPositionRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleCursorPosition(Buffer->Header.Console,
                                            Buffer,
                                            &SetCursorPositionRequest->Position);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

CSR_API(SrvCreateConsoleScreenBuffer)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PCONSOLE_CREATESCREENBUFFER CreateScreenBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.CreateScreenBufferRequest;
    PCSR_PROCESS Process = CsrGetClientThread()->Process;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(Process);
    PCONSRV_CONSOLE Console;
    PCONSOLE_SCREEN_BUFFER Buff;

    PVOID ScreenBufferInfo = NULL;
    TEXTMODE_BUFFER_INFO TextModeInfo = {{80, 25},
                                         DEFAULT_SCREEN_ATTRIB,
                                         DEFAULT_POPUP_ATTRIB ,
                                         TRUE,
                                         CSR_DEFAULT_CURSOR_SIZE};
    GRAPHICS_BUFFER_INFO GraphicsInfo;
    GraphicsInfo.Info = CreateScreenBufferRequest->GraphicsBufferInfo; // HACK for MSVC

    DPRINT("SrvCreateConsoleScreenBuffer\n");

    Status = ConSrvGetConsole(ProcessData, &Console, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    if (CreateScreenBufferRequest->ScreenBufferType == CONSOLE_TEXTMODE_BUFFER)
    {
        ScreenBufferInfo = &TextModeInfo;

        /*
        if (Console->ActiveBuffer)
        {
            TextModeInfo.ScreenBufferSize = Console->ActiveBuffer->ScreenBufferSize;
            if (TextModeInfo.ScreenBufferSize.X == 0) TextModeInfo.ScreenBufferSize.X = 80;
            if (TextModeInfo.ScreenBufferSize.Y == 0) TextModeInfo.ScreenBufferSize.Y = 25;

            TextModeInfo.ScreenAttrib = Console->ActiveBuffer->ScreenBuffer.TextBuffer.ScreenDefaultAttrib;
            TextModeInfo.PopupAttrib  = Console->ActiveBuffer->ScreenBuffer.TextBuffer.PopupDefaultAttrib;

            TextModeInfo.IsCursorVisible = Console->ActiveBuffer->CursorInfo.bVisible;
            TextModeInfo.CursorSize      = Console->ActiveBuffer->CursorInfo.dwSize;
        }
        */

        /*
         * This is Windows' behaviour
         */

        /* Use the current console size. Regularize it if needed. */
        TextModeInfo.ScreenBufferSize = Console->ConsoleSize;
        if (TextModeInfo.ScreenBufferSize.X == 0) TextModeInfo.ScreenBufferSize.X = 1;
        if (TextModeInfo.ScreenBufferSize.Y == 0) TextModeInfo.ScreenBufferSize.Y = 1;

        /* If we have an active screen buffer, use its attributes as the new ones */
        if (Console->ActiveBuffer && GetType(Console->ActiveBuffer) == TEXTMODE_BUFFER)
        {
            PTEXTMODE_SCREEN_BUFFER Buffer = (PTEXTMODE_SCREEN_BUFFER)Console->ActiveBuffer;

            TextModeInfo.ScreenAttrib = Buffer->ScreenDefaultAttrib;
            TextModeInfo.PopupAttrib  = Buffer->PopupDefaultAttrib;

            TextModeInfo.IsCursorVisible = Buffer->CursorInfo.bVisible;
            TextModeInfo.CursorSize      = Buffer->CursorInfo.dwSize;
        }
    }
    else if (CreateScreenBufferRequest->ScreenBufferType == CONSOLE_GRAPHICS_BUFFER)
    {
        /* Get infos from the graphics buffer information structure */
        if (!CsrValidateMessageBuffer(ApiMessage,
                                      (PVOID*)&CreateScreenBufferRequest->GraphicsBufferInfo.lpBitMapInfo,
                                      CreateScreenBufferRequest->GraphicsBufferInfo.dwBitMapInfoLength,
                                      sizeof(BYTE)))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quit;
        }

        ScreenBufferInfo = &GraphicsInfo;

        /* Initialize shared variables */
        // CreateScreenBufferRequest->GraphicsBufferInfo.hMutex
        CreateScreenBufferRequest->hMutex   = GraphicsInfo.Info.hMutex   = INVALID_HANDLE_VALUE;
        // CreateScreenBufferRequest->GraphicsBufferInfo.lpBitMap
        CreateScreenBufferRequest->lpBitMap = GraphicsInfo.Info.lpBitMap = NULL;

        /* A graphics screen buffer is never inheritable */
        CreateScreenBufferRequest->InheritHandle = FALSE;
    }

    Status = ConDrvCreateScreenBuffer(&Buff,
                                      (PCONSOLE)Console,
                                      Process->ProcessHandle,
                                      CreateScreenBufferRequest->ScreenBufferType,
                                      ScreenBufferInfo);
    if (!NT_SUCCESS(Status)) goto Quit;

    /* Insert the new handle inside the process handles table */
    RtlEnterCriticalSection(&ProcessData->HandleTableLock);

    Status = ConSrvInsertObject(ProcessData,
                                &CreateScreenBufferRequest->OutputHandle,
                                &Buff->Header,
                                CreateScreenBufferRequest->DesiredAccess,
                                CreateScreenBufferRequest->InheritHandle,
                                CreateScreenBufferRequest->ShareMode);

    RtlLeaveCriticalSection(&ProcessData->HandleTableLock);

    if (!NT_SUCCESS(Status)) goto Quit;

    if (CreateScreenBufferRequest->ScreenBufferType == CONSOLE_GRAPHICS_BUFFER)
    {
        PGRAPHICS_SCREEN_BUFFER Buffer = (PGRAPHICS_SCREEN_BUFFER)Buff;
        /*
         * Initialize the graphics buffer information structure
         * and give it back to the client.
         */
        // CreateScreenBufferRequest->GraphicsBufferInfo.hMutex
        CreateScreenBufferRequest->hMutex   = Buffer->ClientMutex;
        // CreateScreenBufferRequest->GraphicsBufferInfo.lpBitMap
        CreateScreenBufferRequest->lpBitMap = Buffer->ClientBitMap;
    }

Quit:
    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleActiveScreenBuffer(IN PCONSOLE Console,
                                   IN PCONSOLE_SCREEN_BUFFER Buffer);
CSR_API(SrvSetConsoleActiveScreenBuffer)
{
    NTSTATUS Status;
    PCONSOLE_SETACTIVESCREENBUFFER SetScreenBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetScreenBufferRequest;
    PCONSOLE_SCREEN_BUFFER Buffer;

    DPRINT("SrvSetConsoleActiveScreenBuffer\n");

    Status = ConSrvGetScreenBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                   SetScreenBufferRequest->OutputHandle,
                                   &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleActiveScreenBuffer(Buffer->Header.Console,
                                                Buffer);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}


/* CSR THREADS FOR WriteConsole ***********************************************/

static NTSTATUS
DoWriteConsole(IN PCSR_API_MESSAGE ApiMessage,
               IN PCSR_THREAD ClientThread,
               IN BOOLEAN CreateWaitBlock OPTIONAL);

// Wait function CSR_WAIT_FUNCTION
static BOOLEAN
NTAPI
WriteConsoleThread(IN PLIST_ENTRY WaitList,
                   IN PCSR_THREAD WaitThread,
                   IN PCSR_API_MESSAGE WaitApiMessage,
                   IN PVOID WaitContext,
                   IN PVOID WaitArgument1,
                   IN PVOID WaitArgument2,
                   IN ULONG WaitFlags)
{
    NTSTATUS Status;

    DPRINT("WriteConsoleThread - WaitContext = 0x%p, WaitArgument1 = 0x%p, WaitArgument2 = 0x%p, WaitFlags = %lu\n", WaitContext, WaitArgument1, WaitArgument2, WaitFlags);

    /*
     * If we are notified of the process termination via a call
     * to CsrNotifyWaitBlock triggered by CsrDestroyProcess or
     * CsrDestroyThread, just return.
     */
    if (WaitFlags & CsrProcessTerminating)
    {
        Status = STATUS_THREAD_IS_TERMINATING;
        goto Quit;
    }

    Status = DoWriteConsole(WaitApiMessage, WaitThread, FALSE);

Quit:
    if (Status != STATUS_PENDING)
    {
        WaitApiMessage->Status = Status;
    }

    return (Status == STATUS_PENDING ? FALSE : TRUE);
}

NTSTATUS NTAPI
ConDrvWriteConsole(IN PCONSOLE Console,
                   IN PTEXTMODE_SCREEN_BUFFER ScreenBuffer,
                   IN BOOLEAN Unicode,
                   IN PVOID StringBuffer,
                   IN ULONG NumCharsToWrite,
                   OUT PULONG NumCharsWritten OPTIONAL);
static NTSTATUS
DoWriteConsole(IN PCSR_API_MESSAGE ApiMessage,
               IN PCSR_THREAD ClientThread,
               IN BOOLEAN CreateWaitBlock OPTIONAL)
{
    NTSTATUS Status;
    PCONSOLE_WRITECONSOLE WriteConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteConsoleRequest;
    PTEXTMODE_SCREEN_BUFFER ScreenBuffer;

    PVOID Buffer;
    ULONG NrCharactersWritten = 0;
    ULONG CharSize = (WriteConsoleRequest->Unicode ? sizeof(WCHAR) : sizeof(CHAR));

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(ClientThread->Process),
                                     WriteConsoleRequest->OutputHandle,
                                     &ScreenBuffer, GENERIC_WRITE, FALSE);
    if (!NT_SUCCESS(Status)) return Status;

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than eighty
     * bytes are written. Otherwise a new buffer is used.
     * The client-side expects that we know this behaviour.
     */
    if (WriteConsoleRequest->UsingStaticBuffer &&
        WriteConsoleRequest->NumBytes <= sizeof(WriteConsoleRequest->StaticBuffer))
    {
        /*
         * Adjust the internal pointer, because its old value points to
         * the static buffer in the original ApiMessage structure.
         */
        // WriteConsoleRequest->Buffer = WriteConsoleRequest->StaticBuffer;
        Buffer = WriteConsoleRequest->StaticBuffer;
    }
    else
    {
        Buffer = WriteConsoleRequest->Buffer;
    }

    DPRINT("Calling ConDrvWriteConsole\n");
    Status = ConDrvWriteConsole(ScreenBuffer->Header.Console,
                                ScreenBuffer,
                                WriteConsoleRequest->Unicode,
                                Buffer,
                                WriteConsoleRequest->NumBytes / CharSize, // NrCharactersToWrite
                                &NrCharactersWritten);
    DPRINT("ConDrvWriteConsole returned (%d ; Status = 0x%08x)\n",
           NrCharactersWritten, Status);

    if (Status == STATUS_PENDING)
    {
        if (CreateWaitBlock)
        {
            PCONSRV_CONSOLE Console = (PCONSRV_CONSOLE)ScreenBuffer->Header.Console;

            if (!CsrCreateWait(&Console->WriteWaitQueue,
                               WriteConsoleThread,
                               ClientThread,
                               ApiMessage,
                               NULL))
            {
                /* Fail */
                Status = STATUS_NO_MEMORY;
                goto Quit;
            }
        }

        /* Wait until we un-pause the console */
        // Status = STATUS_PENDING;
    }
    else
    {
        /* We read all what we wanted. Set the number of bytes written. */
        WriteConsoleRequest->NumBytes = NrCharactersWritten * CharSize;
    }

Quit:
    ConSrvReleaseScreenBuffer(ScreenBuffer, FALSE);
    return Status;
}


/* TEXT OUTPUT APIS ***********************************************************/

NTSTATUS NTAPI
ConDrvReadConsoleOutput(IN PCONSOLE Console,
                        IN PTEXTMODE_SCREEN_BUFFER Buffer,
                        IN BOOLEAN Unicode,
                        OUT PCHAR_INFO CharInfo/*Buffer*/,
                        IN OUT PSMALL_RECT ReadRegion);
CSR_API(SrvReadConsoleOutput)
{
    NTSTATUS Status;
    PCONSOLE_READOUTPUT ReadOutputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ReadOutputRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    ULONG NumCells;
    PCHAR_INFO CharInfo;

    DPRINT("SrvReadConsoleOutput\n");

    NumCells = (ReadOutputRequest->ReadRegion.Right - ReadOutputRequest->ReadRegion.Left + 1) *
               (ReadOutputRequest->ReadRegion.Bottom - ReadOutputRequest->ReadRegion.Top + 1);

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than one
     * cell is read. Otherwise a new buffer is used.
     * The client-side expects that we know this behaviour.
     */
    if (NumCells <= 1)
    {
        /*
         * Adjust the internal pointer, because its old value points to
         * the static buffer in the original ApiMessage structure.
         */
        // ReadOutputRequest->CharInfo = &ReadOutputRequest->StaticBuffer;
        CharInfo = &ReadOutputRequest->StaticBuffer;
    }
    else
    {
        if (!CsrValidateMessageBuffer(ApiMessage,
                                      (PVOID*)&ReadOutputRequest->CharInfo,
                                      NumCells,
                                      sizeof(CHAR_INFO)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        CharInfo = ReadOutputRequest->CharInfo;
    }

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     ReadOutputRequest->OutputHandle,
                                     &Buffer, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvReadConsoleOutput(Buffer->Header.Console,
                                     Buffer,
                                     ReadOutputRequest->Unicode,
                                     CharInfo,
                                     &ReadOutputRequest->ReadRegion);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvWriteConsoleOutput(IN PCONSOLE Console,
                         IN PTEXTMODE_SCREEN_BUFFER Buffer,
                         IN BOOLEAN Unicode,
                         IN PCHAR_INFO CharInfo/*Buffer*/,
                         IN OUT PSMALL_RECT WriteRegion);
CSR_API(SrvWriteConsoleOutput)
{
    NTSTATUS Status;
    PCONSOLE_WRITEOUTPUT WriteOutputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteOutputRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;
    PCSR_PROCESS Process = CsrGetClientThread()->Process;

    ULONG NumCells;
    PCHAR_INFO CharInfo;

    DPRINT("SrvWriteConsoleOutput\n");

    NumCells = (WriteOutputRequest->WriteRegion.Right - WriteOutputRequest->WriteRegion.Left + 1) *
               (WriteOutputRequest->WriteRegion.Bottom - WriteOutputRequest->WriteRegion.Top + 1);

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(Process),
                                     WriteOutputRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /*
     * Validate the message buffer if we do not use a process' heap buffer
     * (CsrAllocateCaptureBuffer succeeded because we haven't allocated
     * a too large (>= 64 kB, size of the CSR heap) data buffer).
     */
    if (!WriteOutputRequest->UseVirtualMemory)
    {
        /*
         * For optimization purposes, Windows (and hence ReactOS, too, for
         * compatibility reasons) uses a static buffer if no more than one
         * cell is written. Otherwise a new buffer is used.
         * The client-side expects that we know this behaviour.
         */
        if (NumCells <= 1)
        {
            /*
             * Adjust the internal pointer, because its old value points to
             * the static buffer in the original ApiMessage structure.
             */
            // WriteOutputRequest->CharInfo = &WriteOutputRequest->StaticBuffer;
            CharInfo = &WriteOutputRequest->StaticBuffer;
        }
        else
        {
            if (!CsrValidateMessageBuffer(ApiMessage,
                                          (PVOID*)&WriteOutputRequest->CharInfo,
                                          NumCells,
                                          sizeof(CHAR_INFO)))
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            CharInfo = WriteOutputRequest->CharInfo;
        }
    }
    else
    {
        /*
         * This was not the case: we use a heap buffer. Retrieve its contents.
         */
        ULONG Size = NumCells * sizeof(CHAR_INFO);

        CharInfo = ConsoleAllocHeap(HEAP_ZERO_MEMORY, Size);
        if (CharInfo == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Quit;
        }

        Status = NtReadVirtualMemory(Process->ProcessHandle,
                                     WriteOutputRequest->CharInfo,
                                     CharInfo,
                                     Size,
                                     NULL);
        if (!NT_SUCCESS(Status))
        {
            ConsoleFreeHeap(CharInfo);
            // Status = STATUS_NO_MEMORY;
            goto Quit;
        }
    }

    Status = ConDrvWriteConsoleOutput(Buffer->Header.Console,
                                      Buffer,
                                      WriteOutputRequest->Unicode,
                                      CharInfo,
                                      &WriteOutputRequest->WriteRegion);

    /* Free the temporary buffer if we used the process' heap buffer */
    if (WriteOutputRequest->UseVirtualMemory && CharInfo)
        ConsoleFreeHeap(CharInfo);

Quit:
    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

CSR_API(SrvWriteConsole)
{
    NTSTATUS Status;
    PCONSOLE_WRITECONSOLE WriteConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteConsoleRequest;

    DPRINT("SrvWriteConsole\n");

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than eighty
     * bytes are written. Otherwise a new buffer is used.
     * The client-side expects that we know this behaviour.
     */
    if (WriteConsoleRequest->UsingStaticBuffer &&
        WriteConsoleRequest->NumBytes <= sizeof(WriteConsoleRequest->StaticBuffer))
    {
        /*
         * Adjust the internal pointer, because its old value points to
         * the static buffer in the original ApiMessage structure.
         */
        // WriteConsoleRequest->Buffer = WriteConsoleRequest->StaticBuffer;
    }
    else
    {
        if (!CsrValidateMessageBuffer(ApiMessage,
                                      (PVOID)&WriteConsoleRequest->Buffer,
                                      WriteConsoleRequest->NumBytes,
                                      sizeof(BYTE)))
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    Status = DoWriteConsole(ApiMessage, CsrGetClientThread(), TRUE);

    if (Status == STATUS_PENDING) *ReplyCode = CsrReplyPending;

    return Status;
}

NTSTATUS NTAPI
ConDrvReadConsoleOutputString(IN PCONSOLE Console,
                              IN PTEXTMODE_SCREEN_BUFFER Buffer,
                              IN CODE_TYPE CodeType,
                              OUT PVOID StringBuffer,
                              IN ULONG NumCodesToRead,
                              IN PCOORD ReadCoord,
                              // OUT PCOORD EndCoord,
                              OUT PULONG NumCodesRead OPTIONAL);
CSR_API(SrvReadConsoleOutputString)
{
    NTSTATUS Status;
    PCONSOLE_READOUTPUTCODE ReadOutputCodeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ReadOutputCodeRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;
    ULONG CodeSize;

    PVOID pCode;

    DPRINT("SrvReadConsoleOutputString\n");

    switch (ReadOutputCodeRequest->CodeType)
    {
        case CODE_ASCII:
            CodeSize = RTL_FIELD_SIZE(CODE_ELEMENT, AsciiChar);
            break;

        case CODE_UNICODE:
            CodeSize = RTL_FIELD_SIZE(CODE_ELEMENT, UnicodeChar);
            break;

        case CODE_ATTRIBUTE:
            CodeSize = RTL_FIELD_SIZE(CODE_ELEMENT, Attribute);
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than eighty
     * bytes are read. Otherwise a new buffer is used.
     * The client-side expects that we know this behaviour.
     */
    if (ReadOutputCodeRequest->NumCodes * CodeSize <= sizeof(ReadOutputCodeRequest->CodeStaticBuffer))
    {
        /*
         * Adjust the internal pointer, because its old value points to
         * the static buffer in the original ApiMessage structure.
         */
        // ReadOutputCodeRequest->pCode = ReadOutputCodeRequest->CodeStaticBuffer;
        pCode = ReadOutputCodeRequest->CodeStaticBuffer;
    }
    else
    {
        if (!CsrValidateMessageBuffer(ApiMessage,
                                      (PVOID*)&ReadOutputCodeRequest->pCode,
                                      ReadOutputCodeRequest->NumCodes,
                                      CodeSize))
        {
            return STATUS_INVALID_PARAMETER;
        }

        pCode = ReadOutputCodeRequest->pCode;
    }

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     ReadOutputCodeRequest->OutputHandle,
                                     &Buffer, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status))
    {
        ReadOutputCodeRequest->NumCodes = 0;
        return Status;
    }

    Status = ConDrvReadConsoleOutputString(Buffer->Header.Console,
                                           Buffer,
                                           ReadOutputCodeRequest->CodeType,
                                           pCode,
                                           ReadOutputCodeRequest->NumCodes,
                                           &ReadOutputCodeRequest->Coord,
                                           // &ReadOutputCodeRequest->EndCoord,
                                           &ReadOutputCodeRequest->NumCodes);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvWriteConsoleOutputString(IN PCONSOLE Console,
                               IN PTEXTMODE_SCREEN_BUFFER Buffer,
                               IN CODE_TYPE CodeType,
                               IN PVOID StringBuffer,
                               IN ULONG NumCodesToWrite,
                               IN PCOORD WriteCoord,
                               // OUT PCOORD EndCoord,
                               OUT PULONG NumCodesWritten OPTIONAL);
CSR_API(SrvWriteConsoleOutputString)
{
    NTSTATUS Status;
    PCONSOLE_WRITEOUTPUTCODE WriteOutputCodeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteOutputCodeRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;
    ULONG CodeSize;

    PVOID pCode;

    DPRINT("SrvWriteConsoleOutputString\n");

    switch (WriteOutputCodeRequest->CodeType)
    {
        case CODE_ASCII:
            CodeSize = RTL_FIELD_SIZE(CODE_ELEMENT, AsciiChar);
            break;

        case CODE_UNICODE:
            CodeSize = RTL_FIELD_SIZE(CODE_ELEMENT, UnicodeChar);
            break;

        case CODE_ATTRIBUTE:
            CodeSize = RTL_FIELD_SIZE(CODE_ELEMENT, Attribute);
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than eighty
     * bytes are written. Otherwise a new buffer is used.
     * The client-side expects that we know this behaviour.
     */
    if (WriteOutputCodeRequest->NumCodes * CodeSize <= sizeof(WriteOutputCodeRequest->CodeStaticBuffer))
    {
        /*
         * Adjust the internal pointer, because its old value points to
         * the static buffer in the original ApiMessage structure.
         */
        // WriteOutputCodeRequest->pCode = WriteOutputCodeRequest->CodeStaticBuffer;
        pCode = WriteOutputCodeRequest->CodeStaticBuffer;
    }
    else
    {
        if (!CsrValidateMessageBuffer(ApiMessage,
                                      (PVOID*)&WriteOutputCodeRequest->pCode,
                                      WriteOutputCodeRequest->NumCodes,
                                      CodeSize))
        {
            return STATUS_INVALID_PARAMETER;
        }

        pCode = WriteOutputCodeRequest->pCode;
    }

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     WriteOutputCodeRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status))
    {
        WriteOutputCodeRequest->NumCodes = 0;
        return Status;
    }

    Status = ConDrvWriteConsoleOutputString(Buffer->Header.Console,
                                            Buffer,
                                            WriteOutputCodeRequest->CodeType,
                                            pCode,
                                            WriteOutputCodeRequest->NumCodes,
                                            &WriteOutputCodeRequest->Coord,
                                            // &WriteOutputCodeRequest->EndCoord,
                                            &WriteOutputCodeRequest->NumCodes);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvFillConsoleOutput(IN PCONSOLE Console,
                        IN PTEXTMODE_SCREEN_BUFFER Buffer,
                        IN CODE_TYPE CodeType,
                        IN CODE_ELEMENT Code,
                        IN ULONG NumCodesToWrite,
                        IN PCOORD WriteCoord,
                        OUT PULONG NumCodesWritten OPTIONAL);
CSR_API(SrvFillConsoleOutput)
{
    NTSTATUS Status;
    PCONSOLE_FILLOUTPUTCODE FillOutputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.FillOutputRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;
    CODE_TYPE CodeType = FillOutputRequest->CodeType;

    DPRINT("SrvFillConsoleOutput\n");

    if ( (CodeType != CODE_ASCII    ) &&
         (CodeType != CODE_UNICODE  ) &&
         (CodeType != CODE_ATTRIBUTE) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     FillOutputRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status))
    {
        FillOutputRequest->NumCodes = 0;
        return Status;
    }

    Status = ConDrvFillConsoleOutput(Buffer->Header.Console,
                                     Buffer,
                                     CodeType,
                                     FillOutputRequest->Code,
                                     FillOutputRequest->NumCodes,
                                     &FillOutputRequest->WriteCoord,
                                     &FillOutputRequest->NumCodes);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvGetConsoleScreenBufferInfo(IN  PCONSOLE Console,
                                 IN  PTEXTMODE_SCREEN_BUFFER Buffer,
                                 OUT PCOORD ScreenBufferSize,
                                 OUT PCOORD CursorPosition,
                                 OUT PCOORD ViewOrigin,
                                 OUT PCOORD ViewSize,
                                 OUT PCOORD MaximumViewSize,
                                 OUT PWORD  Attributes);
CSR_API(SrvGetConsoleScreenBufferInfo)
{
    NTSTATUS Status;
    PCONSOLE_GETSCREENBUFFERINFO ScreenBufferInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ScreenBufferInfoRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT("SrvGetConsoleScreenBufferInfo\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     ScreenBufferInfoRequest->OutputHandle,
                                     &Buffer, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvGetConsoleScreenBufferInfo(Buffer->Header.Console,
                                              Buffer,
                                              &ScreenBufferInfoRequest->ScreenBufferSize,
                                              &ScreenBufferInfoRequest->CursorPosition,
                                              &ScreenBufferInfoRequest->ViewOrigin,
                                              &ScreenBufferInfoRequest->ViewSize,
                                              &ScreenBufferInfoRequest->MaximumViewSize,
                                              &ScreenBufferInfoRequest->Attributes);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleTextAttribute(IN PCONSOLE Console,
                              IN PTEXTMODE_SCREEN_BUFFER Buffer,
                              IN WORD Attributes);
CSR_API(SrvSetConsoleTextAttribute)
{
    NTSTATUS Status;
    PCONSOLE_SETTEXTATTRIB SetTextAttribRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetTextAttribRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT("SrvSetConsoleTextAttribute\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     SetTextAttribRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleTextAttribute(Buffer->Header.Console,
                                           Buffer,
                                           SetTextAttribRequest->Attributes);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleScreenBufferSize(IN PCONSOLE Console,
                                 IN PTEXTMODE_SCREEN_BUFFER Buffer,
                                 IN PCOORD Size);
CSR_API(SrvSetConsoleScreenBufferSize)
{
    NTSTATUS Status;
    PCONSOLE_SETSCREENBUFFERSIZE SetScreenBufferSizeRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetScreenBufferSizeRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT("SrvSetConsoleScreenBufferSize\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     SetScreenBufferSizeRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleScreenBufferSize(Buffer->Header.Console,
                                              Buffer,
                                              &SetScreenBufferSizeRequest->Size);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvScrollConsoleScreenBuffer(IN PCONSOLE Console,
                                IN PTEXTMODE_SCREEN_BUFFER Buffer,
                                IN BOOLEAN Unicode,
                                IN PSMALL_RECT ScrollRectangle,
                                IN BOOLEAN UseClipRectangle,
                                IN PSMALL_RECT ClipRectangle OPTIONAL,
                                IN PCOORD DestinationOrigin,
                                IN CHAR_INFO FillChar);
CSR_API(SrvScrollConsoleScreenBuffer)
{
    NTSTATUS Status;
    PCONSOLE_SCROLLSCREENBUFFER ScrollScreenBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ScrollScreenBufferRequest;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT("SrvScrollConsoleScreenBuffer\n");

    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     ScrollScreenBufferRequest->OutputHandle,
                                     &Buffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvScrollConsoleScreenBuffer(Buffer->Header.Console,
                                             Buffer,
                                             ScrollScreenBufferRequest->Unicode,
                                             &ScrollScreenBufferRequest->ScrollRectangle,
                                             ScrollScreenBufferRequest->UseClipRectangle,
                                             &ScrollScreenBufferRequest->ClipRectangle,
                                             &ScrollScreenBufferRequest->DestinationOrigin,
                                             ScrollScreenBufferRequest->Fill);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvSetConsoleWindowInfo(IN PCONSOLE Console,
                           IN PTEXTMODE_SCREEN_BUFFER Buffer,
                           IN BOOLEAN Absolute,
                           IN PSMALL_RECT WindowRect);
CSR_API(SrvSetConsoleWindowInfo)
{
    NTSTATUS Status;
    PCONSOLE_SETWINDOWINFO SetWindowInfoRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.SetWindowInfoRequest;
    // PCONSOLE_SCREEN_BUFFER Buffer;
    PTEXTMODE_SCREEN_BUFFER Buffer;

    DPRINT1("SrvSetConsoleWindowInfo(0x%08x, %d, {L%d, T%d, R%d, B%d}) called\n",
            SetWindowInfoRequest->OutputHandle, SetWindowInfoRequest->Absolute,
            SetWindowInfoRequest->WindowRect.Left ,
            SetWindowInfoRequest->WindowRect.Top  ,
            SetWindowInfoRequest->WindowRect.Right,
            SetWindowInfoRequest->WindowRect.Bottom);

    // ConSrvGetScreenBuffer
    Status = ConSrvGetTextModeBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                     SetWindowInfoRequest->OutputHandle,
                                     &Buffer, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvSetConsoleWindowInfo(Buffer->Header.Console,
                                        Buffer,
                                        SetWindowInfoRequest->Absolute,
                                        &SetWindowInfoRequest->WindowRect);

    ConSrvReleaseScreenBuffer(Buffer, TRUE);
    return Status;
}

/* EOF */
