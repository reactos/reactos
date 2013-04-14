/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/coninput.c
 * PURPOSE:         Console Input functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "conio.h"
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


#define ConsoleInputUnicodeCharToAnsiChar(Console, dChar, sWChar) \
    WideCharToMultiByte((Console)->CodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleInputAnsiCharToUnicodeChar(Console, dWChar, sChar) \
    MultiByteToWideChar((Console)->CodePage, 0, (sChar), 1, (dWChar), 1)

typedef struct ConsoleInput_t
{
    LIST_ENTRY ListEntry;
    INPUT_RECORD InputEvent;
} ConsoleInput;

typedef struct _GET_INPUT_INFO
{
    PCSR_THREAD           CallingThread;    // The thread which called the input API.
    PVOID                 HandleEntry;      // The handle data associated with the wait thread.
    PCONSOLE_INPUT_BUFFER InputBuffer;      // The input buffer corresponding to the handle.
} GET_INPUT_INFO, *PGET_INPUT_INFO;


/* PRIVATE FUNCTIONS **********************************************************/

static VOID FASTCALL
ConioInputEventToAnsi(PCONSOLE Console, PINPUT_RECORD InputEvent)
{
    if (InputEvent->EventType == KEY_EVENT)
    {
        WCHAR UnicodeChar = InputEvent->Event.KeyEvent.uChar.UnicodeChar;
        InputEvent->Event.KeyEvent.uChar.UnicodeChar = 0;
        ConsoleInputUnicodeCharToAnsiChar(Console,
                                          &InputEvent->Event.KeyEvent.uChar.AsciiChar,
                                          &UnicodeChar);
    }
}

NTSTATUS FASTCALL
ConioProcessInputEvent(PCONSOLE Console,
                       PINPUT_RECORD InputEvent)
{
    ConsoleInput *ConInRec;

    /* Check for pause or unpause */
    if (InputEvent->EventType == KEY_EVENT && InputEvent->Event.KeyEvent.bKeyDown)
    {
        WORD vk = InputEvent->Event.KeyEvent.wVirtualKeyCode;
        if (!(Console->PauseFlags & PAUSED_FROM_KEYBOARD))
        {
            DWORD cks = InputEvent->Event.KeyEvent.dwControlKeyState;
            if (Console->InputBuffer.Mode & ENABLE_LINE_INPUT &&
                (vk == VK_PAUSE || (vk == 'S' &&
                                    (cks & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) &&
                                    !(cks & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)))))
            {
                ConioPause(Console, PAUSED_FROM_KEYBOARD);
                return STATUS_SUCCESS;
            }
        }
        else
        {
            if ((vk < VK_SHIFT || vk > VK_CAPITAL) && vk != VK_LWIN &&
                vk != VK_RWIN && vk != VK_NUMLOCK && vk != VK_SCROLL)
            {
                ConioUnpause(Console, PAUSED_FROM_KEYBOARD);
                return STATUS_SUCCESS;
            }
        }
    }

    /* add event to the queue */
    ConInRec = RtlAllocateHeap(ConSrvHeap, 0, sizeof(ConsoleInput));
    if (ConInRec == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;
    ConInRec->InputEvent = *InputEvent;
    InsertTailList(&Console->InputBuffer.InputEvents, &ConInRec->ListEntry);

    SetEvent(Console->InputBuffer.ActiveEvent);
    CsrNotifyWait(&Console->InputBuffer.ReadWaitQueue,
                  WaitAny,
                  NULL,
                  NULL);
    if (!IsListEmpty(&Console->InputBuffer.ReadWaitQueue))
    {
        CsrDereferenceWait(&Console->InputBuffer.ReadWaitQueue);
    }

    return STATUS_SUCCESS;
}

VOID FASTCALL
PurgeInputBuffer(PCONSOLE Console)
{
    PLIST_ENTRY CurrentEntry;
    ConsoleInput* Event;

    while (!IsListEmpty(&Console->InputBuffer.InputEvents))
    {
        CurrentEntry = RemoveHeadList(&Console->InputBuffer.InputEvents);
        Event = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
        RtlFreeHeap(ConSrvHeap, 0, Event);
    }

    CloseHandle(Console->InputBuffer.ActiveEvent);
}

static DWORD FASTCALL
ConioGetShiftState(PBYTE KeyState, LPARAM lParam)
{
    DWORD ssOut = 0;

    if (KeyState[VK_CAPITAL] & 0x01)
        ssOut |= CAPSLOCK_ON;

    if (KeyState[VK_NUMLOCK] & 0x01)
        ssOut |= NUMLOCK_ON;

    if (KeyState[VK_SCROLL] & 0x01)
        ssOut |= SCROLLLOCK_ON;

    if (KeyState[VK_SHIFT] & 0x80)
        ssOut |= SHIFT_PRESSED;

    if (KeyState[VK_LCONTROL] & 0x80)
        ssOut |= LEFT_CTRL_PRESSED;
    if (KeyState[VK_RCONTROL] & 0x80)
        ssOut |= RIGHT_CTRL_PRESSED;

    if (KeyState[VK_LMENU] & 0x80)
        ssOut |= LEFT_ALT_PRESSED;
    if (KeyState[VK_RMENU] & 0x80)
        ssOut |= RIGHT_ALT_PRESSED;

    /* See WM_CHAR MSDN documentation for instance */
    if (lParam & 0x01000000)
        ssOut |= ENHANCED_KEY;

    return ssOut;
}

VOID WINAPI
ConioProcessKey(PCONSOLE Console, MSG* msg)
{
    static BYTE KeyState[256] = { 0 };
    /* MSDN mentions that you should use the last virtual key code received
     * when putting a virtual key identity to a WM_CHAR message since multiple
     * or translated keys may be involved. */
    static UINT LastVirtualKey = 0;
    DWORD ShiftState;
    UINT RepeatCount;
    WCHAR UnicodeChar;
    UINT VirtualKeyCode;
    UINT VirtualScanCode;
    BOOL Down = FALSE;
    INPUT_RECORD er;
    BOOLEAN Fake;          // synthesized, not a real event
    BOOLEAN NotChar;       // message should not be used to return a character

    if (NULL == Console)
    {
        DPRINT1("No Active Console!\n");
        return;
    }

    RepeatCount = 1;
    VirtualScanCode = (msg->lParam >> 16) & 0xff;
    Down = msg->message == WM_KEYDOWN || msg->message == WM_CHAR ||
           msg->message == WM_SYSKEYDOWN || msg->message == WM_SYSCHAR;

    GetKeyboardState(KeyState);
    ShiftState = ConioGetShiftState(KeyState, msg->lParam);

    if (msg->message == WM_CHAR || msg->message == WM_SYSCHAR)
    {
        VirtualKeyCode = LastVirtualKey;
        UnicodeChar = msg->wParam;
    }
    else
    {
        WCHAR Chars[2];
        INT RetChars = 0;

        VirtualKeyCode = msg->wParam;
        RetChars = ToUnicodeEx(VirtualKeyCode,
                               VirtualScanCode,
                               KeyState,
                               Chars,
                               2,
                               0,
                               0);
        UnicodeChar = (1 == RetChars ? Chars[0] : 0);
    }

    er.EventType = KEY_EVENT;
    er.Event.KeyEvent.bKeyDown = Down;
    er.Event.KeyEvent.wRepeatCount = RepeatCount;
    er.Event.KeyEvent.uChar.UnicodeChar = UnicodeChar;
    er.Event.KeyEvent.dwControlKeyState = ShiftState;
    er.Event.KeyEvent.wVirtualKeyCode = VirtualKeyCode;
    er.Event.KeyEvent.wVirtualScanCode = VirtualScanCode;

    if (ConioProcessKeyCallback(Console,
                                msg,
                                KeyState[VK_MENU],
                                ShiftState,
                                VirtualKeyCode,
                                Down))
    {
        return;
    }

    Fake = UnicodeChar &&
            (msg->message != WM_CHAR && msg->message != WM_SYSCHAR &&
             msg->message != WM_KEYUP && msg->message != WM_SYSKEYUP);
    NotChar = (msg->message != WM_CHAR && msg->message != WM_SYSCHAR);
    if (NotChar) LastVirtualKey = msg->wParam;

    DPRINT("CONSRV: %s %s %s %s %02x %02x '%lc' %04x\n",
           Down ? "down" : "up  ",
           (msg->message == WM_CHAR || msg->message == WM_SYSCHAR) ?
           "char" : "key ",
           Fake ? "fake" : "real",
           NotChar ? "notc" : "char",
           VirtualScanCode,
           VirtualKeyCode,
           (UnicodeChar >= L' ') ? UnicodeChar : L'.',
           ShiftState);

    if (Fake) return;

    /* process Ctrl-C and Ctrl-Break */
    if (Console->InputBuffer.Mode & ENABLE_PROCESSED_INPUT &&
            er.Event.KeyEvent.bKeyDown &&
            ((er.Event.KeyEvent.wVirtualKeyCode == VK_PAUSE) ||
             (er.Event.KeyEvent.wVirtualKeyCode == 'C')) &&
            (er.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED) || KeyState[VK_CONTROL] & 0x80))
    {
        DPRINT1("Console_Api Ctrl-C\n");
        ConSrvConsoleProcessCtrlEvent(Console, 0, CTRL_C_EVENT);

        if (Console->LineBuffer && !Console->LineComplete)
        {
            /* Line input is in progress; end it */
            Console->LinePos = Console->LineSize = 0;
            Console->LineComplete = TRUE;
        }
        return;
    }

    if (0 != (er.Event.KeyEvent.dwControlKeyState
              & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
            && (VK_UP == er.Event.KeyEvent.wVirtualKeyCode
                || VK_DOWN == er.Event.KeyEvent.wVirtualKeyCode))
    {
        if (er.Event.KeyEvent.bKeyDown)
        {
            /* scroll up or down */
            if (VK_UP == er.Event.KeyEvent.wVirtualKeyCode)
            {
                /* only scroll up if there is room to scroll up into */
                if (Console->ActiveBuffer->CursorPosition.Y != Console->ActiveBuffer->ScreenBufferSize.Y - 1)
                {
                    Console->ActiveBuffer->VirtualY = (Console->ActiveBuffer->VirtualY +
                                                       Console->ActiveBuffer->ScreenBufferSize.Y - 1) %
                                                       Console->ActiveBuffer->ScreenBufferSize.Y;
                    Console->ActiveBuffer->CursorPosition.Y++;
                }
            }
            else
            {
                /* only scroll down if there is room to scroll down into */
                if (Console->ActiveBuffer->CursorPosition.Y != 0)
                {
                    Console->ActiveBuffer->VirtualY = (Console->ActiveBuffer->VirtualY + 1) %
                                                       Console->ActiveBuffer->ScreenBufferSize.Y;
                    Console->ActiveBuffer->CursorPosition.Y--;
                }
            }
            ConioDrawConsole(Console);
        }
        return;
    }
    ConioProcessInputEvent(Console, &er);
}

static NTSTATUS
WaitBeforeReading(IN PGET_INPUT_INFO InputInfo,
                  IN PCSR_API_MESSAGE ApiMessage,
                  IN CSR_WAIT_FUNCTION WaitFunction OPTIONAL,
                  IN BOOL CreateWaitBlock OPTIONAL)
{
    if (CreateWaitBlock)
    {
        PGET_INPUT_INFO CapturedInputInfo;

        CapturedInputInfo = RtlAllocateHeap(ConSrvHeap, 0, sizeof(GET_INPUT_INFO));
        if (!CapturedInputInfo) return STATUS_NO_MEMORY;

        RtlMoveMemory(CapturedInputInfo, InputInfo, sizeof(GET_INPUT_INFO));

        if (!CsrCreateWait(&InputInfo->InputBuffer->ReadWaitQueue,
                           WaitFunction,
                           InputInfo->CallingThread,
                           ApiMessage,
                           CapturedInputInfo,
                           NULL))
        {
            RtlFreeHeap(ConSrvHeap, 0, CapturedInputInfo);
            return STATUS_NO_MEMORY;
        }
    }

    /* Wait for input */
    return STATUS_PENDING;
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
        RtlFreeHeap(ConSrvHeap, 0, InputInfo);
    }

    return (Status == STATUS_PENDING ? FALSE : TRUE);
}

static NTSTATUS
ReadInputBuffer(IN PGET_INPUT_INFO InputInfo,
                IN BOOL Wait,   // TRUE --> Read ; FALSE --> Peek
                IN PCSR_API_MESSAGE ApiMessage,
                IN BOOL CreateWaitBlock OPTIONAL)
{
    PCONSOLE_INPUT_BUFFER InputBuffer = InputInfo->InputBuffer;

    if (IsListEmpty(&InputBuffer->InputEvents))
    {
        if (Wait)
        {
            return WaitBeforeReading(InputInfo,
                                     ApiMessage,
                                     ReadInputBufferThread,
                                     CreateWaitBlock);
        }
        else
        {
            /* No input available and we don't wait, so we return success */
            return STATUS_SUCCESS;
        }
    }
    else
    {
        PCONSOLE_GETINPUT GetInputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetInputRequest;
        PLIST_ENTRY CurrentInput;
        ConsoleInput* Input;
        ULONG Length = GetInputRequest->Length;
        PINPUT_RECORD InputRecord = GetInputRequest->InputRecord;

        /* Only get input if there is any */
        CurrentInput = InputBuffer->InputEvents.Flink;

        while ( CurrentInput != &InputBuffer->InputEvents &&
                GetInputRequest->InputsRead < Length )
        {
            Input = CONTAINING_RECORD(CurrentInput, ConsoleInput, ListEntry);

            GetInputRequest->InputsRead++;
            *InputRecord = Input->InputEvent;

            if (GetInputRequest->Unicode == FALSE)
            {
                ConioInputEventToAnsi(InputBuffer->Header.Console, InputRecord);
            }

            InputRecord++;
            CurrentInput = CurrentInput->Flink;

            if (Wait) // TRUE --> Read, we remove inputs from the buffer ; FALSE --> Peek, we keep inputs.
            {
                RemoveEntryList(&Input->ListEntry);
                RtlFreeHeap(ConSrvHeap, 0, Input);
            }
        }

        if (IsListEmpty(&InputBuffer->InputEvents))
        {
            ResetEvent(InputBuffer->ActiveEvent);
        }

        /* We read all the inputs available, we return success */
        return STATUS_SUCCESS;
    }
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
        RtlFreeHeap(ConSrvHeap, 0, InputInfo);
    }

    return (Status == STATUS_PENDING ? FALSE : TRUE);
}

static NTSTATUS
ReadChars(IN PGET_INPUT_INFO InputInfo,
          IN PCSR_API_MESSAGE ApiMessage,
          IN BOOL CreateWaitBlock OPTIONAL)
{
    BOOL WaitForMoreToRead = TRUE; // TRUE : Wait if more to read ; FALSE : Don't wait.

    PCONSOLE_READCONSOLE ReadConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ReadConsoleRequest;
    PCONSOLE_INPUT_BUFFER InputBuffer = InputInfo->InputBuffer;
    PCONSOLE Console = InputBuffer->Header.Console;
    PLIST_ENTRY CurrentEntry;
    ConsoleInput *Input;
    PCHAR Buffer = (PCHAR)ReadConsoleRequest->Buffer;
    PWCHAR UnicodeBuffer = (PWCHAR)Buffer;
    ULONG nNumberOfCharsToRead = ReadConsoleRequest->NrCharactersToRead;

    /* We haven't read anything (yet) */

    if (InputBuffer->Mode & ENABLE_LINE_INPUT)
    {
        if (Console->LineBuffer == NULL)
        {
            /* Starting a new line */
            Console->LineMaxSize = (WORD)max(256, nNumberOfCharsToRead);
            Console->LineBuffer = RtlAllocateHeap(ConSrvHeap, 0, Console->LineMaxSize * sizeof(WCHAR));
            if (Console->LineBuffer == NULL)
            {
                return STATUS_NO_MEMORY;
            }
            Console->LineComplete = FALSE;
            Console->LineUpPressed = FALSE;
            Console->LineInsertToggle = 0;
            Console->LineWakeupMask = ReadConsoleRequest->CtrlWakeupMask;
            Console->LineSize = ReadConsoleRequest->NrCharactersRead;
            Console->LinePos = Console->LineSize;

            /*
             * Pre-filling the buffer is only allowed in the Unicode API,
             * so we don't need to worry about ANSI <-> Unicode conversion.
             */
            memcpy(Console->LineBuffer, Buffer, Console->LineSize * sizeof(WCHAR));
            if (Console->LineSize == Console->LineMaxSize)
            {
                Console->LineComplete = TRUE;
                Console->LinePos = 0;
            }
        }

        /* If we don't have a complete line yet, process the pending input */
        while ( !Console->LineComplete &&
                !IsListEmpty(&InputBuffer->InputEvents) )
        {
            /* Remove input event from queue */
            CurrentEntry = RemoveHeadList(&InputBuffer->InputEvents);
            if (IsListEmpty(&InputBuffer->InputEvents))
            {
                ResetEvent(InputBuffer->ActiveEvent);
            }
            Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);

            /* Only pay attention to key down */
            if (KEY_EVENT == Input->InputEvent.EventType
                    && Input->InputEvent.Event.KeyEvent.bKeyDown)
            {
                LineInputKeyDown(Console, &Input->InputEvent.Event.KeyEvent);
                ReadConsoleRequest->ControlKeyState = Input->InputEvent.Event.KeyEvent.dwControlKeyState;
            }
            RtlFreeHeap(ConSrvHeap, 0, Input);
        }

        /* Check if we have a complete line to read from */
        if (Console->LineComplete)
        {
            while ( ReadConsoleRequest->NrCharactersRead < nNumberOfCharsToRead &&
                    Console->LinePos != Console->LineSize )
            {
                WCHAR Char = Console->LineBuffer[Console->LinePos++];

                if (ReadConsoleRequest->Unicode)
                {
                    UnicodeBuffer[ReadConsoleRequest->NrCharactersRead] = Char;
                }
                else
                {
                    ConsoleInputUnicodeCharToAnsiChar(Console,
                                                      &Buffer[ReadConsoleRequest->NrCharactersRead],
                                                      &Char);
                }

                ReadConsoleRequest->NrCharactersRead++;
            }

            if (Console->LinePos == Console->LineSize)
            {
                /* Entire line has been read */
                RtlFreeHeap(ConSrvHeap, 0, Console->LineBuffer);
                Console->LineBuffer = NULL;
            }

            WaitForMoreToRead = FALSE;
        }
    }
    else
    {
        /* Character input */
        while ( ReadConsoleRequest->NrCharactersRead < nNumberOfCharsToRead &&
                !IsListEmpty(&InputBuffer->InputEvents) )
        {
            /* Remove input event from queue */
            CurrentEntry = RemoveHeadList(&InputBuffer->InputEvents);
            if (IsListEmpty(&InputBuffer->InputEvents))
            {
                ResetEvent(InputBuffer->ActiveEvent);
            }
            Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);

            /* Only pay attention to valid ascii chars, on key down */
            if (KEY_EVENT == Input->InputEvent.EventType
                    && Input->InputEvent.Event.KeyEvent.bKeyDown
                    && Input->InputEvent.Event.KeyEvent.uChar.UnicodeChar != L'\0')
            {
                WCHAR Char = Input->InputEvent.Event.KeyEvent.uChar.UnicodeChar;

                if (ReadConsoleRequest->Unicode)
                {
                    UnicodeBuffer[ReadConsoleRequest->NrCharactersRead] = Char;
                }
                else
                {
                    ConsoleInputUnicodeCharToAnsiChar(Console,
                                                      &Buffer[ReadConsoleRequest->NrCharactersRead],
                                                      &Char);
                }

                ReadConsoleRequest->NrCharactersRead++;

                /* Did read something */
                WaitForMoreToRead = FALSE;
            }
            RtlFreeHeap(ConSrvHeap, 0, Input);
        }
    }

    /* We haven't completed a read, so start a wait */
    if (WaitForMoreToRead == TRUE)
    {
        return WaitBeforeReading(InputInfo,
                                 ApiMessage,
                                 ReadCharsThread,
                                 CreateWaitBlock);
    }
    else /* We read all what we wanted, we return success */
    {
        return STATUS_SUCCESS;
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

    ReadConsoleRequest->NrCharactersRead = 0;

    InputInfo.CallingThread = CsrGetClientThread();
    InputInfo.HandleEntry   = HandleEntry;
    InputInfo.InputBuffer   = InputBuffer;

    Status = ReadChars(&InputInfo,
                       ApiMessage,
                       TRUE);

    ConSrvReleaseInputBuffer(InputBuffer, TRUE);

    if (Status == STATUS_PENDING)
        *ReplyCode = CsrReplyPending;

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

    GetInputRequest->InputsRead = 0;

    Status = ConSrvGetInputBufferAndHandleEntry(ProcessData, GetInputRequest->InputHandle, &InputBuffer, &HandleEntry, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    InputInfo.CallingThread = CsrGetClientThread();
    InputInfo.HandleEntry   = HandleEntry;
    InputInfo.InputBuffer   = InputBuffer;

    Status = ReadInputBuffer(&InputInfo,
                             GetInputRequest->bRead,
                             ApiMessage,
                             TRUE);

    ConSrvReleaseInputBuffer(InputBuffer, TRUE);

    if (Status == STATUS_PENDING)
        *ReplyCode = CsrReplyPending;

    return Status;
}

CSR_API(SrvWriteConsoleInput)
{
    NTSTATUS Status;
    PCONSOLE_WRITEINPUT WriteInputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.WriteInputRequest;
    PINPUT_RECORD InputRecord;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCONSOLE_INPUT_BUFFER InputBuffer;
    PCONSOLE Console;
    DWORD Length;
    DWORD i;

    DPRINT("SrvWriteConsoleInput\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&WriteInputRequest->InputRecord,
                                  WriteInputRequest->Length,
                                  sizeof(INPUT_RECORD)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConSrvGetInputBuffer(ProcessData, WriteInputRequest->InputHandle, &InputBuffer, GENERIC_WRITE, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    Console = InputBuffer->Header.Console;
    InputRecord = WriteInputRequest->InputRecord;
    Length = WriteInputRequest->Length;

    for (i = 0; i < Length && NT_SUCCESS(Status); i++)
    {
        if (!WriteInputRequest->Unicode &&
            InputRecord->EventType == KEY_EVENT)
        {
            CHAR AsciiChar = InputRecord->Event.KeyEvent.uChar.AsciiChar;
            ConsoleInputAnsiCharToUnicodeChar(Console,
                                              &InputRecord->Event.KeyEvent.uChar.UnicodeChar,
                                              &AsciiChar);
        }

        Status = ConioProcessInputEvent(Console, InputRecord++);
    }

    ConSrvReleaseInputBuffer(InputBuffer, TRUE);

    WriteInputRequest->Length = i;

    return Status;
}

CSR_API(SrvFlushConsoleInputBuffer)
{
    NTSTATUS Status;
    PCONSOLE_FLUSHINPUTBUFFER FlushInputBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.FlushInputBufferRequest;
    PLIST_ENTRY CurrentEntry;
    PCONSOLE_INPUT_BUFFER InputBuffer;
    ConsoleInput* Event;

    DPRINT("SrvFlushConsoleInputBuffer\n");

    Status = ConSrvGetInputBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                                 FlushInputBufferRequest->InputHandle,
                                 &InputBuffer,
                                 GENERIC_WRITE,
                                 TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Discard all entries in the input event queue */
    while (!IsListEmpty(&InputBuffer->InputEvents))
    {
        CurrentEntry = RemoveHeadList(&InputBuffer->InputEvents);
        Event = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
        RtlFreeHeap(ConSrvHeap, 0, Event);
    }
    ResetEvent(InputBuffer->ActiveEvent);

    ConSrvReleaseInputBuffer(InputBuffer, TRUE);
    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleNumberOfInputEvents)
{
    NTSTATUS Status;
    PCONSOLE_GETNUMINPUTEVENTS GetNumInputEventsRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetNumInputEventsRequest;
    PCONSOLE_INPUT_BUFFER InputBuffer;
    PLIST_ENTRY CurrentInput;
    DWORD NumEvents;

    DPRINT("SrvGetConsoleNumberOfInputEvents\n");

    Status = ConSrvGetInputBuffer(ConsoleGetPerProcessData(CsrGetClientThread()->Process), GetNumInputEventsRequest->InputHandle, &InputBuffer, GENERIC_READ, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    CurrentInput = InputBuffer->InputEvents.Flink;
    /* GetNumInputEventsRequest->NumInputEvents = */ NumEvents = 0;

    /* If there are any events ... */
    while (CurrentInput != &InputBuffer->InputEvents)
    {
        CurrentInput = CurrentInput->Flink;
        NumEvents++;
    }

    ConSrvReleaseInputBuffer(InputBuffer, TRUE);

    GetNumInputEventsRequest->NumInputEvents = NumEvents;

    return STATUS_SUCCESS;
}

/* EOF */
