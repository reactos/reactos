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
#include "include/conio.h"
#include "include/conio2.h"
#include "handle.h"
#include "lineinput.h"

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


typedef struct _GET_INPUT_INFO
{
    PCSR_THREAD           CallingThread;    // The thread which called the input API.
    PVOID                 HandleEntry;      // The handle data associated with the wait thread.
    PCONSOLE_INPUT_BUFFER InputBuffer;      // The input buffer corresponding to the handle.
} GET_INPUT_INFO, *PGET_INPUT_INFO;


/* PRIVATE FUNCTIONS **********************************************************/

static NTSTATUS
WaitBeforeReading(IN PGET_INPUT_INFO InputInfo,
                  IN PCSR_API_MESSAGE ApiMessage,
                  IN CSR_WAIT_FUNCTION WaitFunction OPTIONAL,
                  IN BOOL CreateWaitBlock OPTIONAL)
{
    if (CreateWaitBlock)
    {
        PGET_INPUT_INFO CapturedInputInfo;

        CapturedInputInfo = ConsoleAllocHeap(0, sizeof(GET_INPUT_INFO));
        if (!CapturedInputInfo) return STATUS_NO_MEMORY;

        RtlMoveMemory(CapturedInputInfo, InputInfo, sizeof(GET_INPUT_INFO));

        if (!CsrCreateWait(&InputInfo->InputBuffer->ReadWaitQueue,
                           WaitFunction,
                           InputInfo->CallingThread,
                           ApiMessage,
                           CapturedInputInfo,
                           NULL))
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
          IN BOOL CreateWaitBlock OPTIONAL);

// Wait function CSR_WAIT_FUNCTION
static BOOLEAN
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
    Status = ReadChars(InputInfo,
                       WaitApiMessage,
                       FALSE);

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
                  IN ULONG NumCharsToRead,
                  OUT PULONG NumCharsRead OPTIONAL);
static NTSTATUS
ReadChars(IN PGET_INPUT_INFO InputInfo,
          IN PCSR_API_MESSAGE ApiMessage,
          IN BOOL CreateWaitBlock OPTIONAL)
{
    NTSTATUS Status;
    PCONSOLE_READCONSOLE ReadConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ReadConsoleRequest;
    PCONSOLE_INPUT_BUFFER InputBuffer = InputInfo->InputBuffer;
    CONSOLE_READCONSOLE_CONTROL ReadControl;

    ReadControl.nLength           = sizeof(CONSOLE_READCONSOLE_CONTROL);
    ReadControl.nInitialChars     = ReadConsoleRequest->NrCharactersRead;
    ReadControl.dwCtrlWakeupMask  = ReadConsoleRequest->CtrlWakeupMask;
    ReadControl.dwControlKeyState = ReadConsoleRequest->ControlKeyState;

    Status = ConDrvReadConsole(InputBuffer->Header.Console,
                               InputBuffer,
                               ReadConsoleRequest->Unicode,
                               ReadConsoleRequest->Buffer,
                               &ReadControl,
                               ReadConsoleRequest->NrCharactersToRead,
                               &ReadConsoleRequest->NrCharactersRead);

    ReadConsoleRequest->ControlKeyState = ReadControl.dwControlKeyState;

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
        /* We read all what we wanted, we return the error code we were given */
        return Status;
        // return STATUS_SUCCESS;
    }
}

static NTSTATUS
ReadInputBuffer(IN PGET_INPUT_INFO InputInfo,
                IN BOOL Wait,
                IN PCSR_API_MESSAGE ApiMessage,
                IN BOOL CreateWaitBlock OPTIONAL);

// Wait function CSR_WAIT_FUNCTION
static BOOLEAN
ReadInputBufferThread(IN PLIST_ENTRY WaitList,
                      IN PCSR_THREAD WaitThread,
                      IN PCSR_API_MESSAGE WaitApiMessage,
                      IN PVOID WaitContext,
                      IN PVOID WaitArgument1,
                      IN PVOID WaitArgument2,
                      IN ULONG WaitFlags)
{
    NTSTATUS Status;
    PCONSOLE_GETINPUT GetInputRequest = &((PCONSOLE_API_MESSAGE)WaitApiMessage)->Data.GetInputRequest;
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
    Status = ReadInputBuffer(InputInfo,
                             GetInputRequest->bRead,
                             WaitApiMessage,
                             FALSE);

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
                      IN BOOLEAN WaitForMoreEvents,
                      IN BOOLEAN Unicode,
                      OUT PINPUT_RECORD InputRecord,
                      IN ULONG NumEventsToRead,
                      OUT PULONG NumEventsRead);
static NTSTATUS
ReadInputBuffer(IN PGET_INPUT_INFO InputInfo,
                IN BOOL Wait,   // TRUE --> Read ; FALSE --> Peek
                IN PCSR_API_MESSAGE ApiMessage,
                IN BOOL CreateWaitBlock OPTIONAL)
{
    NTSTATUS Status;
    PCONSOLE_GETINPUT GetInputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetInputRequest;
    PCONSOLE_INPUT_BUFFER InputBuffer = InputInfo->InputBuffer;

    // GetInputRequest->InputsRead = 0;

    Status = ConDrvGetConsoleInput(InputBuffer->Header.Console,
                                   InputBuffer,
                                   Wait,
                                   GetInputRequest->Unicode,
                                   GetInputRequest->InputRecord,
                                   GetInputRequest->Length,
                                   &GetInputRequest->InputsRead);

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
        /* We read all what we wanted, we return the error code we were given */
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

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&ReadConsoleRequest->Buffer,
                                  ReadConsoleRequest->BufferSize,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (ReadConsoleRequest->NrCharactersRead > ReadConsoleRequest->NrCharactersToRead)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetInputBufferAndHandleEntry(ProcessData, ReadConsoleRequest->InputHandle, &InputBuffer, &HandleEntry, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    // This member is set by the caller (IntReadConsole in kernel32)
    // ReadConsoleRequest->NrCharactersRead = 0;

    InputInfo.CallingThread = CsrGetClientThread();
    InputInfo.HandleEntry   = HandleEntry;
    InputInfo.InputBuffer   = InputBuffer;

    Status = ReadChars(&InputInfo,
                       ApiMessage,
                       TRUE);

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

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&GetInputRequest->InputRecord,
                                  GetInputRequest->Length,
                                  sizeof(INPUT_RECORD)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetInputBufferAndHandleEntry(ProcessData, GetInputRequest->InputHandle, &InputBuffer, &HandleEntry, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    GetInputRequest->InputsRead = 0;

    InputInfo.CallingThread = CsrGetClientThread();
    InputInfo.HandleEntry   = HandleEntry;
    InputInfo.InputBuffer   = InputBuffer;

    Status = ReadInputBuffer(&InputInfo,
                             GetInputRequest->bRead,
                             ApiMessage,
                             TRUE);

    ConSrvReleaseInputBuffer(InputBuffer, TRUE);

    if (Status == STATUS_PENDING) *ReplyCode = CsrReplyPending;

    return Status;
}

NTSTATUS NTAPI
ConDrvWriteConsoleInput(IN PCONSOLE Console,
                        IN PCONSOLE_INPUT_BUFFER InputBuffer,
                        IN BOOLEAN Unicode,
                        IN PINPUT_RECORD InputRecord,
                        IN ULONG NumEventsToWrite,
                        OUT PULONG NumEventsWritten OPTIONAL);
CSR_API(SrvWriteConsoleInput)
{
    NTSTATUS Status;
    PCONSOLE_WRITEINPUT WriteInputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteInputRequest;
    PCONSOLE_INPUT_BUFFER InputBuffer;
    ULONG NumEventsWritten;

    DPRINT("SrvWriteConsoleInput\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&WriteInputRequest->InputRecord,
                                  WriteInputRequest->Length,
                                  sizeof(INPUT_RECORD)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetInputBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                  WriteInputRequest->InputHandle,
                                  &InputBuffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    NumEventsWritten = 0;
    Status = ConDrvWriteConsoleInput(InputBuffer->Header.Console,
                                     InputBuffer,
                                     WriteInputRequest->Unicode,
                                     WriteInputRequest->InputRecord,
                                     WriteInputRequest->Length,
                                     &NumEventsWritten);
    WriteInputRequest->Length = NumEventsWritten;

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
                                    OUT PULONG NumEvents);
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
                                                 &GetNumInputEventsRequest->NumInputEvents);

    ConSrvReleaseInputBuffer(InputBuffer, TRUE);
    return Status;
}

/* EOF */
