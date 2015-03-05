/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver DLL
 * FILE:            win32ss/user/winsrv/consrv/condrv/coninput.c
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


#define ConsoleInputUnicodeCharToAnsiChar(Console, dChar, sWChar) \
    WideCharToMultiByte((Console)->CodePage, 0, (sWChar), 1, (dChar), 1, NULL, NULL)

#define ConsoleInputAnsiCharToUnicodeChar(Console, dWChar, sChar) \
    MultiByteToWideChar((Console)->CodePage, 0, (sChar), 1, (dWChar), 1)

typedef struct ConsoleInput_t
{
    LIST_ENTRY ListEntry;
    INPUT_RECORD InputEvent;
} ConsoleInput;


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
                                   !(cks & (LEFT_ALT_PRESSED  | RIGHT_ALT_PRESSED)))))
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

    /* Add event to the queue */
    ConInRec = ConsoleAllocHeap(0, sizeof(ConsoleInput));
    if (ConInRec == NULL) return STATUS_INSUFFICIENT_RESOURCES;

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
        ConsoleFreeHeap(Event);
    }

    CloseHandle(Console->InputBuffer.ActiveEvent);
}

VOID NTAPI
ConDrvProcessKey(IN PCONSOLE Console,
                 IN BOOLEAN Down,
                 IN UINT VirtualKeyCode,
                 IN UINT VirtualScanCode,
                 IN WCHAR UnicodeChar,
                 IN ULONG ShiftState,
                 IN BYTE KeyStateCtrl)
{
    INPUT_RECORD er;

    /* process Ctrl-C and Ctrl-Break */
    if ( Console->InputBuffer.Mode & ENABLE_PROCESSED_INPUT &&
         Down && (VirtualKeyCode == VK_PAUSE || VirtualKeyCode == 'C') &&
         (ShiftState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED) || KeyStateCtrl & 0x80) )
    {
        DPRINT1("Console_Api Ctrl-C\n");
        ConDrvConsoleProcessCtrlEvent(Console, 0, CTRL_C_EVENT);

        if (Console->LineBuffer && !Console->LineComplete)
        {
            /* Line input is in progress; end it */
            Console->LinePos = Console->LineSize = 0;
            Console->LineComplete = TRUE;
        }
        return;
    }

    if ( (ShiftState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) != 0 &&
         (VK_UP == VirtualKeyCode || VK_DOWN == VirtualKeyCode) )
    {
        if (!Down) return;

        /* scroll up or down */
        if (VK_UP == VirtualKeyCode)
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
        return;
    }

    er.EventType                        = KEY_EVENT;
    er.Event.KeyEvent.bKeyDown          = Down;
    er.Event.KeyEvent.wRepeatCount      = 1;
    er.Event.KeyEvent.wVirtualKeyCode   = VirtualKeyCode;
    er.Event.KeyEvent.wVirtualScanCode  = VirtualScanCode;
    er.Event.KeyEvent.uChar.UnicodeChar = UnicodeChar;
    er.Event.KeyEvent.dwControlKeyState = ShiftState;

    ConioProcessInputEvent(Console, &er);
}


/* PUBLIC DRIVER APIS *********************************************************/

NTSTATUS NTAPI
ConDrvReadConsole(IN PCONSOLE Console,
                  IN PCONSOLE_INPUT_BUFFER InputBuffer,
                  IN BOOLEAN Unicode,
                  OUT PVOID Buffer,
                  IN OUT PCONSOLE_READCONSOLE_CONTROL ReadControl,
                  IN ULONG NumCharsToRead,
                  OUT PULONG NumCharsRead OPTIONAL)
{
    // STATUS_PENDING : Wait if more to read ; STATUS_SUCCESS : Don't wait.
    NTSTATUS Status = STATUS_PENDING;
    PLIST_ENTRY CurrentEntry;
    ConsoleInput *Input;
    ULONG i = ReadControl->nInitialChars;

    if (Console == NULL || InputBuffer == NULL || /* Buffer == NULL  || */
        ReadControl == NULL || ReadControl->nLength != sizeof(CONSOLE_READCONSOLE_CONTROL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validity checks */
    ASSERT(Console == InputBuffer->Header.Console);
    ASSERT( (Buffer != NULL && NumCharsToRead >= 0) ||
            (Buffer == NULL && NumCharsToRead == 0) );

    /* We haven't read anything (yet) */

    if (InputBuffer->Mode & ENABLE_LINE_INPUT)
    {
        if (Console->LineBuffer == NULL)
        {
            /* Starting a new line */
            Console->LineMaxSize = (WORD)max(256, NumCharsToRead);

            Console->LineBuffer = ConsoleAllocHeap(0, Console->LineMaxSize * sizeof(WCHAR));
            if (Console->LineBuffer == NULL) return STATUS_NO_MEMORY;

            Console->LineComplete = FALSE;
            Console->LineUpPressed = FALSE;
            Console->LineInsertToggle = 0;
            Console->LineWakeupMask = ReadControl->dwCtrlWakeupMask;
            Console->LineSize = ReadControl->nInitialChars;
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
        while (!Console->LineComplete && !IsListEmpty(&InputBuffer->InputEvents))
        {
            /* Remove input event from queue */
            CurrentEntry = RemoveHeadList(&InputBuffer->InputEvents);
            if (IsListEmpty(&InputBuffer->InputEvents))
            {
                ResetEvent(InputBuffer->ActiveEvent);
            }
            Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);

            /* Only pay attention to key down */
            if (Input->InputEvent.EventType == KEY_EVENT &&
                Input->InputEvent.Event.KeyEvent.bKeyDown)
            {
                LineInputKeyDown(Console, &Input->InputEvent.Event.KeyEvent);
                ReadControl->dwControlKeyState = Input->InputEvent.Event.KeyEvent.dwControlKeyState;
            }
            ConsoleFreeHeap(Input);
        }

        /* Check if we have a complete line to read from */
        if (Console->LineComplete)
        {
            while (i < NumCharsToRead && Console->LinePos != Console->LineSize)
            {
                WCHAR Char = Console->LineBuffer[Console->LinePos++];

                if (Unicode)
                {
                    ((PWCHAR)Buffer)[i] = Char;
                }
                else
                {
                    ConsoleInputUnicodeCharToAnsiChar(Console, &((PCHAR)Buffer)[i], &Char);
                }
                ++i;
            }

            if (Console->LinePos == Console->LineSize)
            {
                /* Entire line has been read */
                ConsoleFreeHeap(Console->LineBuffer);
                Console->LineBuffer = NULL;
            }

            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        /* Character input */
        while (i < NumCharsToRead && !IsListEmpty(&InputBuffer->InputEvents))
        {
            /* Remove input event from queue */
            CurrentEntry = RemoveHeadList(&InputBuffer->InputEvents);
            if (IsListEmpty(&InputBuffer->InputEvents))
            {
                ResetEvent(InputBuffer->ActiveEvent);
            }
            Input = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);

            /* Only pay attention to valid ASCII chars, on key down */
            if (Input->InputEvent.EventType == KEY_EVENT  &&
                Input->InputEvent.Event.KeyEvent.bKeyDown &&
                Input->InputEvent.Event.KeyEvent.uChar.UnicodeChar != L'\0')
            {
                WCHAR Char = Input->InputEvent.Event.KeyEvent.uChar.UnicodeChar;

                if (Unicode)
                {
                    ((PWCHAR)Buffer)[i] = Char;
                }
                else
                {
                    ConsoleInputUnicodeCharToAnsiChar(Console, &((PCHAR)Buffer)[i], &Char);
                }
                ++i;

                /* Did read something */
                Status = STATUS_SUCCESS;
            }
            ConsoleFreeHeap(Input);
        }
    }

    if (NumCharsRead) *NumCharsRead = i;

    return Status;
}

NTSTATUS NTAPI
ConDrvGetConsoleInput(IN PCONSOLE Console,
                      IN PCONSOLE_INPUT_BUFFER InputBuffer,
                      IN BOOLEAN WaitForMoreEvents,
                      IN BOOLEAN Unicode,
                      OUT PINPUT_RECORD InputRecord,
                      IN ULONG NumEventsToRead,
                      OUT PULONG NumEventsRead OPTIONAL)
{
    PLIST_ENTRY CurrentInput;
    ConsoleInput* Input;
    ULONG i = 0;

    if (Console == NULL || InputBuffer == NULL /* || InputRecord == NULL */)
        return STATUS_INVALID_PARAMETER;

    /* Validity checks */
    ASSERT(Console == InputBuffer->Header.Console);
    ASSERT( (InputRecord != NULL && NumEventsToRead >= 0) ||
            (InputRecord == NULL && NumEventsToRead == 0) );

    // Do NOT do that !! Use the existing number of events already read, if any...
    // if (NumEventsRead) *NumEventsRead = 0;

    if (IsListEmpty(&InputBuffer->InputEvents))
    {
        /*
         * No input is available. Wait for more input if requested,
         * otherwise, we don't wait, so we return success.
         */
        return (WaitForMoreEvents ? STATUS_PENDING : STATUS_SUCCESS);
    }

    /* Only get input if there is any */
    CurrentInput = InputBuffer->InputEvents.Flink;
    if (NumEventsRead) i = *NumEventsRead; // We will read the remaining events...

    while ((CurrentInput != &InputBuffer->InputEvents) && (i < NumEventsToRead))
    {
        Input = CONTAINING_RECORD(CurrentInput, ConsoleInput, ListEntry);

        *InputRecord = Input->InputEvent;

        if (!Unicode)
        {
            ConioInputEventToAnsi(InputBuffer->Header.Console, InputRecord);
        }

        ++InputRecord;
        ++i;
        CurrentInput = CurrentInput->Flink;

        if (WaitForMoreEvents) // TRUE --> Read, we remove inputs from the buffer ; FALSE --> Peek, we keep inputs.
        {
            RemoveEntryList(&Input->ListEntry);
            ConsoleFreeHeap(Input);
        }
    }

    if (NumEventsRead) *NumEventsRead = i;

    if (IsListEmpty(&InputBuffer->InputEvents))
    {
        ResetEvent(InputBuffer->ActiveEvent);
    }

    /* We read all the inputs available, we return success */
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvWriteConsoleInput(IN PCONSOLE Console,
                        IN PCONSOLE_INPUT_BUFFER InputBuffer,
                        IN BOOLEAN Unicode,
                        IN PINPUT_RECORD InputRecord,
                        IN ULONG NumEventsToWrite,
                        OUT PULONG NumEventsWritten OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    if (Console == NULL || InputBuffer == NULL /* || InputRecord == NULL */)
        return STATUS_INVALID_PARAMETER;

    /* Validity checks */
    ASSERT(Console == InputBuffer->Header.Console);
    ASSERT( (InputRecord != NULL && NumEventsToWrite >= 0) ||
            (InputRecord == NULL && NumEventsToWrite == 0) );

    // Do NOT do that !! Use the existing number of events already written, if any...
    // if (NumEventsWritten) *NumEventsWritten = 0;

    for (i = (NumEventsWritten ? *NumEventsWritten : 0); i < NumEventsToWrite && NT_SUCCESS(Status); ++i)
    {
        if (InputRecord->EventType == KEY_EVENT && !Unicode)
        {
            CHAR AsciiChar = InputRecord->Event.KeyEvent.uChar.AsciiChar;
            ConsoleInputAnsiCharToUnicodeChar(Console,
                                              &InputRecord->Event.KeyEvent.uChar.UnicodeChar,
                                              &AsciiChar);
        }

        Status = ConioProcessInputEvent(Console, InputRecord++);
    }

    if (NumEventsWritten) *NumEventsWritten = i;

    return Status;
}

NTSTATUS NTAPI
ConDrvFlushConsoleInputBuffer(IN PCONSOLE Console,
                              IN PCONSOLE_INPUT_BUFFER InputBuffer)
{
    PLIST_ENTRY CurrentEntry;
    ConsoleInput* Event;

    if (Console == NULL || InputBuffer == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == InputBuffer->Header.Console);

    /* Discard all entries in the input event queue */
    while (!IsListEmpty(&InputBuffer->InputEvents))
    {
        CurrentEntry = RemoveHeadList(&InputBuffer->InputEvents);
        Event = CONTAINING_RECORD(CurrentEntry, ConsoleInput, ListEntry);
        ConsoleFreeHeap(Event);
    }
    ResetEvent(InputBuffer->ActiveEvent);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvGetConsoleNumberOfInputEvents(IN PCONSOLE Console,
                                    IN PCONSOLE_INPUT_BUFFER InputBuffer,
                                    OUT PULONG NumEvents)
{
    PLIST_ENTRY CurrentInput;

    if (Console == NULL || InputBuffer == NULL || NumEvents == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == InputBuffer->Header.Console);

    *NumEvents = 0;

    /* If there are any events ... */
    CurrentInput = InputBuffer->InputEvents.Flink;
    while (CurrentInput != &InputBuffer->InputEvents)
    {
        CurrentInput = CurrentInput->Flink;
        (*NumEvents)++;
    }

    return STATUS_SUCCESS;
}

/* EOF */
