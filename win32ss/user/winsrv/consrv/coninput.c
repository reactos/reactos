/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/coninput.c
 * PURPOSE:         Console Input functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define ConSrvGetInputBuffer(ProcessData, Handle, Ptr, Access, LockConsole)     \
    ConSrvGetObject((ProcessData), (Handle), (PCONSOLE_IO_OBJECT*)(Ptr), NULL,  \
                    (Access), (LockConsole), INPUT_BUFFER)
#define ConSrvGetInputBufferAndHandleEntry(ProcessData, Handle, Ptr, Entry, Access, LockConsole)    \
    ConSrvGetObject((ProcessData), (Handle), (PCONSOLE_IO_OBJECT*)(Ptr), (Entry),                   \
                    (Access), (LockConsole), INPUT_BUFFER)
#define ConSrvReleaseInputBuffer(Buff, IsConsoleLocked) \
    ConSrvReleaseObject(&(Buff)->Header, (IsConsoleLocked))


/*
 * From MSDN:
 * "The lpMultiByteStr and lpWideCharStr pointers must not be the same.
 *  If they are the same, the function fails, and GetLastError returns
 *  ERROR_INVALID_PARAMETER."
 */
#define ConsoleInputUnicodeToAnsiChar(Console, dChar, sWChar) \
    ASSERT((ULONG_PTR)dChar != (ULONG_PTR)sWChar); \
    WideCharToMultiByte((Console)->InputCodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleInputAnsiToUnicodeChar(Console, dWChar, sChar) \
    ASSERT((ULONG_PTR)dWChar != (ULONG_PTR)sChar); \
    MultiByteToWideChar((Console)->InputCodePage, 0, (sChar), 1, (dWChar), 1)


typedef struct _GET_INPUT_INFO
{
    PCSR_THREAD           CallingThread;    // The thread which called the input API.
    PVOID                 HandleEntry;      // The handle data associated with the wait thread.
    PCONSOLE_INPUT_BUFFER InputBuffer;      // The input buffer corresponding to the handle.
} GET_INPUT_INFO, *PGET_INPUT_INFO;


/* PRIVATE FUNCTIONS **********************************************************/

static VOID
ConioInputEventToAnsi(PCONSOLE Console, PINPUT_RECORD InputEvent)
{
    if (InputEvent->EventType == KEY_EVENT)
    {
        WCHAR UnicodeChar = InputEvent->Event.KeyEvent.uChar.UnicodeChar;
        InputEvent->Event.KeyEvent.uChar.UnicodeChar = 0;
        ConsoleInputUnicodeToAnsiChar(Console,
                                      &InputEvent->Event.KeyEvent.uChar.AsciiChar,
                                      &UnicodeChar);
    }
}

static VOID
ConioInputEventToUnicode(PCONSOLE Console, PINPUT_RECORD InputEvent)
{
    if (InputEvent->EventType == KEY_EVENT)
    {
        CHAR AsciiChar = InputEvent->Event.KeyEvent.uChar.AsciiChar;
        InputEvent->Event.KeyEvent.uChar.AsciiChar = 0;
        ConsoleInputAnsiToUnicodeChar(Console,
                                      &InputEvent->Event.KeyEvent.uChar.UnicodeChar,
                                      &AsciiChar);
    }
}

static ULONG
PreprocessInput(PCONSRV_CONSOLE Console,
                PINPUT_RECORD InputEvent,
                ULONG NumEventsToWrite)
{
    ULONG NumEvents;

    /*
     * Loop each event, and for each, check for pause or unpause
     * and perform adequate behaviour.
     */
    for (NumEvents = NumEventsToWrite; NumEvents > 0; --NumEvents)
    {
        /* Check for pause or unpause */
        if (InputEvent->EventType == KEY_EVENT && InputEvent->Event.KeyEvent.bKeyDown)
        {
            WORD vk = InputEvent->Event.KeyEvent.wVirtualKeyCode;
            if (!(Console->PauseFlags & PAUSED_FROM_KEYBOARD))
            {
                DWORD cks = InputEvent->Event.KeyEvent.dwControlKeyState;
                if (Console->InputBuffer.Mode & ENABLE_LINE_INPUT &&
                    (vk == VK_PAUSE ||
                    (vk == 'S' && (cks & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) &&
                                 !(cks & (LEFT_ALT_PRESSED  | RIGHT_ALT_PRESSED)))))
                {
                    ConioPause(Console, PAUSED_FROM_KEYBOARD);

                    /* Skip the event */
                    RtlMoveMemory(InputEvent,
                                  InputEvent + 1,
                                  (NumEvents - 1) * sizeof(INPUT_RECORD));
                    --NumEventsToWrite;
                    continue;
                }
            }
            else
            {
                if ((vk < VK_SHIFT || vk > VK_CAPITAL) && vk != VK_LWIN &&
                    vk != VK_RWIN && vk != VK_NUMLOCK && vk != VK_SCROLL)
                {
                    ConioUnpause(Console, PAUSED_FROM_KEYBOARD);

                    /* Skip the event */
                    RtlMoveMemory(InputEvent,
                                  InputEvent + 1,
                                  (NumEvents - 1) * sizeof(INPUT_RECORD));
                    --NumEventsToWrite;
                    continue;
                }
            }
        }

        /* Go to the next event */
        ++InputEvent;
    }

    return NumEventsToWrite;
}

static VOID
PostprocessInput(PCONSRV_CONSOLE Console)
{
    CsrNotifyWait(&Console->ReadWaitQueue,
                  FALSE,
                  NULL,
                  NULL);
    if (!IsListEmpty(&Console->ReadWaitQueue))
    {
        CsrDereferenceWait(&Console->ReadWaitQueue);
    }
}


NTSTATUS NTAPI
ConDrvWriteConsoleInput(IN PCONSOLE Console,
                        IN PCONSOLE_INPUT_BUFFER InputBuffer,
                        IN BOOLEAN AppendToEnd,
                        IN PINPUT_RECORD InputRecord,
                        IN ULONG NumEventsToWrite,
                        OUT PULONG NumEventsWritten OPTIONAL);
static NTSTATUS
ConioAddInputEvents(PCONSRV_CONSOLE Console,
                    PINPUT_RECORD InputRecords, // InputEvent
                    ULONG NumEventsToWrite,
                    PULONG NumEventsWritten,
                    BOOLEAN AppendToEnd)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (NumEventsWritten) *NumEventsWritten = 0;

    NumEventsToWrite = PreprocessInput(Console, InputRecords, NumEventsToWrite);
    if (NumEventsToWrite == 0) return STATUS_SUCCESS;

    // Status = ConDrvAddInputEvents(Console,
                                  // InputRecords,
                                  // NumEventsToWrite,
                                  // NumEventsWritten,
                                  // AppendToEnd);

    Status = ConDrvWriteConsoleInput((PCONSOLE)Console,
                                     &Console->InputBuffer,
                                     AppendToEnd,
                                     InputRecords,
                                     NumEventsToWrite,
                                     NumEventsWritten);

    // if (NT_SUCCESS(Status))
    if (Status == STATUS_SUCCESS) PostprocessInput(Console);

    return Status;
}

/* FIXME: This function can be called by CONDRV, in ConioResizeBuffer() in text.c */
NTSTATUS
ConioProcessInputEvent(PCONSRV_CONSOLE Console,
                       PINPUT_RECORD InputEvent)
{
    ULONG NumEventsWritten;
    return ConioAddInputEvents(Console,
                               InputEvent,
                               1,
                               &NumEventsWritten,
                               TRUE);
}


static NTSTATUS
WaitBeforeReading(IN PGET_INPUT_INFO InputInfo,
                  IN PCSR_API_MESSAGE ApiMessage,
                  IN CSR_WAIT_FUNCTION WaitFunction OPTIONAL,
                  IN BOOLEAN CreateWaitBlock OPTIONAL)
{
    if (CreateWaitBlock)
    {
        PGET_INPUT_INFO CapturedInputInfo;
        PCONSRV_CONSOLE Console = (PCONSRV_CONSOLE)InputInfo->InputBuffer->Header.Console;

        CapturedInputInfo = ConsoleAllocHeap(0, sizeof(GET_INPUT_INFO));
        if (!CapturedInputInfo) return STATUS_NO_MEMORY;

        RtlMoveMemory(CapturedInputInfo, InputInfo, sizeof(GET_INPUT_INFO));

        if (!CsrCreateWait(&Console->ReadWaitQueue,
                           WaitFunction,
                           InputInfo->CallingThread,
                           ApiMessage,
                           CapturedInputInfo))
        {
            ConsoleFreeHeap(CapturedInputInfo);
            return STATUS_NO_MEMORY;
        }
    }

    /* Wait for input */
    return STATUS_PENDING;
}

static NTSTATUS
ReadChars(IN PGET_INPUT_INFO InputInfo,
          IN PCSR_API_MESSAGE ApiMessage,
          IN BOOLEAN CreateWaitBlock OPTIONAL);

// Wait function CSR_WAIT_FUNCTION
static BOOLEAN
NTAPI
ReadCharsThread(IN PLIST_ENTRY WaitList,
                IN PCSR_THREAD WaitThread,
                IN PCSR_API_MESSAGE WaitApiMessage,
                IN PVOID WaitContext,
                IN PVOID WaitArgument1,
                IN PVOID WaitArgument2,
                IN ULONG WaitFlags)
{
    NTSTATUS Status;
    PGET_INPUT_INFO InputInfo = (PGET_INPUT_INFO)WaitContext;

    PVOID InputHandle = WaitArgument2;

    DPRINT("ReadCharsThread - WaitContext = 0x%p, WaitArgument1 = 0x%p, WaitArgument2 = 0x%p, WaitFlags = %lu\n", WaitContext, WaitArgument1, WaitArgument2, WaitFlags);

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

    /*
     * Somebody is closing a handle to this input buffer,
     * by calling ConSrvCloseHandleEntry.
     * See whether we are linked to that handle (ie. we
     * are a waiter for this handle), and if so, return.
     * Otherwise, ignore the call and continue waiting.
     */
    if (InputHandle != NULL)
    {
        Status = (InputHandle == InputInfo->HandleEntry ? STATUS_ALERTED
                                                        : STATUS_PENDING);
        goto Quit;
    }

    /*
     * If we go there, that means we are notified for some new input.
     * The console is therefore already locked.
     */
    Status = ReadChars(InputInfo, WaitApiMessage, FALSE);

Quit:
    if (Status != STATUS_PENDING)
    {
        WaitApiMessage->Status = Status;
        ConsoleFreeHeap(InputInfo);
    }

    return (Status == STATUS_PENDING ? FALSE : TRUE);
}

NTSTATUS NTAPI
ConDrvReadConsole(IN PCONSOLE Console,
                  IN PCONSOLE_INPUT_BUFFER InputBuffer,
                  IN BOOLEAN Unicode,
                  OUT PVOID Buffer,
                  IN OUT PCONSOLE_READCONSOLE_CONTROL ReadControl,
                  IN PVOID Parameter OPTIONAL,
                  IN ULONG NumCharsToRead,
                  OUT PULONG NumCharsRead OPTIONAL);
static NTSTATUS
ReadChars(IN PGET_INPUT_INFO InputInfo,
          IN PCSR_API_MESSAGE ApiMessage,
          IN BOOLEAN CreateWaitBlock OPTIONAL)
{
    NTSTATUS Status;
    PCONSOLE_READCONSOLE ReadConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ReadConsoleRequest;
    PCONSOLE_INPUT_BUFFER InputBuffer = InputInfo->InputBuffer;
    CONSOLE_READCONSOLE_CONTROL ReadControl;

    UNICODE_STRING ExeName;

    PVOID Buffer;
    ULONG NrCharactersRead = 0;
    ULONG CharSize = (ReadConsoleRequest->Unicode ? sizeof(WCHAR) : sizeof(CHAR));

    /* Retrieve the executable name, if needed */
    if (ReadConsoleRequest->InitialNumBytes == 0 &&
        ReadConsoleRequest->ExeLength <= sizeof(ReadConsoleRequest->StaticBuffer))
    {
        ExeName.Length = ExeName.MaximumLength = ReadConsoleRequest->ExeLength;
        ExeName.Buffer = (PWCHAR)ReadConsoleRequest->StaticBuffer;
    }
    else
    {
        ExeName.Length = ExeName.MaximumLength = 0;
        ExeName.Buffer = NULL;
    }

    /* Build the ReadControl structure */
    ReadControl.nLength           = sizeof(CONSOLE_READCONSOLE_CONTROL);
    ReadControl.nInitialChars     = ReadConsoleRequest->InitialNumBytes / CharSize;
    ReadControl.dwCtrlWakeupMask  = ReadConsoleRequest->CtrlWakeupMask;
    ReadControl.dwControlKeyState = ReadConsoleRequest->ControlKeyState;

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than eighty
     * bytes are read. Otherwise a new buffer is used.
     * The client-side expects that we know this behaviour.
     */
    if (ReadConsoleRequest->CaptureBufferSize <= sizeof(ReadConsoleRequest->StaticBuffer))
    {
        /*
         * Adjust the internal pointer, because its old value points to
         * the static buffer in the original ApiMessage structure.
         */
        // ReadConsoleRequest->Buffer = ReadConsoleRequest->StaticBuffer;
        Buffer = ReadConsoleRequest->StaticBuffer;
    }
    else
    {
        Buffer = ReadConsoleRequest->Buffer;
    }

    DPRINT("Calling ConDrvReadConsole(%wZ)\n", &ExeName);
    Status = ConDrvReadConsole(InputBuffer->Header.Console,
                               InputBuffer,
                               ReadConsoleRequest->Unicode,
                               Buffer,
                               &ReadControl,
                               &ExeName,
                               ReadConsoleRequest->NumBytes / CharSize, // NrCharactersToRead
                               &NrCharactersRead);
    DPRINT("ConDrvReadConsole returned (%d ; Status = 0x%08x)\n",
           NrCharactersRead, Status);

    // ReadConsoleRequest->ControlKeyState = ReadControl.dwControlKeyState;

    if (Status == STATUS_PENDING)
    {
        /* We haven't completed a read, so start a wait */
        return WaitBeforeReading(InputInfo,
                                 ApiMessage,
                                 ReadCharsThread,
                                 CreateWaitBlock);
    }
    else
    {
        /*
         * We read all what we wanted. Set the number of bytes read and
         * return the error code we were given.
         */
        ReadConsoleRequest->NumBytes = NrCharactersRead * CharSize;
        ReadConsoleRequest->ControlKeyState = ReadControl.dwControlKeyState;

        return Status;
        // return STATUS_SUCCESS;
    }
}

static NTSTATUS
ReadInputBuffer(IN PGET_INPUT_INFO InputInfo,
                IN PCSR_API_MESSAGE ApiMessage,
                IN BOOLEAN CreateWaitBlock OPTIONAL);

// Wait function CSR_WAIT_FUNCTION
static BOOLEAN
NTAPI
ReadInputBufferThread(IN PLIST_ENTRY WaitList,
                      IN PCSR_THREAD WaitThread,
                      IN PCSR_API_MESSAGE WaitApiMessage,
                      IN PVOID WaitContext,
                      IN PVOID WaitArgument1,
                      IN PVOID WaitArgument2,
                      IN ULONG WaitFlags)
{
    NTSTATUS Status;
    PGET_INPUT_INFO InputInfo = (PGET_INPUT_INFO)WaitContext;

    PVOID InputHandle = WaitArgument2;

    DPRINT("ReadInputBufferThread - WaitContext = 0x%p, WaitArgument1 = 0x%p, WaitArgument2 = 0x%p, WaitFlags = %lu\n", WaitContext, WaitArgument1, WaitArgument2, WaitFlags);

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

    /*
     * Somebody is closing a handle to this input buffer,
     * by calling ConSrvCloseHandleEntry.
     * See whether we are linked to that handle (ie. we
     * are a waiter for this handle), and if so, return.
     * Otherwise, ignore the call and continue waiting.
     */
    if (InputHandle != NULL)
    {
        Status = (InputHandle == InputInfo->HandleEntry ? STATUS_ALERTED
                                                        : STATUS_PENDING);
        goto Quit;
    }

    /*
     * If we go there, that means we are notified for some new input.
     * The console is therefore already locked.
     */
    Status = ReadInputBuffer(InputInfo, WaitApiMessage, FALSE);

Quit:
    if (Status != STATUS_PENDING)
    {
        WaitApiMessage->Status = Status;
        ConsoleFreeHeap(InputInfo);
    }

    return (Status == STATUS_PENDING ? FALSE : TRUE);
}

NTSTATUS NTAPI
ConDrvGetConsoleInput(IN PCONSOLE Console,
                      IN PCONSOLE_INPUT_BUFFER InputBuffer,
                      IN BOOLEAN KeepEvents,
                      IN BOOLEAN WaitForMoreEvents,
                      OUT PINPUT_RECORD InputRecord,
                      IN ULONG NumEventsToRead,
                      OUT PULONG NumEventsRead OPTIONAL);
static NTSTATUS
ReadInputBuffer(IN PGET_INPUT_INFO InputInfo,
                IN PCSR_API_MESSAGE ApiMessage,
                IN BOOLEAN CreateWaitBlock OPTIONAL)
{
    NTSTATUS Status;
    PCONSOLE_GETINPUT GetInputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetInputRequest;
    PCONSOLE_INPUT_BUFFER InputBuffer = InputInfo->InputBuffer;
    ULONG NumEventsRead;

    PINPUT_RECORD InputRecord;

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than five
     * input records are read. Otherwise a new buffer is used.
     * The client-side expects that we know this behaviour.
     */
    if (GetInputRequest->NumRecords <= sizeof(GetInputRequest->RecordStaticBuffer)/sizeof(INPUT_RECORD))
    {
        /*
         * Adjust the internal pointer, because its old value points to
         * the static buffer in the original ApiMessage structure.
         */
        // GetInputRequest->RecordBufPtr = GetInputRequest->RecordStaticBuffer;
        InputRecord = GetInputRequest->RecordStaticBuffer;
    }
    else
    {
        InputRecord = GetInputRequest->RecordBufPtr;
    }

    NumEventsRead = 0;
    Status = ConDrvGetConsoleInput(InputBuffer->Header.Console,
                                   InputBuffer,
                                   (GetInputRequest->Flags & CONSOLE_READ_KEEPEVENT) != 0,
                                   (GetInputRequest->Flags & CONSOLE_READ_CONTINUE ) == 0,
                                   InputRecord,
                                   GetInputRequest->NumRecords,
                                   &NumEventsRead);

    if (Status == STATUS_PENDING)
    {
        /* We haven't completed a read, so start a wait */
        return WaitBeforeReading(InputInfo,
                                 ApiMessage,
                                 ReadInputBufferThread,
                                 CreateWaitBlock);
    }
    else
    {
        /*
         * We read all what we wanted. Set the number of events read and
         * return the error code we were given.
         */
        GetInputRequest->NumRecords = NumEventsRead;

        if (NT_SUCCESS(Status))
        {
            /* Now translate everything to ANSI */
            if (!GetInputRequest->Unicode)
            {
                for (; NumEventsRead > 0; --NumEventsRead)
                {
                    ConioInputEventToAnsi(InputBuffer->Header.Console, --InputRecord);
                }
            }
        }

        return Status;
        // return STATUS_SUCCESS;
    }
}


/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvReadConsole)
{
    NTSTATUS Status;
    PCONSOLE_READCONSOLE ReadConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ReadConsoleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PVOID HandleEntry;
    PCONSOLE_INPUT_BUFFER InputBuffer;
    GET_INPUT_INFO InputInfo;

    DPRINT("SrvReadConsole\n");

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than eighty
     * bytes are read. Otherwise a new buffer is used.
     * The client-side expects that we know this behaviour.
     */
    if (ReadConsoleRequest->CaptureBufferSize <= sizeof(ReadConsoleRequest->StaticBuffer))
    {
        /*
         * Adjust the internal pointer, because its old value points to
         * the static buffer in the original ApiMessage structure.
         */
        // ReadConsoleRequest->Buffer = ReadConsoleRequest->StaticBuffer;
    }
    else
    {
        if (!CsrValidateMessageBuffer(ApiMessage,
                                      (PVOID*)&ReadConsoleRequest->Buffer,
                                      ReadConsoleRequest->CaptureBufferSize,
                                      sizeof(BYTE)))
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    if (ReadConsoleRequest->InitialNumBytes > ReadConsoleRequest->NumBytes)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetInputBufferAndHandleEntry(ProcessData, ReadConsoleRequest->InputHandle, &InputBuffer, &HandleEntry, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    InputInfo.CallingThread = CsrGetClientThread();
    InputInfo.HandleEntry   = HandleEntry;
    InputInfo.InputBuffer   = InputBuffer;

    Status = ReadChars(&InputInfo, ApiMessage, TRUE);

    ConSrvReleaseInputBuffer(InputBuffer, TRUE);

    if (Status == STATUS_PENDING) *ReplyCode = CsrReplyPending;

    return Status;
}

CSR_API(SrvGetConsoleInput)
{
    NTSTATUS Status;
    PCONSOLE_GETINPUT GetInputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetInputRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PVOID HandleEntry;
    PCONSOLE_INPUT_BUFFER InputBuffer;
    GET_INPUT_INFO InputInfo;

    DPRINT("SrvGetConsoleInput\n");

    if (GetInputRequest->Flags & ~(CONSOLE_READ_KEEPEVENT | CONSOLE_READ_CONTINUE))
        return STATUS_INVALID_PARAMETER;

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than five
     * input records are read. Otherwise a new buffer is used.
     * The client-side expects that we know this behaviour.
     */
    if (GetInputRequest->NumRecords <= sizeof(GetInputRequest->RecordStaticBuffer)/sizeof(INPUT_RECORD))
    {
        /*
         * Adjust the internal pointer, because its old value points to
         * the static buffer in the original ApiMessage structure.
         */
        // GetInputRequest->RecordBufPtr = GetInputRequest->RecordStaticBuffer;
    }
    else
    {
        if (!CsrValidateMessageBuffer(ApiMessage,
                                      (PVOID*)&GetInputRequest->RecordBufPtr,
                                      GetInputRequest->NumRecords,
                                      sizeof(INPUT_RECORD)))
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    Status = ConSrvGetInputBufferAndHandleEntry(ProcessData, GetInputRequest->InputHandle, &InputBuffer, &HandleEntry, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    InputInfo.CallingThread = CsrGetClientThread();
    InputInfo.HandleEntry   = HandleEntry;
    InputInfo.InputBuffer   = InputBuffer;

    Status = ReadInputBuffer(&InputInfo, ApiMessage, TRUE);

    ConSrvReleaseInputBuffer(InputBuffer, TRUE);

    if (Status == STATUS_PENDING) *ReplyCode = CsrReplyPending;

    return Status;
}

#if 0
NTSTATUS NTAPI
ConDrvWriteConsoleInput(IN PCONSOLE Console,
                        IN PCONSOLE_INPUT_BUFFER InputBuffer,
                        IN BOOLEAN AppendToEnd,
                        IN PINPUT_RECORD InputRecord,
                        IN ULONG NumEventsToWrite,
                        OUT PULONG NumEventsWritten OPTIONAL);
#endif
CSR_API(SrvWriteConsoleInput)
{
    NTSTATUS Status;
    PCONSOLE_WRITEINPUT WriteInputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteInputRequest;
    PCONSOLE_INPUT_BUFFER InputBuffer;
    ULONG NumEventsWritten;

    PINPUT_RECORD InputRecord;

    DPRINT("SrvWriteConsoleInput\n");

    /*
     * For optimization purposes, Windows (and hence ReactOS, too, for
     * compatibility reasons) uses a static buffer if no more than five
     * input records are written. Otherwise a new buffer is used.
     * The client-side expects that we know this behaviour.
     */
    if (WriteInputRequest->NumRecords <= sizeof(WriteInputRequest->RecordStaticBuffer)/sizeof(INPUT_RECORD))
    {
        /*
         * Adjust the internal pointer, because its old value points to
         * the static buffer in the original ApiMessage structure.
         */
        // WriteInputRequest->RecordBufPtr = WriteInputRequest->RecordStaticBuffer;
        InputRecord = WriteInputRequest->RecordStaticBuffer;
    }
    else
    {
        if (!CsrValidateMessageBuffer(ApiMessage,
                                      (PVOID*)&WriteInputRequest->RecordBufPtr,
                                      WriteInputRequest->NumRecords,
                                      sizeof(INPUT_RECORD)))
        {
            return STATUS_INVALID_PARAMETER;
        }

        InputRecord = WriteInputRequest->RecordBufPtr;
    }

    Status = ConSrvGetInputBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                  WriteInputRequest->InputHandle,
                                  &InputBuffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status))
    {
        WriteInputRequest->NumRecords = 0;
        return Status;
    }

    /* First translate everything to UNICODE */
    if (!WriteInputRequest->Unicode)
    {
        ULONG i;
        for (i = 0; i < WriteInputRequest->NumRecords; ++i)
        {
            ConioInputEventToUnicode(InputBuffer->Header.Console, &InputRecord[i]);
        }
    }

    /* Now, add the events */
    NumEventsWritten = 0;
    Status = ConioAddInputEvents((PCONSRV_CONSOLE)InputBuffer->Header.Console,
                                 // InputBuffer,
                                 InputRecord,
                                 WriteInputRequest->NumRecords,
                                 &NumEventsWritten,
                                 WriteInputRequest->AppendToEnd);

    // Status = ConDrvWriteConsoleInput(InputBuffer->Header.Console,
                                     // InputBuffer,
                                     // WriteInputRequest->AppendToEnd,
                                     // InputRecord,
                                     // WriteInputRequest->NumRecords,
                                     // &NumEventsWritten);

    WriteInputRequest->NumRecords = NumEventsWritten;

    ConSrvReleaseInputBuffer(InputBuffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvFlushConsoleInputBuffer(IN PCONSOLE Console,
                              IN PCONSOLE_INPUT_BUFFER InputBuffer);
CSR_API(SrvFlushConsoleInputBuffer)
{
    NTSTATUS Status;
    PCONSOLE_FLUSHINPUTBUFFER FlushInputBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.FlushInputBufferRequest;
    PCONSOLE_INPUT_BUFFER InputBuffer;

    DPRINT("SrvFlushConsoleInputBuffer\n");

    Status = ConSrvGetInputBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                  FlushInputBufferRequest->InputHandle,
                                  &InputBuffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvFlushConsoleInputBuffer(InputBuffer->Header.Console,
                                           InputBuffer);

    ConSrvReleaseInputBuffer(InputBuffer, TRUE);
    return Status;
}

NTSTATUS NTAPI
ConDrvGetConsoleNumberOfInputEvents(IN PCONSOLE Console,
                                    IN PCONSOLE_INPUT_BUFFER InputBuffer,
                                    OUT PULONG NumberOfEvents);
CSR_API(SrvGetConsoleNumberOfInputEvents)
{
    NTSTATUS Status;
    PCONSOLE_GETNUMINPUTEVENTS GetNumInputEventsRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetNumInputEventsRequest;
    PCONSOLE_INPUT_BUFFER InputBuffer;

    DPRINT("SrvGetConsoleNumberOfInputEvents\n");

    Status = ConSrvGetInputBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                  GetNumInputEventsRequest->InputHandle,
                                  &InputBuffer, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ConDrvGetConsoleNumberOfInputEvents(InputBuffer->Header.Console,
                                                 InputBuffer,
                                                 &GetNumInputEventsRequest->NumberOfEvents);

    ConSrvReleaseInputBuffer(InputBuffer, TRUE);
    return Status;
}

/* EOF */
