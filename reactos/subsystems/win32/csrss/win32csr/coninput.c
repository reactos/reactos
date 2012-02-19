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
    PCHAR Buffer;
    PWCHAR UnicodeBuffer;
    ULONG i = 0;
    ULONG nNumberOfCharsToRead, CharSize;
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;

    DPRINT("CsrReadConsole\n");

    CharSize = (Request->Data.ReadConsoleRequest.Unicode ? sizeof(WCHAR) : sizeof(CHAR));

    nNumberOfCharsToRead = Request->Data.ReadConsoleRequest.NrCharactersToRead;

    Buffer = (PCHAR)Request->Data.ReadConsoleRequest.Buffer;
    UnicodeBuffer = (PWCHAR)Buffer;
    if (!Win32CsrValidateBuffer(ProcessData, Buffer, nNumberOfCharsToRead, CharSize))
        return STATUS_ACCESS_VIOLATION;

    if (Request->Data.ReadConsoleRequest.NrCharactersRead * sizeof(WCHAR) > nNumberOfCharsToRead * CharSize)
        return STATUS_INVALID_PARAMETER;

    Status = ConioLockConsole(ProcessData, Request->Data.ReadConsoleRequest.ConsoleHandle,
                              &Console, GENERIC_READ);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }
    Request->Data.ReadConsoleRequest.EventHandle = ProcessData->ConsoleEvent;

    Status = STATUS_PENDING; /* we haven't read anything (yet) */
    if (Console->Mode & ENABLE_LINE_INPUT)
    {
        if (Console->LineBuffer == NULL)
        {
            /* Starting a new line */
            Console->LineMaxSize = max(256, nNumberOfCharsToRead);
            Console->LineBuffer = HeapAlloc(Win32CsrApiHeap, 0, Console->LineMaxSize * sizeof(WCHAR));
            if (Console->LineBuffer == NULL)
            {
                Status = STATUS_NO_MEMORY;
                goto done;
            }
            Console->LineComplete = FALSE;
            Console->LineUpPressed = FALSE;
            Console->LineInsertToggle = 0;
            Console->LineWakeupMask = Request->Data.ReadConsoleRequest.CtrlWakeupMask;
            Console->LineSize = Request->Data.ReadConsoleRequest.NrCharactersRead;
            Console->LinePos = Console->LineSize;
            /* pre-filling the buffer is only allowed in the Unicode API,
             * so we don't need to worry about conversion */
            memcpy(Console->LineBuffer, Buffer, Console->LineSize * sizeof(WCHAR));
            if (Console->LineSize == Console->LineMaxSize)
            {
                Console->LineComplete = TRUE;
                Console->LinePos = 0;
            }
        }

        /* If we don't have a complete line yet, process the pending input */
        while (!Console->LineComplete && !IsListEmpty(&Console->InputEvents))
        {
            /* remove input event from queue */
            CurrentEntry = RemoveHeadList(&Console->InputEvents);
            if (IsListEmpty(&Console->InputEvents))
            {
                ResetEvent(Console->ActiveEvent);
            }
            Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);

            /* only pay attention to key down */
            if (KEY_EVENT == Input->InputEvent.EventType
                    && Input->InputEvent.Event.KeyEvent.bKeyDown)
            {
                LineInputKeyDown(Console, &Input->InputEvent.Event.KeyEvent);
                Request->Data.ReadConsoleRequest.ControlKeyState = Input->InputEvent.Event.KeyEvent.dwControlKeyState;
            }
            HeapFree(Win32CsrApiHeap, 0, Input);
        }

        /* Check if we have a complete line to read from */
        if (Console->LineComplete)
        {
            while (i < nNumberOfCharsToRead && Console->LinePos != Console->LineSize)
            {
                WCHAR Char = Console->LineBuffer[Console->LinePos++];
                if (Request->Data.ReadConsoleRequest.Unicode)
                    UnicodeBuffer[i++] = Char;
                else
                    ConsoleInputUnicodeCharToAnsiChar(Console, &Buffer[i++], &Char);
            }
            if (Console->LinePos == Console->LineSize)
            {
                /* Entire line has been read */
                HeapFree(Win32CsrApiHeap, 0, Console->LineBuffer);
                Console->LineBuffer = NULL;
            }
            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        /* Character input */
        while (i < nNumberOfCharsToRead && !IsListEmpty(&Console->InputEvents))
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
                    && Input->InputEvent.Event.KeyEvent.uChar.UnicodeChar != L'\0')
            {
                WCHAR Char = Input->InputEvent.Event.KeyEvent.uChar.UnicodeChar;
                if (Request->Data.ReadConsoleRequest.Unicode)
                    UnicodeBuffer[i++] = Char;
                else
                    ConsoleInputUnicodeCharToAnsiChar(Console, &Buffer[i++], &Char);
                Status = STATUS_SUCCESS; /* did read something */
            }
            HeapFree(Win32CsrApiHeap, 0, Input);
        }
    }
done:
    Request->Data.ReadConsoleRequest.NrCharactersRead = i;
    ConioUnlockConsole(Console);

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

static NTSTATUS FASTCALL
ConioProcessChar(PCSRSS_CONSOLE Console,
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
    ConInRec = RtlAllocateHeap(Win32CsrApiHeap, 0, sizeof(ConsoleInput));
    if (ConInRec == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;
    ConInRec->InputEvent = *InputEvent;
    InsertTailList(&Console->InputEvents, &ConInRec->ListEntry);
    SetEvent(Console->ActiveEvent);
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
ConioProcessKey(MSG *msg, PCSRSS_CONSOLE Console, BOOL TextMode)
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

    DPRINT  ("csrss: %s %s %s %s %02x %02x '%lc' %04x\n",
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
        PCSR_PROCESS current;
        PLIST_ENTRY current_entry;
        DPRINT1("Console_Api Ctrl-C\n");
        current_entry = Console->ProcessList.Flink;
        while (current_entry != &Console->ProcessList)
        {
            current = CONTAINING_RECORD(current_entry, CSR_PROCESS, ConsoleLink);
            current_entry = current_entry->Flink;
            ConioConsoleCtrlEvent((DWORD)CTRL_C_EVENT, current);
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

CSR_API(CsrReadInputEvent)
{
    PLIST_ENTRY CurrentEntry;
    PCSRSS_CONSOLE Console;
    NTSTATUS Status;
    BOOLEAN Done = FALSE;
    ConsoleInput *Input;

    DPRINT("CsrReadInputEvent\n");

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

        if (Done)
        {
            Request->Data.ReadInputRequest.MoreEvents = TRUE;
            break;
        }

        RemoveEntryList(&Input->ListEntry);

        if (!Done)
        {
            Request->Data.ReadInputRequest.Input = Input->InputEvent;
            if (Request->Data.ReadInputRequest.Unicode == FALSE)
            {
                ConioInputEventToAnsi(Console, &Request->Data.ReadInputRequest.Input);
            }
            Done = TRUE;
        }

        HeapFree(Win32CsrApiHeap, 0, Input);
    }

    if (Done)
        Status = STATUS_SUCCESS;
    else
        Status = STATUS_PENDING;

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

    ConioUnlockConsole(Console);

    return STATUS_SUCCESS;
}

CSR_API(CsrGetNumberOfConsoleInputEvents)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE Console;
    PLIST_ENTRY CurrentItem;
    DWORD NumEvents;

    DPRINT("CsrGetNumberOfConsoleInputEvents\n");

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
        CurrentItem = CurrentItem->Flink;
        NumEvents++;
    }

    ConioUnlockConsole(Console);

    Request->Data.GetNumInputEventsRequest.NumInputEvents = NumEvents;

    return STATUS_SUCCESS;
}

CSR_API(CsrPeekConsoleInput)
{
    NTSTATUS Status;
    PCSRSS_CONSOLE Console;
    DWORD Length;
    PLIST_ENTRY CurrentItem;
    PINPUT_RECORD InputRecord;
    ConsoleInput* Item;
    UINT NumItems;

    DPRINT("CsrPeekConsoleInput\n");

    Status = ConioLockConsole(ProcessData, Request->Data.GetNumInputEventsRequest.ConsoleHandle, &Console, GENERIC_READ);
    if(! NT_SUCCESS(Status))
    {
        return Status;
    }

    InputRecord = Request->Data.PeekConsoleInputRequest.InputRecord;
    Length = Request->Data.PeekConsoleInputRequest.Length;

    if (!Win32CsrValidateBuffer(ProcessData, InputRecord, Length, sizeof(INPUT_RECORD)))
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

            ++NumItems;
            *InputRecord = Item->InputEvent;

            if (Request->Data.PeekConsoleInputRequest.Unicode == FALSE)
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
    DWORD i;

    DPRINT("CsrWriteConsoleInput\n");

    Status = ConioLockConsole(ProcessData, Request->Data.WriteConsoleInputRequest.ConsoleHandle, &Console, GENERIC_WRITE);
    if (! NT_SUCCESS(Status))
    {
        return Status;
    }

    InputRecord = Request->Data.WriteConsoleInputRequest.InputRecord;
    Length = Request->Data.WriteConsoleInputRequest.Length;

    if (!Win32CsrValidateBuffer(ProcessData, InputRecord, Length, sizeof(INPUT_RECORD)))
    {
        ConioUnlockConsole(Console);
        return STATUS_ACCESS_VIOLATION;
    }

    for (i = 0; i < Length && NT_SUCCESS(Status); i++)
    {
        if (!Request->Data.WriteConsoleInputRequest.Unicode &&
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

    Request->Data.WriteConsoleInputRequest.Length = i;

    return Status;
}

/* EOF */
