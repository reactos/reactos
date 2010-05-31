/*
 * reactos/subsys/csrss/win32csr/conio.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#define NDEBUG
#include "w32csr.h"
#include <debug.h>

/* GLOBALS *******************************************************************/

#define ConsoleInputUnicodeCharToAnsiChar(Console, dChar, sWChar) \
    WideCharToMultiByte((Console)->CodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleInputAnsiCharToUnicodeChar(Console, dWChar, sChar) \
    MultiByteToWideChar((Console)->CodePage, 0, (sChar), 1, (dWChar), 1)

/* FUNCTIONS *****************************************************************/

CSR_API(CsrReadConsole)
{
    PLIST_ENTRY CurrentEntry;
    ConsoleInput *Input;
    PUCHAR Buffer;
    PWCHAR UnicodeBuffer;
    ULONG i;
    ULONG nNumberOfCharsToRead, CharSize;
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;

    DPRINT("CsrReadConsole\n");

    CharSize = (Request->Data.ReadConsoleRequest.Unicode ? sizeof(WCHAR) : sizeof(CHAR));

    /* truncate length to CSRSS_MAX_READ_CONSOLE_REQUEST */
    nNumberOfCharsToRead = min(Request->Data.ReadConsoleRequest.NrCharactersToRead, CSRSS_MAX_READ_CONSOLE / CharSize);
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Buffer = Request->Data.ReadConsoleRequest.Buffer;
    UnicodeBuffer = (PWCHAR)Buffer;
    Status = ConioLockConsole(ProcessData, Request->Data.ReadConsoleRequest.ConsoleHandle,
                              &Console, GENERIC_READ);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Request->Data.ReadConsoleRequest.EventHandle = ProcessData->ConsoleEvent;
    for (i = 0; i < nNumberOfCharsToRead && Console->InputEvents.Flink != &Console->InputEvents; i++)
    {
        /* remove input event from queue */
        CurrentEntry = RemoveHeadList(&Console->InputEvents);
        if (IsListEmpty(&Console->InputEvents))
        {
            ResetEvent(Console->ActiveEvent);
        }
        Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);

        /* only pay attention to valid ascii chars, on key down */
        if (KEY_EVENT == Input->InputEvent.EventType
                && Input->InputEvent.Event.KeyEvent.bKeyDown
                && Input->InputEvent.Event.KeyEvent.uChar.AsciiChar != '\0')
        {
            /*
             * backspace handling - if we are in charge of echoing it then we handle it here
             * otherwise we treat it like a normal char.
             */
            if ('\b' == Input->InputEvent.Event.KeyEvent.uChar.AsciiChar && 0
                    != (Console->Mode & ENABLE_ECHO_INPUT))
            {
                /* echo if it has not already been done, and either we or the client has chars to be deleted */
                if (! Input->Echoed
                        && (0 !=  i || Request->Data.ReadConsoleRequest.nCharsCanBeDeleted))
                {
                    ConioWriteConsole(Console, Console->ActiveBuffer,
                                      &Input->InputEvent.Event.KeyEvent.uChar.AsciiChar, 1, TRUE);
                }
                if (0 != i)
                {
                    i -= 2;        /* if we already have something to return, just back it up by 2 */
                }
                else
                {
                    /* otherwise, return STATUS_NOTIFY_CLEANUP to tell client to back up its buffer */
                    Console->WaitingChars--;
                    ConioUnlockConsole(Console);
                    HeapFree(Win32CsrApiHeap, 0, Input);
                    Request->Data.ReadConsoleRequest.NrCharactersRead = 0;
                    return STATUS_NOTIFY_CLEANUP;

                }
                Request->Data.ReadConsoleRequest.nCharsCanBeDeleted--;
                Input->Echoed = TRUE;   /* mark as echoed so we don't echo it below */
            }
            /* do not copy backspace to buffer */
            else
            {
                if(Request->Data.ReadConsoleRequest.Unicode)
                    ConsoleInputAnsiCharToUnicodeChar(Console, &UnicodeBuffer[i], &Input->InputEvent.Event.KeyEvent.uChar.AsciiChar);
                else
                    Buffer[i] = Input->InputEvent.Event.KeyEvent.uChar.AsciiChar;
            }
            /* echo to screen if enabled and we did not already echo the char */
            if (0 != (Console->Mode & ENABLE_ECHO_INPUT)
                    && ! Input->Echoed
                    && '\r' != Input->InputEvent.Event.KeyEvent.uChar.AsciiChar)
            {
                ConioWriteConsole(Console, Console->ActiveBuffer,
                                  &Input->InputEvent.Event.KeyEvent.uChar.AsciiChar, 1, TRUE);
            }
        }
        else
        {
            i--;
        }
        Console->WaitingChars--;
        HeapFree(Win32CsrApiHeap, 0, Input);
    }
    Request->Data.ReadConsoleRequest.NrCharactersRead = i;
    if (0 == i)
    {
        Status = STATUS_PENDING;    /* we didn't read anything */
    }
    else if (0 != (Console->Mode & ENABLE_LINE_INPUT))
    {
        if (0 == Console->WaitingLines ||
                (Request->Data.ReadConsoleRequest.Unicode ? (L'\n' != UnicodeBuffer[i - 1]) : ('\n' != Buffer[i - 1])))
        {
            Status = STATUS_PENDING; /* line buffered, didn't get a complete line */
        }
        else
        {
            Console->WaitingLines--;
            Status = STATUS_SUCCESS; /* line buffered, did get a complete line */
        }
    }
    else
    {
        Status = STATUS_SUCCESS;  /* not line buffered, did read something */
    }

    if (Status == STATUS_PENDING)
    {
        Console->EchoCount = nNumberOfCharsToRead - i;
    }
    else
    {
        Console->EchoCount = 0;             /* if the client is no longer waiting on input, do not echo */
    }

    ConioUnlockConsole(Console);

    if (CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE) + i * CharSize > sizeof(CSR_API_MESSAGE))
    {
        Request->Header.u1.s1.TotalLength = CSR_API_MESSAGE_HEADER_SIZE(CSRSS_READ_CONSOLE) + i * CharSize;
        Request->Header.u1.s1.DataLength = Request->Header.u1.s1.TotalLength - sizeof(PORT_MESSAGE);
    }

    return Status;
}

static VOID FASTCALL
ConioInputEventToAnsi(PCSRSS_CONSOLE Console, PINPUT_RECORD InputEvent)
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

static VOID FASTCALL
ConioProcessChar(PCSRSS_CONSOLE Console,
                 ConsoleInput *KeyEventRecord)
{
    BOOL updown;
    ConsoleInput *TempInput;

    if (KeyEventRecord->InputEvent.EventType == KEY_EVENT &&
        KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown)
    {
        WORD vk = KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode;
        if (!(Console->PauseFlags & PAUSED_FROM_KEYBOARD))
        {
            DWORD cks = KeyEventRecord->InputEvent.Event.KeyEvent.dwControlKeyState;
            if (Console->Mode & ENABLE_LINE_INPUT &&
                (vk == VK_PAUSE || (vk == 'S' &&
                                    (cks & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) &&
                                    !(cks & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)))))
            {
                ConioPause(Console, PAUSED_FROM_KEYBOARD);
                HeapFree(Win32CsrApiHeap, 0, KeyEventRecord);
                return;
            }
        }
        else
        {
            if ((vk < VK_SHIFT || vk > VK_CAPITAL) && vk != VK_LWIN &&
                vk != VK_RWIN && vk != VK_NUMLOCK && vk != VK_SCROLL)
            {
                ConioUnpause(Console, PAUSED_FROM_KEYBOARD);
                HeapFree(Win32CsrApiHeap, 0, KeyEventRecord);
                return;
            }
        }
    }

    if (0 != (Console->Mode & (ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT)))
    {
        switch(KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar)
        {
        case '\r':
            /* first add the \r */
            KeyEventRecord->InputEvent.EventType = KEY_EVENT;
            updown = KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown;
            KeyEventRecord->Echoed = FALSE;
            KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
            KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar = '\r';
            InsertTailList(&Console->InputEvents, &KeyEventRecord->ListEntry);
            Console->WaitingChars++;
            KeyEventRecord = HeapAlloc(Win32CsrApiHeap, 0, sizeof(ConsoleInput));
            if (NULL == KeyEventRecord)
            {
                DPRINT1("Failed to allocate KeyEventRecord\n");
                return;
            }
            KeyEventRecord->InputEvent.EventType = KEY_EVENT;
            KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown = updown;
            KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualKeyCode = 0;
            KeyEventRecord->InputEvent.Event.KeyEvent.wVirtualScanCode = 0;
            KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar = '\n';
            KeyEventRecord->Fake = TRUE;
            break;
        }
    }
    /* add event to the queue */
    InsertTailList(&Console->InputEvents, &KeyEventRecord->ListEntry);
    Console->WaitingChars++;
    /* if line input mode is enabled, only wake the client on enter key down */
    if (0 == (Console->Mode & ENABLE_LINE_INPUT)
            || Console->EarlyReturn
            || ('\n' == KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar
                && KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown))
    {
        if ('\n' == KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar)
        {
            Console->WaitingLines++;
        }
    }
    KeyEventRecord->Echoed = FALSE;
    if (0 != (Console->Mode & (ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT))
            && '\b' == KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar
            && KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown)
    {
        /* walk the input queue looking for a char to backspace */
        for (TempInput = (ConsoleInput *) Console->InputEvents.Blink;
                TempInput != (ConsoleInput *) &Console->InputEvents
                && (KEY_EVENT == TempInput->InputEvent.EventType
                    || ! TempInput->InputEvent.Event.KeyEvent.bKeyDown
                    || '\b' == TempInput->InputEvent.Event.KeyEvent.uChar.AsciiChar);
                TempInput = (ConsoleInput *) TempInput->ListEntry.Blink)
        {
            /* NOP */;
        }
        /* if we found one, delete it, otherwise, wake the client */
        if (TempInput != (ConsoleInput *) &Console->InputEvents)
        {
            /* delete previous key in queue, maybe echo backspace to screen, and do not place backspace on queue */
            RemoveEntryList(&TempInput->ListEntry);
            if (TempInput->Echoed)
            {
                ConioWriteConsole(Console, Console->ActiveBuffer,
                                  &KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar,
                                  1, TRUE);
            }
            HeapFree(Win32CsrApiHeap, 0, TempInput);
            RemoveEntryList(&KeyEventRecord->ListEntry);
            HeapFree(Win32CsrApiHeap, 0, KeyEventRecord);
            Console->WaitingChars -= 2;
            return;
        }
    }
    else
    {
        /* echo chars if we are supposed to and client is waiting for some */
        if (0 != (Console->Mode & ENABLE_ECHO_INPUT) && Console->EchoCount
                && KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar
                && KeyEventRecord->InputEvent.Event.KeyEvent.bKeyDown
                && '\r' != KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar)
        {
            /* mark the char as already echoed */
            ConioWriteConsole(Console, Console->ActiveBuffer,
                              &KeyEventRecord->InputEvent.Event.KeyEvent.uChar.AsciiChar,
                              1, TRUE);
            Console->EchoCount--;
            KeyEventRecord->Echoed = TRUE;
        }
    }

    /* Console->WaitingChars++; */
    SetEvent(Console->ActiveEvent);
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
ConioProcessKey(MSG *msg, PCSRSS_CONSOLE Console, BOOL TextMode)
{
    static BYTE KeyState[256] = { 0 };
    /* MSDN mentions that you should use the last virtual key code received
     * when putting a virtual key identity to a WM_CHAR message since multiple
     * or translated keys may be involved. */
    static UINT LastVirtualKey = 0;
    DWORD ShiftState;
    ConsoleInput *ConInRec;
    UINT RepeatCount;
    CHAR AsciiChar;
    WCHAR UnicodeChar;
    UINT VirtualKeyCode;
    UINT VirtualScanCode;
    BOOL Down = FALSE;
    INPUT_RECORD er;
    ULONG ResultSize = 0;

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

    if (0 == ResultSize)
    {
        AsciiChar = 0;
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

    if (NULL == Console)
    {
        DPRINT1("No Active Console!\n");
        return;
    }

    ConInRec = HeapAlloc(Win32CsrApiHeap, 0, sizeof(ConsoleInput));

    if (NULL == ConInRec)
    {
        return;
    }

    ConInRec->InputEvent = er;
    ConInRec->Fake = UnicodeChar &&
                     (msg->message != WM_CHAR && msg->message != WM_SYSCHAR &&
                      msg->message != WM_KEYUP && msg->message != WM_SYSKEYUP);
    ConInRec->NotChar = (msg->message != WM_CHAR && msg->message != WM_SYSCHAR);
    ConInRec->Echoed = FALSE;
    if (ConInRec->NotChar)
        LastVirtualKey = msg->wParam;

    DPRINT  ("csrss: %s %s %s %s %02x %02x '%c' %04x\n",
             Down ? "down" : "up  ",
             (msg->message == WM_CHAR || msg->message == WM_SYSCHAR) ?
             "char" : "key ",
             ConInRec->Fake ? "fake" : "real",
             ConInRec->NotChar ? "notc" : "char",
             VirtualScanCode,
             VirtualKeyCode,
             (AsciiChar >= ' ') ? AsciiChar : '.',
             ShiftState);

    if (ConInRec->Fake && ConInRec->NotChar)
    {
        HeapFree(Win32CsrApiHeap, 0, ConInRec);
        return;
    }

    /* process Ctrl-C and Ctrl-Break */
    if (Console->Mode & ENABLE_PROCESSED_INPUT &&
            er.Event.KeyEvent.bKeyDown &&
            ((er.Event.KeyEvent.wVirtualKeyCode == VK_PAUSE) ||
             (er.Event.KeyEvent.wVirtualKeyCode == 'C')) &&
            (er.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)))
    {
        PCSRSS_PROCESS_DATA current;
        PLIST_ENTRY current_entry;
        DPRINT1("Console_Api Ctrl-C\n");
        current_entry = Console->ProcessList.Flink;
        while (current_entry != &Console->ProcessList)
        {
            current = CONTAINING_RECORD(current_entry, CSRSS_PROCESS_DATA, ProcessEntry);
            current_entry = current_entry->Flink;
            ConioConsoleCtrlEvent((DWORD)CTRL_C_EVENT, current);
        }
        HeapFree(Win32CsrApiHeap, 0, ConInRec);
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
        HeapFree(Win32CsrApiHeap, 0, ConInRec);
        return;
    }
    /* FIXME - convert to ascii */
    ConioProcessChar(Console, ConInRec);
}

CSR_API(CsrReadInputEvent)
{
    PLIST_ENTRY CurrentEntry;
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;
    BOOLEAN Done = FALSE;
    ConsoleInput *Input;

    DPRINT("CsrReadInputEvent\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
    Request->Data.ReadInputRequest.Event = ProcessData->ConsoleEvent;

    Status = ConioLockConsole(ProcessData, Request->Data.ReadInputRequest.ConsoleHandle, &Console, GENERIC_READ);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    /* only get input if there is any */
    CurrentEntry = Console->InputEvents.Flink;
    while (CurrentEntry != &Console->InputEvents)
    {
        Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
        CurrentEntry = CurrentEntry->Flink;

        if (Done && !Input->Fake)
        {
            Request->Data.ReadInputRequest.MoreEvents = TRUE;
            break;
        }

        RemoveEntryList(&Input->ListEntry);

        if (!Done && !Input->Fake)
        {
            Request->Data.ReadInputRequest.Input = Input->InputEvent;
            if (Request->Data.ReadInputRequest.Unicode == FALSE)
            {
                ConioInputEventToAnsi(Console, &Request->Data.ReadInputRequest.Input);
            }
            Done = TRUE;
        }

        if (Input->InputEvent.EventType == KEY_EVENT)
        {
            if (0 != (Console->Mode & ENABLE_LINE_INPUT)
                    && Input->InputEvent.Event.KeyEvent.bKeyDown
                    && '\r' == Input->InputEvent.Event.KeyEvent.uChar.AsciiChar)
            {
                Console->WaitingLines--;
            }
            Console->WaitingChars--;
        }
        HeapFree(Win32CsrApiHeap, 0, Input);
    }

    if (Done)
    {
        Status = STATUS_SUCCESS;
        Console->EarlyReturn = FALSE;
    }
    else
    {
        Status = STATUS_PENDING;
        Console->EarlyReturn = TRUE;  /* mark for early return */
    }

    if (IsListEmpty(&Console->InputEvents))
    {
        ResetEvent(Console->ActiveEvent);
    }

    ConioUnlockConsole(Console);

    return Status;
}

CSR_API(CsrFlushInputBuffer)
{
    PLIST_ENTRY CurrentEntry;
    PCSRSS_CONSOLE Console;
    ConsoleInput* Input;
    NTSTATUS Status;

    DPRINT("CsrFlushInputBuffer\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
    Status = ConioLockConsole(ProcessData,
                              Request->Data.FlushInputBufferRequest.ConsoleInput,
                              &Console,
                              GENERIC_WRITE);
    if(! NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Discard all entries in the input event queue */
    while (!IsListEmpty(&Console->InputEvents))
    {
        CurrentEntry = RemoveHeadList(&Console->InputEvents);
        Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
        /* Destroy the event */
        HeapFree(Win32CsrApiHeap, 0, Input);
    }
    ResetEvent(Console->ActiveEvent);
    Console->WaitingChars=0;

    ConioUnlockConsole(Console);

    return STATUS_SUCCESS;
}

CSR_API(CsrGetNumberOfConsoleInputEvents)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE Console;
    PLIST_ENTRY CurrentItem;
    DWORD NumEvents;
    ConsoleInput *Input;

    DPRINT("CsrGetNumberOfConsoleInputEvents\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = Request->Header.u1.s1.TotalLength - sizeof(PORT_MESSAGE);

    Status = ConioLockConsole(ProcessData, Request->Data.GetNumInputEventsRequest.ConsoleHandle, &Console, GENERIC_READ);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    CurrentItem = Console->InputEvents.Flink;
    NumEvents = 0;

    /* If there are any events ... */
    while (CurrentItem != &Console->InputEvents)
    {
        Input = CONTAINING_RECORD(CurrentItem, ConsoleInput, ListEntry);
        CurrentItem = CurrentItem->Flink;
        if (!Input->Fake)
        {
            NumEvents++;
        }
    }

    ConioUnlockConsole(Console);

    Request->Data.GetNumInputEventsRequest.NumInputEvents = NumEvents;

    return STATUS_SUCCESS;
}

CSR_API(CsrPeekConsoleInput)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE Console;
    DWORD Size;
    DWORD Length;
    PLIST_ENTRY CurrentItem;
    PINPUT_RECORD InputRecord;
    ConsoleInput* Item;
    UINT NumItems;

    DPRINT("CsrPeekConsoleInput\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioLockConsole(ProcessData, Request->Data.GetNumInputEventsRequest.ConsoleHandle, &Console, GENERIC_READ);
    if(! NT_SUCCESS(Status))
    {
        return Status;
    }

    InputRecord = Request->Data.PeekConsoleInputRequest.InputRecord;
    Length = Request->Data.PeekConsoleInputRequest.Length;
    Size = Length * sizeof(INPUT_RECORD);

    if (((PVOID)InputRecord < ProcessData->CsrSectionViewBase)
            || (((ULONG_PTR)InputRecord + Size) > ((ULONG_PTR)ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
        ConioUnlockConsole(Console);
        return STATUS_ACCESS_VIOLATION;
    }

    NumItems = 0;

    if (! IsListEmpty(&Console->InputEvents))
    {
        CurrentItem = Console->InputEvents.Flink;

        while (CurrentItem != &Console->InputEvents && NumItems < Length)
        {
            Item = CONTAINING_RECORD(CurrentItem, ConsoleInput, ListEntry);

            if (Item->Fake)
            {
                CurrentItem = CurrentItem->Flink;
                continue;
            }

            ++NumItems;
            *InputRecord = Item->InputEvent;

            if (Request->Data.ReadInputRequest.Unicode == FALSE)
            {
                ConioInputEventToAnsi(Console, InputRecord);
            }

            InputRecord++;
            CurrentItem = CurrentItem->Flink;
        }
    }

    ConioUnlockConsole(Console);

    Request->Data.PeekConsoleInputRequest.Length = NumItems;

    return STATUS_SUCCESS;
}

CSR_API(CsrWriteConsoleInput)
{
    PINPUT_RECORD InputRecord;
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;
    DWORD Length;
    DWORD Size;
    DWORD i;
    ConsoleInput* Record;

    DPRINT("CsrWriteConsoleInput\n");

    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    Status = ConioLockConsole(ProcessData, Request->Data.WriteConsoleInputRequest.ConsoleHandle, &Console, GENERIC_WRITE);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    InputRecord = Request->Data.WriteConsoleInputRequest.InputRecord;
    Length = Request->Data.WriteConsoleInputRequest.Length;
    Size = Length * sizeof(INPUT_RECORD);

    if (((PVOID)InputRecord < ProcessData->CsrSectionViewBase)
            || (((ULONG_PTR)InputRecord + Size) > ((ULONG_PTR)ProcessData->CsrSectionViewBase + ProcessData->CsrSectionViewSize)))
    {
        ConioUnlockConsole(Console);
        return STATUS_ACCESS_VIOLATION;
    }

    for (i = 0; i < Length; i++)
    {
        Record = HeapAlloc(Win32CsrApiHeap, 0, sizeof(ConsoleInput));
        if (NULL == Record)
        {
            ConioUnlockConsole(Console);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Record->Echoed = FALSE;
        Record->Fake = FALSE;
        //Record->InputEvent = *InputRecord++;
        memcpy(&Record->InputEvent, &InputRecord[i], sizeof(INPUT_RECORD));
        if (KEY_EVENT == Record->InputEvent.EventType)
        {
            /* FIXME - convert from unicode to ascii!! */
            ConioProcessChar(Console, Record);
        }
    }

    ConioUnlockConsole(Console);

    Request->Data.WriteConsoleInputRequest.Length = i;

    return STATUS_SUCCESS;
}

/* EOF */
