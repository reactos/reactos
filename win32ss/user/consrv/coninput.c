/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/coninput.c
 * PURPOSE:         Console I/O functions
 * PROGRAMMERS:
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "conio.h"
#include "tuiconsole.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

#define ConsoleInputUnicodeCharToAnsiChar(Console, dChar, sWChar) \
    WideCharToMultiByte((Console)->CodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleInputAnsiCharToUnicodeChar(Console, dWChar, sChar) \
    MultiByteToWideChar((Console)->CodePage, 0, (sChar), 1, (dWChar), 1)


typedef struct _GET_INPUT_INFO
{
    PCONSOLE_PROCESS_DATA ProcessData;
    PCONSOLE Console;
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

static NTSTATUS FASTCALL
ConioProcessChar(PCONSOLE Console,
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
            if (Console->Mode & ENABLE_LINE_INPUT &&
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
    InsertTailList(&Console->InputEvents, &ConInRec->ListEntry);

    SetEvent(Console->ActiveEvent);
    CsrNotifyWait(&Console->ReadWaitQueue,
                  WaitAny,
                  NULL,
                  NULL);

    return STATUS_SUCCESS;
}

static DWORD FASTCALL
ConioGetShiftState(PBYTE KeyState)
{
    DWORD ssOut = 0;

    if (KeyState[VK_CAPITAL] & 1)
        ssOut |= CAPSLOCK_ON;

    if (KeyState[VK_NUMLOCK] & 1)
        ssOut |= NUMLOCK_ON;

    if (KeyState[VK_SCROLL] & 1)
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

    return ssOut;
}

VOID WINAPI
ConioProcessKey(MSG *msg, PCONSOLE Console, BOOL TextMode)
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

    RepeatCount = 1;
    VirtualScanCode = (msg->lParam >> 16) & 0xff;
    Down = msg->message == WM_KEYDOWN || msg->message == WM_CHAR ||
           msg->message == WM_SYSKEYDOWN || msg->message == WM_SYSCHAR;

    GetKeyboardState(KeyState);
    ShiftState = ConioGetShiftState(KeyState);

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

    if (TextMode)
    {
        if (0 != (ShiftState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
                && VK_TAB == VirtualKeyCode)
        {
            if (Down)
            {
                TuiSwapConsole(ShiftState & SHIFT_PRESSED ? -1 : 1);
            }

            return;
        }
        else if (VK_MENU == VirtualKeyCode && ! Down)
        {
            if (TuiSwapConsole(0))
            {
                return;
            }
        }
    }
    else
    {
        if ((ShiftState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED) || KeyState[VK_MENU] & 0x80) &&
            (VirtualKeyCode == VK_ESCAPE || VirtualKeyCode == VK_TAB || VirtualKeyCode == VK_SPACE))
        {
           DefWindowProcW( msg->hwnd, msg->message, msg->wParam, msg->lParam);
           return;
        }
    }

    if (NULL == Console)
    {
        DPRINT1("No Active Console!\n");
        return;
    }

    Fake = UnicodeChar &&
            (msg->message != WM_CHAR && msg->message != WM_SYSCHAR &&
            msg->message != WM_KEYUP && msg->message != WM_SYSKEYUP);
    NotChar = (msg->message != WM_CHAR && msg->message != WM_SYSCHAR);
    if (NotChar)
        LastVirtualKey = msg->wParam;

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

    if (Fake)
        return;

    /* process Ctrl-C and Ctrl-Break */
    if (Console->Mode & ENABLE_PROCESSED_INPUT &&
            er.Event.KeyEvent.bKeyDown &&
            ((er.Event.KeyEvent.wVirtualKeyCode == VK_PAUSE) ||
             (er.Event.KeyEvent.wVirtualKeyCode == 'C')) &&
            (er.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED) || KeyState[VK_CONTROL] & 0x80))
    {
        PCONSOLE_PROCESS_DATA current;
        PLIST_ENTRY current_entry;

        DPRINT1("Console_Api Ctrl-C\n");

        current_entry = Console->ProcessList.Flink;
        while (current_entry != &Console->ProcessList)
        {
            current = CONTAINING_RECORD(current_entry, CONSOLE_PROCESS_DATA, ConsoleLink);
            current_entry = current_entry->Flink;
            ConioConsoleCtrlEvent(CTRL_C_EVENT, current);
        }
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
                if (Console->ActiveBuffer->CurrentY != Console->ActiveBuffer->MaxY - 1)
                {
                    Console->ActiveBuffer->VirtualY = (Console->ActiveBuffer->VirtualY +
                                                       Console->ActiveBuffer->MaxY - 1) %
                                                      Console->ActiveBuffer->MaxY;
                    Console->ActiveBuffer->CurrentY++;
                }
            }
            else
            {
                /* only scroll down if there is room to scroll down into */
                if (Console->ActiveBuffer->CurrentY != 0)
                {
                    Console->ActiveBuffer->VirtualY = (Console->ActiveBuffer->VirtualY + 1) %
                                                      Console->ActiveBuffer->MaxY;
                    Console->ActiveBuffer->CurrentY--;
                }
            }
            ConioDrawConsole(Console);
        }
        return;
    }
    ConioProcessChar(Console, &er);
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

        memmove(CapturedInputInfo, InputInfo, sizeof(GET_INPUT_INFO));

        if (!CsrCreateWait(&InputInfo->Console->ReadWaitQueue,
                           WaitFunction,
                           CsrGetClientThread(),
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

    Status = ReadInputBuffer(InputInfo,
                             GetInputRequest->bRead,
                             WaitApiMessage,
                             FALSE);

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
    if (IsListEmpty(&InputInfo->Console->InputEvents))
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
        CurrentInput = InputInfo->Console->InputEvents.Flink;

        while ( CurrentInput != &InputInfo->Console->InputEvents &&
                GetInputRequest->InputsRead < Length )
        {
            Input = CONTAINING_RECORD(CurrentInput, ConsoleInput, ListEntry);

            GetInputRequest->InputsRead++;
            *InputRecord = Input->InputEvent;

            if (GetInputRequest->Unicode == FALSE)
            {
                ConioInputEventToAnsi(InputInfo->Console, InputRecord);
            }

            InputRecord++;
            CurrentInput = CurrentInput->Flink;

            if (Wait) // TRUE --> Read, we remove inputs from the buffer ; FALSE --> Peek, we keep inputs.
            {
                RemoveEntryList(&Input->ListEntry);
                RtlFreeHeap(ConSrvHeap, 0, Input);
            }
        }

        if (IsListEmpty(&InputInfo->Console->InputEvents))
        {
            ResetEvent(InputInfo->Console->ActiveEvent);
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

    Status = ReadChars(InputInfo,
                       WaitApiMessage,
                       FALSE);

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
    PLIST_ENTRY CurrentEntry;
    ConsoleInput *Input;
    PCHAR Buffer = (PCHAR)ReadConsoleRequest->Buffer;
    PWCHAR UnicodeBuffer = (PWCHAR)Buffer;
    ULONG nNumberOfCharsToRead = ReadConsoleRequest->NrCharactersToRead;

    /* We haven't read anything (yet) */

    if (InputInfo->Console->Mode & ENABLE_LINE_INPUT)
    {
        if (InputInfo->Console->LineBuffer == NULL)
        {
            /* Starting a new line */
            InputInfo->Console->LineMaxSize = max(256, nNumberOfCharsToRead);
            InputInfo->Console->LineBuffer = RtlAllocateHeap(ConSrvHeap, 0, InputInfo->Console->LineMaxSize * sizeof(WCHAR));
            if (InputInfo->Console->LineBuffer == NULL)
            {
                return STATUS_NO_MEMORY;
            }
            InputInfo->Console->LineComplete = FALSE;
            InputInfo->Console->LineUpPressed = FALSE;
            InputInfo->Console->LineInsertToggle = 0;
            InputInfo->Console->LineWakeupMask = ReadConsoleRequest->CtrlWakeupMask;
            InputInfo->Console->LineSize = ReadConsoleRequest->NrCharactersRead;
            InputInfo->Console->LinePos = InputInfo->Console->LineSize;

            /*
             * Pre-filling the buffer is only allowed in the Unicode API,
             * so we don't need to worry about ANSI <-> Unicode conversion.
             */
            memcpy(InputInfo->Console->LineBuffer, Buffer, InputInfo->Console->LineSize * sizeof(WCHAR));
            if (InputInfo->Console->LineSize == InputInfo->Console->LineMaxSize)
            {
                InputInfo->Console->LineComplete = TRUE;
                InputInfo->Console->LinePos = 0;
            }
        }

        /* If we don't have a complete line yet, process the pending input */
        while ( !InputInfo->Console->LineComplete &&
                !IsListEmpty(&InputInfo->Console->InputEvents) )
        {
            /* Remove input event from queue */
            CurrentEntry = RemoveHeadList(&InputInfo->Console->InputEvents);
            if (IsListEmpty(&InputInfo->Console->InputEvents))
            {
                ResetEvent(InputInfo->Console->ActiveEvent);
            }
            Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);

            /* Only pay attention to key down */
            if (KEY_EVENT == Input->InputEvent.EventType
                    && Input->InputEvent.Event.KeyEvent.bKeyDown)
            {
                LineInputKeyDown(InputInfo->Console, &Input->InputEvent.Event.KeyEvent);
                ReadConsoleRequest->ControlKeyState = Input->InputEvent.Event.KeyEvent.dwControlKeyState;
            }
            RtlFreeHeap(ConSrvHeap, 0, Input);
        }

        /* Check if we have a complete line to read from */
        if (InputInfo->Console->LineComplete)
        {
            while ( ReadConsoleRequest->NrCharactersRead < nNumberOfCharsToRead &&
                    InputInfo->Console->LinePos != InputInfo->Console->LineSize )
            {
                WCHAR Char = InputInfo->Console->LineBuffer[InputInfo->Console->LinePos++];

                if (ReadConsoleRequest->Unicode)
                {
                    UnicodeBuffer[ReadConsoleRequest->NrCharactersRead] = Char;
                }
                else
                {
                    ConsoleInputUnicodeCharToAnsiChar(InputInfo->Console,
                                                      &Buffer[ReadConsoleRequest->NrCharactersRead],
                                                      &Char);
                }

                ReadConsoleRequest->NrCharactersRead++;
            }

            if (InputInfo->Console->LinePos == InputInfo->Console->LineSize)
            {
                /* Entire line has been read */
                RtlFreeHeap(ConSrvHeap, 0, InputInfo->Console->LineBuffer);
                InputInfo->Console->LineBuffer = NULL;
            }

            WaitForMoreToRead = FALSE;
        }
    }
    else
    {
        /* Character input */
        while ( ReadConsoleRequest->NrCharactersRead < nNumberOfCharsToRead &&
                !IsListEmpty(&InputInfo->Console->InputEvents) )
        {
            /* Remove input event from queue */
            CurrentEntry = RemoveHeadList(&InputInfo->Console->InputEvents);
            if (IsListEmpty(&InputInfo->Console->InputEvents))
            {
                ResetEvent(InputInfo->Console->ActiveEvent);
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
                    ConsoleInputUnicodeCharToAnsiChar(InputInfo->Console,
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


/* PUBLIC APIS ****************************************************************/

CSR_API(SrvGetConsoleInput)
{
    NTSTATUS Status;
    PCONSOLE_GETINPUT GetInputRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetInputRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCONSOLE Console;
    GET_INPUT_INFO InputInfo;

    DPRINT("SrvGetConsoleInput\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&GetInputRequest->InputRecord,
                                  GetInputRequest->Length,
                                  sizeof(INPUT_RECORD)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConioLockConsole(ProcessData, GetInputRequest->InputHandle, &Console, GENERIC_READ);
    if(!NT_SUCCESS(Status)) return Status;

    GetInputRequest->InputsRead = 0;

    InputInfo.ProcessData = ProcessData; // ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    InputInfo.Console     = Console;

    Status = ReadInputBuffer(&InputInfo,
                             GetInputRequest->bRead,
                             ApiMessage,
                             TRUE);

    ConioUnlockConsole(Console);

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

    Status = ConioLockConsole(ProcessData, WriteInputRequest->InputHandle, &Console, GENERIC_WRITE);
    if (!NT_SUCCESS(Status)) return Status;

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

        Status = ConioProcessChar(Console, InputRecord++);
    }

    ConioUnlockConsole(Console);

    WriteInputRequest->Length = i;

    return Status;
}

CSR_API(SrvReadConsole)
{
    NTSTATUS Status;
    PCONSOLE_READCONSOLE ReadConsoleRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.ReadConsoleRequest;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    PCONSOLE Console;
    GET_INPUT_INFO InputInfo;

    DPRINT("SrvReadConsole\n");

    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&ReadConsoleRequest->Buffer,
                                  ReadConsoleRequest->BufferSize,
                                  sizeof(BYTE)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    // if (Request->Data.ReadConsoleRequest.NrCharactersRead * sizeof(WCHAR) > nNumberOfCharsToRead * CharSize)
    if (ReadConsoleRequest->NrCharactersRead > ReadConsoleRequest->NrCharactersToRead)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ConioLockConsole(ProcessData, ReadConsoleRequest->InputHandle, &Console, GENERIC_READ);
    if (!NT_SUCCESS(Status)) return Status;

    ReadConsoleRequest->NrCharactersRead = 0;

    InputInfo.ProcessData = ProcessData; // ConsoleGetPerProcessData(CsrGetClientThread()->Process);
    InputInfo.Console     = Console;

    Status = ReadChars(&InputInfo,
                       ApiMessage,
                       TRUE);

    ConioUnlockConsole(Console);

    if (Status == STATUS_PENDING)
        *ReplyCode = CsrReplyPending;

    return Status;
}

CSR_API(SrvFlushConsoleInputBuffer)
{
    NTSTATUS Status;
    PCONSOLE_FLUSHINPUTBUFFER FlushInputBufferRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.FlushInputBufferRequest;
    PLIST_ENTRY CurrentEntry;
    PCONSOLE Console;
    ConsoleInput* Input;

    DPRINT("SrvFlushConsoleInputBuffer\n");

    Status = ConioLockConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              FlushInputBufferRequest->InputHandle,
                              &Console,
                              GENERIC_WRITE);
    if(!NT_SUCCESS(Status)) return Status;

    /* Discard all entries in the input event queue */
    while (!IsListEmpty(&Console->InputEvents))
    {
        CurrentEntry = RemoveHeadList(&Console->InputEvents);
        Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
        /* Destroy the event */
        RtlFreeHeap(ConSrvHeap, 0, Input);
    }
    ResetEvent(Console->ActiveEvent);

    ConioUnlockConsole(Console);

    return STATUS_SUCCESS;
}

CSR_API(SrvGetConsoleNumberOfInputEvents)
{
    NTSTATUS Status;
    PCONSOLE_GETNUMINPUTEVENTS GetNumInputEventsRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.GetNumInputEventsRequest;
    PCONSOLE Console;
    PLIST_ENTRY CurrentInput;
    DWORD NumEvents;

    DPRINT("SrvGetConsoleNumberOfInputEvents\n");

    Status = ConioLockConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process), GetNumInputEventsRequest->InputHandle, &Console, GENERIC_READ);
    if (!NT_SUCCESS(Status)) return Status;

    CurrentInput = Console->InputEvents.Flink;
    NumEvents = 0;

    /* If there are any events ... */
    while (CurrentInput != &Console->InputEvents)
    {
        CurrentInput = CurrentInput->Flink;
        NumEvents++;
    }

    ConioUnlockConsole(Console);

    GetNumInputEventsRequest->NumInputEvents = NumEvents;

    return STATUS_SUCCESS;
}

/* EOF */
